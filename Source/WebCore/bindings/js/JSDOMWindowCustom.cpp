/*
 * Copyright (C) 2007, 2008, 2009, 2010 Apple Inc. All rights reserved.
 * Copyright (C) 2011 Google Inc. All rights reserved.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public License
 * along with this library; see the file COPYING.LIB.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 */

#include "config.h"
#include "JSDOMWindowCustom.h"

#include "Frame.h"
#include "HTMLCollection.h"
#include "HTMLDocument.h"
#include "JSEvent.h"
#include "JSEventListener.h"
#include "JSHTMLAudioElement.h"
#include "JSHTMLCollection.h"
#include "JSHTMLOptionElement.h"
#include "JSImageConstructor.h"
#include "JSMessagePortCustom.h"
#include "JSWorker.h"
#include "Location.h"
#include "ScheduledAction.h"
#include "Settings.h"
#include "JSNodeProxy.h"
#include "JSMainThreadExecState.h"

#include <sstream>
#include <iostream>

#if ENABLE(IOS_TOUCH_EVENTS)
#include "JSTouchConstructorIOS.h"
#include "JSTouchListConstructorIOS.h"
#endif

#if ENABLE(WEB_AUDIO)
#include "JSAudioContext.h"
#endif

#if ENABLE(WEB_SOCKETS)
#include "JSWebSocket.h"
#endif

#if ENABLE(USER_MESSAGE_HANDLERS)
#include "JSWebKitNamespace.h"
#endif
#include <runtime/JSONObject.h>

using namespace JSC;

