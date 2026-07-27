#include "client/events.h"

#include <pvxs/client.h>

namespace binding {

namespace {

Napi::FunctionReference disconnected_ctor_;
Napi::FunctionReference finished_ctor_;
Napi::FunctionReference interrupted_ctor_;
Napi::FunctionReference timeout_ctor_;

}  // namespace

class RemoteErrorWrap : public Napi::ObjectWrap<RemoteErrorWrap> {
public:
    static Napi::FunctionReference constructor_;

    static Napi::Object Init(Napi::Env env, Napi::Object exports) {
        Napi::Function func = DefineClass(
            env,
            "RemoteError",
            {
                InstanceAccessor("message", &RemoteErrorWrap::GetMessage, nullptr),
            });
        constructor_ = Napi::Persistent(func);
        constructor_.SuppressDestruct();
        exports.Set("RemoteError", func);
        return exports;
    }

    static Napi::Object New(Napi::Env env, const std::string& message) {
        return constructor_.New({Napi::String::New(env, message)});
    }

    explicit RemoteErrorWrap(const Napi::CallbackInfo& info)
        : Napi::ObjectWrap<RemoteErrorWrap>(info) {
        if (info.Length() > 0 && info[0].IsString()) {
            message_ = info[0].As<Napi::String>().Utf8Value();
        }
    }

private:
    Napi::Value GetMessage(const Napi::CallbackInfo& info) {
        return Napi::String::New(info.Env(), message_);
    }

    std::string message_;
};

Napi::FunctionReference RemoteErrorWrap::constructor_;

class ConnectedWrap : public Napi::ObjectWrap<ConnectedWrap> {
public:
    static Napi::FunctionReference constructor_;

    static Napi::Object Init(Napi::Env env, Napi::Object exports) {
        Napi::Function func = DefineClass(
            env,
            "Connected",
            {
                InstanceAccessor("peerName", &ConnectedWrap::GetPeerName, nullptr),
                InstanceAccessor("message", &ConnectedWrap::GetMessage, nullptr),
            });
        constructor_ = Napi::Persistent(func);
        constructor_.SuppressDestruct();
        exports.Set("Connected", func);
        return exports;
    }

    static Napi::Object New(Napi::Env env, const pvxs::client::Connected& con) {
        return constructor_.New({Napi::String::New(env, con.peerName)});
    }

    explicit ConnectedWrap(const Napi::CallbackInfo& info)
        : Napi::ObjectWrap<ConnectedWrap>(info) {
        if (info.Length() > 0 && info[0].IsString()) {
            peer_name_ = info[0].As<Napi::String>().Utf8Value();
        }
    }

private:
    Napi::Value GetPeerName(const Napi::CallbackInfo& info) {
        return Napi::String::New(info.Env(), peer_name_);
    }

    Napi::Value GetMessage(const Napi::CallbackInfo& info) {
        return Napi::String::New(info.Env(), peer_name_);
    }

    std::string peer_name_;
};

Napi::FunctionReference ConnectedWrap::constructor_;

class SimpleEventWrap : public Napi::ObjectWrap<SimpleEventWrap> {
public:
    static void Init(Napi::Env env,
                     Napi::Object exports,
                     const char* name,
                     Napi::FunctionReference& ctor) {
        Napi::Function func = DefineClass(env, name, {});
        ctor = Napi::Persistent(func);
        ctor.SuppressDestruct();
        exports.Set(name, func);
    }

    explicit SimpleEventWrap(const Napi::CallbackInfo& info)
        : Napi::ObjectWrap<SimpleEventWrap>(info) {}
};

Napi::Object InitClientEvents(Napi::Env env, Napi::Object exports) {
    RemoteErrorWrap::Init(env, exports);
    ConnectedWrap::Init(env, exports);
    SimpleEventWrap::Init(env, exports, "Disconnected", disconnected_ctor_);
    SimpleEventWrap::Init(env, exports, "Finished", finished_ctor_);
    SimpleEventWrap::Init(env, exports, "Interrupted", interrupted_ctor_);
    SimpleEventWrap::Init(env, exports, "Timeout", timeout_ctor_);
    return exports;
}

Napi::Value MonitorEventToJs(Napi::Env env, const std::exception& e) {
    if (const auto* err = dynamic_cast<const pvxs::client::RemoteError*>(&e)) {
        return RemoteErrorWrap::New(env, err->what());
    }
    if (const auto* con = dynamic_cast<const pvxs::client::Connected*>(&e)) {
        return ConnectedWrap::New(env, *con);
    }
    // Finished inherits Disconnect - check Finished first.
    if (dynamic_cast<const pvxs::client::Finished*>(&e)) {
        return finished_ctor_.New({});
    }
    if (dynamic_cast<const pvxs::client::Disconnect*>(&e)) {
        return disconnected_ctor_.New({});
    }
    if (dynamic_cast<const pvxs::client::Interrupted*>(&e)) {
        return interrupted_ctor_.New({});
    }
    if (dynamic_cast<const pvxs::client::Timeout*>(&e)) {
        return timeout_ctor_.New({});
    }
    return Napi::Error::New(env, e.what()).Value();
}

Napi::Value FinishedEvent(Napi::Env env) {
    return finished_ctor_.New({});
}

Napi::Value DisconnectedEvent(Napi::Env env) {
    return disconnected_ctor_.New({});
}

}  // namespace binding
