
#include "config.h"
#include "NodeProxy.h"
#include "JSNodeProxy.h"


#include <runtime/JSLock.h>
#include <runtime/JSONObject.h>

using namespace std;
namespace WebCore {
    JSCallbackData* NodeProxy::m_data=0;
    int NodeProxy::ExeCnt=0;
    int NodeProxy::FunProxyCnt=0;


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
                printf("getProp faild:%s\n", *v8::String::Utf8Value(message->Get()));
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

    char NodeProxy::v8typeof(const char* prop){
        ostringstream ostr;
        ostr<<"typeof("<<prop<<")"<<std::endl;
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
                printf("v8typeof faild:%s\n", *v8::String::Utf8Value(message->Get()));
                return 0;
            }
            v8::String::Utf8Value str(result);
            const char* res=*str;
            printf("v8typeof %s = %s.\n",ostr.str().c_str(),res);
            return (char)(*((const char*)(*str)));
        }

    }

    v8::Handle<v8::Value> NodeProxy::execStringInV8(const char* str,v8::Isolate* isolate){
        if(!isolate){
           isolate = v8::Isolate::GetCurrent();
        }
        v8::Local<v8::Context> g_context =
            v8::Local<v8::Context>::New(isolate, node::g_context);
        v8::Context::Scope cscope(g_context);{
            v8::TryCatch try_catch;
            v8::Local<v8::Script> script = v8::Script::Compile(v8::String::NewFromUtf8(isolate,str));
            v8::Handle<v8::Value> result = script->Run();
            if (try_catch.HasCaught()) {
                v8::Handle<v8::Message> message = try_catch.Message();
                printf("execStringInV8 failed: %s err=%s\n",str,*v8::String::Utf8Value(message->Get()));
            }
            
            //v8::String::Utf8Value str1(result);
            //const char* res=*str1;
            //printf("execStringInV8 %s res=%s\n",str,res);

            return result;
        }
    }

    void NodeProxy::setProperty(ExecState* exec,const char* prop,JSValue value){
        ostringstream ostr;
        if(value.isFunction()){
            printf("setProperty unhandled function\n");
        }else if(value.isString()){
            ostr<<mModuleName<<"."<<prop<<"="<<"'"<<value.toString(exec)->value(exec).utf8().data()<<"'";
        }else if(value.isInt32()){
            ostr<<mModuleName<<"."<<prop<<"="<<value.asInt32();
        }else if(value.isUInt32()){
            ostr<<mModuleName<<"."<<prop<<"="<<value.asUInt32();
        }else if(value.isDouble()){
            ostr<<mModuleName<<"."<<prop<<"="<<value.asDouble();
        }else if(value.isTrue()){
            ostr<<mModuleName<<"."<<prop<<"=true";
        }else if(value.isFalse()){
            ostr<<mModuleName<<"."<<prop<<"=false";
        }else if(value.isNumber()){
            ostr<<mModuleName<<"."<<prop<<"="<<value.toString(exec)->value(exec).utf8().data();
        }else if(value.isUndefinedOrNull()){
            ostr<<mModuleName<<"."<<prop<<"=null";
        }else if(value.isObject()){
            const char* s = value.toString(exec)->value(exec).utf8().data();
            String ss = JSONStringify(exec,value,0);
            printf("setProperty unhandled object s=%s js=%s\n",s,ss.utf8().data());
            if(ss.startsWith('[')){//object is an array
            }else if(ss.startsWith('{')){//a json or a object,convert to json 
                //TODO: if is object, convert to json will be empty
                ostr<<mModuleName<<"."<<prop<<"=JSON.parse('"<<ss.utf8().data()<<"')";
            }
        }else{
            printf("setProperty unkown type\n");
            ostr<<mModuleName<<"."<<prop<<"="<<"'"<<value.toString(exec)->value(exec).utf8().data()<<"'";
        }
        v8::Isolate* isolate = v8::Isolate::GetCurrent();
        v8::HandleScope scope(isolate);
        execStringInV8(ostr.str().c_str());
        printf("NodeProxy setProperty %s\n",ostr.str().c_str());

    }

    JSValue NodeProxy::v8data2jsc(v8::Isolate* isolate, v8::Local<v8::Value> v,
            ExecState* exec,std::string v8refname){

            v8::String::Utf8Value str(v);
            const char* res=*str;
            printf("v8data2jsc v8 v = %s IsNumber=%d\n",res,v->IsNumber());

            if (v->IsUndefined() || v->IsNull()) {
                printf("v8data2jsc v8 is null\n");
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
                printf("v8data2jsc v is str exec=%p\n",exec);
                if(!exec){
                    return JSC::jsUndefined();
                }
                v8::Local<v8::String> rv = v->ToString();
                JSString* jsv = jsString(exec, *v8::String::Utf8Value(rv));
                return jsv->toPrimitive(exec,PreferString);//TODO: set process.k=chinese, get k return e
            }else if (v->IsArray()){
                printf("v8data2jsc v is array\n");
                //V8_INLINE static Array* Cast(Value* obj);
                return JSC::jsUndefined();
            }else if (v->IsObject()){
                printf("v8data2jsc v8 is obj \n");
                if(!exec){
                    return JSC::jsUndefined();
                }
                PassRefPtr<NodeProxy> np = new NodeProxy;
                np->globalObject=jsCast<JSDOMGlobalObject*>(exec->lexicalGlobalObject());
                np->mModuleName=v8refname;
                JSValue jnp = toJS(exec, np->globalObject, WTF::getPtr(np));
                return jnp;
            }else if (v->IsSymbol()) {
                printf("v8data2jsc v8 is symb\n");
            }else if (v->IsFunction()) {
                printf("v8data2jsc v8 is func\n");
            }else if (v->IsExternal()) {
                printf("v8data2jsc v8 is external\n");
            }else if (v->IsDate()) {
                printf("v8data2jsc v8 is data\n");
            }else if (v->IsSymbolObject()) {
                printf("v8data2jsc v8 is symbol o\n");
            }else if (v->IsStringObject()) {
                printf("v8data2jsc v8 is stro o\n");
            }else if (v->IsNativeError()) {
                printf("v8data2jsc v8 is e\n");
            }else if (v->IsPromise()) {
                printf("v8data2jsc v8 is p\n");
            }
            printf("v8data2jsc v8 is unknown\n");
            return JSC::jsUndefined();
    }

    static void v82jscCb(const v8::FunctionCallbackInfo<v8::Value>& args) {
        static int pcnt=0;
        int argc = args.Length();
        MarkedArgumentBuffer jsargs;
        ExecState* exec = NodeProxy::m_data->globalObject()->globalExec();
        for (int i = 0; i < argc; i++){
            v8::HandleScope handle_scope(args.GetIsolate());
            v8::Local<v8::Value> v = args[i];
            if (v->IsUndefined() || v->IsNull()) {
                jsargs.append(JSC::jsNull());
            }else if (v->IsString()) {
                v8::Local<v8::String> rv = v->ToString();
                JSString* jsv = jsString(NodeProxy::m_data->globalObject()->globalExec(), 
                        *v8::String::Utf8Value(rv));
                jsargs.append(jsv->toPrimitive(exec,PreferString));
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
            osModule<<"NPReq_"<<module<<"_"<<moduleid;
            mModuleName = osModule.str();

            ostringstream oscb;
            oscb<<"NPReq_"<<module<<"_cb_"<<moduleid;
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


    //static 
    void NodeProxy::convertJscArgs2String(ExecState* exec,int startindex,std::ostringstream& ostr){
        bool pa=false;
        for(int i=startindex;i<exec->argumentCount();i++){
            JSValue arg=exec->uncheckedArgument(i);
            if(arg.isString()){
                ostr<< (pa?",'":"'") <<arg.toString(exec)->value(exec).ascii().data()<<"'";  
                pa=true;
            }else if(arg.isNumber()){
                ostr<< (pa?",":"") <<arg.toNumber(exec);  
                pa=true;
            }else if(arg.isFunction()){
                ostr<< (pa?",":"") <<"nodeproxyCb";  
                if(NodeProxy::m_data !=0){
                    delete NodeProxy::m_data;
                }
                JSDOMGlobalObject* globalObject =jsCast<JSDOMGlobalObject*>(exec->lexicalGlobalObject());
                NodeProxy::m_data=new JSCallbackData(asObject(arg), globalObject);
            }else if(arg.isObject()){//to be the last selection
                JSValue isnodeproxyobj=arg.get(exec,Identifier(exec,"isNodeProxyObj"));
                if(isnodeproxyobj.isTrue()){
                    JSNodeProxy* jnp = jsCast<JSNodeProxy*>(arg);
                    printf("%s param is nodeproxyobj,module=%s\n",__func__,
                        jnp->impl().mModuleName.c_str());
                    ostr<<(pa?",":"")<<jnp->impl().mModuleName.c_str();
                    continue; 
                }
                const char* s = arg.toString(exec)->value(exec).utf8().data();
                String ss = JSONStringify(exec,arg,0);
                //printf("setProperty unhandled object s=%s js=%s\n",s,ss.utf8().data());
                if(ss.startsWith('[')){//object is an array
                }else if(ss.startsWith('{')){//a json or a object,convert to json 
                    //TODO: if is object, convert to json will be empty
                    ostr<<(pa?",":"")<<"JSON.parse('"<<ss.utf8().data()<<"')";
                    pa=true;
                }
            }else{
                printf("%s TODO unkown arg %d type\n",i);
            }
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
            convertJscArgs2String(exec,0,ostr);
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
