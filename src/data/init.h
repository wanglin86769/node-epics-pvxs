#pragma once

#include <napi.h>

namespace binding {

void InitDataModule(Napi::Env env, Napi::Object exports);

}  // namespace binding
