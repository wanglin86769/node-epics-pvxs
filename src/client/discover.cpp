#include "client/discover.h"

#include "client/events.h"
#include "common/tsfn.h"

namespace binding {
namespace {

using namespace pvxs::client;

Napi::Object DiscoveredToObject(Napi::Env env, const Discovered& item) {
    Napi::Object out = Napi::Object::New(env);
    out.Set("event", static_cast<int>(item.event));
    out.Set("peerVersion", item.peerVersion);
    out.Set("peer", item.peer);
    out.Set("proto", item.proto);
    out.Set("server", item.server);
    return out;
}

}  // namespace

Napi::FunctionReference DiscoverWrap::constructor_;

bool DiscoverWrap::IsInstance(const Napi::Value& value) {
    if (constructor_.IsEmpty()) {
        return false;
    }
    return value.IsObject() && value.As<Napi::Object>().InstanceOf(constructor_.Value());
}

DiscoverWrap::DiscoverWrap(const Napi::CallbackInfo& info)
    : Napi::ObjectWrap<DiscoverWrap>(info) {
    tsfn_ = MakeTsfn(info.Env(), "pvxsDiscover");
}

DiscoverWrap::~DiscoverWrap() {
    if (callback_self_) {
        *callback_self_ = nullptr;
        callback_self_.reset();
    }
    if (op_) {
        op_->cancel();
        op_.reset();
    }
}

void DiscoverWrap::Finalize(Napi::Env env) {
    Cleanup(env);
}

Napi::Object DiscoverWrap::Create(Napi::Env env) {
    return constructor_.New({});
}

void DiscoverWrap::Attach(std::shared_ptr<Operation> op,
                          std::shared_ptr<DiscoverWrap*> callback_self) {
    op_ = std::move(op);
    callback_self_ = std::move(callback_self);
    if (callback_self_) {
        *callback_self_ = this;
    }
}

void DiscoverWrap::PushDiscovered(const Discovered& item) {
    auto holder = callback_self_;
    if (!holder || !*holder) {
        return;
    }

    struct EventData {
        std::shared_ptr<DiscoverWrap*> holder;
        Discovered item;
    };
    auto* data = new EventData{holder, item};
    const napi_status status = tsfn_.NonBlockingCall(
        data, [](Napi::Env cb_env, Napi::Function, EventData* payload) {
            if (payload->holder && *payload->holder) {
                (*payload->holder)
                    ->PushEvent(cb_env, DiscoveredToObject(cb_env, payload->item));
            }
            delete payload;
        });
    if (status != napi_ok) {
        delete data;
    }
}

void DiscoverWrap::PushEvent(Napi::Env env, Napi::Object value) {
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

void DiscoverWrap::FinishStream(Napi::Env env) {
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

void DiscoverWrap::Cleanup(Napi::Env env) {
    if (callback_self_) {
        *callback_self_ = nullptr;
        callback_self_.reset();
    }
    if (op_) {
        op_->cancel();
        op_.reset();
    }
    FinishStream(env);
    if (tsfn_) {
        tsfn_.Release();
        tsfn_ = Napi::ThreadSafeFunction();
    }
}

Napi::Value DiscoverWrap::Cancel(const Napi::CallbackInfo& info) {
    Napi::Env env = info.Env();
    bool ok = false;
    if (op_) {
        ok = op_->cancel();
        op_.reset();
    }
    Cleanup(env);
    return Napi::Boolean::New(env, ok);
}

Napi::Value DiscoverWrap::Name(const Napi::CallbackInfo& info) {
    if (!op_) {
        return info.Env().Undefined();
    }
    return Napi::String::New(info.Env(), op_->name());
}

Napi::Value DiscoverWrap::Next(const Napi::CallbackInfo& info) {
    Napi::Env env = info.Env();
    auto deferred = Napi::Promise::Deferred::New(env);

    {
        std::lock_guard<std::mutex> lock(mutex_);
        if (finished_) {
            deferred.Resolve(FinishedEvent(env));
            return deferred.Promise();
        }
        if (!queue_.empty()) {
            Napi::Object value = queue_.front().Value();
            queue_.pop_front();
            deferred.Resolve(value);
            return deferred.Promise();
        }
        waiters_.push_back(deferred);
    }
    return deferred.Promise();
}

Napi::Object DiscoverWrap::Init(Napi::Env env, Napi::Object exports) {
    Napi::Function func = DefineClass(
        env,
        "Discover",
        {
            InstanceMethod("cancel", &DiscoverWrap::Cancel),
            InstanceMethod("name", &DiscoverWrap::Name),
            InstanceMethod("next", &DiscoverWrap::Next),
        });
    constructor_ = Napi::Persistent(func);
    constructor_.SuppressDestruct();
    exports.Set("Discover", func);
    return exports;
}

}  // namespace binding
