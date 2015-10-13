// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef NET_SOCKET_SSL_CLIENT_SOCKET_POOL_H_
#define NET_SOCKET_SSL_CLIENT_SOCKET_POOL_H_

#include <map>
#include <string>
#include <vector>

#include "base/memory/ref_counted.h"
#include "base/memory/scoped_ptr.h"
#include "base/time/time.h"
#include "net/base/privacy_mode.h"
#include "net/dns/host_resolver.h"
#include "net/http/http_response_info.h"
#include "net/socket/client_socket_pool.h"
#include "net/socket/client_socket_pool_base.h"
#include "net/socket/client_socket_pool_histograms.h"
#include "net/socket/ssl_client_socket.h"
#include "net/ssl/ssl_config_service.h"

namespace net {

class CertVerifier;
class ClientSocketFactory;
class ConnectJobFactory;
class CTVerifier;
class HostPortPair;
class HttpProxyClientSocketPool;
class HttpProxySocketParams;
class SOCKSClientSocketPool;
class SOCKSSocketParams;
class SSLClientSocket;
class TransportClientSocketPool;
class TransportSecurityState;
class TransportSocketParams;

class NET_EXPORT_PRIVATE SSLSocketParams
    : public base::RefCounted<SSLSocketParams> {
 public:
  enum ConnectionType { DIRECT, SOCKS_PROXY, HTTP_PROXY };

  // Exactly one of |direct_params|, |socks_proxy_params|, and
  // |http_proxy_params| must be non-NULL.
  SSLSocketParams(
      const scoped_refptr<TransportSocketParams>& direct_params,
      const scoped_refptr<SOCKSSocketParams>& socks_proxy_params,
      const scoped_refptr<HttpProxySocketParams>& http_proxy_params,
      const HostPortPair& host_and_port,
      const SSLConfig& ssl_config,
      PrivacyMode privacy_mode,
      int load_flags,
      bool force_spdy_over_ssl,
      bool want_spdy_over_npn);

  // Returns the type of the underlying connection.
  ConnectionType GetConnectionType() const;

  // Must be called only when GetConnectionType() returns DIRECT.
  const scoped_refptr<TransportSocketParams>&
      GetDirectConnectionParams() const;

  // Must be called only when GetConnectionType() returns SOCKS_PROXY.
  const scoped_refptr<SOCKSSocketParams>&
      GetSocksProxyConnectionParams() const;

  // Must be called only when GetConnectionType() returns HTTP_PROXY.
  const scoped_refptr<HttpProxySocketParams>&
      GetHttpProxyConnectionParams() const;

  const HostPortPair& host_and_port() const { return host_and_port_; }
  const SSLConfig& ssl_config() const { return ssl_config_; }
  PrivacyMode privacy_mode() const { return privacy_mode_; }
  int load_flags() const { return load_flags_; }
  bool force_spdy_over_ssl() const { return force_spdy_over_ssl_; }
  bool want_spdy_over_npn() const { return want_spdy_over_npn_; }
  bool ignore_limits() const { return ignore_limits_; }

 private:
  friend class base::RefCounted<SSLSocketParams>;
  ~SSLSocketParams();

  const scoped_refptr<TransportSocketParams> direct_params_;
  const scoped_refptr<SOCKSSocketParams> socks_proxy_params_;
  const scoped_refptr<HttpProxySocketParams> http_proxy_params_;
  const HostPortPair host_and_port_;
  const SSLConfig ssl_config_;
  const PrivacyMode privacy_mode_;
  const int load_flags_;
  const bool force_spdy_over_ssl_;
  const bool want_spdy_over_npn_;
  bool ignore_limits_;

  DISALLOW_COPY_AND_ASSIGN(SSLSocketParams);
};

// SSLConnectJobMessenger handles communication between concurrent
// SSLConnectJobs that share the same SSL session cache key.
//
// SSLConnectJobMessengers tell the session cache when a certain
// connection should be monitored for success or failure, and
// tell SSLConnectJobs when to pause or resume their connections.
class SSLConnectJobMessenger {
 public:
  struct SocketAndCallback {
    SocketAndCallback(SSLClientSocket* ssl_socket,
                      const base::Closure& job_resumption_callback);
    ~SocketAndCallback();

    SSLClientSocket* socket;
    base::Closure callback;
  };

  typedef std::vector<SocketAndCallback> SSLPendingSocketsAndCallbacks;

  SSLConnectJobMessenger();
  ~SSLConnectJobMessenger();

  // Removes |socket| from the set of sockets being monitored. This
  // guarantees that |job_resumption_callback| will not be called for
  // the socket.
  void RemovePendingSocket(SSLClientSocket* ssl_socket);

  // Returns true if |ssl_socket|'s Connect() method should be called.
  bool CanProceed(SSLClientSocket* ssl_socket);

  // Configures the SSLConnectJobMessenger to begin monitoring |ssl_socket|'s
  // connection status. After a successful connection, or an error,
  // the messenger will determine which sockets that have been added
  // via AddPendingSocket() to allow to proceed.
  void MonitorConnectionResult(SSLClientSocket* ssl_socket);

