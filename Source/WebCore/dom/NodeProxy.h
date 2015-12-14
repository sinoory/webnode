
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


using namespace JSC;

namespace WebCore {
class NodeProxy : public ScriptWrappable, public RefCounted<NodeProxy> {
    public:
    virtual ~NodeProxy();

    static PassRefPtr<NodeProxy> create(){
        return adoptRef(new NodeProxy());
    }

    void hello();
    //return 0: object , 1:class
    //requireObjFromClass : if set true and require result is a class,
    //                      then new a NodeProxy object to js
    int require(const char* module,bool requireObjFromClass=false,const char* constructParams=0);
    JSValue exeMethod(ExecState* exec);



    static JSCallbackData* m_data;

    JSDOMGlobalObject* globalObject;

    bool lastRequireIsObject;

    std::string mMethod;
    std::string mProperty;
    std::string mModuleName;
    std::string mModuleCb;
};

}

#endif //NodeProxy_h
