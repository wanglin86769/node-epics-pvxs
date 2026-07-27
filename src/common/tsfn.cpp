#include "common/tsfn.h"

namespace binding {

/** Create a TSFN with an empty JS placeholder, unlimited queue, thread count 1. */
Napi::ThreadSafeFunction MakeTsfn(Napi::Env env, const char* name) {
    return Napi::ThreadSafeFunction::New(
        env,
        Napi::Function::New(env, [](const Napi::CallbackInfo&) {}),
        name,
        0,
        1);
}

}  // namespace binding
