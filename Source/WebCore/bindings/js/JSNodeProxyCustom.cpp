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
EncodedJSValue jsNodeProxyGenralFunc(ExecState* exec, JSObject* slotBase, EncodedJSValue thisValue, PropertyName);

EncodedJSValue JSC_HOST_CALL jsNodeProxyGeneralMethodFunc(ExecState* exec);
#if 1
bool JSNodeProxy::getOwnPropertySlot(JSObject* object, ExecState* exec, PropertyName propertyName, PropertySlot& slot)
{
    JSNodeProxy* thisObject = jsCast<JSNodeProxy*>(object);
    NodeProxy& impl = thisObject->impl();
    if(propertyName=="write"){
        printf("JSNodeProxy::getOwnPropertySlot handle write() exec=%p\n",exec);
        //for js : var res=np.write;
        //slot.setCustom(thisObject, ReadOnly | DontDelete | DontEnum, jsNodeProxyGenralFunc);


        //for js : var res=np.write();
        slot.setCustom(thisObject, ReadOnly | DontDelete | DontEnum, nonCachingStaticFunctionGetter<jsNodeProxyGeneralMethodFunc, 0>);
        impl.mMethod=std::string((char*)(propertyName.uid()->characters8()));
        return true;
    }

    //v8::Platform* platform = v8::platform::CreateDefaultPlatform();
    //v8::V8::Initialize();
    printf("JSNodeProxy::getOwnPropertySlot propertyName=%s\n",propertyName.uid()->characters8());
    return false;
}
EncodedJSValue jsNodeProxyGenralFunc(ExecState* exec, JSObject* slotBase, EncodedJSValue thisValue, PropertyName)
{
    UNUSED_PARAM(exec);
    UNUSED_PARAM(slotBase);
    UNUSED_PARAM(thisValue);
    JSNodeProxy* castedThis = 0;//toJSDOMWindow(JSValue::decode(thisValue));
    //DOMWindow& impl = castedThis->impl();
    JSValue result = jsBoolean(false);
    //JSValue result = jsBoolean(impl.closed());
    printf("JSNodeProxy 1 jsNodeProxyGenralFunc\n");
    return JSValue::encode(result);
}

JSNodeProxy* toJSNodeProxy(JSValue value)
{
    if (!value.isObject())
        return 0;
    const ClassInfo* classInfo = asObject(value)->classInfo();
    if (classInfo == JSNodeProxy::info())
        return jsCast<JSNodeProxy*>(asObject(value));
    return 0;
}

EncodedJSValue JSC_HOST_CALL jsNodeProxyGeneralMethodFunc(ExecState* exec){
    JSValue falseresult = jsBoolean(false);
    JSValue trueresult = jsBoolean(true);
    printf("JSNodeProxy  jsNodeProxyGeneralMethodFunc exec=%p\n",exec);


    JSNodeProxy* castedThis = toJSNodeProxy(exec->thisValue().toThis(exec, NotStrictMode));
    if (UNLIKELY(!castedThis))
        return throwVMTypeError(exec);
    ASSERT_GC_OBJECT_INHERITS(castedThis, JSNodeProxy::info());
    NodeProxy& impl = castedThis->impl();
    ScriptExecutionContext* scriptContext = jsCast<JSDOMGlobalObject*>(exec->lexicalGlobalObject())->scriptExecutionContext();
    if (!scriptContext)
        return JSValue::encode(falseresult);
    //impl.focus(scriptContext);
    return JSValue::encode(impl.exeMethod(exec));

}

bool JSNodeProxy::getOwnPropertySlotByIndex(JSObject* object, ExecState* exec, unsigned index, PropertySlot& slot)
{
    return false;
}
#endif
}



