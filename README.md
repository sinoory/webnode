# git 断点续传
(1)先再本地执行git init,创建git本地库
(2) git fetch git://repo.or.cz/tomato.git

如果中途掉线，继续执行上面(2)命令

(3):git checkout FETCH_HEAD

#逻辑


# 方案
使用代理，中转jsc与v8


# 添加v8 、nw node源码

------------------------------- CMakeLists.txt --------------------------------
index 59d781e..85faf4a 100644
@@ -19,6 +19,7 @@ set(WEBKIT2_DIR "${CMAKE_SOURCE_DIR}/Source/WebKit2")
 set(THIRDPARTY_DIR "${CMAKE_SOURCE_DIR}/Source/ThirdParty")
 set(PLATFORM_DIR "${CMAKE_SOURCE_DIR}/Source/Platform")
 set(CHROMIUM_SOURCES_DIR "${CMAKE_SOURCE_DIR}/Source/Chromium")
+set(NODE_ROOT_DIR "${CMAKE_SOURCE_DIR}/Source/src")
 
 set(TOOLS_DIR "${CMAKE_SOURCE_DIR}/Tools")
 

------------------------ Source/WebKit2/CMakeLists.txt ------------------------
index de1f516..5d90a1a 100644
@@ -146,6 +146,9 @@ set(WebKit2_INCLUDE_DIRECTORIES
     "${DERIVED_SOURCES_WEBKIT2_DIR}"
     "${DERIVED_SOURCES_WEBKIT2_DIR}/include"
     "${CMAKE_BINARY_DIR}"
+    "${NODE_ROOT_DIR}"
+    "${NODE_ROOT_DIR}/v8"
+    "${NODE_ROOT_DIR}/v8/include"
     "${CMAKE_SOURCE_DIR}/Source"
 )
 
@@ -774,6 +777,17 @@ set_target_properties(WebKit2 PROPERTIES FOLDER "WebKit")
 set_target_properties(WebKit2 PROPERTIES LINK_FLAGS "-fPIC")
 set_target_properties(WebKit2 PROPERTIES INSTALL_RPATH "${BROWSER_LIB_INSTALL_DIR}")
 
+target_link_libraries(WebKit2 "-Wl,--whole-archive ../../Source/src/out/Release/obj/v8/tools/gyp/libv8_base.a -Wl,--no-whole-archive")
+target_link_libraries(WebKit2 "-Wl,--whole-archive ../../Source/src/out/Release/obj/v8/tools/gyp/libv8_libbase.a -Wl,--no-whole-archive")
+target_link_libraries(WebKit2 "-Wl,--whole-archive ../../Source/src/out/Release/obj/v8/tools/gyp/libv8_snapshot.a -Wl,--no-whole-archive")
+target_link_libraries(WebKit2 "-Wl,--whole-archive ../../Source/src/out/Release/obj/v8/tools/gyp/libv8_libplatform.a -Wl,--no-whole-archive")
+target_link_libraries(WebKit2 "-Wl,--whole-archive ../../Source/src/out/Release/obj/third_party/zlib/libchrome_zlib.a -Wl,--no-whole-archive")
+target_link_libraries(WebKit2 "-Wl,--whole-archive ../../Source/src/out/Release/obj/third_party/node/deps/cares/libcares.a -Wl,--no-whole-archive")
+target_link_libraries(WebKit2 "-Wl,--whole-archive ../../Source/src/out/Release/obj/third_party/node/deps/openssl/libopenssl.a -Wl,--no-whole-archive")
+target_link_libraries(WebKit2 "-Wl,--whole-archive ../../Source/src/out/Release/obj/third_party/node/deps/http_parser/libhttp_parser.a -Wl,--no-whole-archive")
+target_link_libraries(WebKit2 "-Wl,--whole-archive ../../Source/src/out/Release/obj/third_party/node/libnode.a -Wl,--no-whole-archive")
+target_link_libraries(WebKit2 "-Wl,--whole-archive ../../Source/src/out/Release/obj/third_party/node/deps/uv/libuv.a -Wl,--no-whole-archive")
+



# 初始化node、v8：
+static bool nodeInited=false; 
+void initNode(){
+    if(nodeInited){
+        return ;
+    }
+    nodeInited=true;
+    printf("initNode ok\n");
+
+    v8::Platform* platform = v8::platform::CreateDefaultPlatform();
+    v8::V8::InitializePlatform(platform);
+    v8::Isolate* isolate = v8::Isolate::New();
+    isolate->Enter(); 
+
+    //TODO:need right param
+    int argc = 2;
+    char* argv[]={"app --appname=nw --type=renderer --nodejs --working-directory=.","index.js"};
+    node::SetupUv(argc, argv);
+
+    //TODO:ShellArrayBufferAllocator ??
+    ShellArrayBufferAllocator array_buffer_allocator;
+    v8::V8::SetArrayBufferAllocator(&array_buffer_allocator);
+    v8::V8::Initialize();
+
+    {
+    v8::Isolate* isolate = v8::Isolate::GetCurrent();
+    v8::HandleScope scope(isolate);
+
+    //TODO: add window_bindings.js v8::RegisterExtension
+    v8::Local<v8::Context> context = v8::Context::New(isolate);
+    node::g_context.Reset(isolate, context);
+    context->SetSecurityToken(v8_str("nw-token"));
+    context->Enter();
+    context->SetEmbedderData(0, v8_str("node"));
+
+    node::SetupContext(argc, argv, context);
+
+    }
+
+}
 
 WebProcess& WebProcess::singleton()
 {
@@ -208,6 +264,7 @@ WebProcess::WebProcess()
     addSupplement<WebMediaKeyStorageManager>();
 #endif
     m_plugInAutoStartOriginHashes.add(SessionID::defaultSessionID(), HashMap<unsigned, double>());
+    initNode();
 }
 



