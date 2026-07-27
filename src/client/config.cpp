#include "client/config.h"

#include "common/errors.h"

#include <map>
#include <stdexcept>
#include <string>
#include <utility>

namespace binding {
namespace {

pvxs::client::Config::defs_t ParseDefsObject(const Napi::Value& value) {
    if (!value.IsObject() || value.IsArray()) {
        throw std::invalid_argument("Config defs must be a plain object");
    }
    Napi::Object obj = value.As<Napi::Object>();
    Napi::Array names = obj.GetPropertyNames();
    pvxs::client::Config::defs_t defs;
    for (uint32_t i = 0; i < names.Length(); ++i) {
        const std::string key = names.Get(i).As<Napi::String>().Utf8Value();
        Napi::Value val = obj.Get(key);
        if (val.IsString()) {
            defs[key] = val.As<Napi::String>().Utf8Value();
        } else if (val.IsNumber()) {
            defs[key] = val.ToString().Utf8Value();
        } else if (val.IsBoolean()) {
            defs[key] = val.As<Napi::Boolean>().Value() ? "YES" : "NO";
        } else if (val.IsNull() || val.IsUndefined()) {
            continue;
        } else {
            throw std::invalid_argument(
                "Config def \"" + key + "\" must be a string, number, or boolean");
        }
    }
    return defs;
}

}  // namespace

Napi::FunctionReference ClientConfigWrap::constructor_;

ClientConfigWrap::ClientConfigWrap(const Napi::CallbackInfo& info)
    : Napi::ObjectWrap<ClientConfigWrap>(info) {}

bool ClientConfigWrap::IsInstance(const Napi::Value& value) {
    if (constructor_.IsEmpty()) {
        return false;
    }
    return value.IsObject() &&
           value.As<Napi::Object>().InstanceOf(constructor_.Value());
}

pvxs::client::Config ClientConfigWrap::FromJs(const Napi::Value& value) {
    return Napi::ObjectWrap<ClientConfigWrap>::Unwrap(value.As<Napi::Object>())
        ->config_;
}

pvxs::client::Config ClientConfigWrap::FromDefsObject(const Napi::Value& value) {
    pvxs::client::Config conf;
    conf.applyDefs(ParseDefsObject(value));
    return conf;
}

Napi::Object ClientConfigWrap::ToJs(Napi::Env env, pvxs::client::Config config) {
    Napi::Object obj = constructor_.New({});
    Napi::ObjectWrap<ClientConfigWrap>::Unwrap(obj)->config_ = std::move(config);
    return obj;
}

Napi::Value ClientConfigWrap::FromEnv(const Napi::CallbackInfo& info) {
    Napi::Env env = info.Env();
    try {
        return ToJs(env, pvxs::client::Config::fromEnv());
    } catch (const std::exception& e) {
        ThrowFromPvxs(env, e);
        return env.Undefined();
    }
}

Napi::Value ClientConfigWrap::FromDefs(const Napi::CallbackInfo& info) {
    Napi::Env env = info.Env();
    if (info.Length() < 1) {
        Napi::TypeError::New(env, "Config.fromDefs(defs) requires an object")
            .ThrowAsJavaScriptException();
        return env.Undefined();
    }
    try {
        return ToJs(env, FromDefsObject(info[0]));
    } catch (const std::exception& e) {
        ThrowFromPvxs(env, e);
        return env.Undefined();
    }
}

Napi::Object ClientConfigWrap::Init(Napi::Env env, Napi::Object exports) {
    Napi::Function func = DefineClass(
        env,
        "Config",
        {
            StaticMethod("fromEnv", &ClientConfigWrap::FromEnv),
            StaticMethod("fromDefs", &ClientConfigWrap::FromDefs),
        });
    constructor_ = Napi::Persistent(func);
    constructor_.SuppressDestruct();
    exports.Set("Config", func);
    return exports;
}

}  // namespace binding
