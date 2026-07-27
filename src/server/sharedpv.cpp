#include "server/sharedpv.h"

#include "common/errors.h"
#include "common/tsfn.h"
#include "data/value_wrap.h"
#include "server/exec_op.h"

#include <memory>
#include <utility>

namespace binding {

Napi::FunctionReference SharedPVWrap::constructor_;

bool SharedPVWrap::IsInstance(const Napi::Value& value) {
    if (constructor_.IsEmpty()) {
        return false;
    }
    return value.IsObject() &&
           value.As<Napi::Object>().InstanceOf(constructor_.Value());
}

pvxs::server::SharedPV SharedPVWrap::UnwrapPV(const Napi::Value& value) {
    return Napi::ObjectWrap<SharedPVWrap>::Unwrap(value.As<Napi::Object>())->pv_;
}

Napi::Object SharedPVWrap::Create(Napi::Env env, pvxs::server::SharedPV pv) {
    Napi::Object obj = constructor_.New({});
    Napi::ObjectWrap<SharedPVWrap>::Unwrap(obj)->pv_ = std::move(pv);
    return obj;
}

SharedPVWrap::SharedPVWrap(const Napi::CallbackInfo& info)
    : Napi::ObjectWrap<SharedPVWrap>(info) {
    Napi::Env env = info.Env();
    put_tsfn_ = MakeTsfn(env, "pvxsOnPut");
    rpc_tsfn_ = MakeTsfn(env, "pvxsOnRpc");
}

SharedPVWrap::~SharedPVWrap() {
    if (pv_) {
        pv_.onPut(nullptr);
        pv_.onRPC(nullptr);
    }
    if (put_tsfn_) {
        put_tsfn_.Release();
    }
    if (rpc_tsfn_) {
        rpc_tsfn_.Release();
    }
}

void SharedPVWrap::RunHandler(Napi::Env env,
                              HandlerState* payload,
                              Napi::FunctionReference& handler) {
    try {
        Napi::HandleScope scope(env);
        Napi::Object self_obj = payload->wrap->Value();
        Napi::Object op_obj =
            ExecOpWrap::Create(env, payload->op, std::move(payload->value));
        auto* op_wrap = Napi::ObjectWrap<ExecOpWrap>::Unwrap(op_obj);
        Napi::Value ret = handler.Call({self_obj, op_obj});
        // JS throw becomes Napi::Error under NAPI_CPP_EXCEPTIONS (caught below).
        // Async handlers return a Promise before op.reply()/error(); that is
        // not supported — require synchronous completion.
        if (ret.IsPromise()) {
            payload->error =
                "SharedPV onPut/onRPC handler must be synchronous "
                "(async functions / returned Promises are not supported)";
            return;
        }
        payload->finished = op_wrap->finished();
    } catch (const std::exception& e) {
        payload->error = e.what();
    }
}

Napi::Value SharedPVWrap::BuildMailbox(const Napi::CallbackInfo& info) {
    Napi::Env env = info.Env();
    try {
        return Create(env, pvxs::server::SharedPV::buildMailbox());
    } catch (const std::exception& e) {
        ThrowFromPvxs(env, e);
        return env.Undefined();
    }
}

Napi::Value SharedPVWrap::BuildReadonly(const Napi::CallbackInfo& info) {
    Napi::Env env = info.Env();
    try {
        return Create(env, pvxs::server::SharedPV::buildReadonly());
    } catch (const std::exception& e) {
        ThrowFromPvxs(env, e);
        return env.Undefined();
    }
}

Napi::Value SharedPVWrap::Open(const Napi::CallbackInfo& info) {
    Napi::Env env = info.Env();
    if (info.Length() < 1) {
        Napi::TypeError::New(env, "open(initial) requires a Value")
            .ThrowAsJavaScriptException();
        return env.Undefined();
    }
    try {
        pv_.open(ValueFromJsArg(env, info[0]));
    } catch (const std::exception& e) {
        ThrowFromPvxs(env, e);
    }
    return env.Undefined();
}

Napi::Value SharedPVWrap::Close(const Napi::CallbackInfo& info) {
    try {
        pv_.close();
    } catch (const std::exception& e) {
        ThrowFromPvxs(info.Env(), e);
    }
    return info.Env().Undefined();
}

Napi::Value SharedPVWrap::IsOpen(const Napi::CallbackInfo& info) {
    Napi::Env env = info.Env();
    try {
        return Napi::Boolean::New(env, pv_.isOpen());
    } catch (const std::exception& e) {
        ThrowFromPvxs(env, e);
        return env.Undefined();
    }
}

Napi::Value SharedPVWrap::Post(const Napi::CallbackInfo& info) {
    Napi::Env env = info.Env();
    if (info.Length() < 1) {
        Napi::TypeError::New(env, "post(value) requires a value")
            .ThrowAsJavaScriptException();
        return env.Undefined();
    }
    try {
        if (PvxsValueWrap::IsInstance(info[0])) {
            pv_.post(PvxsValueWrap::FromJs(info[0]));
        } else {
            pvxs::Value current = pv_.fetch();
            AssignFromJs(current, info[0]);
            pv_.post(current);
        }
    } catch (const std::exception& e) {
        ThrowFromPvxs(env, e);
    }
    return env.Undefined();
}

Napi::Value SharedPVWrap::Current(const Napi::CallbackInfo& info) {
    Napi::Env env = info.Env();
    try {
        return PvxsValueWrap::ToJs(env, pv_.fetch());
    } catch (const std::exception& e) {
        ThrowFromPvxs(env, e);
        return env.Undefined();
    }
}

Napi::Value SharedPVWrap::OnPut(const Napi::CallbackInfo& info) {
    Napi::Env env = info.Env();
    if (info.Length() < 1 || !info[0].IsFunction()) {
        Napi::TypeError::New(env, "onPut(callback) requires a function")
            .ThrowAsJavaScriptException();
        return env.Undefined();
    }
    put_handler_ = Napi::Persistent(info[0].As<Napi::Function>());
    SharedPVWrap* self = this;
    pv_.onPut([self](pvxs::server::SharedPV&,
                     std::unique_ptr<pvxs::server::ExecOp>&& rawop,
                     pvxs::Value&& val) {
        if (self->put_handler_.IsEmpty()) {
            if (rawop) {
                rawop->error("no onPut handler installed");
            }
            return;
        }

        auto* payload = new HandlerState{
            self,
            std::shared_ptr<pvxs::server::ExecOp>(std::move(rawop)),
            std::move(val),
            false,
            {}};

        self->put_tsfn_.BlockingCall(
            payload,
            [](Napi::Env cb_env, Napi::Function, HandlerState* data) {
                RunHandler(cb_env, data, data->wrap->put_handler_);
                if (!data->error.empty() && data->op) {
                    data->op->error(data->error);
                } else if (!data->finished && data->op) {
                    data->op->error("onPut handler did not call reply() or error()");
                }
                delete data;
            });
    });
    return env.Undefined();
}

Napi::Value SharedPVWrap::OnRpc(const Napi::CallbackInfo& info) {
    Napi::Env env = info.Env();
    if (info.Length() < 1 || !info[0].IsFunction()) {
        Napi::TypeError::New(env, "onRPC(callback) requires a function")
            .ThrowAsJavaScriptException();
        return env.Undefined();
    }
    rpc_handler_ = Napi::Persistent(info[0].As<Napi::Function>());
    SharedPVWrap* self = this;
    pv_.onRPC([self](pvxs::server::SharedPV&,
                     std::unique_ptr<pvxs::server::ExecOp>&& rawop,
                     pvxs::Value&& val) {
        if (self->rpc_handler_.IsEmpty()) {
            if (rawop) {
                rawop->error("no onRPC handler installed");
            }
            return;
        }

        auto* payload = new HandlerState{
            self,
            std::shared_ptr<pvxs::server::ExecOp>(std::move(rawop)),
            std::move(val),
            false,
            {}};

        self->rpc_tsfn_.BlockingCall(
            payload,
            [](Napi::Env cb_env, Napi::Function, HandlerState* data) {
                RunHandler(cb_env, data, data->wrap->rpc_handler_);
                if (!data->error.empty() && data->op) {
                    data->op->error(data->error);
                } else if (!data->finished && data->op) {
                    data->op->error("onRPC handler did not call reply() or error()");
                }
                delete data;
            });
    });
    return env.Undefined();
}

Napi::Object SharedPVWrap::Init(Napi::Env env, Napi::Object exports) {
    Napi::Function func = DefineClass(
        env,
        "SharedPV",
        {
            StaticMethod("buildMailbox", &SharedPVWrap::BuildMailbox),
            StaticMethod("buildReadonly", &SharedPVWrap::BuildReadonly),
            InstanceMethod("open", &SharedPVWrap::Open),
            InstanceMethod("close", &SharedPVWrap::Close),
            InstanceMethod("isOpen", &SharedPVWrap::IsOpen),
            InstanceMethod("post", &SharedPVWrap::Post),
            InstanceMethod("current", &SharedPVWrap::Current),
            InstanceMethod("onPut", &SharedPVWrap::OnPut),
            InstanceMethod("onRPC", &SharedPVWrap::OnRpc),
        });
    constructor_ = Napi::Persistent(func);
    constructor_.SuppressDestruct();
    exports.Set("SharedPV", func);
    return exports;
}

}  // namespace binding