# Window添加require，以便js使用require(模块）
---------------------- Source/WebCore/page/DOMWindow.idl ----------------------
+    NodeProxy require(DOMString module);


Source/WebCore/dom/NodeProxy.idl
[
    CustomGetOwnPropertySlot,   
    CustomPutFunction,
] interface NodeProxy {
    [Custom] readonly attribute boolean  isNodeProxyObj; //use Custom to genereate getOwnPropertySlot
};



---------------------- Source/WebCore/page/DOMWindow.cpp ----------------------
+#include "NodeProxy.h"
+PassRefPtr<NodeProxy> DOMWindow::require(const String& module){
+    //printf("DOMWindow::require(%s)\n",module.characters8());
+    //printf("DOMWindow::require(%s)\n",module.ascii().data());
+    NodeProxy* np = new NodeProxy();
+    np->require(module.ascii().data());
+    return np;
+}

./Source/WebCore/bindings/js/JSDOMWindowCustom.cpp
Js Require 后，window返回 function __NODE_PROXY_CLS__xx（）{
}

JSValue JSDOMWindow::require(ExecState* exec){
    const String& module(exec->argument(0).isEmpty() ? String() : exec->argument(0).toString(exec)->value(exec));
    std::ostringstream ostr;
    ostr << "function __NODE_PROXY_CLS__"<<++NodeProxy::FunProxyCnt<<"(){"
                "var args=Array.prototype.slice.call(arguments);"
                "args.splice(0,0,'"<<module.ascii().data()<<"'); " //p1 is module name
                "return node_require_obj_from_class.apply(window,args);"//调用window函数
            "} ; "
            "__NODE_PROXY_CLS__"<<NodeProxy::FunProxyCnt<<".__nodejs_func_prop_get__"<<NodeProxy::FunProxyCnt<<"=function(prop){"
                " return node_get_prop_from_required_class('"<< module.ascii().data() <<"',prop); "//调用window函数
            "} ;"
            "__NODE_PROXY_CLS__"<<NodeProxy::FunProxyCnt<<".__nodejs_func_prop_set__"<<NodeProxy::FunProxyCnt<<"=function(prop,value){"
                "node_set_prop_from_required_class('"<<module.ascii().data()<<"',prop,value);"//调用window函数
            "} ; eval(__NODE_PROXY_CLS__"<<NodeProxy::FunProxyCnt<<") " << std::endl;
    printf("===JSDOMWindow::require %s\n",ostr.str().c_str());
    JSValue evaluationException;
    String jsstr=String::fromUTF8WithLatin1Fallback(ostr.str().c_str(),strlen(ostr.str().c_str()));
    SourceCode jsc = makeSource(jsstr, "nodeproxycls");
    JSValue returnValue = JSMainThreadExecState::evaluate(exec,jsc,JSValue(), &evaluationException);
    return returnValue; 

}

PassRefPtr<NodeProxy> DOMWindow::_require_obj_from_class_(const String& module,const char* constructParams){       
    PassRefPtr<NodeProxy> np = NodeProxy::create();
    np->require(module.ascii().data(),true,constructParams);
    return np;
}

    int NodeProxy::require(const char* module,bool requireObjFromClass,const char* constructParams){
            ostr <<"var "<<mModuleName<< " =global.require('" << module << "');" << std::endl;
            v8::Local<v8::Script> script = v8::Script::Compile(v8::String::NewFromUtf8(isolate,
                        ostr.str().c_str()));
            script->Run();

            ostr.str("");
            ostr << "typeof(" << mModuleName << ");" << std::endl;
            script = v8::Script::Compile(v8::String::NewFromUtf8(isolate,ostr.str().c_str()));
            v8::Handle<v8::Value> result = script->Run();


            if(strstr(cstr,"function")){
                lastRequireIsObject=false;
                if(requireObjFromClass){
                    ostr.str("");
                    //convert class to object
                    ostr<<mModuleName<<" = new "<< mModuleName <<"("<<(constructParams?constructParams:"")<<");"<<std::endl;
                    v8::Local<v8::Script> script = v8::Script::Compile(v8::String::NewFromUtf8(isolate,
                        ostr.str().c_str()));
                    script->Run();
                }
                return 1;

            }
            lastRequireIsObject=true;



----------------------- Source/WebCore/dom/NodeProxy.cpp -----------------------
new file mode 100644
index 0000000..071183f
+    void NodeProxy::require(const char* module){
+        printf("NodeProxy::require(%s)\n",module);
+        v8::Isolate* isolate = v8::Isolate::GetCurrent();
+        v8::HandleScope scope(isolate);
+        v8::Local<v8::Context> g_context =
+            v8::Local<v8::Context>::New(isolate, node::g_context);
+        v8::Context::Scope cscope(g_context);{
+            v8::TryCatch try_catch;
+            ostringstream ostr;
+            ostr << "global.require('" << module << "')" << std::endl;
+            v8::Local<v8::Script> script = v8::Script::Compile(v8::String::NewFromUtf8(isolate,
+                        ostr.str().c_str()));
+            script->Run();
+            if (try_catch.HasCaught()) {
+                v8::Handle<v8::Message> message = try_catch.Message();
+                printf("require(%s) faild:%s\n", module,*v8::String::Utf8Value(message->Get()));
+            }
+
+
+        }
+    }

