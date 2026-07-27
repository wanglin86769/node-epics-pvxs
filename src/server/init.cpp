#include "server/init.h"

#include "server/exec_op.h"
#include "server/server.h"
#include "server/sharedpv.h"
#include "server/static_source.h"

namespace binding {

void InitServerModule(Napi::Env env, Napi::Object exports) {
    ExecOpWrap::Init(env, exports);
    SharedPVWrap::Init(env, exports);
    StaticSourceWrap::Init(env, exports);
    ServerWrap::Init(env, exports);
}

}  // namespace binding
