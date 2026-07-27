#pragma once

#include <napi.h>

namespace binding {

void InitClientModule(Napi::Env env, Napi::Object exports);

}  // namespace binding