  // Adds |socket| to the list of sockets waiting to Connect(). When
  // the messenger has determined that it's an appropriate time for |socket|
  // to connect, it will invoke |callback|.
  //
  // Note: It is an error to call AddPendingSocket() without having first
  // called MonitorConnectionResult() and configuring a socket that WILL
  // have Connect() called on it.
  void AddPendingSocket(SSLClientSocket* ssl_socket,
                        const base::Closure& callback);

 private:
  // Processes pending callbacks when a socket completes its SSL handshake --
  // either successfully or unsuccessfully.
  void OnSSLHandshakeCompleted();

  // Runs all callbacks stored in |pending_sockets_and_callbacks_|.
  void RunAllCallbacks(
      const SSLPendingSocketsAndCallbacks& pending_socket_and_callbacks);

  base::WeakPtrFactory<SSLConnectJobMessenger> weak_factory_;

  SSLPendingSocketsAndCallbacks pending_sockets_and_callbacks_;
  // Note: this field is a vector to allow for future design changes. Currently,
  // this vector should only ever have one entry.
  std::vector<SSLClientSocket*> connecting_sockets_;
};

// SSLConnectJob handles the SSL handshake after setting up the underlying
// connection as specified in the params.
class SSLConnectJob : public ConnectJob {
 public:
  // Note: the SSLConnectJob does not own |messenger| so it must outlive the
  // job.
  SSLConnectJob(const std::string& group_name,
                RequestPriority priority,
                const scoped_refptr<SSLSocketParams>& params,
                const base::TimeDelta& timeout_duration,
                TransportClientSocketPool* transport_pool,
                SOCKSClientSocketPool* socks_pool,
                HttpProxyClientSocketPool* http_proxy_pool,
                ClientSocketFactory* client_socket_factory,
                HostResolver* host_resolver,
                const SSLClientSocketContext& context,
                SSLConnectJobMessenger* messenger,
                Delegate* delegate,
                NetLog* net_log);
  virtual ~SSLConnectJob();

  // ConnectJob methods.
  virtual LoadState GetLoadState() const OVERRIDE;

  virtual void GetAdditionalErrorState(ClientSocketHandle * handle) OVERRIDE;

 private:
  enum State {
    STATE_TRANSPORT_CONNECT,
    STATE_TRANSPORT_CONNECT_COMPLETE,
    STATE_SOCKS_CONNECT,
    STATE_SOCKS_CONNECT_COMPLETE,
    STATE_TUNNEL_CONNECT,
    STATE_TUNNEL_CONNECT_COMPLETE,
    STATE_CREATE_SSL_SOCKET,
    STATE_CHECK_FOR_RESUME,
    STATE_SSL_CONNECT,
    STATE_SSL_CONNECT_COMPLETE,
    STATE_NONE,
  };

  void OnIOComplete(int result);

  // Runs the state transition loop.
  int DoLoop(int result);

  int DoTransportConnect();
  int DoTransportConnectComplete(int result);
  int DoSOCKSConnect();
  int DoSOCKSConnectComplete(int result);
  int DoTunnelConnect();
  int DoTunnelConnectComplete(int result);
  int DoCreateSSLSocket();
  int DoCheckForResume();
  int DoSSLConnect();
  int DoSSLConnectComplete(int result);

  // Tells a waiting SSLConnectJob to resume its SSL connection.
  void ResumeSSLConnection();

  // Returns the initial state for the state machine based on the
  // |connection_type|.
  static State GetInitialState(SSLSocketParams::ConnectionType connection_type);

  // Starts the SSL connection process.  Returns OK on success and
  // ERR_IO_PENDING if it cannot immediately service the request.
  // Otherwise, it returns a net error code.
  virtual int ConnectInternal() OVERRIDE;

  scoped_refptr<SSLSocketParams> params_;
  TransportClientSocketPool* const transport_pool_;
  SOCKSClientSocketPool* const socks_pool_;
  HttpProxyClientSocketPool* const http_proxy_pool_;
  ClientSocketFactory* const client_socket_factory_;
  HostResolver* const host_resolver_;

  const SSLClientSocketContext context_;

  State next_state_;
  CompletionCallback io_callback_;
  scoped_ptr<ClientSocketHandle> transport_socket_handle_;
  scoped_ptr<SSLClientSocket> ssl_socket_;

  SSLConnectJobMessenger* messenger_;
  HttpResponseInfo error_response_info_;

  base::WeakPtrFactory<SSLConnectJob> weak_factory_;

