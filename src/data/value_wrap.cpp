#include "data/value_wrap.h"

#include "common/errors.h"
#include "data/value_convert.h"

#include <pvxs/data.h>

#include <sstream>

namespace binding {
namespace {

bool IsPlainObject(const Napi::Value& val) {
    return val.IsObject() && !val.IsArray() && !val.IsBuffer() &&
           !val.IsTypedArray() && !val.IsDate() && !PvxsValueWrap::IsInstance(val);
}

}  // namespace

Napi::FunctionReference PvxsValueWrap::constructor_;

PvxsValueWrap::PvxsValueWrap(const Napi::CallbackInfo& info)
    : Napi::ObjectWrap<PvxsValueWrap>(info) {}

bool PvxsValueWrap::IsInstance(const Napi::Value& value) {
    if (constructor_.IsEmpty()) {
        return false;
    }
    return value.IsObject() && value.As<Napi::Object>().InstanceOf(constructor_.Value());
}

pvxs::Value PvxsValueWrap::FromJs(const Napi::Value& value) {
    return Napi::ObjectWrap<PvxsValueWrap>::Unwrap(value.As<Napi::Object>())->value_;
}

pvxs::Value ValueFromJsArg(Napi::Env env, const Napi::Value& arg) {
    if (PvxsValueWrap::IsInstance(arg)) {
        return PvxsValueWrap::FromJs(arg);
    }
    Napi::TypeError::New(env, "expected a pvxs.data.Value instance")
        .ThrowAsJavaScriptException();
    return pvxs::Value();
}

Napi::Object PvxsValueWrap::ToJs(Napi::Env env, const pvxs::Value& value) {
    Napi::Object obj = constructor_.New({});
    auto* wrap = Napi::ObjectWrap<PvxsValueWrap>::Unwrap(obj);
    wrap->value_ = value;
    return obj;
}

Napi::Value PvxsValueWrap::GetField(const Napi::CallbackInfo& info) {
    Napi::Env env = info.Env();
    if (info.Length() < 1 || !info[0].IsString()) {
        Napi::TypeError::New(env, "get(name) requires a string").ThrowAsJavaScriptException();
        return env.Undefined();
    }

    const std::string name = info[0].As<Napi::String>().Utf8Value();

    try {
        pvxs::Value field = value_[name];
        if (!field.valid()) {
            return env.Null();
        }
        return ToJs(env, field);
    } catch (const std::exception& e) {
        ThrowFromPvxs(env, e);
        return env.Undefined();
    }
}

Napi::Value PvxsValueWrap::SetField(const Napi::CallbackInfo& info) {
    Napi::Env env = info.Env();
    if (info.Length() < 2 || !info[0].IsString()) {
        Napi::TypeError::New(env, "set(name, value) requires a string name and a value")
            .ThrowAsJavaScriptException();
        return env.Undefined();
    }

    try {
        binding::AssignFieldFromJs(value_, info[0].As<Napi::String>().Utf8Value(), info[1]);
    } catch (const std::exception& e) {
        ThrowFromPvxs(env, e);
    }
    return env.Undefined();
}

Napi::Value PvxsValueWrap::AssignFrom(const Napi::CallbackInfo& info) {
    Napi::Env env = info.Env();
    if (info.Length() < 1) {
        Napi::TypeError::New(env, "assign(value) requires an argument")
            .ThrowAsJavaScriptException();
        return env.Undefined();
    }

    try {
        binding::AssignFromJs(value_, info[0]);
    } catch (const std::exception& e) {
        ThrowFromPvxs(env, e);
    }
    return env.Undefined();
}

Napi::Value PvxsValueWrap::ToObject(const Napi::CallbackInfo& info) {
    Napi::Env env = info.Env();
    try {
        return binding::ValueToObject(env, value_);
    } catch (const std::exception& e) {
        ThrowFromPvxs(env, e);
        return env.Undefined();
    }
}

Napi::Value PvxsValueWrap::AsBool(const Napi::CallbackInfo& info) {
    Napi::Env env = info.Env();
    try {
        return Napi::Boolean::New(env, value_.as<bool>());
    } catch (const std::exception& e) {
        ThrowFromPvxs(env, e);
        return env.Undefined();
    }
}

Napi::Value PvxsValueWrap::AsInt(const Napi::CallbackInfo& info) {
    Napi::Env env = info.Env();
    try {
        return Napi::Number::New(env, static_cast<double>(value_.as<int64_t>()));
    } catch (const std::exception& e) {
        ThrowFromPvxs(env, e);
        return env.Undefined();
    }
}

Napi::Value PvxsValueWrap::AsFloat(const Napi::CallbackInfo& info) {
    Napi::Env env = info.Env();
    try {
        return Napi::Number::New(env, value_.as<double>());
    } catch (const std::exception& e) {
        ThrowFromPvxs(env, e);
        return env.Undefined();
    }
}

Napi::Value PvxsValueWrap::AsString(const Napi::CallbackInfo& info) {
    Napi::Env env = info.Env();
    try {
        return Napi::String::New(env, value_.as<std::string>());
    } catch (const std::exception& e) {
        ThrowFromPvxs(env, e);
        return env.Undefined();
    }
}

Napi::Value PvxsValueWrap::AsArray(const Napi::CallbackInfo& info) {
    Napi::Env env = info.Env();
    try {
        return SharedArrayToJs(env, value_.as<pvxs::shared_array<const void>>());
    } catch (const std::exception& e) {
        ThrowFromPvxs(env, e);
        return env.Undefined();
    }
}

Napi::Value PvxsValueWrap::Clone(const Napi::CallbackInfo& info) {
    Napi::Env env = info.Env();
    try {
        return ToJs(env, value_.clone());
    } catch (const std::exception& e) {
        ThrowFromPvxs(env, e);
        return env.Undefined();
    }
}

Napi::Value PvxsValueWrap::CloneEmpty(const Napi::CallbackInfo& info) {
    Napi::Env env = info.Env();
    try {
        return ToJs(env, value_.cloneEmpty());
    } catch (const std::exception& e) {
        ThrowFromPvxs(env, e);
        return env.Undefined();
    }
}

Napi::Value PvxsValueWrap::EqualInst(const Napi::CallbackInfo& info) {
    Napi::Env env = info.Env();
    if (info.Length() < 1 || !IsInstance(info[0])) {
        return Napi::Boolean::New(env, false);
    }
    return Napi::Boolean::New(env, value_.equalInst(FromJs(info[0])));
}

Napi::Value PvxsValueWrap::EqualType(const Napi::CallbackInfo& info) {
    Napi::Env env = info.Env();
    if (info.Length() < 1 || !IsInstance(info[0])) {
        return Napi::Boolean::New(env, false);
    }
    return Napi::Boolean::New(env, value_.equalType(FromJs(info[0])));
}

Napi::Value PvxsValueWrap::Id(const Napi::CallbackInfo& info) {
    return Napi::String::New(info.Env(), value_.id());
}

Napi::Value PvxsValueWrap::Valid(const Napi::CallbackInfo& info) {
    return Napi::Boolean::New(info.Env(), value_.valid());
}

Napi::Value PvxsValueWrap::NMembers(const Napi::CallbackInfo& info) {
    return Napi::Number::New(info.Env(), static_cast<double>(value_.nmembers()));
}

Napi::Value PvxsValueWrap::StorageType(const Napi::CallbackInfo& info) {
    return Napi::Number::New(info.Env(), static_cast<int>(value_.storageType()));
}

Napi::Value PvxsValueWrap::TypeCode(const Napi::CallbackInfo& info) {
    return Napi::Number::New(info.Env(), static_cast<int>(value_.type().code));
}

Napi::Value PvxsValueWrap::ToStringTag(const Napi::CallbackInfo& info) {
    std::ostringstream ss;
    ss << value_;
    return Napi::String::New(info.Env(), ss.str());
}

Napi::Object PvxsValueWrap::Init(Napi::Env env, Napi::Object exports) {
    Napi::Function func = DefineClass(
        env,
        "PvxsValue",
        {
            InstanceMethod("get", &PvxsValueWrap::GetField),
            InstanceMethod("set", &PvxsValueWrap::SetField),
            InstanceMethod("assign", &PvxsValueWrap::AssignFrom),
            InstanceMethod("toObject", &PvxsValueWrap::ToObject),
            InstanceMethod("asBool", &PvxsValueWrap::AsBool),
            InstanceMethod("asInt", &PvxsValueWrap::AsInt),
            InstanceMethod("asFloat", &PvxsValueWrap::AsFloat),
            InstanceMethod("asString", &PvxsValueWrap::AsString),
            InstanceMethod("asArray", &PvxsValueWrap::AsArray),
            InstanceMethod("clone", &PvxsValueWrap::Clone),
            InstanceMethod("cloneEmpty", &PvxsValueWrap::CloneEmpty),
            InstanceMethod("equalInst", &PvxsValueWrap::EqualInst),
            InstanceMethod("equalType", &PvxsValueWrap::EqualType),
            InstanceMethod("id", &PvxsValueWrap::Id),
            InstanceMethod("valid", &PvxsValueWrap::Valid),
            InstanceMethod("nmembers", &PvxsValueWrap::NMembers),
            InstanceMethod("storageType", &PvxsValueWrap::StorageType),
            InstanceMethod("typeCode", &PvxsValueWrap::TypeCode),
            InstanceMethod("toString", &PvxsValueWrap::ToStringTag),
        });

    constructor_ = Napi::Persistent(func);
    constructor_.SuppressDestruct();
    exports.Set("Value", func);
    return exports;
}

void AssignFieldFromJs(pvxs::Value& parent, const std::string& name, const Napi::Value& js) {
    pvxs::Value field = parent.lookup(name);

    if (PvxsValueWrap::IsInstance(js)) {
        field.assign(PvxsValueWrap::FromJs(js));
        return;
    }

    if (field.type().isarray()) {
        if (!js.IsArray()) {
            throw pvxs::NoConvert("Array fields require a JavaScript Array");
        }
        AssignArrayFromJs(field, js.As<Napi::Array>());
        return;
    }

    if (IsPlainObject(js) || (js.IsObject() && field.nmembers() > 0)) {
        AssignFromJs(field, js);
        return;
    }

    AssignScalarFromJs(field, js);
}

void AssignFromJs(pvxs::Value& dest, const Napi::Value& js) {
    if (PvxsValueWrap::IsInstance(js)) {
        dest.assign(PvxsValueWrap::FromJs(js));
        return;
    }

    if (!js.IsObject() || js.IsArray()) {
        AssignScalarFromJs(dest, js);
        return;
    }

    Napi::Object obj = js.As<Napi::Object>();
    Napi::Array names = obj.GetPropertyNames();
    for (uint32_t i = 0; i < names.Length(); ++i) {
        const std::string key = names.Get(i).As<Napi::String>().Utf8Value();
        AssignFieldFromJs(dest, key, obj.Get(key));
    }
}

}  // namespace binding
