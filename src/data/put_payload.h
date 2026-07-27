#pragma once

#include <napi.h>

#include <pvxs/data.h>

#include <cstdint>
#include <string>
#include <vector>

namespace binding {

/** One flat field assignment for put (path may be empty = root). */
struct PutField {
    std::string path;

    enum class FieldType { Bool, Int, Float, String, BoolArray, IntArray, FloatArray, StringArray };
    FieldType field_type = FieldType::Int;

    bool b = false;
    int64_t i64 = 0;
    double f64 = 0.0;
    std::string str;

    std::vector<bool> bools;
    std::vector<int64_t> i64s;
    std::vector<double> f64s;
    std::vector<std::string> strs;
};

/**
 * Put payload prepared on the main thread.
 * Either `value` is set (from a Value instance), or `fields` lists field updates.
 */
struct PutPayload {
    pvxs::Value value;
    std::vector<PutField> fields;
};

struct ParsePutPayloadResult {
    PutPayload payload;
    std::string error;  // empty on success
};

/** Main thread only: JS object/scalar/Value/array -> PutPayload. */
ParsePutPayloadResult ParsePutPayload(const Napi::Value& data);

/** Worker / any thread: apply payload onto a typed destination Value. */
void ApplyPutPayload(pvxs::Value& dest, const PutPayload& payload);

}  // namespace binding
