#pragma once

#include <napi.h>

#include <pvxs/client.h>

#include <memory>
#include <string>

namespace binding {

/**
 * Start get/put/rpc/list; returns a Promise with .cancel() attached.
 * Operation is created on the main thread (exec); the worker only wait()s.
 */

Napi::Promise RunGet(Napi::Env env,
                     pvxs::client::Context& ctx,
                     const std::string& name,
                     double timeout_sec);

Napi::Promise RunPut(Napi::Env env,
                     pvxs::client::Context& ctx,
                     const std::string& name,
                     const Napi::Value& data,
                     double timeout_sec);

Napi::Promise RunRpc(Napi::Env env,
                     pvxs::client::Context& ctx,
                     const std::string& name,
                     const Napi::Object& args,
                     double timeout_sec);

Napi::Promise RunList(Napi::Env env,
                      pvxs::client::Context& ctx,
                      const std::string& server,
                      double timeout_sec);

}  // namespace binding
