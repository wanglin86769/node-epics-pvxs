#pragma once

#include <napi.h>

#include <pvxs/data.h>
#include <pvxs/sharedpv.h>

#include <memory>

namespace binding {

class ExecOpWrap : public Napi::ObjectWrap<ExecOpWrap> {
public:
    static Napi::Object Init(Napi::Env env, Napi::Object exports);

    static Napi::Object Create(Napi::Env env,
                               const std::shared_ptr<pvxs::server::ExecOp>& op,
                               pvxs::Value&& request);

    explicit ExecOpWrap(const Napi::CallbackInfo& info);

    bool finished() const { return finished_; }

private:
    static Napi::FunctionReference constructor_;

    Napi::Value Reply(const Napi::CallbackInfo& info);
    Napi::Value Error(const Napi::CallbackInfo& info);
    Napi::Value RequestValue(const Napi::CallbackInfo& info);

    bool finished_ = false;
    std::shared_ptr<pvxs::server::ExecOp> op_;
    pvxs::Value request_;
};

}  // namespace binding
