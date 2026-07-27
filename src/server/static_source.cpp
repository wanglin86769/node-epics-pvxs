#include "server/static_source.h"

#include "common/errors.h"
#include "server/sharedpv.h"

namespace binding {

Napi::FunctionReference StaticSourceWrap::constructor_;

bool StaticSourceWrap::IsInstance(const Napi::Value& value) {
    if (constructor_.IsEmpty()) {
        return false;
    }
    return value.IsObject() &&
           value.As<Napi::Object>().InstanceOf(constructor_.Value());
}

StaticSourceWrap::StaticSourceWrap(const Napi::CallbackInfo& info)
    : Napi::ObjectWrap<StaticSourceWrap>(info) {
    src_ = pvxs::server::StaticSource::build();
}

void StaticSourceWrap::ClearRefs() {
    for (auto& entry : pvs_) {
        entry.second.Reset();
    }
    pvs_.clear();
}

StaticSourceWrap::~StaticSourceWrap() {
    try {
        if (src_) {
            src_.close();
        }
    } catch (...) {
        // Do not throw from destructor.
    }
    ClearRefs();
}

Napi::Value StaticSourceWrap::Build(const Napi::CallbackInfo& info) {
    return constructor_.New({});
}

Napi::Value StaticSourceWrap::Add(const Napi::CallbackInfo& info) {
    Napi::Env env = info.Env();
    if (info.Length() < 2 || !info[0].IsString() || !SharedPVWrap::IsInstance(info[1])) {
        Napi::TypeError::New(env, "add(name, sharedPV) requires a string and SharedPV")
            .ThrowAsJavaScriptException();
        return env.Undefined();
    }
    const std::string name = info[0].As<Napi::String>().Utf8Value();
    Napi::Object pv_obj = info[1].As<Napi::Object>();
    try {
        src_.add(name, SharedPVWrap::UnwrapPV(pv_obj));
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

Napi::Value StaticSourceWrap::Remove(const Napi::CallbackInfo& info) {
    Napi::Env env = info.Env();
    if (info.Length() < 1 || !info[0].IsString()) {
        Napi::TypeError::New(env, "remove(name) requires a string")
            .ThrowAsJavaScriptException();
        return env.Undefined();
    }
    const std::string name = info[0].As<Napi::String>().Utf8Value();
    try {
        src_.remove(name);
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

Napi::Value StaticSourceWrap::List(const Napi::CallbackInfo& info) {
    Napi::Env env = info.Env();
    try {
        Napi::Object out = Napi::Object::New(env);
        for (const auto& entry : src_.list()) {
            auto it = pvs_.find(entry.first);
            if (it != pvs_.end() && !it->second.IsEmpty()) {
                out.Set(entry.first, it->second.Value());
            } else {
                out.Set(entry.first, SharedPVWrap::Create(env, entry.second));
            }
        }
        return out;
    } catch (const std::exception& e) {
        ThrowFromPvxs(env, e);
        return env.Undefined();
    }
}

Napi::Value StaticSourceWrap::Close(const Napi::CallbackInfo& info) {
    try {
        src_.close();
        ClearRefs();
    } catch (const std::exception& e) {
        ThrowFromPvxs(info.Env(), e);
    }
    return info.Env().Undefined();
}

Napi::Object StaticSourceWrap::Init(Napi::Env env, Napi::Object exports) {
    Napi::Function func = DefineClass(
        env,
        "StaticSource",
        {
            StaticMethod("build", &StaticSourceWrap::Build),
            InstanceMethod("add", &StaticSourceWrap::Add),
            InstanceMethod("remove", &StaticSourceWrap::Remove),
            InstanceMethod("list", &StaticSourceWrap::List),
            InstanceMethod("close", &StaticSourceWrap::Close),
        });
    constructor_ = Napi::Persistent(func);
    constructor_.SuppressDestruct();
    exports.Set("StaticSource", func);
    return exports;
}

}  // namespace binding