namespace WebCore {

void JSDOMWindow::visitAdditionalChildren(SlotVisitor& visitor)
{
    if (Frame* frame = impl().frame())
        visitor.addOpaqueRoot(frame);
}

static EncodedJSValue childFrameGetter(ExecState* exec, JSObject* slotBase, EncodedJSValue, PropertyName propertyName)
{
    return JSValue::encode(toJS(exec, jsCast<JSDOMWindow*>(slotBase)->impl().frame()->tree().scopedChild(propertyNameToAtomicString(propertyName))->document()->domWindow()));
}

static EncodedJSValue namedItemGetter(ExecState* exec, JSObject* slotBase, EncodedJSValue, PropertyName propertyName)
{
    JSDOMWindowBase* thisObj = jsCast<JSDOMWindow*>(slotBase);
    Document* document = thisObj->impl().frame()->document();

    ASSERT(BindingSecurity::shouldAllowAccessToDOMWindow(exec, thisObj->impl()));
    ASSERT(is<HTMLDocument>(document));

    AtomicStringImpl* atomicPropertyName = propertyName.publicName();
    if (!atomicPropertyName || !downcast<HTMLDocument>(*document).hasWindowNamedItem(*atomicPropertyName))
        return JSValue::encode(jsUndefined());

    if (UNLIKELY(downcast<HTMLDocument>(*document).windowNamedItemContainsMultipleElements(*atomicPropertyName))) {
        Ref<HTMLCollection> collection = document->windowNamedItems(atomicPropertyName);
        ASSERT(collection->length() > 1);
        return JSValue::encode(toJS(exec, thisObj->globalObject(), WTF::getPtr(collection)));
    }

    return JSValue::encode(toJS(exec, thisObj->globalObject(), downcast<HTMLDocument>(*document).windowNamedItem(*atomicPropertyName)));
}

#if ENABLE(USER_MESSAGE_HANDLERS)
static EncodedJSValue jsDOMWindowWebKit(ExecState* exec, JSObject*, EncodedJSValue thisValue, PropertyName)
{
    JSDOMWindow* castedThis = toJSDOMWindow(JSValue::decode(thisValue));
    if (!BindingSecurity::shouldAllowAccessToDOMWindow(exec, castedThis->impl()))
        return JSValue::encode(jsUndefined());
    return JSValue::encode(toJS(exec, castedThis->globalObject(), castedThis->impl().webkitNamespace()));
}
#endif

EncodedJSValue jsNodeProxyGlobalObject(ExecState* exec, JSObject* slotBase, EncodedJSValue thisValue, PropertyName);

bool JSDOMWindow::getOwnPropertySlot(JSObject* object, ExecState* exec, PropertyName propertyName, PropertySlot& slot)
{
    JSDOMWindow* thisObject = jsCast<JSDOMWindow*>(object);
    // When accessing a Window cross-domain, functions are always the native built-in ones, and they
    // are not affected by properties changed on the Window or anything in its prototype chain.
    // This is consistent with the behavior of Firefox.

    // We don't want any properties other than "close" and "closed" on a frameless window (i.e. one whose page got closed,
    // or whose iframe got removed).
    // FIXME: This doesn't fully match Firefox, which allows at least toString in addition to those.
    if (!thisObject->impl().frame()) {
        // The following code is safe for cross-domain and same domain use.
        // It ignores any custom properties that might be set on the DOMWindow (including a custom prototype).
        if (propertyName == exec->propertyNames().closed) {
            slot.setCustom(thisObject, ReadOnly | DontDelete | DontEnum, jsDOMWindowClosed);
            return true;
        }
        if (propertyName == exec->propertyNames().close) {
            slot.setCustom(thisObject, ReadOnly | DontDelete | DontEnum, nonCachingStaticFunctionGetter<jsDOMWindowPrototypeFunctionClose, 0>);
            return true;
        }

        // FIXME: We should have a message here that explains why the property access/function call was
        // not allowed. 
        slot.setUndefined();
        return true;
    } else
        slot.setWatchpointSet(thisObject->m_windowCloseWatchpoints);

    // We need to check for cross-domain access here without printing the generic warning message
    // because we always allow access to some function, just different ones depending whether access
    // is allowed.
    String errorMessage;
    bool allowsAccess = shouldAllowAccessToDOMWindow(exec, thisObject->impl(), errorMessage);
    
    // Look for overrides before looking at any of our own properties, but ignore overrides completely
    // if this is cross-domain access.
    if (allowsAccess && JSGlobalObject::getOwnPropertySlot(thisObject, exec, propertyName, slot))
        return true;
    
    // We need this code here because otherwise JSDOMWindowBase will stop the search before we even get to the
    // prototype due to the blanket same origin (shouldAllowAccessToDOMWindow) check at the end of getOwnPropertySlot.
    // Also, it's important to get the implementation straight out of the DOMWindow prototype regardless of
    // what prototype is actually set on this object.
    if (propertyName == exec->propertyNames().blur) {
        slot.setCustom(thisObject, ReadOnly | DontDelete | DontEnum, nonCachingStaticFunctionGetter<jsDOMWindowPrototypeFunctionBlur, 0>);
        return true;
    } else if (propertyName == exec->propertyNames().close) {
        slot.setCustom(thisObject, ReadOnly | DontDelete | DontEnum, nonCachingStaticFunctionGetter<jsDOMWindowPrototypeFunctionClose, 0>);
        return true;
    } else if (propertyName == exec->propertyNames().focus) {
        slot.setCustom(thisObject, ReadOnly | DontDelete | DontEnum, nonCachingStaticFunctionGetter<jsDOMWindowPrototypeFunctionFocus, 0>);
        return true;
    } else if (propertyName == exec->propertyNames().postMessage) {
        slot.setCustom(thisObject, ReadOnly | DontDelete | DontEnum, nonCachingStaticFunctionGetter<jsDOMWindowPrototypeFunctionPostMessage, 2>);
        return true;
    } else if (propertyName == exec->propertyNames().showModalDialog) {
        if (!DOMWindow::canShowModalDialog(thisObject->impl().frame())) {
            slot.setUndefined();
            return true;
        }
    } else if (propertyName == exec->propertyNames().toString) {
        // Allow access to toString() cross-domain, but always Object.prototype.toString.
        if (!allowsAccess) {
            slot.setCustom(thisObject, ReadOnly | DontDelete | DontEnum, objectToStringFunctionGetter);
            return true;
        }
    } else if(propertyName == "process"){
        slot.setCustom(thisObject, ReadOnly | DontDelete | DontEnum, jsNodeProxyGlobalObject);
        return true;
    }

    const HashTableValue* entry = JSDOMWindow::info()->staticPropHashTable->entry(propertyName);
    if (entry) {
        slot.setCacheableCustom(thisObject, allowsAccess ? entry->attributes() : ReadOnly | DontDelete | DontEnum, entry->propertyGetter());
        return true;
    }

#if ENABLE(USER_MESSAGE_HANDLERS)
    if (propertyName == exec->propertyNames().webkit && thisObject->impl().shouldHaveWebKitNamespaceForWorld(thisObject->world())) {
        slot.setCacheableCustom(thisObject, allowsAccess ? DontDelete | ReadOnly : ReadOnly | DontDelete | DontEnum, jsDOMWindowWebKit);
        return true;
    }
#endif

    // Do prototype lookup early so that functions and attributes in the prototype can have
    // precedence over the index and name getters.  
    JSValue proto = thisObject->prototype();
    if (proto.isObject()) {
        if (asObject(proto)->getPropertySlot(exec, propertyName, slot)) {
            if (!allowsAccess) {
                thisObject->printErrorMessage(errorMessage);
                slot.setUndefined();
            }
            return true;
        }
    }

    // After this point it is no longer valid to cache any results because of
    // the impure nature of the property accesses which follow. We can move this 
    // statement further down when we add ways to mitigate these impurities with, 
    // for example, watchpoints.
    slot.disableCaching();

    // Check for child frames by name before built-in properties to
    // match Mozilla. This does not match IE, but some sites end up
    // naming frames things that conflict with window properties that
    // are in Moz but not IE. Since we have some of these, we have to do
    // it the Moz way.
    if (thisObject->impl().frame()->tree().scopedChild(propertyNameToAtomicString(propertyName))) {
        slot.setCustom(thisObject, ReadOnly | DontDelete | DontEnum, childFrameGetter);
        return true;
    }

    // FIXME: Search the whole frame hierarchy somewhere around here.
    // We need to test the correct priority order.

    // allow window[1] or parent[1] etc. (#56983)
    unsigned i = propertyName.asIndex();
    if (i < thisObject->impl().frame()->tree().scopedChildCount()) {
        ASSERT(i != PropertyName::NotAnIndex);
        slot.setValue(thisObject, ReadOnly | DontDelete | DontEnum,
            toJS(exec, thisObject->impl().frame()->tree().scopedChild(i)->document()->domWindow()));
        return true;
    }

    if (!allowsAccess) {
        thisObject->printErrorMessage(errorMessage);
        slot.setUndefined();
        return true;
    }

    // Allow shortcuts like 'Image1' instead of document.images.Image1
    Document* document = thisObject->impl().frame()->document();
    if (is<HTMLDocument>(*document)) {
        AtomicStringImpl* atomicPropertyName = propertyName.publicName();
        if (atomicPropertyName && downcast<HTMLDocument>(*document).hasWindowNamedItem(*atomicPropertyName)) {
            slot.setCustom(thisObject, ReadOnly | DontDelete | DontEnum, namedItemGetter);
            return true;
        }
    }

    return Base::getOwnPropertySlot(thisObject, exec, propertyName, slot);
}

EncodedJSValue jsNodeProxyGlobalObject(ExecState* exec, JSObject* slotBase, EncodedJSValue thisValue, PropertyName propertyName)
{
    PassRefPtr<NodeProxy> np = new NodeProxy;
    np->globalObject=jsCast<JSDOMGlobalObject*>(exec->lexicalGlobalObject());
    np->mModuleName=std::string((const char*)(propertyName.uid()->utf8().data()));
    JSValue jnp = toJS(exec, np->globalObject, WTF::getPtr(np));
    return JSValue::encode(jnp);

}

bool JSDOMWindow::getOwnPropertySlotByIndex(JSObject* object, ExecState* exec, unsigned index, PropertySlot& slot)
{
    JSDOMWindow* thisObject = jsCast<JSDOMWindow*>(object);
    
    if (!thisObject->impl().frame()) {
        // FIXME: We should have a message here that explains why the property access/function call was
        // not allowed. 
        slot.setUndefined();
        return true;
    }

    // We need to check for cross-domain access here without printing the generic warning message
    // because we always allow access to some function, just different ones depending whether access
    // is allowed.
    String errorMessage;
    bool allowsAccess = shouldAllowAccessToDOMWindow(exec, thisObject->impl(), errorMessage);

    // Look for overrides before looking at any of our own properties, but ignore overrides completely
    // if this is cross-domain access.
    if (allowsAccess && JSGlobalObject::getOwnPropertySlotByIndex(thisObject, exec, index, slot))
        return true;
    
    PropertyName propertyName = Identifier::from(exec, index);
    
    // Check for child frames by name before built-in properties to
    // match Mozilla. This does not match IE, but some sites end up
    // naming frames things that conflict with window properties that
    // are in Moz but not IE. Since we have some of these, we have to do
    // it the Moz way.
    if (thisObject->impl().frame()->tree().scopedChild(propertyNameToAtomicString(propertyName))) {
        slot.setCustom(thisObject, ReadOnly | DontDelete | DontEnum, childFrameGetter);
        return true;
    }
    
    // Do prototype lookup early so that functions and attributes in the prototype can have
    // precedence over the index and name getters.  
    JSValue proto = thisObject->prototype();
    if (proto.isObject()) {
        if (asObject(proto)->getPropertySlot(exec, index, slot)) {
            if (!allowsAccess) {
                thisObject->printErrorMessage(errorMessage);
                slot.setUndefined();
            }
            return true;
        }
    }

    // FIXME: Search the whole frame hierarchy somewhere around here.
    // We need to test the correct priority order.

    // allow window[1] or parent[1] etc. (#56983)
    if (index < thisObject->impl().frame()->tree().scopedChildCount()) {
        ASSERT(index != PropertyName::NotAnIndex);
        slot.setValue(thisObject, ReadOnly | DontDelete | DontEnum,
            toJS(exec, thisObject->impl().frame()->tree().scopedChild(index)->document()->domWindow()));
        return true;
    }

    if (!allowsAccess) {
        thisObject->printErrorMessage(errorMessage);
        slot.setUndefined();
        return true;
    }

    // Allow shortcuts like 'Image1' instead of document.images.Image1
    Document* document = thisObject->impl().frame()->document();
    if (is<HTMLDocument>(*document)) {
        AtomicStringImpl* atomicPropertyName = propertyName.publicName();
        if (atomicPropertyName && downcast<HTMLDocument>(*document).hasWindowNamedItem(*atomicPropertyName)) {
            slot.setCustom(thisObject, ReadOnly | DontDelete | DontEnum, namedItemGetter);
            return true;
        }
    }

    return Base::getOwnPropertySlotByIndex(thisObject, exec, index, slot);
}

JSValue JSDOMWindow::require(ExecState* exec){
    const String& module(exec->argument(0).isEmpty() ? String() : exec->argument(0).toString(exec)->value(exec));
    PassRefPtr<NodeProxy> np = impl().require(module);
    //printf("======JSDOMWindow::require lastRequireIsObject=%d\n",np->lastRequireIsObject);
    if(np->lastRequireIsObject){
        JSValue result = toJS(exec, this->globalObject(), WTF::getPtr(np));
        return result;
    }

    std::ostringstream ostr;
    //TODO: also need pass arguments to requireObjFromClass to new a real object
    ostr << "function __NODE_PROXY_CLS__"<<++NodeProxy::FunProxyCnt<<"(){return node_require_obj_from_class('"<< module.ascii().data() <<"');} ; __NODE_PROXY_CLS__"<<NodeProxy::FunProxyCnt<<".__nodejs_func_prop_get__"<<NodeProxy::FunProxyCnt<<"=function(prop){ return node_get_prop_from_required_class('"<< module.ascii().data() <<"',prop); } ;__NODE_PROXY_CLS__"<<NodeProxy::FunProxyCnt<<".__nodejs_func_prop_set__=function(prop,value){node_set_prop_from_required_class('"<<module.ascii().data()<<"',prop,value);} ; eval(__NODE_PROXY_CLS__"<<NodeProxy::FunProxyCnt<<") " << std::endl;
    printf("===JSDOMWindow::require %s\n",ostr.str().c_str());
    JSValue evaluationException;
    String jsstr=String::fromUTF8WithLatin1Fallback(ostr.str().c_str(),strlen(ostr.str().c_str()));
    SourceCode jsc = makeSource(jsstr, "nodeproxycls");
    JSValue returnValue = JSMainThreadExecState::evaluate(exec,jsc,JSValue(), &evaluationException);
    return returnValue;

}
JSValue JSDOMWindow::node_require_obj_from_class(ExecState* exec){
    const String& module(exec->argument(0).isEmpty() ? String() : exec->argument(0).toString(exec)->value(exec));
    PassRefPtr<NodeProxy> np = impl()._require_obj_from_class_(module);
    printf("======JSDOMWindow::require obj from class\n");
    JSValue result = toJS(exec, this->globalObject(), WTF::getPtr(np));
    return result;

}

JSValue JSDOMWindow::node_get_prop_from_required_class(ExecState* exec){
    const String& module(exec->argument(0).toString(exec)->value(exec));
    const String& prop(exec->argument(1).toString(exec)->value(exec));
    std::ostringstream ostr;
    ostr<<EXE_RES_VAR<<++NodeProxy::ExeCnt
        <<"=require('"<<module.utf8().data()<<"')."<<prop.utf8().data();

    v8::HandleScope scope(v8::Isolate::GetCurrent());
    v8::Handle<v8::Value> result = NodeProxy::execStringInV8(ostr.str().c_str());

    //
    ostr.str("");
    ostr<<EXE_RES_VAR<<NodeProxy::ExeCnt;
    char type = NodeProxy::v8typeof(ostr.str().c_str());

    printf("%s %s type=%c \n",__func__,ostr.str().c_str(),type);
    if(type=='f'){
        ostr.str("");
        ostr << "function __NODE_PROXY_CLS__"<<++NodeProxy::FunProxyCnt<<"(){"
                    "var args=Array.prototype.slice.call(arguments);"
                    "args.splice(0,0,'"<<EXE_RES_VAR<<NodeProxy::ExeCnt<<"'); " //p1 is func
                    "return node_proxy_cls_exe_fun.apply(window,args);"
                "} ; "
                "__NODE_PROXY_CLS__"<<NodeProxy::FunProxyCnt<<".__nodejs_func_prop_get__"<<NodeProxy::FunProxyCnt<<"=function(prop){"
                    " return node_proxy_cls_get_prop('"<< EXE_RES_VAR<<NodeProxy::ExeCnt <<"',prop); "
                "} ; "
                "__NODE_PROXY_CLS__"<<NodeProxy::FunProxyCnt<<".__nodejs_func_prop_set__"<<NodeProxy::FunProxyCnt<<"=function(prop,value){"
                    "node_proxy_cls_set_prop('"<<EXE_RES_VAR<<NodeProxy::ExeCnt<<"',prop,value);"
                "} ; "
                "eval(__NODE_PROXY_CLS__"<<NodeProxy::FunProxyCnt<<") " << std::endl;
        printf("%s f=%s\n",__func__,ostr.str().c_str());
        JSValue evaluationException;
        String jsstr=String::fromUTF8WithLatin1Fallback(ostr.str().c_str(),strlen(ostr.str().c_str()));
        SourceCode jsc = makeSource(jsstr, "nodeproxycls");
        JSValue returnValue = JSMainThreadExecState::evaluate(exec,jsc,JSValue(), &evaluationException);
        return returnValue;
    }else {
        printf("%s res is not obj isolate=%p\n",__func__,v8::Isolate::GetCurrent());
        v8::Local<v8::Integer> i = v8::Integer::New(v8::Isolate::GetCurrent(),32);
        return NodeProxy::v8data2jsc(v8::Isolate::GetCurrent(),result,exec,ostr.str().c_str());
    }
    return jsNull();
}

JSValue JSDOMWindow::node_set_prop_from_required_class(ExecState* exec){
    return jsNull();
}

JSValue JSDOMWindow::node_proxy_cls_get_prop(ExecState* exec){
    const String& module(exec->argument(0).toString(exec)->value(exec));
    const String& prop(exec->argument(1).toString(exec)->value(exec));
    printf("%s get %s\n",__func__,prop.utf8().data());

    std::ostringstream ostr;
    ostr<<EXE_RES_VAR<<++NodeProxy::ExeCnt<<"="<<module.utf8().data()<<"."<<prop.utf8().data();

    v8::Isolate* isolate = v8::Isolate::GetCurrent();
    v8::HandleScope scope(isolate);
    v8::Handle<v8::Value> result = NodeProxy::execStringInV8(ostr.str().c_str());

    ostr.str("");
    ostr<<EXE_RES_VAR<<NodeProxy::ExeCnt;
    char type = NodeProxy::v8typeof(ostr.str().c_str());

    printf("%s %s type=%c \n",__func__,ostr.str().c_str(),type);
    if(type=='o'){
        PassRefPtr<NodeProxy> np = new NodeProxy;
        np->globalObject=jsCast<JSDOMGlobalObject*>(exec->lexicalGlobalObject());
        np->mModuleName=std::string(ostr.str());
        JSValue jnp = toJS(exec, np->globalObject, WTF::getPtr(np));
        return jnp;
    }else{
        return NodeProxy::v8data2jsc(v8::Isolate::GetCurrent(),result,exec,ostr.str().c_str());
    }

    return jsUndefined();
}

JSValue JSDOMWindow::node_proxy_cls_set_prop(ExecState* exec){
    const String& module(exec->argument(0).toString(exec)->value(exec));
    const String& prop(exec->argument(1).toString(exec)->value(exec));
    JSValue value=exec->uncheckedArgument(2);
    printf("%s set %s\n",__func__,prop.utf8().data());

    std::ostringstream ostr;
    ostr<<module.utf8().data()<<"."<<prop.utf8().data()<<"=";

    bool p=false;
    if(value.isFunction()){
        printf("=======%s value=func\n",__func__);
        ostr <<"nodeproxyCb";  
        if(NodeProxy::m_data !=0){
            delete NodeProxy::m_data;
        }
        NodeProxy::m_data=new JSCallbackData(asObject(value), jsCast<JSDOMGlobalObject*>(exec->lexicalGlobalObject()));
        p=true;
    }else if(value.isString()){
        ostr<<value.toString(exec)->value(exec).utf8().data()<<"'";
        p=true;
    }else if(value.isNumber()){
        ostr<<value.toNumber(exec);
        p=true;
    }else if(value.isObject()){//to be the last one
        JSValue isnodeproxyobj=value.get(exec,Identifier(exec,"isNodeProxyObj"));
        if(isnodeproxyobj.isTrue()){
            JSNodeProxy* jnp = jsCast<JSNodeProxy*>(value);
            printf("%s param is nodeproxyobj,module=%s\n",__func__,
                    jnp->impl().mModuleName.c_str());
            ostr<<jnp->impl().mModuleName.c_str();
            p=true;
        }else{
            const char* s = value.toString(exec)->value(exec).utf8().data();
            String ss = JSONStringify(exec,value,0);
            if(ss.startsWith('[')){//object is an array
                printf("%s TODO: array param\n",__func__);
            }else if(ss.startsWith('{')){//a json or a object,convert to json 
                //TODO: if is object, convert to json will be empty
                ostr<<"JSON.parse('"<<ss.utf8().data()<<"')";
                p=true;
            }else{
                printf("%s TODO: unkown type\n",__func__);
            }
        }
    }else{
        printf("%s TODO: unkown type\n",__func__);
    }
    if(p){
        v8::Isolate* isolate = v8::Isolate::GetCurrent();
        v8::HandleScope scope(isolate);
        v8::Handle<v8::Value> result = NodeProxy::execStringInV8(ostr.str().c_str());
    }
    return jsNull();
}

JSValue JSDOMWindow::node_proxy_cls_exe_fun(ExecState* exec){
    std::ostringstream ostr;

    v8::Isolate* isolate = v8::Isolate::GetCurrent();
    v8::HandleScope scope(isolate);
    v8::Local<v8::Context> g_context =
        v8::Local<v8::Context>::New(isolate, node::g_context);

    JSValue func=exec->uncheckedArgument(0);
    ostr<<EXE_RES_VAR<<++NodeProxy::ExeCnt<<" = " <<func.toString(exec)->value(exec).utf8().data()<<"(";
    printf("%s argc=%d early=%s\n",__func__,exec->argumentCount(),ostr.str().c_str());

    v8::Context::Scope cscope(g_context);{
        v8::TryCatch try_catch;
        bool pa=false;
        for(int i=1;i<exec->argumentCount();i++){
            JSValue value=exec->uncheckedArgument(i);

            if(value.isFunction()){
                ostr<< (pa?",":"") <<"nodeproxyCb";  
                if(NodeProxy::m_data !=0){
                    delete NodeProxy::m_data;
                }
                NodeProxy::m_data=new JSCallbackData(asObject(value), jsCast<JSDOMGlobalObject*>(exec->lexicalGlobalObject()));
            }else if(value.isString()){
                printf("string param=%s\n",value.toString(exec)->value(exec).utf8().data());
                ostr<<(pa?",'":"'")<<value.toString(exec)->value(exec).utf8().data()<<"'";
                pa=true;
            }else if(value.isNumber()){
                ostr<< (pa?",":"") <<value.toNumber(exec);
                pa=true;
            }else if(value.isObject()){//to be the last one
                JSValue isnodeproxyobj=value.get(exec,Identifier(exec,"isNodeProxyObj"));
                if(isnodeproxyobj.isTrue()){
                    JSNodeProxy* jnp = jsCast<JSNodeProxy*>(value);
                    printf("%s param is nodeproxyobj,module=%s\n",__func__,
                        jnp->impl().mModuleName.c_str());
                    ostr<<(pa?",":"")<<jnp->impl().mModuleName.c_str();
                    continue; 
                }
                const char* s = value.toString(exec)->value(exec).utf8().data();
                String ss = JSONStringify(exec,value,0);
                printf("setProperty unhandled object s=%s js=%s\n",s,ss.utf8().data());
                if(ss.startsWith('[')){//object is an array
                }else if(ss.startsWith('{')){//a json or a object,convert to json 
                    //TODO: if is object, convert to json will be empty
                    ostr<<(pa?",":"")<<"JSON.parse('"<<ss.utf8().data()<<"')";
                    pa=true;
                }
            }else{
                printf("%s TODO: unkown type\n",__func__);
            }
        }
        ostr <<");";
        printf("%s exes=%s\n",__func__,ostr.str().c_str());

        v8::Handle<v8::Value> result = NodeProxy::execStringInV8(ostr.str().c_str());

        ostr.str("");
        ostr<<EXE_RES_VAR<<NodeProxy::ExeCnt;

        printf("%s %s  \n",__func__,ostr.str().c_str());
        if(result->IsObject()){
            PassRefPtr<NodeProxy> np = new NodeProxy;
            np->globalObject=jsCast<JSDOMGlobalObject*>(exec->lexicalGlobalObject());
            np->mModuleName=std::string(ostr.str());
            JSValue jnp = toJS(exec, np->globalObject, WTF::getPtr(np));
            return jnp;
        }else{
            return NodeProxy::v8data2jsc(v8::Isolate::GetCurrent(),result,exec,ostr.str().c_str());
        }

    }
    return jsNull();
}

void JSDOMWindow::put(JSCell* cell, ExecState* exec, PropertyName propertyName, JSValue value, PutPropertySlot& slot)
{
    JSDOMWindow* thisObject = jsCast<JSDOMWindow*>(cell);
    if (!thisObject->impl().frame())
        return;

    // Optimization: access JavaScript global variables directly before involving the DOM.
    if (thisObject->JSGlobalObject::hasOwnPropertyForWrite(exec, propertyName)) {
        if (BindingSecurity::shouldAllowAccessToDOMWindow(exec, thisObject->impl()))
            JSGlobalObject::put(thisObject, exec, propertyName, value, slot);
        return;
    }

    if (lookupPut(exec, propertyName, thisObject, value, *s_info.staticPropHashTable, slot))
        return;

    if (BindingSecurity::shouldAllowAccessToDOMWindow(exec, thisObject->impl()))
        Base::put(thisObject, exec, propertyName, value, slot);
}

void JSDOMWindow::putByIndex(JSCell* cell, ExecState* exec, unsigned index, JSValue value, bool shouldThrow)
{
    JSDOMWindow* thisObject = jsCast<JSDOMWindow*>(cell);
    if (!thisObject->impl().frame())
        return;
    
    PropertyName propertyName = Identifier::from(exec, index);

    // Optimization: access JavaScript global variables directly before involving the DOM.
    if (thisObject->JSGlobalObject::hasOwnPropertyForWrite(exec, propertyName)) {
        if (BindingSecurity::shouldAllowAccessToDOMWindow(exec, thisObject->impl()))
            JSGlobalObject::putByIndex(thisObject, exec, index, value, shouldThrow);
        return;
    }
    
    if (BindingSecurity::shouldAllowAccessToDOMWindow(exec, thisObject->impl()))
        Base::putByIndex(thisObject, exec, index, value, shouldThrow);
}

bool JSDOMWindow::deleteProperty(JSCell* cell, ExecState* exec, PropertyName propertyName)
{
    JSDOMWindow* thisObject = jsCast<JSDOMWindow*>(cell);
    // Only allow deleting properties by frames in the same origin.
    if (!BindingSecurity::shouldAllowAccessToDOMWindow(exec, thisObject->impl()))
        return false;
    return Base::deleteProperty(thisObject, exec, propertyName);
}

bool JSDOMWindow::deletePropertyByIndex(JSCell* cell, ExecState* exec, unsigned propertyName)
{
    JSDOMWindow* thisObject = jsCast<JSDOMWindow*>(cell);
    // Only allow deleting properties by frames in the same origin.
    if (!BindingSecurity::shouldAllowAccessToDOMWindow(exec, thisObject->impl()))
        return false;
    return Base::deletePropertyByIndex(thisObject, exec, propertyName);
}

uint32_t JSDOMWindow::getEnumerableLength(ExecState* exec, JSObject* object)
{
    JSDOMWindow* thisObject = jsCast<JSDOMWindow*>(object);
    // Only allow the window to enumerated by frames in the same origin.
    if (!BindingSecurity::shouldAllowAccessToDOMWindow(exec, thisObject->impl()))
        return 0;
    return Base::getEnumerableLength(exec, thisObject);
}

void JSDOMWindow::getStructurePropertyNames(JSObject* object, ExecState* exec, PropertyNameArray& propertyNames, EnumerationMode mode)
{
    JSDOMWindow* thisObject = jsCast<JSDOMWindow*>(object);
    // Only allow the window to enumerated by frames in the same origin.
    if (!BindingSecurity::shouldAllowAccessToDOMWindow(exec, thisObject->impl()))
        return;
    Base::getStructurePropertyNames(thisObject, exec, propertyNames, mode);
}

void JSDOMWindow::getGenericPropertyNames(JSObject* object, ExecState* exec, PropertyNameArray& propertyNames, EnumerationMode mode)
{
    JSDOMWindow* thisObject = jsCast<JSDOMWindow*>(object);
    // Only allow the window to enumerated by frames in the same origin.
    if (!BindingSecurity::shouldAllowAccessToDOMWindow(exec, thisObject->impl()))
        return;
    Base::getGenericPropertyNames(thisObject, exec, propertyNames, mode);
}

void JSDOMWindow::getPropertyNames(JSObject* object, ExecState* exec, PropertyNameArray& propertyNames, EnumerationMode mode)
{
    JSDOMWindow* thisObject = jsCast<JSDOMWindow*>(object);
    // Only allow the window to enumerated by frames in the same origin.
    if (!BindingSecurity::shouldAllowAccessToDOMWindow(exec, thisObject->impl()))
        return;
    Base::getPropertyNames(thisObject, exec, propertyNames, mode);
}

void JSDOMWindow::getOwnPropertyNames(JSObject* object, ExecState* exec, PropertyNameArray& propertyNames, EnumerationMode mode)
{
    JSDOMWindow* thisObject = jsCast<JSDOMWindow*>(object);
    // Only allow the window to enumerated by frames in the same origin.
    if (!BindingSecurity::shouldAllowAccessToDOMWindow(exec, thisObject->impl()))
        return;
    Base::getOwnPropertyNames(thisObject, exec, propertyNames, mode);
}

bool JSDOMWindow::defineOwnProperty(JSC::JSObject* object, JSC::ExecState* exec, JSC::PropertyName propertyName, const JSC::PropertyDescriptor& descriptor, bool shouldThrow)
{
    JSDOMWindow* thisObject = jsCast<JSDOMWindow*>(object);
    // Only allow defining properties in this way by frames in the same origin, as it allows setters to be introduced.
    if (!BindingSecurity::shouldAllowAccessToDOMWindow(exec, thisObject->impl()))
        return false;

    // Don't allow shadowing location using accessor properties.
    if (descriptor.isAccessorDescriptor() && propertyName == Identifier(exec, "location"))
        return false;

    return Base::defineOwnProperty(thisObject, exec, propertyName, descriptor, shouldThrow);
}

// Custom Attributes

void JSDOMWindow::setLocation(ExecState* exec, JSValue value)
{
#if ENABLE(DASHBOARD_SUPPORT)
    // To avoid breaking old widgets, make "var location =" in a top-level frame create
    // a property named "location" instead of performing a navigation (<rdar://problem/5688039>).
    if (Frame* activeFrame = activeDOMWindow(exec).frame()) {
        if (activeFrame->settings().usesDashboardBackwardCompatibilityMode() && !activeFrame->tree().parent()) {
            if (BindingSecurity::shouldAllowAccessToDOMWindow(exec, impl()))
                putDirect(exec->vm(), Identifier(exec, "location"), value);
            return;
        }
    }
#endif

    String locationString = value.toString(exec)->value(exec);
    if (exec->hadException())
        return;

    if (Location* location = impl().location())
        location->setHref(locationString, activeDOMWindow(exec), firstDOMWindow(exec));
}

JSValue JSDOMWindow::event(ExecState* exec) const
{
    Event* event = currentEvent();
    if (!event)
        return jsUndefined();
    return toJS(exec, const_cast<JSDOMWindow*>(this), event);
}

JSValue JSDOMWindow::image(ExecState* exec) const
{
    return getDOMConstructor<JSImageConstructor>(exec->vm(), this);
}

#if ENABLE(IOS_TOUCH_EVENTS)
JSValue JSDOMWindow::touch(ExecState* exec) const
{
    return getDOMConstructor<JSTouchConstructor>(exec->vm(), this);
}

JSValue JSDOMWindow::touchList(ExecState* exec) const
{
    return getDOMConstructor<JSTouchListConstructor>(exec->vm(), this);
}
#endif

// Custom functions

JSValue JSDOMWindow::open(ExecState* exec)
{
    String urlString = valueToStringWithUndefinedOrNullCheck(exec, exec->argument(0));
    if (exec->hadException())
        return jsUndefined();
    AtomicString frameName = exec->argument(1).isUndefinedOrNull() ? "_blank" : exec->argument(1).toString(exec)->value(exec);
    if (exec->hadException())
        return jsUndefined();
    String windowFeaturesString = valueToStringWithUndefinedOrNullCheck(exec, exec->argument(2));
    if (exec->hadException())
        return jsUndefined();

    RefPtr<DOMWindow> openedWindow = impl().open(urlString, frameName, windowFeaturesString, activeDOMWindow(exec), firstDOMWindow(exec));
    if (!openedWindow)
        return jsUndefined();
    return toJS(exec, openedWindow.get());
}

class DialogHandler {
public:
    explicit DialogHandler(ExecState* exec)
        : m_exec(exec)
    {
    }

