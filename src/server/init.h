#pragma once

#include <napi.h>

namespace binding {

void InitServerModule(Napi::Env env, Napi::Object exports);

}  // namespace binding
