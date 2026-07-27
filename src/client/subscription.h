#pragma once

#include <napi.h>

#include <pvxs/client.h>

#include <deque>
#include <memory>
#include <mutex>

namespace binding {

class SubscriptionWrap : public Napi::ObjectWrap<SubscriptionWrap> {
public:
    static Napi::Object Init(Napi::Env env, Napi::Object exports);
    static Napi::Object Create(Napi::Env env);
    static bool IsInstance(const Napi::Value& value);

    void Attach(std::shared_ptr<pvxs::client::Subscription> sub,
                std::shared_ptr<SubscriptionWrap*> callback_self);
    void HandleMonitorEvent(pvxs::client::Subscription& active);
    /** p4p-style: first event is Disconnected (subscription starts unconnected). */
    void PushInitialDisconnected(Napi::Env env);

    explicit SubscriptionWrap(const Napi::CallbackInfo& info);
    ~SubscriptionWrap() override;
    void Finalize(Napi::Env env) override;

private:
    static Napi::FunctionReference constructor_;

    Napi::Value Cancel(const Napi::CallbackInfo& info);
    Napi::Value Pause(const Napi::CallbackInfo& info);
    Napi::Value Resume(const Napi::CallbackInfo& info);
    Napi::Value Name(const Napi::CallbackInfo& info);
    Napi::Value Next(const Napi::CallbackInfo& info);

    void PushEvent(Napi::Env env, Napi::Value value);
    /** Resolve pending next() waiters with Finished; clear queue. */
    void FinishStream(Napi::Env env);
    /** Stop callbacks, cancel pvxs, finish stream, release TSFN. */
    void Cleanup(Napi::Env env);

    std::shared_ptr<pvxs::client::Subscription> sub_;
    /** Shared with the pvxs event callback; nulled on cancel/finalize. */
    std::shared_ptr<SubscriptionWrap*> callback_self_;
    bool finished_ = false;

    Napi::ThreadSafeFunction tsfn_;
    std::mutex mutex_;
    std::deque<Napi::Reference<Napi::Value>> queue_;
    std::deque<Napi::Promise::Deferred> waiters_;
};

}  // namespace binding
