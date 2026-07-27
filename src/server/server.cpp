#include "server/server.h"

#include "client/config.h"
#include "common/errors.h"
#include "server/sharedpv.h"
#include "server/static_source.h"

namespace binding {

Napi::FunctionReference ServerWrap::constructor_;

ServerWrap::ServerWrap(const Napi::CallbackInfo& info)
    : Napi::ObjectWrap<ServerWrap>(info) {}

void ServerWrap::ClearRefs() {
    for (auto& entry : pvs_) {
        entry.second.Reset();
    }
    pvs_.clear();
    for (auto& entry : sources_) {
        entry.second.Reset();
    }
    sources_.clear();
}

ServerWrap::~ServerWrap() {
    server_.stop();
    ClearRefs();
}

Napi::Value ServerWrap::FromEnv(const Napi::CallbackInfo& info) {
    Napi::Env env = info.Env();
    try {
        Napi::Object obj = constructor_.New({});
        Napi::ObjectWrap<ServerWrap>::Unwrap(obj)->server_ =
            pvxs::server::Server::fromEnv();
        return obj;
    } catch (const std::exception& e) {
        ThrowFromPvxs(env, e);
        return env.Undefined();
    }
}

Napi::Value ServerWrap::FromIsolated(const Napi::CallbackInfo& info) {
    Napi::Env env = info.Env();
    try {
        Napi::Object obj = constructor_.New({});
        Napi::ObjectWrap<ServerWrap>::Unwrap(obj)->server_ =
            pvxs::server::Server(pvxs::server::Config::isolated());
        return obj;
    } catch (const std::exception& e) {
        ThrowFromPvxs(env, e);
        return env.Undefined();
    }
}

Napi::Value ServerWrap::AddPV(const Napi::CallbackInfo& info) {
    Napi::Env env = info.Env();
    if (info.Length() < 2 || !info[0].IsString() || !SharedPVWrap::IsInstance(info[1])) {
        Napi::TypeError::New(env, "addPV(name, sharedPV) requires a string and SharedPV")
            .ThrowAsJavaScriptException();
        return env.Undefined();
    }
    const std::string name = info[0].As<Napi::String>().Utf8Value();
    Napi::Object pv_obj = info[1].As<Napi::Object>();
    try {
        server_.addPV(name, SharedPVWrap::UnwrapPV(pv_obj));
        auto it = pvs_.find(name);
        if (it != pvs_.end()) {
            it->second.Reset();
            it->second = Napi::Persistent(pv_obj);
        } else {
            pvs_.emplace(name, Napi::Persistent(pv_obj));
        }
    } catch (const std::exception& e) {
        ThrowFromPvxs(env, e);
    }
    return env.Undefined();
}

Napi::Value ServerWrap::RemovePV(const Napi::CallbackInfo& info) {
    Napi::Env env = info.Env();
    if (info.Length() < 1 || !info[0].IsString()) {
        Napi::TypeError::New(env, "removePV(name) requires a string")
            .ThrowAsJavaScriptException();
        return env.Undefined();
    }
    const std::string name = info[0].As<Napi::String>().Utf8Value();
    try {
        server_.removePV(name);
        auto it = pvs_.find(name);
        if (it != pvs_.end()) {
            it->second.Reset();
            pvs_.erase(it);
        }
    } catch (const std::exception& e) {
        ThrowFromPvxs(env, e);
    }
    return env.Undefined();
}

Napi::Value ServerWrap::AddSource(const Napi::CallbackInfo& info) {
    Napi::Env env = info.Env();
    if (info.Length() < 2 || !info[0].IsString() ||
        !StaticSourceWrap::IsInstance(info[1])) {
        Napi::TypeError::New(
            env, "addSource(name, staticSource) requires a string and StaticSource")
            .ThrowAsJavaScriptException();
        return env.Undefined();
    }
    const std::string name = info[0].As<Napi::String>().Utf8Value();
    Napi::Object src_obj = info[1].As<Napi::Object>();
    try {
        auto* src_wrap = Napi::ObjectWrap<StaticSourceWrap>::Unwrap(src_obj);
        server_.addSource(name, src_wrap->SourcePtr());
        auto it = sources_.find(name);
        if (it != sources_.end()) {
            it->second.Reset();
            it->second = Napi::Persistent(src_obj);
        } else {
            sources_.emplace(name, Napi::Persistent(src_obj));
        }
    } catch (const std::exception& e) {
        ThrowFromPvxs(env, e);
    }
    return env.Undefined();
}

Napi::Value ServerWrap::RemoveSource(const Napi::CallbackInfo& info) {
    Napi::Env env = info.Env();
    if (info.Length() < 1 || !info[0].IsString()) {
        Napi::TypeError::New(env, "removeSource(name) requires a string")
            .ThrowAsJavaScriptException();
        return env.Undefined();
    }
    const std::string name = info[0].As<Napi::String>().Utf8Value();
    try {
        server_.removeSource(name);
        auto it = sources_.find(name);
        if (it != sources_.end()) {
            it->second.Reset();
            sources_.erase(it);
        }
    } catch (const std::exception& e) {
        ThrowFromPvxs(env, e);
    }
    return env.Undefined();
}

Napi::Value ServerWrap::Start(const Napi::CallbackInfo& info) {
    Napi::Env env = info.Env();
    try {
        server_.start();
    } catch (const std::exception& e) {
        ThrowFromPvxs(env, e);
    }
    return env.Undefined();
}

Napi::Value ServerWrap::Stop(const Napi::CallbackInfo& info) {
    Napi::Env env = info.Env();
    try {
        server_.stop();
    } catch (const std::exception& e) {
        ThrowFromPvxs(env, e);
    }
    return env.Undefined();
}

Napi::Value ServerWrap::ClientConfig(const Napi::CallbackInfo& info) {
    Napi::Env env = info.Env();
    try {
        return ClientConfigWrap::ToJs(env, server_.clientConfig());
    } catch (const std::exception& e) {
        ThrowFromPvxs(env, e);
        return env.Undefined();
    }
}

Napi::Object ServerWrap::Init(Napi::Env env, Napi::Object exports) {
    Napi::Function func = DefineClass(
        env,
        "Server",
        {
            StaticMethod("fromEnv", &ServerWrap::FromEnv),
            StaticMethod("fromIsolated", &ServerWrap::FromIsolated),
            InstanceMethod("addPV", &ServerWrap::AddPV),
            InstanceMethod("removePV", &ServerWrap::RemovePV),
            InstanceMethod("addSource", &ServerWrap::AddSource),
            InstanceMethod("removeSource", &ServerWrap::RemoveSource),
            InstanceMethod("start", &ServerWrap::Start),
            InstanceMethod("stop", &ServerWrap::Stop),
            InstanceMethod("clientConfig", &ServerWrap::ClientConfig),
        });
    constructor_ = Napi::Persistent(func);
    constructor_.SuppressDestruct();
    exports.Set("Server", func);
    return exports;
}

}  // namespace binding
