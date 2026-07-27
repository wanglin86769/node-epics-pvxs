#include "data/value_convert.h"

#include <pvxs/data.h>

#include <cstdint>
#include <vector>

namespace binding {
namespace {

using namespace pvxs;

template <typename T>
shared_array<const void> VectorToSharedArray(const std::vector<T>& src) {
    shared_array<const T> arr(src.begin(), src.end());
    return arr.template castTo<const void>();
}

Napi::Array SharedArrayToStringArray(const Napi::Env& env,
                                     const shared_array<const void>& sa) {
    auto arr = sa.castTo<const std::string>();
    Napi::Array out = Napi::Array::New(env, arr.size());
    for (size_t i = 0; i < arr.size(); ++i) {
        out.Set(i, arr[i]);
    }
    return out;
}

template <typename T>
Napi::Array SharedArrayToNumberList(const Napi::Env& env,
                                    const shared_array<const void>& sa) {
    auto arr = sa.castTo<const T>();
    Napi::Array out = Napi::Array::New(env, arr.size());
    for (size_t i = 0; i < arr.size(); ++i) {
        out.Set(i, arr[i]);
    }
    return out;
}

Napi::Array SharedArrayToBoolList(const Napi::Env& env,
                                  const shared_array<const void>& sa) {
    auto arr = sa.castTo<const bool>();
    Napi::Array out = Napi::Array::New(env, arr.size());
    for (size_t i = 0; i < arr.size(); ++i) {
        out.Set(i, Napi::Boolean::New(env, arr[i]));
    }
    return out;
}

void LoadNumericSequence(const Napi::Array& arr, bool is_float, Value& field) {
    const size_t len = arr.Length();
    if (is_float) {
        std::vector<double> values(len);
        for (size_t i = 0; i < len; ++i) {
            values[i] = arr.Get(i).As<Napi::Number>().DoubleValue();
        }
        field.from(VectorToSharedArray(values));
    } else {
        std::vector<int64_t> values(len);
        for (size_t i = 0; i < len; ++i) {
            values[i] = static_cast<int64_t>(arr.Get(i).As<Napi::Number>().Int64Value());
        }
        field.from(VectorToSharedArray(values));
    }
}

void LoadStringSequence(const Napi::Array& arr, Value& field) {
    std::vector<std::string> values(arr.Length());
    for (size_t i = 0; i < arr.Length(); ++i) {
        values[i] = arr.Get(i).As<Napi::String>().Utf8Value();
    }
    field.from(VectorToSharedArray(values));
}

void LoadBoolSequence(const Napi::Array& arr, Value& field) {
    const size_t len = arr.Length();
    std::vector<bool> values(len);
    for (size_t i = 0; i < len; ++i) {
        Napi::Value item = arr.Get(i);
        if (item.IsBoolean()) {
            values[i] = item.As<Napi::Boolean>().Value();
        } else if (item.IsNumber()) {
            values[i] = item.As<Napi::Number>().Uint32Value() != 0;
        } else {
            throw NoConvert("BoolA array requires boolean[]");
        }
    }
    shared_array<const bool> barr(values.begin(), values.end());
    field.from(barr.template castTo<const void>());
}

}  // namespace

using namespace pvxs;

void AssignScalarFromJs(Value& dest, const Napi::Value& js) {
    if (js.IsBoolean()) {
        dest.from(js.As<Napi::Boolean>().Value());
    } else if (js.IsNumber()) {
        const double num = js.As<Napi::Number>().DoubleValue();
        if (dest.type().code == TypeCode::Float32 ||
            dest.type().code == TypeCode::Float64 ||
            dest.type().code == TypeCode::Float32A ||
            dest.type().code == TypeCode::Float64A ||
            dest.storageType() == StoreType::Real) {
            dest.from(num);
        } else if (dest.type().isunsigned()) {
            dest.from(static_cast<uint64_t>(num));
        } else {
            dest.from(static_cast<int64_t>(num));
        }
    } else if (js.IsString()) {
        dest.from(js.As<Napi::String>().Utf8Value());
    } else {
        throw NoConvert("Unsupported scalar assignment");
    }
}

void AssignArrayFromJs(Value& field, const Napi::Array& arr) {
    if (field.type().code == TypeCode::StringA) {
        LoadStringSequence(arr, field);
    } else if (field.type().code == TypeCode::BoolA) {
        LoadBoolSequence(arr, field);
    } else {
        LoadNumericSequence(arr, field.type().kind() == Kind::Real, field);
    }
}

Napi::Value SharedArrayToJs(const Napi::Env& env,
                            const shared_array<const void>& sa) {
    switch (sa.original_type()) {
        case ArrayType::String:
            return SharedArrayToStringArray(env, sa);
        case ArrayType::Bool:
            return SharedArrayToBoolList(env, sa);
        case ArrayType::UInt8:
            return SharedArrayToNumberList<uint8_t>(env, sa);
        case ArrayType::UInt16:
            return SharedArrayToNumberList<uint16_t>(env, sa);
        case ArrayType::UInt32:
            return SharedArrayToNumberList<uint32_t>(env, sa);
        case ArrayType::UInt64:
            return SharedArrayToNumberList<uint64_t>(env, sa);
        case ArrayType::Int8:
            return SharedArrayToNumberList<int8_t>(env, sa);
        case ArrayType::Int16:
            return SharedArrayToNumberList<int16_t>(env, sa);
        case ArrayType::Int32:
            return SharedArrayToNumberList<int32_t>(env, sa);
        case ArrayType::Int64:
            return SharedArrayToNumberList<int64_t>(env, sa);
        case ArrayType::Float32:
            return SharedArrayToNumberList<float>(env, sa);
        case ArrayType::Float64:
            return SharedArrayToNumberList<double>(env, sa);
        default:
            throw std::runtime_error("Array conversion not implemented");
    }
}

Napi::Value ScalarToJs(const Napi::Env& env, const Value& v) {
    switch (v.storageType()) {
        case StoreType::Bool:
            return Napi::Boolean::New(env, v.as<bool>());
        case StoreType::Integer:
            return Napi::Number::New(env, static_cast<double>(v.as<int64_t>()));
        case StoreType::UInteger:
            return Napi::Number::New(env, static_cast<double>(v.as<uint64_t>()));
        case StoreType::Real:
            return Napi::Number::New(env, v.as<double>());
        case StoreType::String:
            return Napi::String::New(env, v.as<std::string>());
        case StoreType::Array:
            return SharedArrayToJs(env, v.as<shared_array<const void>>());
        default:
            return env.Null();
    }
}

Napi::Value ValueToObject(const Napi::Env& env, const Value& v) {
    if (v.nmembers() == 0) {
        return ScalarToJs(env, v);
    }

    Napi::Object out = Napi::Object::New(env);
    for (const auto& child : v.ichildren()) {
        const std::string name = v.nameOf(child);
        if (child.nmembers() > 0) {
            out.Set(name, ValueToObject(env, child));
        } else {
            out.Set(name, ScalarToJs(env, child));
        }
    }
    return out;
}

}  // namespace binding
