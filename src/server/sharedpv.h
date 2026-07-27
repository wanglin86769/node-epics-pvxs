#pragma once

#include <napi.h>

#include <pvxs/sharedpv.h>

#include <memory>
#include <string>

namespace binding {

class SharedPVWrap : public Napi::ObjectWrap<SharedPVWrap> {
public:
    static Napi::Object Init(Napi::Env env, Napi::Object exports);

    static bool IsInstance(const Napi::Value& value);
    static pvxs::server::SharedPV UnwrapPV(const Napi::Value& value);
    static Napi::Object Create(Napi::Env env, pvxs::server::SharedPV pv);

    explicit SharedPVWrap(const Napi::CallbackInfo& info);
    ~SharedPVWrap() override;

private:
    static Napi::FunctionReference constructor_;

    struct HandlerState {
        SharedPVWrap* wrap;
        std::shared_ptr<pvxs::server::ExecOp> op;
        pvxs::Value value;
        bool finished;
        std::string error;
    };

    static void RunHandler(Napi::Env env,
                           HandlerState* payload,
                           Napi::FunctionReference& handler);

    static Napi::Value BuildMailbox(const Napi::CallbackInfo& info);
    static Napi::Value BuildReadonly(const Napi::CallbackInfo& info);

    Napi::Value Open(const Napi::CallbackInfo& info);
    Napi::Value Close(const Napi::CallbackInfo& info);
    Napi::Value IsOpen(const Napi::CallbackInfo& info);
    Napi::Value Post(const Napi::CallbackInfo& info);
    Napi::Value Current(const Napi::CallbackInfo& info);
    Napi::Value OnPut(const Napi::CallbackInfo& info);
    Napi::Value OnRpc(const Napi::CallbackInfo& info);

    pvxs::server::SharedPV pv_;
    Napi::FunctionReference put_handler_;
    Napi::FunctionReference rpc_handler_;
    Napi::ThreadSafeFunction put_tsfn_;
    Napi::ThreadSafeFunction rpc_tsfn_;
};

}  // namespace binding
