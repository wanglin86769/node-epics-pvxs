#include "server/exec_op.h"

#include "common/errors.h"
#include "data/value_wrap.h"

namespace binding {

Napi::FunctionReference ExecOpWrap::constructor_;

Napi::Object ExecOpWrap::Create(Napi::Env env,
                                const std::shared_ptr<pvxs::server::ExecOp>& op,
                                pvxs::Value&& request) {
    Napi::Object obj = constructor_.New({});
    auto* wrap = Napi::ObjectWrap<ExecOpWrap>::Unwrap(obj);
    wrap->op_ = op;
    wrap->request_ = std::move(request);
    return obj;
}

ExecOpWrap::ExecOpWrap(const Napi::CallbackInfo& info)
    : Napi::ObjectWrap<ExecOpWrap>(info) {}

Napi::Value ExecOpWrap::Reply(const Napi::CallbackInfo& info) {
    Napi::Env env = info.Env();
    if (!op_) {
        Napi::Error::New(env, "ExecOp is no longer valid").ThrowAsJavaScriptException();
        return env.Undefined();
    }
    try {
        if (info.Length() > 0) {
            op_->reply(ValueFromJsArg(env, info[0]));
        } else {
            op_->reply();
        }
        finished_ = true;
        op_.reset();
    } catch (const std::exception& e) {
        ThrowFromPvxs(env, e);
    }
    return env.Undefined();
}

Napi::Value ExecOpWrap::Error(const Napi::CallbackInfo& info) {
    Napi::Env env = info.Env();
    if (!op_) {
        Napi::Error::New(env, "ExecOp is no longer valid").ThrowAsJavaScriptException();
        return env.Undefined();
    }
    std::string msg = "server error";
    if (info.Length() > 0 && info[0].IsString()) {
        msg = info[0].As<Napi::String>().Utf8Value();
    }
    try {
        op_->error(msg);
        finished_ = true;
        op_.reset();
    } catch (const std::exception& e) {
        ThrowFromPvxs(env, e);
    }
    return env.Undefined();
}

Napi::Value ExecOpWrap::RequestValue(const Napi::CallbackInfo& info) {
    return PvxsValueWrap::ToJs(info.Env(), request_);
}

Napi::Object ExecOpWrap::Init(Napi::Env env, Napi::Object exports) {
    Napi::Function func = DefineClass(
        env,
        "ExecOp",
        {
            InstanceMethod("reply", &ExecOpWrap::Reply),
            InstanceMethod("error", &ExecOpWrap::Error),
            InstanceMethod("value", &ExecOpWrap::RequestValue),
        });
    constructor_ = Napi::Persistent(func);
    constructor_.SuppressDestruct();
    exports.Set("ExecOp", func);
    return exports;
}

}  // namespace binding
