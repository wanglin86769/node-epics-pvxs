#pragma once

#include <napi.h>

#include <exception>
#include <string>

namespace binding {

/**
 * Error delivery by API shape (scheme 1):
 * - Bad JS args: TypeError at the call site
 * - Sync failure: ThrowFromPvxs / ThrowFromMessage
 * - One-shot Promise (get/put/rpc/list): RejectFromPvxs / RejectFromMessage
 * - Monitor/discover streams: typed events (events.h), not these helpers
 */

Napi::Error ErrorFromMessage(Napi::Env env, const std::string& message);
Napi::Error ErrorFromPvxs(Napi::Env env, const std::exception& e);

void ThrowFromPvxs(Napi::Env env, const std::exception& e);
void ThrowFromMessage(Napi::Env env, const std::string& message);

void RejectFromPvxs(Napi::Promise::Deferred& deferred, const std::exception& e);
void RejectFromMessage(Napi::Promise::Deferred& deferred, const std::string& message);

}  // namespace binding
