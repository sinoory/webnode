
#ifndef NodeProxy_h
#define NodeProxy_h

#include "ScriptWrappable.h"
#include <wtf/HashMap.h>
#include <wtf/ListHashSet.h>
#include <wtf/RefCounted.h>
#include <wtf/TypeCasts.h>
#include <wtf/text/AtomicString.h>


namespace WebCore {
class NodeProxy : public ScriptWrappable, public RefCounted<NodeProxy> {
    public:
    void hello();
    void require(const char* module);
    virtual ~NodeProxy();
};

}

#endif //NodeProxy_h
