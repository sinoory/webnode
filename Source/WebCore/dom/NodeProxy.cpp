
#include "config.h"
#include "NodeProxy.h"
#include "JSNodeProxy.h"

#include <sstream>
#include <iostream>

#include <runtime/JSLock.h>

using namespace std;
namespace WebCore {
    JSCallbackData* NodeProxy::m_data=0;
    int NodeProxy::ExeCnt=0;

    static JSValue v8data2jsc(v8::Isolate* isolate, v8::Local<v8::Value> v,
            ExecState* exec=0,std::string v8refname="");

    JSValue NodeProxy::getProp(ExecState* exec,const char* prop){
        ostringstream ostr;
        ostr<<EXE_RES_VAR<<++ExeCnt<<"="<<mModuleName<<"."<<prop<<std::endl;
        printf("NodeProxy getProp %s : %s\n",prop,ostr.str().c_str());
        v8::Isolate* isolate = v8::Isolate::GetCurrent();
        v8::HandleScope scope(isolate);

        v8::Local<v8::Context> g_context =
            v8::Local<v8::Context>::New(isolate, node::g_context);
        v8::Context::Scope cscope(g_context);{
            v8::TryCatch try_catch;
            v8::Local<v8::Script> script = v8::Script::Compile(v8::String::NewFromUtf8(isolate,
                        ostr.str().c_str()));
            v8::Handle<v8::Value> result = script->Run();
            if (try_catch.HasCaught()) {
                v8::Handle<v8::Message> message = try_catch.Message();
                printf("getPropertyType faild:%s\n", *v8::String::Utf8Value(message->Get()));
                return jsUndefined();
            }
            ostr.str("");
            ostr<<EXE_RES_VAR<<ExeCnt;
            return v8data2jsc(isolate,result,exec,ostr.str());
        }
    }

    void NodeProxy::setPropProxy(const char* prop,NodeProxy* np){
        propProxyMap[prop]=np;
    }

    char NodeProxy::getPropertyType(const char* prop){
        ostringstream ostr;
        if(mModuleName.empty()){
            ostr<<"typeof("<<prop<<")"<<std::endl;
        }else{
            ostr<<"typeof("<<mModuleName<<"."<<prop<<")"<<std::endl;
        }
        //TODO:store result for effetionce
        v8::Isolate* isolate = v8::Isolate::GetCurrent();
        v8::HandleScope scope(isolate);

        v8::Local<v8::Context> g_context =
            v8::Local<v8::Context>::New(isolate, node::g_context);
        v8::Context::Scope cscope(g_context);{
            v8::TryCatch try_catch;
            v8::Local<v8::Script> script = v8::Script::Compile(v8::String::NewFromUtf8(isolate,
                        ostr.str().c_str()));
            v8::Handle<v8::Value> result = script->Run();
            if (try_catch.HasCaught()) {
                v8::Handle<v8::Message> message = try_catch.Message();
                printf("getPropertyType faild:%s\n", *v8::String::Utf8Value(message->Get()));
                return 0;
            }
            v8::String::Utf8Value str(result);
            const char* res=*str;
            printf("NodeProxy::getPropertyType %s = %s.\n",ostr.str().c_str(),res);
            return (char)(*((const char*)(*str)));
        }

    }

    void NodeProxy::setProperty(ExecState* exec,const char* prop,JSValue value){
        ostringstream ostr;
        if(value.isString()){
            ostr<<mModuleName<<"."<<prop<<"="<<"'"<<value.toString(exec)->value(exec).utf8().data()<<"'";
        }else if(value.isFunction()){

        }
        printf("NodeProxy setProperty %s\n",ostr.str().c_str());

    }

    static JSValue v8data2jsc(v8::Isolate* isolate, v8::Local<v8::Value> v,
            ExecState* exec,std::string v8refname){

            v8::String::Utf8Value str(v);
            const char* res=*str;
            printf("v8data2jsc v8 v = %s IsNumber=%d\n",res,v->IsNumber());

            if (v->IsUndefined() || v->IsNull()) {
                return JSC::jsNull();
            }else if (v->IsBoolean()) {
                v8::Local<v8::Boolean> rv = v->ToBoolean();
                return (jsBoolean(rv->Value()));
            }else if (v->IsInt32()) {
                v8::Local<v8::Int32> rv = v->ToInt32();
                return (JSValue(rv->Value()));
            }else if (v->IsUint32()) {
                v8::Local<v8::Uint32> rv = v->ToUint32();
                return (JSValue(rv->Value()));
            }else if (v->IsNumber()) {
                v8::Local<v8::Number> rv = v->ToNumber();
                return (JSValue(rv->Value()));
            }else if (v->IsString()) {
                if(!exec){
                    return JSC::jsUndefined();
                }
                v8::Local<v8::String> rv = v->ToString();
                JSString* jsv = jsString(exec, *v8::String::Utf8Value(rv));
                return jsv->toPrimitive(exec,PreferString);
            }else if (v->IsObject()){
                if(!exec){
                    return JSC::jsUndefined();
                }
                PassRefPtr<NodeProxy> np = new NodeProxy;
                np->globalObject=jsCast<JSDOMGlobalObject*>(exec->lexicalGlobalObject());
                np->mModuleName=v8refname;
                JSValue jnp = toJS(exec, np->globalObject, WTF::getPtr(np));
                return jnp;
            }
            return JSC::jsUndefined();
    }

