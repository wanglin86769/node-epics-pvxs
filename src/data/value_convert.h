#pragma once

#include <napi.h>

#include <pvxs/data.h>

#include <string>

namespace binding {

Napi::Value SharedArrayToJs(const Napi::Env& env,
                            const pvxs::shared_array<const void>& sa);

void AssignScalarFromJs(pvxs::Value& dest, const Napi::Value& js);

void AssignArrayFromJs(pvxs::Value& field, const Napi::Array& arr);

Napi::Value ScalarToJs(const Napi::Env& env, const pvxs::Value& v);

Napi::Value ValueToObject(const Napi::Env& env, const pvxs::Value& v);

}  // namespace binding
