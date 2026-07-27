#pragma once

#include <napi.h>

#include <pvxs/server.h>
#include <pvxs/sharedpv.h>

#include <map>
#include <memory>
#include <string>

namespace binding {

class StaticSourceWrap : public Napi::ObjectWrap<StaticSourceWrap> {
public:
    static Napi::Object Init(Napi::Env env, Napi::Object exports);

    static bool IsInstance(const Napi::Value& value);

    explicit StaticSourceWrap(const Napi::CallbackInfo& info);
    ~StaticSourceWrap() override;

    std::shared_ptr<pvxs::server::Source> SourcePtr() const { return src_.source(); }

private:
    static Napi::FunctionReference constructor_;

    static Napi::Value Build(const Napi::CallbackInfo& info);

    Napi::Value Add(const Napi::CallbackInfo& info);
    Napi::Value Remove(const Napi::CallbackInfo& info);
    Napi::Value List(const Napi::CallbackInfo& info);
    Napi::Value Close(const Napi::CallbackInfo& info);

    void ClearRefs();

    pvxs::server::StaticSource src_;
    // Keep JS SharedPV alive while registered on this source.
    std::map<std::string, Napi::ObjectReference> pvs_;
};

}  // namespace binding