    static void v82jscCb(const v8::FunctionCallbackInfo<v8::Value>& args) {
        static int pcnt=0;
        int argc = args.Length();
        MarkedArgumentBuffer jsargs;
        for (int i = 0; i < argc; i++){
            v8::HandleScope handle_scope(args.GetIsolate());
            v8::Local<v8::Value> v = args[i];
            if (v->IsUndefined() || v->IsNull()) {
                jsargs.append(JSC::jsNull());
            }else if (v->IsString()) {
                v8::Local<v8::String> rv = v->ToString();
                JSString* jsv = jsString(NodeProxy::m_data->globalObject()->globalExec(), 
                        *v8::String::Utf8Value(rv));
                //jsargs.append(*jsv);
            }else if (v->IsBoolean()) {
                v8::Local<v8::Boolean> rv = v->ToBoolean();
                jsargs.append(jsBoolean(rv->Value()));
            }else if (v->IsInt32()) {
                v8::Local<v8::Int32> rv = v->ToInt32();
                jsargs.append(JSValue(rv->Value()));
            }else if (v->IsUint32()) {
                v8::Local<v8::Uint32> rv = v->ToUint32();
                jsargs.append(JSValue(rv->Value()));
            }else if (v->IsNumber()) {
                v8::Local<v8::Number> rv = v->ToNumber();
                jsargs.append(JSValue(rv->Value()));
            }else if (v->IsObject()) {
                v8::Local<v8::Object> obj= v->ToObject();
                v8::Local<v8::Context> context = args.GetIsolate()->GetCurrentContext();
                v8::Local<v8::Object> global = context->Global();

                std::ostringstream ostr;
                ostr<<CB_PARAM_VAR<<pcnt++;
                //printf("NodeProxy v82jscCb ostr=%s.\n",ostr.str().c_str());

                global->Set(v8::String::NewFromUtf8(args.GetIsolate(), ostr.str().c_str()), obj);

                PassRefPtr<NodeProxy> np = new NodeProxy;
                np->globalObject = NodeProxy::m_data->globalObject();
                np->mModuleName=ostr.str();

                JSValue jnp = toJS(np->globalObject->globalExec(), np->globalObject, WTF::getPtr(np));
                jsargs.append(jnp);
            }else{
                printf("v82jscCb args[%d] is unknow\n",i);
                jsargs.append(JSC::jsNull());
            }
        }
        printf("NodeProxy v82jscCb jsc cb=%p argc=%d\n",NodeProxy::m_data,argc);
        JSLockHolder lock(NodeProxy::m_data->globalObject()->vm());

        ExecState* exec = NodeProxy::m_data->globalObject()->globalExec();
        //args.append(globalThisValue);
        //args.append(jsNumber(id));
        bool raisedException = false; 
        NodeProxy::m_data->invokeCallback(jsargs, &raisedException);
        //if (exec->hadException()) 
        if (raisedException) {
            //JSValue e ;//= exec->exception();
            //exec->clearException();
            //printf("NodeProxy v82jscCb raisedException=%s\n",e.toString(exec)->value(exec).utf8().data()); 
            printf("NodeProxy v82jscCb raisedException\n"); 
        }

    }

    static void setjsobj(v8::Local<v8::Object> nodeGlobal){
        NODE_SET_METHOD(nodeGlobal, "nodeproxyCb", v82jscCb);
    }

    static void tstcb(){
        v8::Isolate* isolate = v8::Isolate::GetCurrent();
        v8::Local<v8::Context> g_context =
            v8::Local<v8::Context>::New(isolate, node::g_context);
        v8::Context::Scope cscope(g_context);{
            v8::TryCatch try_catch;
            v8::Local<v8::Script> script1 = v8::Script::Compile(v8::String::NewFromUtf8(isolate,
                        "nodeproxyCb();"));
            script1->Run();
            if (try_catch.HasCaught()) {
                v8::Handle<v8::Message> message = try_catch.Message();
                printf("call nodeproxyCb faild:%s\n", *v8::String::Utf8Value(message->Get()));
            }
        }

    }

