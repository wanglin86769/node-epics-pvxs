#pragma once

#include <napi.h>

#include <pvxs/data.h>

#include <string>

namespace binding {

// MSVC can confuse Napi::Value with pvxs::Value inside this wrapper class.
using JsValue = ::Napi::Value;
using JsObject = ::Napi::Object;
using JsEnv = ::Napi::Env;

class PvxsValueWrap : public Napi::ObjectWrap<PvxsValueWrap> {
public:
    static JsObject Init(JsEnv env, JsObject exports);
    static JsObject ToJs(JsEnv env, const pvxs::Value& value);
    static pvxs::Value FromJs(const JsValue& value);
    static bool IsInstance(const JsValue& value);

    pvxs::Value& value() { return value_; }
    const pvxs::Value& value() const { return value_; }

    explicit PvxsValueWrap(const Napi::CallbackInfo& info);

private:
    static Napi::FunctionReference constructor_;

    JsValue GetField(const Napi::CallbackInfo& info);
    JsValue SetField(const Napi::CallbackInfo& info);
    JsValue AssignFrom(const Napi::CallbackInfo& info);
    JsValue ToObject(const Napi::CallbackInfo& info);
    JsValue AsBool(const Napi::CallbackInfo& info);
    JsValue AsInt(const Napi::CallbackInfo& info);
    JsValue AsFloat(const Napi::CallbackInfo& info);
    JsValue AsString(const Napi::CallbackInfo& info);
    JsValue AsArray(const Napi::CallbackInfo& info);
    JsValue Clone(const Napi::CallbackInfo& info);
    JsValue CloneEmpty(const Napi::CallbackInfo& info);
    JsValue EqualInst(const Napi::CallbackInfo& info);
    JsValue EqualType(const Napi::CallbackInfo& info);
    JsValue Id(const Napi::CallbackInfo& info);
    JsValue Valid(const Napi::CallbackInfo& info);
    JsValue NMembers(const Napi::CallbackInfo& info);
    JsValue StorageType(const Napi::CallbackInfo& info);
    JsValue TypeCode(const Napi::CallbackInfo& info);
    JsValue ToStringTag(const Napi::CallbackInfo& info);

    pvxs::Value value_;
};

void AssignFieldFromJs(pvxs::Value& parent, const std::string& name, const Napi::Value& js);

void AssignFromJs(pvxs::Value& dest, const Napi::Value& js);

pvxs::Value ValueFromJsArg(Napi::Env env, const Napi::Value& arg);

}  // namespace binding
