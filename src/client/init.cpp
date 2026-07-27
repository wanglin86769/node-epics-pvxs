#include "client/init.h"

#include "client/config.h"
#include "client/context.h"
#include "client/discover.h"
#include "client/events.h"
#include "client/subscription.h"

namespace binding {

void InitClientModule(Napi::Env env, Napi::Object exports) {
    InitClientEvents(env, exports);

    ClientConfigWrap::Init(env, exports);
    SubscriptionWrap::Init(env, exports);
    DiscoverWrap::Init(env, exports);
    ContextWrap::Init(env, exports);
}

}  // namespace binding
