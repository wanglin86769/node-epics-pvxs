#pragma once

#include <napi.h>

#include <pvxs/client.h>

namespace binding {

class ContextWrap : public Napi::ObjectWrap<ContextWrap> {
public:
    static Napi::Object Init(Napi::Env env, Napi::Object exports);
    static Napi::Value FromEnv(const Napi::CallbackInfo& info);
    static Napi::Value FromConfig(const Napi::CallbackInfo& info);
    static Napi::Object WrapContext(Napi::Env env, pvxs::client::Context&& ctx);

    explicit ContextWrap(const Napi::CallbackInfo& info);

private:
    static Napi::FunctionReference constructor_;

    Napi::Value Close(const Napi::CallbackInfo& info);
    Napi::Value Get(const Napi::CallbackInfo& info);
    Napi::Value Put(const Napi::CallbackInfo& info);
    Napi::Value Rpc(const Napi::CallbackInfo& info);
    Napi::Value List(const Napi::CallbackInfo& info);
    Napi::Value Monitor(const Napi::CallbackInfo& info);
    Napi::Value Discover(const Napi::CallbackInfo& info);

    pvxs::client::Context ctx_;
};

}  // namespace binding