    void dialogCreated(DOMWindow&);
    JSValue returnValue() const;

private:
    ExecState* m_exec;
    RefPtr<Frame> m_frame;
};

inline void DialogHandler::dialogCreated(DOMWindow& dialog)
{
    m_frame = dialog.frame();
    
    // FIXME: This looks like a leak between the normal world and an isolated
    //        world if dialogArguments comes from an isolated world.
    JSDOMWindow* globalObject = toJSDOMWindow(m_frame.get(), normalWorld(m_exec->vm()));
    if (JSValue dialogArguments = m_exec->argument(1))
        globalObject->putDirect(m_exec->vm(), Identifier(m_exec, "dialogArguments"), dialogArguments);
}

inline JSValue DialogHandler::returnValue() const
{
    JSDOMWindow* globalObject = toJSDOMWindow(m_frame.get(), normalWorld(m_exec->vm()));
    if (!globalObject)
        return jsUndefined();
    Identifier identifier(m_exec, "returnValue");
    PropertySlot slot(globalObject);
    if (!JSGlobalObject::getOwnPropertySlot(globalObject, m_exec, identifier, slot))
        return jsUndefined();
    return slot.getValue(m_exec, identifier);
}

JSValue JSDOMWindow::showModalDialog(ExecState* exec)
{
    String urlString = valueToStringWithUndefinedOrNullCheck(exec, exec->argument(0));
    if (exec->hadException())
        return jsUndefined();
    String dialogFeaturesString = valueToStringWithUndefinedOrNullCheck(exec, exec->argument(2));
    if (exec->hadException())
        return jsUndefined();

    DialogHandler handler(exec);

    impl().showModalDialog(urlString, dialogFeaturesString, activeDOMWindow(exec), firstDOMWindow(exec), [&handler](DOMWindow& dialog) {
        handler.dialogCreated(dialog);
    });

    return handler.returnValue();
}

static JSValue handlePostMessage(DOMWindow* impl, ExecState* exec)
{
    MessagePortArray messagePorts;
    ArrayBufferArray arrayBuffers;

    // This function has variable arguments and can be:
    // Per current spec:
    //   postMessage(message, targetOrigin)
    //   postMessage(message, targetOrigin, {sequence of transferrables})
    // Legacy non-standard implementations in webkit allowed:
    //   postMessage(message, {sequence of transferrables}, targetOrigin);
    int targetOriginArgIndex = 1;
    if (exec->argumentCount() > 2) {
        int transferablesArgIndex = 2;
        if (exec->argument(2).isString()) {
            targetOriginArgIndex = 2;
            transferablesArgIndex = 1;
        }
        fillMessagePortArray(exec, exec->argument(transferablesArgIndex), messagePorts, arrayBuffers);
    }
    if (exec->hadException())
        return jsUndefined();

    RefPtr<SerializedScriptValue> message = SerializedScriptValue::create(exec, exec->argument(0),
                                                                         &messagePorts,
                                                                         &arrayBuffers);

    if (exec->hadException())
        return jsUndefined();

    String targetOrigin = valueToStringWithUndefinedOrNullCheck(exec, exec->argument(targetOriginArgIndex));
    if (exec->hadException())
        return jsUndefined();

    ExceptionCode ec = 0;
    impl->postMessage(message.release(), &messagePorts, targetOrigin, activeDOMWindow(exec), ec);
    setDOMException(exec, ec);

    return jsUndefined();
}

JSValue JSDOMWindow::postMessage(ExecState* exec)
{
    return handlePostMessage(&impl(), exec);
}

JSValue JSDOMWindow::setTimeout(ExecState* exec)
{
    ContentSecurityPolicy* contentSecurityPolicy = impl().document() ? impl().document()->contentSecurityPolicy() : 0;
    std::unique_ptr<ScheduledAction> action = ScheduledAction::create(exec, globalObject()->world(), contentSecurityPolicy);
    if (exec->hadException())
        return jsUndefined();

    if (!action)
        return jsNumber(0);

    int delay = exec->argument(1).toInt32(exec);

    ExceptionCode ec = 0;
    int result = impl().setTimeout(WTF::move(action), delay, ec);
    setDOMException(exec, ec);

    return jsNumber(result);
}

JSValue JSDOMWindow::setInterval(ExecState* exec)
{
    ContentSecurityPolicy* contentSecurityPolicy = impl().document() ? impl().document()->contentSecurityPolicy() : 0;
    std::unique_ptr<ScheduledAction> action = ScheduledAction::create(exec, globalObject()->world(), contentSecurityPolicy);
    if (exec->hadException())
        return jsUndefined();
    int delay = exec->argument(1).toInt32(exec);

    if (!action)
        return jsNumber(0);

    ExceptionCode ec = 0;
    int result = impl().setInterval(WTF::move(action), delay, ec);
    setDOMException(exec, ec);

    return jsNumber(result);
}

JSValue JSDOMWindow::addEventListener(ExecState* exec)
{
    Frame* frame = impl().frame();
    if (!frame)
        return jsUndefined();

    JSValue listener = exec->argument(1);
    if (!listener.isObject())
        return jsUndefined();

    impl().addEventListener(exec->argument(0).toString(exec)->toAtomicString(exec), JSEventListener::create(asObject(listener), this, false, globalObject()->world()), exec->argument(2).toBoolean(exec));
    return jsUndefined();
}

JSValue JSDOMWindow::removeEventListener(ExecState* exec)
{
    Frame* frame = impl().frame();
    if (!frame)
        return jsUndefined();

    JSValue listener = exec->argument(1);
    if (!listener.isObject())
        return jsUndefined();

    impl().removeEventListener(exec->argument(0).toString(exec)->toAtomicString(exec), JSEventListener::create(asObject(listener), this, false, globalObject()->world()).ptr(), exec->argument(2).toBoolean(exec));
    return jsUndefined();
}

DOMWindow* JSDOMWindow::toWrapped(JSValue value)
{
    if (!value.isObject())
        return 0;
    JSObject* object = asObject(value);
    if (object->inherits(JSDOMWindow::info()))
        return &jsCast<JSDOMWindow*>(object)->impl();
    if (object->inherits(JSDOMWindowShell::info()))
        return &jsCast<JSDOMWindowShell*>(object)->impl();
    return 0;
}

} // namespace WebCore