    int NodeProxy::require(const char* module,bool requireObjFromClass,const char* constructParams){
        static long moduleid=0;
        v8::Isolate* isolate = v8::Isolate::GetCurrent();
        //printf("NodeProxy::require(%s) i1=%p i2=%p\n",module,isolate,(*node::g_context)->GetIsolate());
        v8::HandleScope scope(isolate);
        v8::Local<v8::Context> g_context =
            v8::Local<v8::Context>::New(isolate, node::g_context);
        

        v8::Context::Scope cscope(g_context);{
            v8::TryCatch try_catch;
            ostringstream osModule;
            osModule<<"NPReq_"<<module<<"_"<<moduleid<<std::endl;
            mModuleName = osModule.str();

            ostringstream oscb;
            oscb<<"NPReq_"<<module<<"_cb_"<<moduleid<<std::endl;
            mModuleCb = oscb.str();
            moduleid++;

            v8::Local<v8::Object> nodeGlobal = g_context->Global();
            setjsobj(nodeGlobal);

            ostringstream ostr;
            //ostr << "process.nextTick();" << std::endl;
            //ostr << "if(!global){noglobal();}else{global();}" << std::endl;
            ostr <<"var "<<mModuleName<< " =global.require('" << module << "');" << std::endl;
            v8::Local<v8::Script> script = v8::Script::Compile(v8::String::NewFromUtf8(isolate,
                        ostr.str().c_str()));
            script->Run();
            if (try_catch.HasCaught()) {
                v8::Handle<v8::Message> message = try_catch.Message();
                printf("require(%s) faild:%s\n", module,*v8::String::Utf8Value(message->Get()));
                return -1;
            }

            ostr.str("");
            ostr << "typeof(" << mModuleName << ");" << std::endl;
            script = v8::Script::Compile(v8::String::NewFromUtf8(isolate,ostr.str().c_str()));
            v8::Handle<v8::Value> result = script->Run();
            if (try_catch.HasCaught()) {
                v8::Handle<v8::Message> message = try_catch.Message();
                printf("get type info faild:%s\n", module,*v8::String::Utf8Value(message->Get()));
                return -1;
            }
            v8::String::Utf8Value str(result);
            const char* cstr = *str;
            printf("require(%s) type = %s\n",module,cstr);
        
            if(strstr(cstr,"function")){
                lastRequireIsObject=false;
                if(requireObjFromClass){
                    ostr.str("");
                    //convert class to object
                    ostr<<mModuleName<<" = new "<< mModuleName <<"("<<(constructParams?constructParams:"")<<");"<<std::endl;
                    printf("======convert c=>o : %s\n",ostr.str().c_str());
                    v8::Local<v8::Script> script = v8::Script::Compile(v8::String::NewFromUtf8(isolate,
                        ostr.str().c_str()));
                    script->Run();
                }
                return 1;

            }
            lastRequireIsObject=true;
            return 0;
        }
    }


    JSValue NodeProxy::exeMethod(ExecState* exec){
        printf("NodeProxy::exeMethod mMethod=%s,argc=%d exe=%p\n",mMethod.c_str(),exec->argumentCount(),exec);

        v8::Isolate* isolate = v8::Isolate::GetCurrent();
        v8::HandleScope scope(isolate);
        v8::Local<v8::Context> g_context =
            v8::Local<v8::Context>::New(isolate, node::g_context);

        ostringstream ostr;
        //do not use var , so return res to be a global object in v8
        //otherwise typeof() will can't got the result
        ostr <<" "<<EXE_RES_VAR<<++ExeCnt<<" = " << mModuleName<< "." <<mMethod<<"(";
        v8::Context::Scope cscope(g_context);{
            v8::TryCatch try_catch;
            bool pa=false;
            for(int i=0;i<exec->argumentCount();i++){
                JSValue arg=exec->uncheckedArgument(i);
                if(arg.isString()){
                    ostr<< (pa?",'":"'") <<arg.toString(exec)->value(exec).ascii().data()<<"'";  
                    pa=true;
                }else if(arg.isNumber()){
                    ostr<< (pa?",":"") <<arg.toNumber(exec);  
                    pa=true;
                }else if(arg.isFunction()){
                    ostr<< (pa?",":"") <<"nodeproxyCb";  
                    if(this->m_data !=0){
                        delete this->m_data;
                    }
                    this->m_data=new JSCallbackData(asObject(arg), this->globalObject);
                }
            }
            ostr <<");"<<std::endl;

            printf("NodeProxy::exeMethod %s , this=%p\n",ostr.str().c_str(),this);
            v8::Local<v8::Script> script = v8::Script::Compile(v8::String::NewFromUtf8(isolate,
                        ostr.str().c_str()));
            v8::Handle<v8::Value> result = script->Run();
            if (try_catch.HasCaught()) {
                v8::Handle<v8::Message> message = try_catch.Message();
                printf("NodeProxy::exeMethod faild:%s\n", *v8::String::Utf8Value(message->Get()));
            }

            ostr.str("");
            ostr<<EXE_RES_VAR<<ExeCnt;
            return v8data2jsc(isolate,result,exec,ostr.str());

        }
        return JSC::jsUndefined();
    }

    NodeProxy::~NodeProxy(){
    }
}
