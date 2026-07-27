#include "client/context.h"

#include "client/config.h"
#include "client/discover.h"
#include "client/subscription.h"
#include "client/worker.h"
#include "common/errors.h"

#include <stdexcept>

namespace binding {
namespace {

constexpr double default_timeout_sec = 5.0;

/** Timeout in seconds (PVXS wait / p4p). Undefined/null -> default. */
double ParseTimeoutSec(const Napi::Value& timeout) {
    if (timeout.IsUndefined() || timeout.IsNull()) {
        return default_timeout_sec;
    }
    if (!timeout.IsNumber()) {
        throw std::invalid_argument("timeout must be a number (seconds)");
    }
    const double sec = timeout.As<Napi::Number>().DoubleValue();
    if (sec < 0.0) {
        throw std::invalid_argument("timeout must be non-negative");
    }
    return sec;
}

}  // namespace

Napi::FunctionReference ContextWrap::constructor_;

ContextWrap::ContextWrap(const Napi::CallbackInfo& info)
    : Napi::ObjectWrap<ContextWrap>(info) {}

Napi::Value ContextWrap::FromEnv(const Napi::CallbackInfo& info) {
    try {
        return WrapContext(info.Env(), pvxs::client::Context::fromEnv());
    } catch (const std::exception& e) {
        ThrowFromPvxs(info.Env(), e);
        return info.Env().Undefined();
    }
}

Napi::Value ContextWrap::FromConfig(const Napi::CallbackInfo& info) {
    Napi::Env env = info.Env();
    if (info.Length() < 1) {
        Napi::TypeError::New(env, "fromConfig(config) requires a client.Config or defs object")
            .ThrowAsJavaScriptException();
        return env.Undefined();
    }
    const bool is_cfg = ClientConfigWrap::IsInstance(info[0]);
    const bool is_defs = info[0].IsObject() && !info[0].IsArray();
    if (!is_cfg && !is_defs) {
        Napi::TypeError::New(env, "fromConfig(config) requires a client.Config or defs object")
            .ThrowAsJavaScriptException();
        return env.Undefined();
    }
    try {
        pvxs::client::Config conf =
            is_cfg ? ClientConfigWrap::FromJs(info[0])
                   : ClientConfigWrap::FromDefsObject(info[0]);
        return WrapContext(env, pvxs::client::Context(std::move(conf)));
    } catch (const std::exception& e) {
        ThrowFromPvxs(env, e);
        return env.Undefined();
    }
}

Napi::Object ContextWrap::WrapContext(Napi::Env env, pvxs::client::Context&& ctx) {
    Napi::Object obj = constructor_.New({});
    Napi::ObjectWrap<ContextWrap>::Unwrap(obj)->ctx_ = std::move(ctx);
    return obj;
}

Napi::Value ContextWrap::Close(const Napi::CallbackInfo& info) {
    ctx_.close();
    return info.Env().Undefined();
}

Napi::Value ContextWrap::Get(const Napi::CallbackInfo& info) {
    Napi::Env env = info.Env();
    if (info.Length() < 1 || !info[0].IsString()) {
        Napi::TypeError::New(env, "get(name[, timeout]) requires a string")
            .ThrowAsJavaScriptException();
        return env.Undefined();
    }
    try {
        const double timeout_sec =
            ParseTimeoutSec(info.Length() > 1 ? info[1] : env.Undefined());
        return RunGet(env, ctx_, info[0].As<Napi::String>().Utf8Value(), timeout_sec);
    } catch (const std::exception& e) {
        ThrowFromPvxs(env, e);
        return env.Undefined();
    }
}

Napi::Value ContextWrap::Put(const Napi::CallbackInfo& info) {
    Napi::Env env = info.Env();
    if (info.Length() < 2 || !info[0].IsString()) {
        Napi::TypeError::New(env, "put(name, data[, timeout]) requires a string name and data object")
            .ThrowAsJavaScriptException();
        return env.Undefined();
    }
    try {
        const double timeout_sec =
            ParseTimeoutSec(info.Length() > 2 ? info[2] : env.Undefined());
        return RunPut(env, ctx_, info[0].As<Napi::String>().Utf8Value(), info[1], timeout_sec);
    } catch (const std::exception& e) {
        ThrowFromPvxs(env, e);
        return env.Undefined();
    }
}

Napi::Value ContextWrap::Rpc(const Napi::CallbackInfo& info) {
    Napi::Env env = info.Env();
    if (info.Length() < 1 || !info[0].IsString()) {
        Napi::TypeError::New(env, "rpc(name[, args][, timeout]) requires a string name")
            .ThrowAsJavaScriptException();
        return env.Undefined();
    }

    // rpc(name), rpc(name, args), rpc(name, args, timeout).
    // args (if present) must be a plain object; timeout is always last.
    Napi::Object args = Napi::Object::New(env);
    if (info.Length() > 1) {
        if (!info[1].IsObject() || info[1].IsArray()) {
            Napi::TypeError::New(env, "rpc(name[, args][, timeout]) args must be an object")
                .ThrowAsJavaScriptException();
            return env.Undefined();
        }
        args = info[1].As<Napi::Object>();
    }

    try {
        const double timeout_sec =
            ParseTimeoutSec(info.Length() > 2 ? info[2] : env.Undefined());
        return RunRpc(env, ctx_, info[0].As<Napi::String>().Utf8Value(), args, timeout_sec);
    } catch (const std::exception& e) {
        ThrowFromPvxs(env, e);
        return env.Undefined();
    }
}

Napi::Value ContextWrap::List(const Napi::CallbackInfo& info) {
    Napi::Env env = info.Env();
    if (info.Length() < 1 || !info[0].IsString()) {
        Napi::TypeError::New(env, "list(server[, timeout]) requires a string")
            .ThrowAsJavaScriptException();
        return env.Undefined();
    }
    try {
        const double timeout_sec =
            ParseTimeoutSec(info.Length() > 1 ? info[1] : env.Undefined());
        return RunList(env, ctx_, info[0].As<Napi::String>().Utf8Value(), timeout_sec);
    } catch (const std::exception& e) {
        ThrowFromPvxs(env, e);
        return env.Undefined();
    }
}

Napi::Value ContextWrap::Monitor(const Napi::CallbackInfo& info) {
    Napi::Env env = info.Env();
    if (info.Length() < 1 || !info[0].IsString()) {
        Napi::TypeError::New(env, "monitor(name) requires a string").ThrowAsJavaScriptException();
        return env.Undefined();
    }

    try {
        const std::string name = info[0].As<Napi::String>().Utf8Value();
        Napi::Object sub_obj = SubscriptionWrap::Create(env);
        auto* wrap = Napi::ObjectWrap<SubscriptionWrap>::Unwrap(sub_obj);

        auto holder = std::make_shared<SubscriptionWrap*>(wrap);
        // Match p4p: mask Connected, deliver Disconnected/Finished.
        auto sub = ctx_.monitor(name)
                       .maskConnected(true)
                       .maskDisconnected(false)
                       .event([holder](pvxs::client::Subscription& active) {
                           if (*holder) {
                               (*holder)->HandleMonitorEvent(active);
                           }
                       })
                       .exec();
        wrap->Attach(std::move(sub), std::move(holder));
        // Like p4p notify_disconnect: subscriptions start disconnected.
        wrap->PushInitialDisconnected(env);
        return sub_obj;
    } catch (const std::exception& e) {
        ThrowFromPvxs(env, e);
        return env.Undefined();
    }
}

Napi::Value ContextWrap::Discover(const Napi::CallbackInfo& info) {
    Napi::Env env = info.Env();
    bool do_ping = true;
    if (info.Length() > 0 && !info[0].IsUndefined() && !info[0].IsNull()) {
        if (!info[0].IsObject() || info[0].IsArray()) {
            Napi::TypeError::New(env, "discover(options) options must be an object")
                .ThrowAsJavaScriptException();
            return env.Undefined();
        }
        Napi::Object opts = info[0].As<Napi::Object>();
        if (opts.Has("ping")) {
            Napi::Value ping = opts.Get("ping");
            if (!ping.IsBoolean()) {
                Napi::TypeError::New(env, "discover(options) ping must be a boolean")
                    .ThrowAsJavaScriptException();
                return env.Undefined();
            }
            do_ping = ping.As<Napi::Boolean>().Value();
        }
    }

    try {
        Napi::Object disc_obj = DiscoverWrap::Create(env);
        auto* wrap = Napi::ObjectWrap<DiscoverWrap>::Unwrap(disc_obj);
        auto holder = std::make_shared<DiscoverWrap*>(wrap);

        auto op = ctx_.discover([holder](const pvxs::client::Discovered& item) {
                      if (*holder) {
                          (*holder)->PushDiscovered(item);
                      }
                  })
                      .pingAll(do_ping)
                      .exec();
        wrap->Attach(std::move(op), std::move(holder));
        return disc_obj;
    } catch (const std::exception& e) {
        ThrowFromPvxs(env, e);
        return env.Undefined();
    }
}

Napi::Object ContextWrap::Init(Napi::Env env, Napi::Object exports) {
    Napi::Function func = DefineClass(
        env,
        "Context",
        {
            StaticMethod("fromEnv", &ContextWrap::FromEnv),
            StaticMethod("fromConfig", &ContextWrap::FromConfig),
            InstanceMethod("close", &ContextWrap::Close),
            InstanceMethod("get", &ContextWrap::Get),
            InstanceMethod("put", &ContextWrap::Put),
            InstanceMethod("rpc", &ContextWrap::Rpc),
            InstanceMethod("list", &ContextWrap::List),
            InstanceMethod("monitor", &ContextWrap::Monitor),
            InstanceMethod("discover", &ContextWrap::Discover),
        });

    constructor_ = Napi::Persistent(func);
    constructor_.SuppressDestruct();
    exports.Set("Context", func);
    return exports;
}

}  // namespace binding
