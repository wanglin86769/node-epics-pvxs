#include "common/errors.h"

namespace binding {

Napi::Error ErrorFromMessage(Napi::Env env, const std::string& message) {
    return Napi::Error::New(env, message);
}

Napi::Error ErrorFromPvxs(Napi::Env env, const std::exception& e) {
    return ErrorFromMessage(env, e.what());
}

void ThrowFromPvxs(Napi::Env env, const std::exception& e) {
    ErrorFromPvxs(env, e).ThrowAsJavaScriptException();
}

void ThrowFromMessage(Napi::Env env, const std::string& message) {
    ErrorFromMessage(env, message).ThrowAsJavaScriptException();
}

void RejectFromPvxs(Napi::Promise::Deferred& deferred, const std::exception& e) {
    deferred.Reject(ErrorFromPvxs(deferred.Env(), e).Value());
}

void RejectFromMessage(Napi::Promise::Deferred& deferred, const std::string& message) {
    deferred.Reject(ErrorFromMessage(deferred.Env(), message).Value());
}

}  // namespace binding
