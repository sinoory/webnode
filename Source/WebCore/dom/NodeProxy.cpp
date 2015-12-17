
#include "config.h"
#include "NodeProxy.h"

#include <sstream>
#include <iostream>

#include <runtime/JSLock.h>

using namespace std;
namespace WebCore {
    JSCallbackData* NodeProxy::m_data=0;
    int NodeProxy::ExeCnt=0;

    void NodeProxy::hello(){
        printf("NodeProxy::hello \n");
    }

    char NodeProxy::getPropertyType(const char* prop){
        ostringstream ostr;
        ostr<<"typeof("<<mModuleName<<"."<<prop<<")"<<std::endl;
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

    static void v82jscCb(const v8::FunctionCallbackInfo<v8::Value>& args) {
        printf("NodeProxy v82jscCb jsc cb=%p \n",NodeProxy::m_data);
        JSLockHolder lock(NodeProxy::m_data->globalObject()->vm());

        ExecState* exec = NodeProxy::m_data->globalObject()->globalExec();
        MarkedArgumentBuffer jsargs;
        bool raisedException = false; 
        NodeProxy::m_data->invokeCallback(jsargs, &raisedException);
        printf("NodeProxy v82jscCb raisedException=%d\n",raisedException); 
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


    char NodeProxy::exeMethod(ExecState* exec){
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
            ostr<<"typeof("<<EXE_RES_VAR<<ExeCnt<<")"<<std::endl;
            script = v8::Script::Compile(v8::String::NewFromUtf8(isolate,ostr.str().c_str()));
            result = script->Run();

            v8::String::Utf8Value str(result);
            const char* res=*str;
            printf("NodeProxy::exeMethod res %s = %s\n",ostr.str().c_str(),res);
            return (char)(*((const char*)(*str)));
        }
        return 'u';
    }

    NodeProxy::~NodeProxy(){
    }
}
