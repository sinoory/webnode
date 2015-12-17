#include "config.h"
#include "JSNodeProxy.h"

#include "JSDOMBinding.h"
#include "JSDataTransfer.h"
#include <runtime/JSLock.h>
#include <wtf/HashMap.h>
#include <wtf/text/AtomicString.h>
#include <v8.h>
#include <libplatform/libplatform.h>

#include <sstream>
#include <iostream>

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
    const char* nm = propertyName.uid()->utf8().data();
    char type = impl.getPropertyType(nm);
    printf("JSNodeProxy::getOwnPropertySlot handle %s type=%c exec=%p argc=%d\n",nm,type,exec,exec->argumentCount());
    if(!impl.globalObject){
        impl.globalObject=thisObject->globalObject();
    }

    if(type=='f'){//function
        slot.setCustom(thisObject, ReadOnly | DontDelete | DontEnum, nonCachingStaticFunctionGetter<jsNodeProxyGeneralMethodFunc, 0>);
        impl.mMethod=std::string(nm);
        return true;
    }else if(type=='o'){//object
        slot.setCustom(thisObject, ReadOnly | DontDelete | DontEnum, jsNodeProxyGenralFunc);
        return true;
    }
#if 0
    if(propertyName=="write"){
        //for js : var res=np.write;
        //slot.setCustom(thisObject, ReadOnly | DontDelete | DontEnum, jsNodeProxyGenralFunc);

        if(!impl.globalObject){
            impl.globalObject=thisObject->globalObject();
        }
        //for js : var res=np.write();
        slot.setCustom(thisObject, ReadOnly | DontDelete | DontEnum, nonCachingStaticFunctionGetter<jsNodeProxyGeneralMethodFunc, 0>);
        impl.mMethod=std::string((char*)(propertyName.uid()->characters8()));
        return true;
    }
#endif

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

    char restype = impl.exeMethod(exec);
    if(restype=='o'){//object
        PassRefPtr<NodeProxy> np = new NodeProxy;
        np->globalObject=jsCast<JSDOMGlobalObject*>(exec->lexicalGlobalObject());
        std::ostringstream ostr;
        ostr<<EXE_RES_VAR<<NodeProxy::ExeCnt<<std::endl;
        np->mModuleName=ostr.str();
        JSValue jnp = toJS(exec, np->globalObject, WTF::getPtr(np));
        return JSValue::encode(jnp);
    }else {
    }

    return JSValue::encode(jsUndefined());
}

bool JSNodeProxy::getOwnPropertySlotByIndex(JSObject* object, ExecState* exec, unsigned index, PropertySlot& slot)
{
    return false;
}
#endif
}



