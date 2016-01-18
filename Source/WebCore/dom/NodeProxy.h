
#ifndef NodeProxy_h
#define NodeProxy_h

#include "ScriptWrappable.h"
#include <wtf/HashMap.h>
#include <wtf/ListHashSet.h>
#include <wtf/RefCounted.h>
#include <wtf/PassRefPtr.h>

#include <wtf/TypeCasts.h>
#include <wtf/text/AtomicString.h>
#include <wtf/text/AtomicStringImpl.h>
//#include <runtime/JSCJSValue.h>
#include "JSDOMBinding.h"
#include "JSCallbackData.h"
#include "JSDOMGlobalObject.h"

#include "third_party/node/src/node_webkit.h"
#include "third_party/node/src/node.h"
#include   <map>

#include <sstream>
#include <iostream>

using namespace JSC;

#define EXE_RES_VAR "_node_proxy_exe_res_"
#define CB_PARAM_VAR "_np_callback_param_"

namespace WebCore {
class NodeProxy : public ScriptWrappable, public RefCounted<NodeProxy> {
    public:
    virtual ~NodeProxy();

    static PassRefPtr<NodeProxy> create(){
        return adoptRef(new NodeProxy());
    }

    static v8::Handle<v8::Value> execStringInV8(const char* str,v8::Isolate* isolate=0);
    static char v8typeof(const char* prop);
    static void convertJscArgs2String(ExecState* exec,int startindex,std::ostringstream& ostr);

    //if v8 value is object,then v8refname must supplied, 
    //  return a jsc nodeprox object which contain the v8 object refname 
    static JSValue v8data2jsc(v8::Isolate* isolate, v8::Local<v8::Value> v,
            ExecState* exec=0,std::string v8refname="");

    bool isNodeProxyObj(){return true;}

    //return 0: object , 1:class
    //requireObjFromClass : if set true and require result is a class,
    //                      then new a NodeProxy object to js
    int require(const char* module,bool requireObjFromClass=false,const char* constructParams=0);
    JSValue exeMethod(ExecState* exec);
    void setProperty(ExecState* exec,const char* prop,JSValue value);

    JSValue getProp(ExecState* exec,const char* prop);
    void setPropProxy(const char* prop,NodeProxy* np);


    static int ExeCnt;
    static int FunProxyCnt;
    static JSCallbackData* m_data;

    JSDOMGlobalObject* globalObject;

    bool lastRequireIsObject;

    std::string mMethod;
    std::string mProperty;
    std::string mModuleName;
    std::string mModuleCb;

    std::map<const char*,NodeProxy*> propProxyMap;
};

}

#endif //NodeProxy_h
