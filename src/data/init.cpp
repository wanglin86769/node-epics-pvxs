#include "data/init.h"

#include "common/errors.h"
#include "data/typedef.h"
#include "data/value_wrap.h"

#include <pvxs/data.h>
#include <pvxs/nt.h>

namespace binding {
namespace {

using namespace pvxs;

Napi::Object MakeTypeCodeEnum(Napi::Env env) {
    Napi::Object tc = Napi::Object::New(env);
    tc.Set("Bool", static_cast<int>(TypeCode::Bool));
    tc.Set("BoolA", static_cast<int>(TypeCode::BoolA));
    tc.Set("Int8", static_cast<int>(TypeCode::Int8));
    tc.Set("Int16", static_cast<int>(TypeCode::Int16));
    tc.Set("Int32", static_cast<int>(TypeCode::Int32));
    tc.Set("Int64", static_cast<int>(TypeCode::Int64));
    tc.Set("UInt8", static_cast<int>(TypeCode::UInt8));
    tc.Set("UInt16", static_cast<int>(TypeCode::UInt16));
    tc.Set("UInt32", static_cast<int>(TypeCode::UInt32));
    tc.Set("UInt64", static_cast<int>(TypeCode::UInt64));
    tc.Set("Int8A", static_cast<int>(TypeCode::Int8A));
    tc.Set("Int16A", static_cast<int>(TypeCode::Int16A));
    tc.Set("Int32A", static_cast<int>(TypeCode::Int32A));
    tc.Set("Int64A", static_cast<int>(TypeCode::Int64A));
    tc.Set("UInt8A", static_cast<int>(TypeCode::UInt8A));
    tc.Set("UInt16A", static_cast<int>(TypeCode::UInt16A));
    tc.Set("UInt32A", static_cast<int>(TypeCode::UInt32A));
    tc.Set("UInt64A", static_cast<int>(TypeCode::UInt64A));
    tc.Set("Float32", static_cast<int>(TypeCode::Float32));
    tc.Set("Float64", static_cast<int>(TypeCode::Float64));
    tc.Set("Float32A", static_cast<int>(TypeCode::Float32A));
    tc.Set("Float64A", static_cast<int>(TypeCode::Float64A));
    tc.Set("String", static_cast<int>(TypeCode::String));
    tc.Set("StringA", static_cast<int>(TypeCode::StringA));
    tc.Set("Struct", static_cast<int>(TypeCode::Struct));
    tc.Set("Union", static_cast<int>(TypeCode::Union));
    tc.Set("Any", static_cast<int>(TypeCode::Any));
    tc.Set("StructA", static_cast<int>(TypeCode::StructA));
    tc.Set("UnionA", static_cast<int>(TypeCode::UnionA));
    tc.Set("AnyA", static_cast<int>(TypeCode::AnyA));
    tc.Set("Null", static_cast<int>(TypeCode::Null));
    return tc;
}

Napi::Object MakeStoreTypeEnum(Napi::Env env) {
    Napi::Object st = Napi::Object::New(env);
    st.Set("Null", static_cast<int>(StoreType::Null));
    st.Set("Bool", static_cast<int>(StoreType::Bool));
    st.Set("UInteger", static_cast<int>(StoreType::UInteger));
    st.Set("Integer", static_cast<int>(StoreType::Integer));
    st.Set("Real", static_cast<int>(StoreType::Real));
    st.Set("String", static_cast<int>(StoreType::String));
    st.Set("Compound", static_cast<int>(StoreType::Compound));
    st.Set("Array", static_cast<int>(StoreType::Array));
    return st;
}

void ApplyNTScalarOptions(nt::NTScalar& builder, const Napi::Object& opt) {
    if (opt.Has("display") && opt.Get("display").IsBoolean()) {
        builder.display = opt.Get("display").As<Napi::Boolean>().Value();
    }
    if (opt.Has("control") && opt.Get("control").IsBoolean()) {
        builder.control = opt.Get("control").As<Napi::Boolean>().Value();
    }
    if (opt.Has("valueAlarm") && opt.Get("valueAlarm").IsBoolean()) {
        builder.valueAlarm = opt.Get("valueAlarm").As<Napi::Boolean>().Value();
    }
    if (opt.Has("form") && opt.Get("form").IsBoolean()) {
        builder.form = opt.Get("form").As<Napi::Boolean>().Value();
    }
}

Napi::Value NTScalarCreate(const Napi::CallbackInfo& info) {
    Napi::Env env = info.Env();
    if (info.Length() < 1 || !info[0].IsNumber()) {
        Napi::TypeError::New(env, "NTScalar.create(typeCode[, initial[, options]])")
            .ThrowAsJavaScriptException();
        return env.Undefined();
    }

    try {
        const auto code = ParseTypeCode(info[0]);
        nt::NTScalar builder(code);
        if (info.Length() > 2 && info[2].IsObject()) {
            ApplyNTScalarOptions(builder, info[2].As<Napi::Object>());
        }
        Value val = builder.create();
        if (info.Length() > 1 && info[1].IsObject()) {
            AssignFromJs(val, info[1]);
        }
        return PvxsValueWrap::ToJs(env, val);
    } catch (const std::exception& e) {
        ThrowFromPvxs(env, e);
        return env.Undefined();
    }
}

Napi::Value NTEnumCreate(const Napi::CallbackInfo& info) {
    Napi::Env env = info.Env();
    try {
        Value val = nt::NTEnum{}.create();
        if (info.Length() > 0) {
            AssignFromJs(val, info[0]);
        }
        return PvxsValueWrap::ToJs(env, val);
    } catch (const std::exception& e) {
        ThrowFromPvxs(env, e);
        return env.Undefined();
    }
}

}  // namespace

void InitDataModule(Napi::Env env, Napi::Object exports) {
    PvxsValueWrap::Init(env, exports);
    exports.Set("TypeCode", MakeTypeCodeEnum(env));
    exports.Set("StoreType", MakeStoreTypeEnum(env));
    exports.Set("NTScalar", Napi::Object::New(env));
    exports.Get("NTScalar").As<Napi::Object>().Set(
        "create", Napi::Function::New(env, NTScalarCreate));
    exports.Set("NTEnum", Napi::Object::New(env));
    exports.Get("NTEnum").As<Napi::Object>().Set(
        "create", Napi::Function::New(env, NTEnumCreate));
    InitTypeDefApi(env, exports);
}

}  // namespace binding
