#include "client/subscription.h"

#include "client/events.h"
#include "common/tsfn.h"
#include "data/value_wrap.h"

#include <exception>

namespace binding {

using namespace pvxs::client;

Napi::FunctionReference SubscriptionWrap::constructor_;

bool SubscriptionWrap::IsInstance(const Napi::Value& value) {
    if (constructor_.IsEmpty()) {
        return false;
    }
    return value.IsObject() && value.As<Napi::Object>().InstanceOf(constructor_.Value());
}

SubscriptionWrap::SubscriptionWrap(const Napi::CallbackInfo& info)
    : Napi::ObjectWrap<SubscriptionWrap>(info) {
    tsfn_ = MakeTsfn(info.Env(), "pvxsMonitor");
}

SubscriptionWrap::~SubscriptionWrap() {
    // Finalize/Cancel run Cleanup (FinishStream + TSFN). Dtor only stops callbacks.
    if (callback_self_) {
        *callback_self_ = nullptr;
        callback_self_.reset();
    }
    if (sub_) {
        sub_->cancel();
        sub_.reset();
    }
}

void SubscriptionWrap::Finalize(Napi::Env env) {
    Cleanup(env);
}

Napi::Object SubscriptionWrap::Create(Napi::Env env) {
    return constructor_.New({});
}

void SubscriptionWrap::Attach(std::shared_ptr<Subscription> sub,
                              std::shared_ptr<SubscriptionWrap*> callback_self) {
    sub_ = std::move(sub);
    callback_self_ = std::move(callback_self);
    if (callback_self_) {
        *callback_self_ = this;
    }
}

void SubscriptionWrap::PushInitialDisconnected(Napi::Env env) {
    PushEvent(env, DisconnectedEvent(env));
}

void SubscriptionWrap::HandleMonitorEvent(Subscription& active) {
    // Hold a copy so in-flight TSFN callbacks can see a nulled pointer after Cleanup.
    auto holder = callback_self_;
    if (!holder || !*holder) {
        return;
    }

    // pvxs fires the event callback when the queue becomes non-empty.
    // Drain every pending update here; otherwise a second update that arrives
    // before JS calls pop() can be stranded without another callback.
    for (;;) {
        struct EventData {
            std::shared_ptr<SubscriptionWrap*> holder;
            enum class Kind { Value, Exception } kind;
            pvxs::Value value;
            std::exception_ptr exception;
        };

        auto* data = new EventData{holder, EventData::Kind::Value, pvxs::Value(), nullptr};
        try {
            pvxs::Value update = active.pop();
            if (!update.valid()) {
                delete data;
                return;
            }
            data->value = std::move(update);
        } catch (...) {
            data->kind = EventData::Kind::Exception;
            data->exception = std::current_exception();
        }

        const napi_status status = tsfn_.NonBlockingCall(
            data, [](Napi::Env cb_env, Napi::Function, EventData* payload) {
                if (payload->holder && *payload->holder) {
                    if (payload->kind == EventData::Kind::Value) {
                        (*payload->holder)
                            ->PushEvent(cb_env, PvxsValueWrap::ToJs(cb_env, payload->value));
                    } else if (payload->exception) {
                        try {
                            std::rethrow_exception(payload->exception);
                        } catch (const std::exception& e) {
                            (*payload->holder)
                                ->PushEvent(cb_env, MonitorEventToJs(cb_env, e));
                        } catch (...) {
                            (*payload->holder)
                                ->PushEvent(
                                    cb_env,
                                    Napi::Error::New(cb_env, "unknown monitor error")
                                        .Value());
                        }
                    }
                }
                delete payload;
            });
        if (status != napi_ok) {
            delete data;
            return;
        }
        // Keep draining after exceptions; otherwise later Values stay queued with no notify.
    }
}

void SubscriptionWrap::PushEvent(Napi::Env env, Napi::Value value) {
    std::lock_guard<std::mutex> lock(mutex_);
    if (finished_) {
        return;
    }
    if (!waiters_.empty()) {
        waiters_.front().Resolve(value);
        waiters_.pop_front();
        return;
    }
    queue_.emplace_back(Napi::Persistent(value));
}

void SubscriptionWrap::FinishStream(Napi::Env env) {
    std::deque<Napi::Promise::Deferred> pending;
    {
        std::lock_guard<std::mutex> lock(mutex_);
        finished_ = true;
        pending.swap(waiters_);
        queue_.clear();
    }
    for (auto& deferred : pending) {
        deferred.Resolve(FinishedEvent(env));
    }
}

void SubscriptionWrap::Cleanup(Napi::Env env) {
    if (callback_self_) {
        *callback_self_ = nullptr;
        callback_self_.reset();
    }
    if (sub_) {
        sub_->cancel();
        sub_.reset();
    }
    FinishStream(env);
    if (tsfn_) {
        tsfn_.Release();
        tsfn_ = Napi::ThreadSafeFunction();
    }
}

Napi::Value SubscriptionWrap::Cancel(const Napi::CallbackInfo& info) {
    Napi::Env env = info.Env();
    bool ok = false;
    if (sub_) {
        ok = sub_->cancel();
        sub_.reset();
    }
    Cleanup(env);
    return Napi::Boolean::New(env, ok);
}

Napi::Value SubscriptionWrap::Pause(const Napi::CallbackInfo& info) {
    if (sub_) {
        sub_->pause(true);
    }
    return info.Env().Undefined();
}

Napi::Value SubscriptionWrap::Resume(const Napi::CallbackInfo& info) {
    if (sub_) {
        sub_->resume();
    }
    return info.Env().Undefined();
}

Napi::Value SubscriptionWrap::Name(const Napi::CallbackInfo& info) {
    if (!sub_) {
        return info.Env().Undefined();
    }
    return Napi::String::New(info.Env(), sub_->name());
}

Napi::Value SubscriptionWrap::Next(const Napi::CallbackInfo& info) {
    Napi::Env env = info.Env();
    auto deferred = Napi::Promise::Deferred::New(env);

    {
        std::lock_guard<std::mutex> lock(mutex_);
        if (finished_) {
            deferred.Resolve(FinishedEvent(env));
            return deferred.Promise();
        }
        if (!queue_.empty()) {
            Napi::Value value = queue_.front().Value();
            queue_.pop_front();
            deferred.Resolve(value);
            return deferred.Promise();
        }
        waiters_.push_back(deferred);
    }
    return deferred.Promise();
}

Napi::Object SubscriptionWrap::Init(Napi::Env env, Napi::Object exports) {
    Napi::Function func = DefineClass(
        env,
        "Subscription",
        {
            InstanceMethod("cancel", &SubscriptionWrap::Cancel),
            InstanceMethod("pause", &SubscriptionWrap::Pause),
            InstanceMethod("resume", &SubscriptionWrap::Resume),
            InstanceMethod("name", &SubscriptionWrap::Name),
            InstanceMethod("next", &SubscriptionWrap::Next),
        });
    constructor_ = Napi::Persistent(func);
    constructor_.SuppressDestruct();
    exports.Set("Subscription", func);
    return exports;
}

}  // namespace binding
