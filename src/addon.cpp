/**
 * node-epics-pvxs native addon entry.
 */

#include <napi.h>

#include <pvxs/version.h>

#include "data/init.h"
#include "client/init.h"
#include "server/init.h"

namespace {

using pvxs::version_abi_int;
using pvxs::version_int;
using pvxs::version_str;

Napi::Value Version(const Napi::CallbackInfo& info) {
    Napi::Env env = info.Env();
    return Napi::String::New(env, version_str());
}

Napi::Value VersionInt(const Napi::CallbackInfo& info) {
    Napi::Env env = info.Env();
    return Napi::Number::New(env, static_cast<double>(version_int()));
}

Napi::Value VersionAbi(const Napi::CallbackInfo& info) {
    Napi::Env env = info.Env();
    return Napi::Number::New(env, static_cast<double>(version_abi_int()));
}

Napi::Object Init(Napi::Env env, Napi::Object exports) {
    exports.Set("version", Napi::Function::New(env, Version));
    exports.Set("versionInt", Napi::Function::New(env, VersionInt));
    exports.Set("versionAbi", Napi::Function::New(env, VersionAbi));

    Napi::Object data = Napi::Object::New(env);
    binding::InitDataModule(env, data);
    exports.Set("data", data);

    Napi::Object client = Napi::Object::New(env);
    binding::InitClientModule(env, client);
    exports.Set("client", client);

    Napi::Object server = Napi::Object::New(env);
    binding::InitServerModule(env, server);
    exports.Set("server", server);
    return exports;
}

}  // namespace

NODE_API_MODULE(pvxs, Init)
