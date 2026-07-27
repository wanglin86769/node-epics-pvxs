#pragma once

#include <napi.h>

#include <pvxs/client.h>

#include <deque>
#include <memory>
#include <mutex>

namespace binding {

class DiscoverWrap : public Napi::ObjectWrap<DiscoverWrap> {
public:
    static Napi::Object Init(Napi::Env env, Napi::Object exports);
    static Napi::Object Create(Napi::Env env);
    static bool IsInstance(const Napi::Value& value);

    void Attach(std::shared_ptr<pvxs::client::Operation> op,
                std::shared_ptr<DiscoverWrap*> callback_self);
    void PushDiscovered(const pvxs::client::Discovered& item);

    explicit DiscoverWrap(const Napi::CallbackInfo& info);
    ~DiscoverWrap() override;
    void Finalize(Napi::Env env) override;

private:
    static Napi::FunctionReference constructor_;

    Napi::Value Cancel(const Napi::CallbackInfo& info);
    Napi::Value Name(const Napi::CallbackInfo& info);
    Napi::Value Next(const Napi::CallbackInfo& info);

    void PushEvent(Napi::Env env, Napi::Object value);
    /** Resolve pending next() waiters with Finished; clear queue. */
    void FinishStream(Napi::Env env);
    /** Stop callbacks, cancel pvxs, finish stream, release TSFN. */
    void Cleanup(Napi::Env env);

    std::shared_ptr<pvxs::client::Operation> op_;
    /** Shared with the pvxs discover callback; nulled on cancel/finalize. */
    std::shared_ptr<DiscoverWrap*> callback_self_;
    bool finished_ = false;

    Napi::ThreadSafeFunction tsfn_;
    std::mutex mutex_;
    std::deque<Napi::Reference<Napi::Object>> queue_;
    std::deque<Napi::Promise::Deferred> waiters_;
};

}  // namespace binding
