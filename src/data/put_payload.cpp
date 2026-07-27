#include "data/put_payload.h"

#include "data/value_wrap.h"

#include <cmath>
#include <stdexcept>

namespace binding {
namespace {

using pvxs::NoConvert;
using pvxs::shared_array;
using pvxs::StoreType;
using pvxs::TypeCode;
using pvxs::Value;

template <typename T>
shared_array<const void> VectorToSharedArray(const std::vector<T>& src) {
    shared_array<const T> arr(src.begin(), src.end());
    return arr.template castTo<const void>();
}

bool IsPlainObject(const Napi::Value& val) {
    return val.IsObject() && !val.IsArray() && !val.IsBuffer() &&
           !val.IsTypedArray() && !val.IsDate() && !PvxsValueWrap::IsInstance(val);
}

PutField ParseScalarField(const std::string& path, const Napi::Value& js) {
    PutField entry;
    entry.path = path;
    if (js.IsBoolean()) {
        entry.field_type = PutField::FieldType::Bool;
        entry.b = js.As<Napi::Boolean>().Value();
    } else if (js.IsNumber()) {
        const double num = js.As<Napi::Number>().DoubleValue();
        double intpart = 0.0;
        if (std::modf(num, &intpart) == 0.0) {
            entry.field_type = PutField::FieldType::Int;
            entry.i64 = static_cast<int64_t>(num);
        } else {
            entry.field_type = PutField::FieldType::Float;
            entry.f64 = num;
        }
    } else if (js.IsString()) {
        entry.field_type = PutField::FieldType::String;
        entry.str = js.As<Napi::String>().Utf8Value();
    } else {
        throw NoConvert("Unsupported scalar type in put payload");
    }
    return entry;
}

PutField ParseArrayField(const std::string& path, const Napi::Array& arr) {
    PutField entry;
    entry.path = path;
    const uint32_t len = arr.Length();
    if (len == 0) {
        // Empty array: treat as int array; Apply selects by field type.
        entry.field_type = PutField::FieldType::IntArray;
        return entry;
    }

    const Napi::Value first = arr.Get(static_cast<uint32_t>(0));
    if (first.IsBoolean()) {
        entry.field_type = PutField::FieldType::BoolArray;
        entry.bools.resize(len);
        for (uint32_t i = 0; i < len; ++i) {
            Napi::Value item = arr.Get(i);
            if (item.IsBoolean()) {
                entry.bools[i] = item.As<Napi::Boolean>().Value();
            } else if (item.IsNumber()) {
                entry.bools[i] = item.As<Napi::Number>().Uint32Value() != 0;
            } else {
                throw NoConvert("Bool array requires boolean/number elements");
            }
        }
        return entry;
    }

    if (first.IsString()) {
        entry.field_type = PutField::FieldType::StringArray;
        entry.strs.resize(len);
        for (uint32_t i = 0; i < len; ++i) {
            Napi::Value item = arr.Get(i);
            if (!item.IsString()) {
                throw NoConvert("String array requires string elements");
            }
            entry.strs[i] = item.As<Napi::String>().Utf8Value();
        }
        return entry;
    }

    if (first.IsNumber()) {
        bool any_float = false;
        for (uint32_t i = 0; i < len; ++i) {
            Napi::Value item = arr.Get(i);
            if (!item.IsNumber()) {
                throw NoConvert("Number array requires number elements");
            }
            const double num = item.As<Napi::Number>().DoubleValue();
            double intpart = 0.0;
            if (std::modf(num, &intpart) != 0.0) {
                any_float = true;
            }
        }
        if (any_float) {
            entry.field_type = PutField::FieldType::FloatArray;
            entry.f64s.resize(len);
            for (uint32_t i = 0; i < len; ++i) {
                entry.f64s[i] = arr.Get(i).As<Napi::Number>().DoubleValue();
            }
        } else {
            entry.field_type = PutField::FieldType::IntArray;
            entry.i64s.resize(len);
            for (uint32_t i = 0; i < len; ++i) {
                entry.i64s[i] =
                    static_cast<int64_t>(arr.Get(i).As<Napi::Number>().Int64Value());
            }
        }
        return entry;
    }

    throw NoConvert("Unsupported array element type in put payload");
}

void CollectPutFields(const Napi::Value& js,
               const std::string& prefix,
               std::vector<PutField>& fields) {
    if (js.IsArray()) {
        fields.push_back(ParseArrayField(prefix, js.As<Napi::Array>()));
        return;
    }

    if (!js.IsObject() || !IsPlainObject(js)) {
        fields.push_back(ParseScalarField(prefix, js));
        return;
    }

    Napi::Object obj = js.As<Napi::Object>();
    Napi::Array names = obj.GetPropertyNames();
    for (uint32_t i = 0; i < names.Length(); ++i) {
        const std::string key = names.Get(i).As<Napi::String>().Utf8Value();
        const std::string path = prefix.empty() ? key : (prefix + "." + key);
        const Napi::Value child = obj.Get(key);

        if (child.IsArray()) {
            fields.push_back(ParseArrayField(path, child.As<Napi::Array>()));
        } else if (IsPlainObject(child)) {
            CollectPutFields(child, path, fields);
        } else {
            fields.push_back(ParseScalarField(path, child));
        }
    }
}

void ApplyScalar(Value& target, const PutField& entry) {
    switch (entry.field_type) {
        case PutField::FieldType::Bool:
            target.from(entry.b);
            break;
        case PutField::FieldType::Int:
            if (target.type().code == TypeCode::Float32 ||
                target.type().code == TypeCode::Float64 ||
                target.storageType() == StoreType::Real) {
                target.from(static_cast<double>(entry.i64));
            } else if (target.type().isunsigned()) {
                target.from(static_cast<uint64_t>(entry.i64));
            } else {
                target.from(entry.i64);
            }
            break;
        case PutField::FieldType::Float:
            target.from(entry.f64);
            break;
        case PutField::FieldType::String:
            target.from(entry.str);
            break;
        default:
            throw NoConvert("Internal error: non-scalar field in ApplyScalar");
    }
}

void ApplyArray(Value& target, const PutField& entry) {
    if (!target.type().isarray()) {
        throw NoConvert("Array put payload requires an array field");
    }

    if (target.type().code == TypeCode::StringA) {
        if (entry.field_type != PutField::FieldType::StringArray) {
            throw NoConvert("StringA field requires a string[]");
        }
        target.from(VectorToSharedArray(entry.strs));
        return;
    }

    if (target.type().code == TypeCode::BoolA) {
        if (entry.field_type != PutField::FieldType::BoolArray) {
            throw NoConvert("BoolA field requires a boolean[]");
        }
        shared_array<const bool> arr(entry.bools.begin(), entry.bools.end());
        target.from(arr.template castTo<const void>());
        return;
    }

    const bool is_float = target.type().kind() == pvxs::Kind::Real;
    if (is_float) {
        if (entry.field_type == PutField::FieldType::FloatArray) {
            target.from(VectorToSharedArray(entry.f64s));
        } else if (entry.field_type == PutField::FieldType::IntArray) {
            std::vector<double> values(entry.i64s.begin(), entry.i64s.end());
            target.from(VectorToSharedArray(values));
        } else {
            throw NoConvert("Numeric array field requires a number[]");
        }
        return;
    }

    if (entry.field_type == PutField::FieldType::IntArray) {
        target.from(VectorToSharedArray(entry.i64s));
    } else if (entry.field_type == PutField::FieldType::FloatArray) {
        std::vector<int64_t> values(entry.f64s.size());
        for (size_t i = 0; i < entry.f64s.size(); ++i) {
            values[i] = static_cast<int64_t>(entry.f64s[i]);
        }
        target.from(VectorToSharedArray(values));
    } else {
        throw NoConvert("Numeric array field requires a number[]");
    }
}

void ApplyField(Value& dest, const PutField& entry) {
    Value target = entry.path.empty() ? dest : dest.lookup(entry.path);
    switch (entry.field_type) {
        case PutField::FieldType::Bool:
        case PutField::FieldType::Int:
        case PutField::FieldType::Float:
        case PutField::FieldType::String:
            ApplyScalar(target, entry);
            break;
        case PutField::FieldType::BoolArray:
        case PutField::FieldType::IntArray:
        case PutField::FieldType::FloatArray:
        case PutField::FieldType::StringArray:
            ApplyArray(target, entry);
            break;
    }
}

}  // namespace

ParsePutPayloadResult ParsePutPayload(const Napi::Value& data) {
    ParsePutPayloadResult out;
    try {
        if (PvxsValueWrap::IsInstance(data)) {
            out.payload.value = PvxsValueWrap::FromJs(data).clone();
            return out;
        }
        CollectPutFields(data, "", out.payload.fields);
    } catch (const std::exception& e) {
        // Includes Napi::Error when NAPI_CPP_EXCEPTIONS is enabled (P1).
        out.error = e.what();
        out.payload = {};
    }
    return out;
}

void ApplyPutPayload(Value& dest, const PutPayload& payload) {
    if (payload.value) {
        dest.assign(payload.value);
        return;
    }
    for (const auto& entry : payload.fields) {
        ApplyField(dest, entry);
    }
}

}  // namespace binding
