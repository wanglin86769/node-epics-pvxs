#pragma once

#include <napi.h>

#include <pvxs/data.h>

namespace binding {

pvxs::TypeCode::code_t ParseTypeCode(const Napi::Value& val);

void InitTypeDefApi(Napi::Env env, Napi::Object exports);

}  // namespace binding
