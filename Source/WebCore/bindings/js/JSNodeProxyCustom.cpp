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
#include "JSMainThreadExecState.h"

using namespace JSC;

namespace WebCore {
JSNodeProxy* toJSNodeProxy(JSValue value);
JSValue JSNodeProxy::isNodeProxyObj(ExecState* exec) const{
    printf("JSNodeProxy::isNodeProxyObj=true\n");
    return jsBoolean(true);
}
EncodedJSValue jsNodeProxyGenralFunc(ExecState* exec, JSObject* slotBase, EncodedJSValue thisValue, PropertyName);

EncodedJSValue JSC_HOST_CALL jsNodeProxyGeneralMethodFunc(ExecState* exec);

//add   CustomPutFunction in idl head
void JSNodeProxy::put(JSCell* cell, ExecState* exec, PropertyName propertyName, JSValue value, PutPropertySlot& slot)
{
    printf("JSNodeProxy::put propertyName=%s\n",propertyName.uid()->utf8().data());
    JSNodeProxy* thisObject = jsCast<JSNodeProxy*>(cell);
    NodeProxy& impl = thisObject->impl();
    impl.setProperty(exec,(const char*)(propertyName.uid()->utf8().data()),value);
}
void JSNodeProxy::putByIndex(JSCell* cell, ExecState* exec, unsigned index, JSValue value, bool shouldThrow)
{
    printf("JSNodeProxy::putByIndex index=%d\n",index);
} 

#if 1
bool JSNodeProxy::getOwnPropertySlot(JSObject* object, ExecState* exec, PropertyName propertyName, PropertySlot& slot)
{
    JSNodeProxy* thisObject = jsCast<JSNodeProxy*>(object);
    NodeProxy& impl = thisObject->impl();
    const char* nm = propertyName.uid()->utf8().data();
    std::ostringstream ostr;
    ostr<<impl.mModuleName<<"."<<nm;
    char type = NodeProxy::v8typeof(ostr.str().c_str());
    printf("JSNodeProxy::getOwnPropertySlot handle %s type=%c exec=%p argc=%d\n",nm,type,exec,exec->argumentCount());
    if(!impl.globalObject){
        impl.globalObject=thisObject->globalObject();
    }

    if(type=='f'){//function
        slot.setCustom(thisObject, ReadOnly | DontDelete | DontEnum, nonCachingStaticFunctionGetter<jsNodeProxyGeneralMethodFunc, 0>);
        impl.mMethod=std::string(nm);
        return true;
    }else {//prop
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
EncodedJSValue jsNodeProxyGenralFunc(ExecState* exec, JSObject* slotBase, EncodedJSValue thisValue, PropertyName propertyName)
{
    printf("jsNodeProxyGenralFunc argc=%d exe=%p\n",exec->argumentCount(),exec);
    if(propertyName=="isNodeProxyObj"){
        printf("%s isNodeProxyObj return ture\n",__func__);
        return JSValue::encode(jsBoolean(true));
    }

    JSNodeProxy* castedThis = toJSNodeProxy(JSValue::decode(thisValue));
    NodeProxy& impl = castedThis->impl();

    return JSValue::encode(impl.getProp(exec,(const char*)(propertyName.uid()->utf8().data())));


    UNUSED_PARAM(exec);
    UNUSED_PARAM(slotBase);
    UNUSED_PARAM(thisValue);
    JSValue result = jsBoolean(false);
    //JSValue result = jsBoolean(impl.closed());
    printf("JSNodeProxy 1 jsNodeProxyGenralFunc\n");

    std::ostringstream ostr;
    ostr << "function testfunc(){print('in testfunc');} ; eval(testfunc) " ;
    printf("===JSDOMWindow::require %s\n",ostr.str().c_str());
    JSValue evaluationException;
    String jsstr=String::fromUTF8WithLatin1Fallback(ostr.str().c_str(),strlen(ostr.str().c_str()));
    SourceCode jsc = makeSource(jsstr, "nodeproxycls");
    JSValue returnValue = JSMainThreadExecState::evaluate(exec,jsc,JSValue(), &evaluationException);
    return JSValue::encode(returnValue);
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

    JSValue res = impl.exeMethod(exec);
    return JSValue::encode(res);
    /*    
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
    */

    return JSValue::encode(jsUndefined());
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

bool JSNodeProxy::getOwnPropertySlotByIndex(JSObject* object, ExecState* exec, unsigned index, PropertySlot& slot)
{
    return false;
}
#endif
}