  DISALLOW_COPY_AND_ASSIGN(SSLConnectJob);
};

class NET_EXPORT_PRIVATE SSLClientSocketPool
    : public ClientSocketPool,
      public HigherLayeredPool,
      public SSLConfigService::Observer {
 public:
  typedef SSLSocketParams SocketParams;

  // Only the pools that will be used are required. i.e. if you never
  // try to create an SSL over SOCKS socket, |socks_pool| may be NULL.
  SSLClientSocketPool(int max_sockets,
                      int max_sockets_per_group,
                      ClientSocketPoolHistograms* histograms,
                      HostResolver* host_resolver,
                      CertVerifier* cert_verifier,
                      ChannelIDService* channel_id_service,
                      TransportSecurityState* transport_security_state,
                      CTVerifier* cert_transparency_verifier,
                      const std::string& ssl_session_cache_shard,
                      ClientSocketFactory* client_socket_factory,
                      TransportClientSocketPool* transport_pool,
                      SOCKSClientSocketPool* socks_pool,
                      HttpProxyClientSocketPool* http_proxy_pool,
                      SSLConfigService* ssl_config_service,
                      bool enable_ssl_connect_job_waiting,
                      NetLog* net_log);

  virtual ~SSLClientSocketPool();

  // ClientSocketPool implementation.
  virtual int RequestSocket(const std::string& group_name,
                            const void* connect_params,
                            RequestPriority priority,
                            ClientSocketHandle* handle,
                            const CompletionCallback& callback,
                            const BoundNetLog& net_log) OVERRIDE;

  virtual void RequestSockets(const std::string& group_name,
                              const void* params,
                              int num_sockets,
                              const BoundNetLog& net_log) OVERRIDE;

  virtual void CancelRequest(const std::string& group_name,
                             ClientSocketHandle* handle) OVERRIDE;

  virtual void ReleaseSocket(const std::string& group_name,
                             scoped_ptr<StreamSocket> socket,
                             int id) OVERRIDE;

  virtual void FlushWithError(int error) OVERRIDE;

  virtual void CloseIdleSockets() OVERRIDE;

  virtual int IdleSocketCount() const OVERRIDE;

  virtual int IdleSocketCountInGroup(
      const std::string& group_name) const OVERRIDE;

  virtual LoadState GetLoadState(
      const std::string& group_name,
      const ClientSocketHandle* handle) const OVERRIDE;

  virtual base::DictionaryValue* GetInfoAsValue(
      const std::string& name,
      const std::string& type,
      bool include_nested_pools) const OVERRIDE;

  virtual base::TimeDelta ConnectionTimeout() const OVERRIDE;

  virtual ClientSocketPoolHistograms* histograms() const OVERRIDE;

  // LowerLayeredPool implementation.
  virtual bool IsStalled() const OVERRIDE;

  virtual void AddHigherLayeredPool(HigherLayeredPool* higher_pool) OVERRIDE;

  virtual void RemoveHigherLayeredPool(HigherLayeredPool* higher_pool) OVERRIDE;

  // HigherLayeredPool implementation.
  virtual bool CloseOneIdleConnection() OVERRIDE;

 private:
  typedef ClientSocketPoolBase<SSLSocketParams> PoolBase;

  // SSLConfigService::Observer implementation.

  // When the user changes the SSL config, we flush all idle sockets so they
  // won't get re-used.
  virtual void OnSSLConfigChanged() OVERRIDE;

  class SSLConnectJobFactory : public PoolBase::ConnectJobFactory {
   public:
    SSLConnectJobFactory(TransportClientSocketPool* transport_pool,
                         SOCKSClientSocketPool* socks_pool,
                         HttpProxyClientSocketPool* http_proxy_pool,
                         ClientSocketFactory* client_socket_factory,
                         HostResolver* host_resolver,
                         const SSLClientSocketContext& context,
                         bool enable_ssl_connect_job_waiting,
                         NetLog* net_log);

    virtual ~SSLConnectJobFactory();

    // ClientSocketPoolBase::ConnectJobFactory methods.
    virtual scoped_ptr<ConnectJob> NewConnectJob(
        const std::string& group_name,
        const PoolBase::Request& request,
        ConnectJob::Delegate* delegate) const OVERRIDE;

    virtual base::TimeDelta ConnectionTimeout() const OVERRIDE;

   private:
    // Maps SSLConnectJob cache keys to SSLConnectJobMessenger objects.
    typedef std::map<std::string, SSLConnectJobMessenger*> MessengerMap;

    TransportClientSocketPool* const transport_pool_;
    SOCKSClientSocketPool* const socks_pool_;
    HttpProxyClientSocketPool* const http_proxy_pool_;
    ClientSocketFactory* const client_socket_factory_;
    HostResolver* const host_resolver_;
    const SSLClientSocketContext context_;
    base::TimeDelta timeout_;
    bool enable_ssl_connect_job_waiting_;
    NetLog* net_log_;
    // |messenger_map_| is currently a pointer so that an element can be
    // added to it inside of the const method NewConnectJob. In the future,
    // elements will be added in a different method.
    // TODO(mshelley) Change this to a non-pointer.
    scoped_ptr<MessengerMap> messenger_map_;

    DISALLOW_COPY_AND_ASSIGN(SSLConnectJobFactory);
  };

  TransportClientSocketPool* const transport_pool_;
  SOCKSClientSocketPool* const socks_pool_;
  HttpProxyClientSocketPool* const http_proxy_pool_;
  PoolBase base_;
  const scoped_refptr<SSLConfigService> ssl_config_service_;

  DISALLOW_COPY_AND_ASSIGN(SSLClientSocketPool);
};

}  // namespace net

#endif  // NET_SOCKET_SSL_CLIENT_SOCKET_POOL_H_
