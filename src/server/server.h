#pragma once

#include <napi.h>

#include <pvxs/server.h>

#include <map>
#include <string>

namespace binding {

class ServerWrap : public Napi::ObjectWrap<ServerWrap> {
public:
    static Napi::Object Init(Napi::Env env, Napi::Object exports);

    explicit ServerWrap(const Napi::CallbackInfo& info);
    ~ServerWrap() override;

private:
    static Napi::FunctionReference constructor_;

    static Napi::Value FromEnv(const Napi::CallbackInfo& info);
    static Napi::Value FromIsolated(const Napi::CallbackInfo& info);

    Napi::Value AddPV(const Napi::CallbackInfo& info);
    Napi::Value RemovePV(const Napi::CallbackInfo& info);
    Napi::Value AddSource(const Napi::CallbackInfo& info);
    Napi::Value RemoveSource(const Napi::CallbackInfo& info);
    Napi::Value Start(const Napi::CallbackInfo& info);
    Napi::Value Stop(const Napi::CallbackInfo& info);
    Napi::Value ClientConfig(const Napi::CallbackInfo& info);

    void ClearRefs();

    pvxs::server::Server server_;
    // Keep JS SharedPV / StaticSource alive while registered (handlers live on wraps).
    std::map<std::string, Napi::ObjectReference> pvs_;
    std::map<std::string, Napi::ObjectReference> sources_;
};

}  // namespace binding
