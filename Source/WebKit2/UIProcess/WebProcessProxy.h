/*
 * Copyright (C) 2010, 2011 Apple Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. AND ITS CONTRIBUTORS ``AS IS''
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL APPLE INC. OR ITS CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
 * THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef WebProcessProxy_h
#define WebProcessProxy_h

#include "APISession.h"
#include "ChildProcessProxy.h"
#include "CustomProtocolManagerProxy.h"
#include "MessageReceiverMap.h"
#include "PlatformProcessIdentifier.h"
#include "PluginInfoStore.h"
#include "ProcessLauncher.h"
#include "ResponsivenessTimer.h"
#include "WebConnectionToWebProcess.h"
#include "WebPageProxy.h"
#include "WebProcessProxyMessages.h"
#include <WebCore/LinkHash.h>
#include <memory>
#include <wtf/Forward.h>
#include <wtf/HashMap.h>
#include <wtf/PassRefPtr.h>
#include <wtf/RefCounted.h>

#if PLATFORM(IOS)
#include "ProcessThrottler.h"
#endif

namespace WebCore {
class ResourceRequest;
class URL;
struct PluginInfo;
};

namespace WebKit {

class DownloadProxyMap;
class WebBackForwardListItem;
class WebPageGroup;
class WebProcessPool;
struct WebNavigationDataStore;
    
class WebProcessProxy : public ChildProcessProxy, ResponsivenessTimer::Client {
public:
    typedef HashMap<uint64_t, RefPtr<WebBackForwardListItem>> WebBackForwardListItemMap;
    typedef HashMap<uint64_t, RefPtr<WebFrameProxy>> WebFrameProxyMap;
    typedef HashMap<uint64_t, WebPageProxy*> WebPageProxyMap;

    static Ref<WebProcessProxy> create(WebProcessPool&);
    ~WebProcessProxy();

    static WebProcessProxy* fromConnection(IPC::Connection* connection)
    {
        return static_cast<WebProcessProxy*>(ChildProcessProxy::fromConnection(connection));
    }

    WebConnection* webConnection() const { return m_webConnection.get(); }

    WebProcessPool& processPool() { return m_processPool; }

    static WebPageProxy* webPage(uint64_t pageID);
    Ref<WebPageProxy> createWebPage(PageClient&, const WebPageConfiguration&);
    void addExistingWebPage(WebPageProxy*, uint64_t pageID);
    void removeWebPage(uint64_t pageID);

    WTF::IteratorRange<WebPageProxyMap::const_iterator::Values> pages() const { return m_pageMap.values(); }
    unsigned pageCount() const { return m_pageMap.size(); }

    void addVisitedLinkProvider(VisitedLinkProvider&);
    void addWebUserContentControllerProxy(WebUserContentControllerProxy&);
    void didDestroyVisitedLinkProvider(VisitedLinkProvider&);
    void didDestroyWebUserContentControllerProxy(WebUserContentControllerProxy&);

    WebBackForwardListItem* webBackForwardItem(uint64_t itemID) const;

    ResponsivenessTimer* responsivenessTimer() { return &m_responsivenessTimer; }

    WebFrameProxy* webFrame(uint64_t) const;
    bool canCreateFrame(uint64_t frameID) const;
    void frameCreated(uint64_t, WebFrameProxy*);
    void disconnectFramesFromPage(WebPageProxy*); // Including main frame.
    size_t frameCountInPage(WebPageProxy*) const; // Including main frame.

    void updateTextCheckerState();

    void registerNewWebBackForwardListItem(WebBackForwardListItem*);

    void willAcquireUniversalFileReadSandboxExtension() { m_mayHaveUniversalFileReadSandboxExtension = true; }
    void assumeReadAccessToBaseURL(const String&);
    bool hasAssumedReadAccessToURL(const WebCore::URL&) const;

    bool checkURLReceivedFromWebProcess(const String&);
    bool checkURLReceivedFromWebProcess(const WebCore::URL&);

    static bool fullKeyboardAccessEnabled();

    DownloadProxy* createDownloadProxy(const WebCore::ResourceRequest&);

    void didSaveToPageCache();
    void releasePageCache();

    void enableSuddenTermination();
    void disableSuddenTermination();

    void requestTermination();

    RefPtr<API::Object> transformHandlesToObjects(API::Object*);
    static RefPtr<API::Object> transformObjectsToHandles(API::Object*);

#if PLATFORM(COCOA)
    RefPtr<ObjCObjectGraph> transformHandlesToObjects(ObjCObjectGraph&);
    static RefPtr<ObjCObjectGraph> transformObjectsToHandles(ObjCObjectGraph&);
#endif

    void windowServerConnectionStateChanged();

    void sendProcessWillSuspend();
    void processReadyToSuspend();
    void sendCancelProcessWillSuspend();
    void didCancelProcessSuspension();
    void sendProcessDidResume();

    void setIsHoldingLockedFiles(bool);

    ProcessThrottler& throttler() { return *m_throttler; }
    
private:
    explicit WebProcessProxy(WebProcessPool&);

    // From ChildProcessProxy
    virtual void getLaunchOptions(ProcessLauncher::LaunchOptions&) override;
    void platformGetLaunchOptions(ProcessLauncher::LaunchOptions&);
    virtual void connectionWillOpen(IPC::Connection&) override;
    virtual void connectionDidClose(IPC::Connection&) override;

    // Called when the web process has crashed or we know that it will terminate soon.
    // Will potentially cause the WebProcessProxy object to be freed.
    void disconnect();

    // IPC message handlers.
    void addBackForwardItem(uint64_t itemID, uint64_t pageID, const PageState&);
    void didDestroyFrame(uint64_t);
    
    void shouldTerminate(bool& shouldTerminate);

    // Plugins
#if ENABLE(NETSCAPE_PLUGIN_API)
    void getPlugins(bool refresh, Vector<WebCore::PluginInfo>& plugins, Vector<WebCore::PluginInfo>& applicationPlugins);
#endif // ENABLE(NETSCAPE_PLUGIN_API)
#if ENABLE(NETSCAPE_PLUGIN_API)
    void getPluginProcessConnection(uint64_t pluginProcessToken, PassRefPtr<Messages::WebProcessProxy::GetPluginProcessConnection::DelayedReply>);
#endif
#if ENABLE(NETWORK_PROCESS)
    void getNetworkProcessConnection(PassRefPtr<Messages::WebProcessProxy::GetNetworkProcessConnection::DelayedReply>);
#endif
#if ENABLE(DATABASE_PROCESS)
    void getDatabaseProcessConnection(PassRefPtr<Messages::WebProcessProxy::GetDatabaseProcessConnection::DelayedReply>);
#endif

    // IPC::Connection::Client
    friend class WebConnectionToWebProcess;
    virtual void didReceiveMessage(IPC::Connection&, IPC::MessageDecoder&) override;
    virtual void didReceiveSyncMessage(IPC::Connection&, IPC::MessageDecoder&, std::unique_ptr<IPC::MessageEncoder>&) override;
    virtual void didClose(IPC::Connection&) override;
    virtual void didReceiveInvalidMessage(IPC::Connection&, IPC::StringReference messageReceiverName, IPC::StringReference messageName) override;
    virtual IPC::ProcessType localProcessType() override { return IPC::ProcessType::UI; }
    virtual IPC::ProcessType remoteProcessType() override { return IPC::ProcessType::Web; }

    // ResponsivenessTimer::Client
    void didBecomeUnresponsive(ResponsivenessTimer*) override;
    void interactionOccurredWhileUnresponsive(ResponsivenessTimer*) override;
    void didBecomeResponsive(ResponsivenessTimer*) override;

    // ProcessLauncher::Client
    virtual void didFinishLaunching(ProcessLauncher*, IPC::Connection::Identifier) override;

    // Implemented in generated WebProcessProxyMessageReceiver.cpp
    void didReceiveWebProcessProxyMessage(IPC::Connection&, IPC::MessageDecoder&);
    void didReceiveSyncWebProcessProxyMessage(IPC::Connection&, IPC::MessageDecoder&, std::unique_ptr<IPC::MessageEncoder>&);

    bool canTerminateChildProcess();

    ResponsivenessTimer m_responsivenessTimer;
    
    RefPtr<WebConnectionToWebProcess> m_webConnection;
    Ref<WebProcessPool> m_processPool;

    bool m_mayHaveUniversalFileReadSandboxExtension; // True if a read extension for "/" was ever granted - we don't track whether WebProcess still has it.
    HashSet<String> m_localPathsWithAssumedReadAccess;

    WebPageProxyMap m_pageMap;
    WebFrameProxyMap m_frameMap;
    WebBackForwardListItemMap m_backForwardListItemMap;

    HashSet<VisitedLinkProvider*> m_visitedLinkProviders;
    HashSet<WebUserContentControllerProxy*> m_webUserContentControllerProxies;

    std::unique_ptr<DownloadProxyMap> m_downloadProxyMap;
    CustomProtocolManagerProxy m_customProtocolManagerProxy;

    int m_numberOfTimesSuddenTerminationWasDisabled;
    std::unique_ptr<ProcessThrottler> m_throttler;
    ProcessThrottler::BackgroundActivityToken m_tokenForHoldingLockedFiles;
};

} // namespace WebKit

#endif // WebProcessProxy_h
