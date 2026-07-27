#pragma once

#include <napi.h>

#include <exception>

namespace binding {

/**
 * Register monitor/discover stream event types on exports
 * (RemoteError, Connected, Disconnected, Finished, Interrupted, Timeout).
 * Not used for get/put/rpc Promise rejects (those use plain Error).
 */
Napi::Object InitClientEvents(Napi::Env env, Napi::Object exports);

/** Map a pvxs client exception to a typed JS event object (monitor/discover). */
Napi::Value MonitorEventToJs(Napi::Env env, const std::exception& e);

/** Construct a Finished event (stream end / cancel). */
Napi::Value FinishedEvent(Napi::Env env);

/** Construct a Disconnected event (including synthetic initial disconnect). */
Napi::Value DisconnectedEvent(Napi::Env env);

}  // namespace binding
