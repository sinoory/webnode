#include "config.h"
#include "JSNodeProxy.h"

#include "JSDOMBinding.h"
#include "JSDataTransfer.h"
#include <runtime/JSLock.h>
#include <wtf/HashMap.h>
#include <wtf/text/AtomicString.h>
#include <v8.h>
#include <libplatform/libplatform.h>
using namespace JSC;

namespace WebCore {
JSValue JSNodeProxy::flag(ExecState* exec) const{
    return jsUndefined();
}
#if 1
bool JSNodeProxy::getOwnPropertySlot(JSObject* object, ExecState* exec, PropertyName propertyName, PropertySlot& slot)
{
    /*
    JSNodeProxy* thisObject = jsCast<JSNodeProxy*>(object);
    ASSERT_GC_OBJECT_INHERITS(thisObject, info());
    return getStaticValueSlot<JSNodeProxy, Base>(exec, JSNodeProxyTable, thisObject, propertyName, slot);
    */

    v8::Platform* platform = v8::platform::CreateDefaultPlatform();
    v8::V8::Initialize();
    printf("JSNodeProxy::getOwnPropertySlot propertyName=%s\n",propertyName.uid()->characters8());
    return false;
}

bool JSNodeProxy::getOwnPropertySlotByIndex(JSObject* object, ExecState* exec, unsigned index, PropertySlot& slot)
{
    return false;
}
#endif
}



