#pragma once

#include <napi.h>

#include <pvxs/client.h>

namespace binding {

class ClientConfigWrap : public Napi::ObjectWrap<ClientConfigWrap> {
public:
    static Napi::Object Init(Napi::Env env, Napi::Object exports);
    static Napi::Object ToJs(Napi::Env env, pvxs::client::Config config);
    static pvxs::client::Config FromJs(const Napi::Value& value);
    /** Build Config from a plain object of EPICS_PVA_* string defs (applyDefs). */
    static pvxs::client::Config FromDefsObject(const Napi::Value& value);
    static bool IsInstance(const Napi::Value& value);

    static Napi::Value FromEnv(const Napi::CallbackInfo& info);
    static Napi::Value FromDefs(const Napi::CallbackInfo& info);

    explicit ClientConfigWrap(const Napi::CallbackInfo& info);

    pvxs::client::Config& config() { return config_; }
    const pvxs::client::Config& config() const { return config_; }

private:
    static Napi::FunctionReference constructor_;

    pvxs::client::Config config_;
};

}  // namespace binding
