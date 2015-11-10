
#include "config.h"
#include "NodeProxy.h"

#include <sstream>
#include <iostream>

#include <runtime/JSLock.h>

using namespace std;
namespace WebCore {
    JSCallbackData* NodeProxy::m_data=0;

    void NodeProxy::hello(){
        printf("NodeProxy::hello \n");
    }

    static void v82jscCb(const v8::FunctionCallbackInfo<v8::Value>& args) {
        printf("NodeProxy v82jscCb jsc cb=%p \n",NodeProxy::m_data);
        JSLockHolder lock(NodeProxy::m_data->globalObject()->vm());

        ExecState* exec = NodeProxy::m_data->globalObject()->globalExec();
        MarkedArgumentBuffer jsargs;
        bool raisedException = false; 
        NodeProxy::m_data->invokeCallback(jsargs, &raisedException);
        printf("NodeProxy v82jscCb raisedException=%d",raisedException); 
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
    void NodeProxy::require(const char* module){
        static long moduleid=0;
        v8::Isolate* isolate = v8::Isolate::GetCurrent();
        printf("NodeProxy::require(%s) i1=%p i2=%p\n",module,isolate,(*node::g_context)->GetIsolate());
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
            }


        }

    }


    JSValue NodeProxy::exeMethod(ExecState* exec){
        printf("NodeProxy::exeMethod mMethod=%s,argc=%d\n",mMethod.c_str(),exec->argumentCount());

        WTF::String cb = exec->uncheckedArgument(1).getString(exec);
        WTF::String p0 = exec->uncheckedArgument(0).getString(exec);
        printf("NodeProxy::exeMethod p0=%s callback=%s argc=%d\n",p0.ascii().data(),cb.ascii().data(),exec->argumentCount());

        JSValue trueresult = jsBoolean(true);

        v8::Isolate* isolate = v8::Isolate::GetCurrent();
        v8::HandleScope scope(isolate);
        v8::Local<v8::Context> g_context =
            v8::Local<v8::Context>::New(isolate, node::g_context);

        v8::Context::Scope cscope(g_context);{
            v8::TryCatch try_catch;
            ostringstream ostr;
            //
            //ostr <<mModuleName<< ".readFile"<<"('/home/sin/tmp/webjs',nodeproxyCb);" << std::endl;
            //ostr <<mModuleName<< ".writeFileSync"<<"('/home/sin/tmp/webjs','wNodePrx');" << std::endl;
            ostr <<mModuleName<< ".writeFile"<<"('/home/sin/tmp/webjs','byNodeProxy',nodeproxyCb);" << std::endl;
            //ostr <<mModuleName<< ".open"<<"('/home/sin/tmp/webjs','r',nodeproxyCb);" << std::endl;
            //ostr <<mModuleName<< "."<<mMethod<<"('~/tmp/webjs','writefile');" << std::endl;
            printf("NodeProxy::exeMethod %s , this=%p\n",ostr.str().c_str(),this);
            v8::Local<v8::Script> script = v8::Script::Compile(v8::String::NewFromUtf8(isolate,
                        ostr.str().c_str()));
            v8::Handle<v8::Value> result = script->Run();//TD:convert v8::result to jsc::result
            if (try_catch.HasCaught()) {
                v8::Handle<v8::Message> message = try_catch.Message();
                printf("NodeProxy::exeMethod faild:%s\n", *v8::String::Utf8Value(message->Get()));
            }


        }

        return trueresult;
    }

    NodeProxy::~NodeProxy(){
    }
}
