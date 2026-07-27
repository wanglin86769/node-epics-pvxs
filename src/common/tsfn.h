#pragma once

#include <napi.h>

namespace binding {

Napi::ThreadSafeFunction MakeTsfn(Napi::Env env, const char* name);

}  // namespace binding
