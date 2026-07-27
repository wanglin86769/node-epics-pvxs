#include "data/typedef.h"

#include "common/errors.h"
#include "data/value_wrap.h"

#include <pvxs/data.h>

#include <stdexcept>
#include <vector>

namespace binding {

using namespace pvxs;

TypeCode::code_t ParseTypeCode(const Napi::Value& val) {
    if (!val.IsNumber()) {
        throw std::invalid_argument("TypeCode must be a number");
    }
    return static_cast<TypeCode::code_t>(val.As<Napi::Number>().Int32Value());
}

namespace {

std::vector<Member> ParseMemberArray(const Napi::Array& arr) {
    std::vector<Member> members;
    members.reserve(arr.Length());

    for (uint32_t i = 0; i < arr.Length(); ++i) {
        if (!arr.Get(i).IsObject()) {
            throw std::invalid_argument("Member must be an object");
        }
        Napi::Object item = arr.Get(i).As<Napi::Object>();
        if (!item.Has("code") || !item.Has("name")) {
            throw std::invalid_argument("Member requires code and name");
        }

        const auto code = ParseTypeCode(item.Get("code"));
        const std::string name = item.Get("name").As<Napi::String>().Utf8Value();

        if (item.Has("children") && item.Get("children").IsArray()) {
            members.emplace_back(code, name, ParseMemberArray(item.Get("children").As<Napi::Array>()));
        } else {
            members.emplace_back(code, name);
        }
    }

    return members;
}

Napi::Value TypeDefCreate(const Napi::CallbackInfo& info) {
    Napi::Env env = info.Env();
    if (info.Length() < 1 || !info[0].IsNumber()) {
        Napi::TypeError::New(env, "TypeDef.create(typeCode[, members]) requires a type code")
            .ThrowAsJavaScriptException();
        return env.Undefined();
    }

    try {
        const auto code = ParseTypeCode(info[0]);
        Value val;
        if (info.Length() > 1 && info[1].IsArray()) {
            val = TypeDef(code, std::string(), ParseMemberArray(info[1].As<Napi::Array>())).create();
        } else {
            val = TypeDef(code).create();
        }
        return PvxsValueWrap::ToJs(env, val);
    } catch (const std::exception& e) {
        ThrowFromPvxs(env, e);
        return env.Undefined();
    }
}

}  // namespace

void InitTypeDefApi(Napi::Env env, Napi::Object exports) {
    Napi::Object typedef_api = Napi::Object::New(env);
    typedef_api.Set("create", Napi::Function::New(env, TypeDefCreate));
    exports.Set("TypeDef", typedef_api);
}

}  // namespace binding
