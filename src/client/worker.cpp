#include "client/worker.h"

#include "common/errors.h"
#include "data/put_payload.h"
#include "data/value_wrap.h"

#include <cmath>
#include <stdexcept>
#include <vector>

namespace binding {
namespace {

using pvxs::client::Context;
using pvxs::client::Operation;
using pvxs::client::RPCBuilder;

struct RpcArg {
    std::string key;
    enum class Kind { Bool, Int, Float, String } kind;
    bool b = false;
    int64_t i64 = 0;
    double f64 = 0.0;
    std::string str;
};

std::vector<RpcArg> ParseRpcArgs(const Napi::Object& args) {
    std::vector<RpcArg> out;
    Napi::Array names = args.GetPropertyNames();
    for (uint32_t i = 0; i < names.Length(); ++i) {
        const std::string key = names.Get(i).As<Napi::String>().Utf8Value();
        Napi::Value val = args.Get(key);
        RpcArg arg;
        arg.key = key;
        if (val.IsBoolean()) {
            arg.kind = RpcArg::Kind::Bool;
            arg.b = val.As<Napi::Boolean>().Value();
        } else if (val.IsNumber()) {
            const double num = val.As<Napi::Number>().DoubleValue();
            double intpart = 0.0;
            if (std::modf(num, &intpart) == 0.0) {
                arg.kind = RpcArg::Kind::Int;
                arg.i64 = static_cast<int64_t>(num);
            } else {
                arg.kind = RpcArg::Kind::Float;
                arg.f64 = num;
            }
        } else if (val.IsString()) {
            arg.kind = RpcArg::Kind::String;
            arg.str = val.As<Napi::String>().Utf8Value();
        } else {
            throw std::runtime_error(
                "rpc args values must be boolean, number, or string");
        }
        out.push_back(std::move(arg));
    }
    return out;
}

void ApplyRpcArgs(RPCBuilder& builder, const std::vector<RpcArg>& args) {
    for (const auto& arg : args) {
        switch (arg.kind) {
            case RpcArg::Kind::Bool:
                builder.arg(arg.key, arg.b);
                break;
            case RpcArg::Kind::Int:
                builder.arg(arg.key, arg.i64);
                break;
            case RpcArg::Kind::Float:
                builder.arg(arg.key, arg.f64);
                break;
            case RpcArg::Kind::String:
                builder.arg(arg.key, arg.str);
                break;
        }
    }
}

/** Worker only blocks on wait(); Operation is created on the main thread. */
class WaitWorker : public Napi::AsyncWorker {
public:
    WaitWorker(Napi::Promise::Deferred deferred,
               std::shared_ptr<Operation> op,
               double timeout_sec)
        : Napi::AsyncWorker(deferred.Env()),
          deferred_(deferred),
          op_(std::move(op)),
          timeout_sec_(timeout_sec) {}

    void Execute() override {
        try {
            result_ = op_->wait(timeout_sec_);
        } catch (const std::exception& e) {
            if (op_) {
                op_->cancel();
            }
            SetError(e.what());
        }
    }

    void OnOK() override {
        deferred_.Resolve(PvxsValueWrap::ToJs(Env(), result_));
    }

    void OnError(const Napi::Error& e) override {
        RejectFromMessage(deferred_, e.Message());
    }

private:
    Napi::Promise::Deferred deferred_;
    std::shared_ptr<Operation> op_;
    double timeout_sec_;
    pvxs::Value result_;
};

Napi::Promise MakeCancellablePromise(Napi::Env env,
                                      Napi::Promise::Deferred deferred,
                                      std::shared_ptr<Operation> op) {
    Napi::Promise promise = deferred.Promise();
    promise.Set(
        "cancel",
        Napi::Function::New(
            env,
            [op](const Napi::CallbackInfo& info) {
                if (op) {
                    op->cancel();
                }
                return info.Env().Undefined();
            }));
    return promise;
}

Napi::Promise QueueWait(Napi::Env env,
                        Napi::Promise::Deferred deferred,
                        std::shared_ptr<Operation> op,
                        double timeout_sec) {
    // Worker and .cancel each hold a shared_ptr to the same Operation.
    auto* worker = new WaitWorker(deferred, op, timeout_sec);
    worker->Queue();
    return MakeCancellablePromise(env, deferred, op);
}

}  // namespace

Napi::Promise RunGet(Napi::Env env,
                     Context& ctx,
                     const std::string& name,
                     double timeout_sec) {
    auto deferred = Napi::Promise::Deferred::New(env);
    try {
        return QueueWait(env, deferred, ctx.get(name).exec(), timeout_sec);
    } catch (const std::exception& e) {
        RejectFromMessage(deferred, e.what());
        return deferred.Promise();
    }
}

Napi::Promise RunPut(Napi::Env env,
                     Context& ctx,
                     const std::string& name,
                     const Napi::Value& data,
                     double timeout_sec) {
    auto deferred = Napi::Promise::Deferred::New(env);
    auto parsed = ParsePutPayload(data);
    if (!parsed.error.empty()) {
        RejectFromMessage(deferred, parsed.error);
        return deferred.Promise();
    }

    try {
        auto payload = std::make_shared<PutPayload>(std::move(parsed.payload));
        auto op = ctx.put(name)
                      .fetchPresent(true)
                      .build([payload](pvxs::Value&& current) -> pvxs::Value {
                          pvxs::Value toput = current.cloneEmpty();
                          ApplyPutPayload(toput, *payload);
                          return toput;
                      })
                      .exec();
        return QueueWait(env, deferred, std::move(op), timeout_sec);
    } catch (const std::exception& e) {
        RejectFromMessage(deferred, e.what());
        return deferred.Promise();
    }
}

Napi::Promise RunRpc(Napi::Env env,
                     Context& ctx,
                     const std::string& name,
                     const Napi::Object& args,
                     double timeout_sec) {
    auto deferred = Napi::Promise::Deferred::New(env);
    try {
        auto parsed = ParseRpcArgs(args);
        auto builder = ctx.rpc(name);
        ApplyRpcArgs(builder, parsed);
        return QueueWait(env, deferred, builder.exec(), timeout_sec);
    } catch (const std::exception& e) {
        RejectFromMessage(deferred, e.what());
        return deferred.Promise();
    }
}

Napi::Promise RunList(Napi::Env env,
                      Context& ctx,
                      const std::string& server,
                      double timeout_sec) {
    auto deferred = Napi::Promise::Deferred::New(env);
    try {
        auto op = ctx.rpc("server")
                      .server(server)
                      .arg("op", "channels")
                      .exec();
        return QueueWait(env, deferred, std::move(op), timeout_sec);
    } catch (const std::exception& e) {
        RejectFromMessage(deferred, e.what());
        return deferred.Promise();
    }
}

}  // namespace binding
