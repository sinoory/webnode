
#include "config.h"
#include "NodeProxy.h"

#include "third_party/node/src/node_webkit.h"
#include "third_party/node/src/node.h"
#include <sstream>
#include <iostream>
using namespace std;
namespace WebCore {
    void NodeProxy::hello(){
        printf("NodeProxy::hello \n");
    }

    void NodeProxy::require(const char* module){
        printf("NodeProxy::require(%s)\n",module);
        v8::Isolate* isolate = v8::Isolate::GetCurrent();
        v8::HandleScope scope(isolate);
        v8::Local<v8::Context> g_context =
            v8::Local<v8::Context>::New(isolate, node::g_context);
        v8::Context::Scope cscope(g_context);{
            v8::TryCatch try_catch;
            ostringstream ostr;
            //ostr << "process.nextTick();" << std::endl;
            //ostr << "if(!global){noglobal();}else{global();}" << std::endl;
            ostr << "global.require('" << module << "')" << std::endl;
            v8::Local<v8::Script> script = v8::Script::Compile(v8::String::NewFromUtf8(isolate,
                        ostr.str().c_str()));
            script->Run();
            if (try_catch.HasCaught()) {
                v8::Handle<v8::Message> message = try_catch.Message();
                printf("require(%s) faild:%s\n", module,*v8::String::Utf8Value(message->Get()));
            }


        }
    }

    NodeProxy::~NodeProxy(){
    }
}
