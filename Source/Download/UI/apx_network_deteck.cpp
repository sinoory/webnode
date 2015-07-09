/*-----------------------------------------------------------
*      Copyright (c)  AppexNetworks, All Rights Reserved.
*
*FileName:     apx_network_deteck.c 
*
*Description:  ÍøÂç¹ÊÕÏÌ½²âÄ£¿é
* 
*History: 
*      Author              Date        	Modification 
*  ----------      ----------  	----------
* 	xyfeng   		2015-3-24     	Initial Draft 
* 
*------------------------------------------------------------*/
/*-----------------------------------------------------------*/
/*                          Include File Header                               */
/*-----------------------------------------------------------*/
/*---Include ANSI C .h File---*/
#include <net/if_arp.h>
#include <linux/rtnetlink.h>
#include <linux/if.h>
        
/*---Include Local.h File---*/
#include "apx_network_deteck.h"

/*------------------------------------------------------------*/
/*                          Private Macros Defines                          */
/*------------------------------------------------------------*/
 #define	APX_NET_PRIVATE		0x5A5A5A5A      

#define	APX_NET_DNS	"grep -i nameserver /etc/resolv.conf |awk '{print $2}'"
 
/*------------------------------------------------------------*/
/*                          Private Type Defines                            */
/*------------------------------------------------------------*/
struct rtnl_handle
{
	int			fd;
	struct sockaddr_nl	local;
	u32			seq;
	u32			dump;
};

typedef struct _interf_s_
{
	struct list_head stList;
	struct nlmsghdr   h;
}nlmsghdr_st;

//#define	IFNAMSIZ	16
typedef struct _interface_s_
{
	char		name[IFNAMSIZ];
	int		index;		/* Link index	*/
	unsigned	state;		/* 1:up 0:down	*/
	struct list_head stList;
	struct list_head stIpList;
}interface_st;

typedef struct _ipaddr_s_
{
	char	label[IFNAMSIZ];
	char	ip[16];		/* ip*/
	int 	prefix;
	struct list_head stList;
}ipaddr_st;
        
        
/*------------------------------------------------------------*/
/*                         Global Variables                                */
/*------------------------------------------------------------*/
/*---Extern Variables---*/
        
/*---Local Variables---*/
static struct rtnl_handle rth = { .fd = -1 };
static struct list_head g_stInterFaceList = LIST_HEAD_INIT( g_stInterFaceList );
        
/*------------------------------------------------------------*/
/*                          Local Function Prototypes                       */
/*------------------------------------------------------------*/
        
         
/*------------------------------------------------------------*/
/*                        Functions                                               */
/*------------------------------------------------------------*/
static int rtnl_open(struct rtnl_handle *prth )
{
	socklen_t addr_len;
	int sndbuf = 32768;
	int rcvbuf = 1024 * 1024;
	
	memset(prth, 0, sizeof(*prth));
	prth->fd = socket(AF_NETLINK, SOCK_RAW | SOCK_CLOEXEC, NETLINK_ROUTE);
	if (prth->fd < 0) {
		perror("Cannot open netlink socket");
		return -1;
	}

	if (setsockopt(prth->fd,SOL_SOCKET,SO_SNDBUF,&sndbuf,sizeof(sndbuf)) < 0) {
		perror("SO_SNDBUF");
		return -1;
	}

	if (setsockopt(prth->fd,SOL_SOCKET,SO_RCVBUF,&rcvbuf,sizeof(rcvbuf)) < 0) {
		perror("SO_RCVBUF");
		return -1;
	}

	memset(&prth->local, 0, sizeof(prth->local));
	prth->local.nl_family = AF_NETLINK;
	prth->local.nl_groups = 0;

	if (bind(prth->fd, (struct sockaddr*)&prth->local, sizeof(prth->local)) < 0) {
		perror("Cannot bind netlink socket");
		return -1;
	}
	addr_len = sizeof(prth->local);
	if (getsockname(prth->fd, (struct sockaddr*)&prth->local, &addr_len) < 0) {
		perror("Cannot getsockname");
		return -1;
	}
	if (addr_len != sizeof(prth->local)) {
		fprintf(stderr, "Wrong address length %d\n", addr_len);
		return -1;
	}
	if (prth->local.nl_family != AF_NETLINK) {
		fprintf(stderr, "Wrong address family %d\n", prth->local.nl_family);
		return -1;
	}
	prth->seq = time(NULL);
	return 0;
}

static void rtnl_close(struct rtnl_handle *prth)
{
	if (prth->fd >= 0) {
		close(prth->fd);
		prth->fd = -1;
	}
}


static int rtnl_request(struct rtnl_handle *prth, int type )
{
	struct {
		struct nlmsghdr nlh;
		struct ifinfomsg ifm;
		/* attribute has to be NLMSG aligned */
		struct rtattr ext_req __attribute__ ((aligned(NLMSG_ALIGNTO)));
		__u32 ext_filter_mask;
	} req;

	memset(&req, 0, sizeof(req));
	req.nlh.nlmsg_len = sizeof(req);
	req.nlh.nlmsg_type = type;
	req.nlh.nlmsg_flags = NLM_F_DUMP|NLM_F_REQUEST;
	req.nlh.nlmsg_pid = 0;
	req.nlh.nlmsg_seq = prth->dump = ++prth->seq;
	req.ifm.ifi_family = AF_PACKET;

	req.ext_req.rta_type = IFLA_EXT_MASK;
	req.ext_req.rta_len = RTA_LENGTH(sizeof(u32));
	req.ext_filter_mask = RTEXT_FILTER_VF;

	return send(prth->fd, (void*)&req, sizeof(req), 0);
}

static int parse_rtattr_flags(struct rtattr *tb[], int max, struct rtattr *rta,
		       int len, unsigned short flags)
{
	unsigned short type;

	memset(tb, 0, sizeof(struct rtattr *) * (max + 1));
	while (RTA_OK(rta, len)) {
		type = rta->rta_type & ~flags;
		if ((type <= max) && (!tb[type]))
			tb[type] = rta;
		rta = RTA_NEXT(rta,len);
	}
	if (len)
		fprintf(stderr, "!!!Deficit %d, rta_len=%d\n", len, rta->rta_len);
	return 0;
}

static int parse_rtattr(struct rtattr *tb[], int max, struct rtattr *rta, int len)
{
	return parse_rtattr_flags(tb, max, rta, len, 0);
}

static inline const char *rta_getattr_str(const struct rtattr *rta)
{
	return (const char *)RTA_DATA(rta);
}

static const char *rt_addr_n2a(int af, int len, const void *addr, char *buf, int buflen)
{
	switch (af) {
	case AF_INET:
	case AF_INET6:
		return inet_ntop(af, addr, buf, buflen);
	default:
		return "???";
	}
}

static void add_interface( struct nlmsghdr *n )
{
	interface_st *pstInter = NULL;
	struct ifinfomsg *ifi = NLMSG_DATA(n);
	struct rtattr * tb[IFLA_MAX+1];
	int len = n->nlmsg_len;

	if( n->nlmsg_type != RTM_NEWLINK ) {
		return;
	}

	if( ifi->ifi_flags & IFF_LOOPBACK ) {
		return;
	}
	
	len -= NLMSG_LENGTH(sizeof(*ifi));
	if( len < 0) {
		return;
	}

	pstInter = calloc( 1, sizeof( interface_st ) );
	if( NULL == pstInter ) {
		return;
	}
	INIT_LIST_HEAD( &pstInter->stIpList );
	
	parse_rtattr(tb, IFLA_MAX, IFLA_RTA(ifi), len);
	if( tb[IFLA_IFNAME] != NULL ) {
		strncpy( pstInter->name, rta_getattr_str(tb[IFLA_IFNAME]),  IFNAMSIZ );
	}
	else {
		strncpy( pstInter->name, "<nil>",  IFNAMSIZ );
	}
	pstInter->index = ifi->ifi_index;
	pstInter->state = ( ifi->ifi_flags & IFF_UP ) ? 1 : 0;
	list_add( &pstInter->stList, &g_stInterFaceList );

	return;
}

static void add_ipaddr(struct nlmsghdr *n )
{
	char flag = 0;
	interface_st *pstInter = NULL;
	ipaddr_st * pstIpaddr = NULL;
	struct ifaddrmsg *ifamsg = NLMSG_DATA( n );
	struct rtattr * rta_tb[IFA_MAX+1];
	char abuf[256];

	if( n->nlmsg_type != RTM_NEWADDR ) {
		return;
	}

	if( n->nlmsg_len < NLMSG_LENGTH(sizeof(*ifamsg))) {
		return ;
	}
	
	if( ifamsg->ifa_family != AF_INET) {
		return;
	}
	
	pstIpaddr = calloc( 1, sizeof( ipaddr_st ) );
	if( NULL == pstIpaddr )
	{
		return;
	}

	parse_rtattr(rta_tb, IFA_MAX, IFA_RTA(ifamsg), n->nlmsg_len - NLMSG_LENGTH(sizeof(*ifamsg)));
	if (!rta_tb[IFA_LOCAL])
		rta_tb[IFA_LOCAL] = rta_tb[IFA_ADDRESS];
	if (!rta_tb[IFA_ADDRESS])
		rta_tb[IFA_ADDRESS] = rta_tb[IFA_LOCAL];

	
	if (rta_tb[IFA_LOCAL]) {
		char *pIp = rt_addr_n2a( ifamsg->ifa_family,
					RTA_PAYLOAD(rta_tb[IFA_LOCAL]),
					RTA_DATA(rta_tb[IFA_LOCAL]),
					abuf, sizeof(abuf));
		strncpy( pstIpaddr->ip, pIp, 16 );

		if (rta_tb[IFA_ADDRESS] == NULL ||
			memcmp(RTA_DATA(rta_tb[IFA_ADDRESS]), RTA_DATA(rta_tb[IFA_LOCAL]),
		   	ifamsg->ifa_family == AF_INET ? 4 : 16) == 0) 
		{
			pstIpaddr->prefix = ifamsg->ifa_prefixlen;
		} 
	}

	if (rta_tb[IFA_LABEL]) {
		strncpy( pstIpaddr->label, rta_getattr_str(rta_tb[IFA_LABEL]), IFNAMSIZ );
	}

	list_for_each_entry( pstInter, &g_stInterFaceList, stList ) {
		if( pstInter->index == ifamsg->ifa_index ) {
			list_add( &pstIpaddr->stList, &pstInter->stIpList );
			flag = 1;
			break;
		}
	}

	if( 0 == flag && pstIpaddr != NULL ){
		free( pstIpaddr );
	}

	return;
}

typedef void (*rtnl_filter_t)(struct nlmsghdr *n, void *);


static void store_nlmsg(struct nlmsghdr *n, void *arg)
{
	struct list_head *pstHead = (struct list_head *)arg;

	if( pstHead == &g_stInterFaceList )
	{
		add_interface( n );
	}
	else if( pstHead == ( void* )APX_NET_PRIVATE )
	{
		add_ipaddr( n );
	}
}

static void free_list( struct list_head *pstHead )
{
	interface_st *pstInterPos = NULL;
	interface_st *pstInterTmp = NULL;
	ipaddr_st *pstIpPos = NULL;
	ipaddr_st *pstIpTmp = NULL;

	if( !list_empty( pstHead ) ) {
		list_for_each_entry_safe( pstInterPos, pstInterTmp, pstHead, stList ) {
			list_del_init( &pstInterPos->stList );
			list_for_each_entry_safe( pstIpPos, pstIpTmp, &pstInterPos->stIpList, stList ){
				list_del_init( &pstIpPos->stList );
				free( pstIpPos );
			}
			free( pstInterPos );
		}
	}
}

static int rtnl_dump(struct rtnl_handle *prth, rtnl_filter_t filter, void *arg1)
{
	struct sockaddr_nl nladdr;
	struct iovec iov;
	struct msghdr msg = {
		.msg_name = &nladdr,
		.msg_namelen = sizeof(nladdr),
		.msg_iov = &iov,
		.msg_iovlen = 1,
	};
	char buf[16384];
	int dump_intr = 0;

	iov.iov_base = buf;
	while (1) {
		int status;
		int found_done = 0;
		int msglen = 0;

		iov.iov_len = sizeof(buf);
		status = recvmsg(prth->fd, &msg, 0);

		if (status < 0) {
			if (errno == EINTR || errno == EAGAIN)
				continue;
			fprintf(stderr, "netlink receive error %s (%d)\n",
				strerror(errno), errno);
			return -1;
		}

		if (status == 0) {
			fprintf(stderr, "EOF on netlink\n");
			return -1;
		}

		//for (a = arg; a->filter; a++) 
		{
			struct nlmsghdr *h = (struct nlmsghdr*)buf;
			msglen = status;

			while (NLMSG_OK(h, msglen)) {

				if (nladdr.nl_pid != 0 ||
					h->nlmsg_pid != prth->local.nl_pid ||
					h->nlmsg_seq != prth->dump)
					goto skip_it;

				if (h->nlmsg_flags & NLM_F_DUMP_INTR)
					dump_intr = 1;

				if (h->nlmsg_type == NLMSG_DONE) {
					found_done = 1;
					break; /* process next filter */
				}
				if (h->nlmsg_type == NLMSG_ERROR) {
					struct nlmsgerr *err = (struct nlmsgerr*)NLMSG_DATA(h);
					if (h->nlmsg_len < NLMSG_LENGTH(sizeof(struct nlmsgerr))) {
						fprintf(stderr,"ERROR truncated\n");
					} else {
						errno = -err->error;
						perror("RTNETLINK answers");
					}
					return -1;
				}
				filter( h, arg1);

skip_it:
				h = NLMSG_NEXT(h, msglen);
			}
		}

		if (found_done) {
			if (dump_intr)
				fprintf(stderr,
					"Dump was interrupted and may be inconsistent.\n");
			return 0;
		}

		if (msg.msg_flags & MSG_TRUNC) {
			fprintf(stderr, "Message truncated\n");
			continue;
		}
		if (msglen) {
			fprintf(stderr, "!!!Remnant of size %d\n", msglen);
			exit(1);
		}
	}
}

#define APX_NET_PING	"ping %s -c 2 |awk -F'[, ]' '/transmitted.*loss/{print $5}' "
int ping_cmd( char* pIp )
{
	size_t sLen = 0;
	char buf[80];
	FILE *pFile = NULL;

	snprintf( buf, sizeof( buf ), APX_NET_PING, pIp );
	pFile = popen( buf, "r" );
	if( NULL == pFile ) {
		return 0;
	}

	fgets( buf, sizeof(buf ), pFile );
	sLen = strlen( buf );
	if( buf[sLen - 1] == '\n')
		buf[sLen - 1] = '\0';
	sLen = strlen( buf );
	if( sLen > 0 && atoi( buf ) == 3  ) {
		pclose( pFile );
		return 0;
	}
	
	pclose( pFile );
	return -1;
}

APX_NETWORK_E dns_deteck( char *pDns )
{	
	char set_flag = 0;
	char unreach_flag = 0;
	int ret = 0;
	size_t sLen = 0;
	FILE *pFile = NULL;
	char buf[80];
	
	pFile = popen( APX_NET_DNS, "r" );
	if( NULL == pFile ) {
		return APX_NET_OK;
	}

	while( fgets( buf, sizeof(buf ), pFile ) != NULL ) {
		sLen = strlen( buf );
		if( buf[sLen - 1] == '\n')
			buf[sLen - 1] = '\0';
		sLen = strlen( buf );
		if( sLen > 0 ) {
			set_flag = 1;
			ret = ping_cmd( buf );
			if(  0 == ret ) {
				pclose( pFile );
				return APX_NET_OK;
			}
			else {
				unreach_flag = 1;
				if( pDns ) {
					strncpy( pDns, buf, sLen + 1 );
					pDns[sLen] = '\0';
				}
			}
		}
	}
	pclose( pFile );

	if( 0 == set_flag ) {
		return APX_NET_DNS_UNSET;
	}
	if( unreach_flag ) {
		return APX_NET_DNS_UNREACH;
	}
	return -1;
}


APX_NETWORK_E apx_network_detect( char *pAddr, char *pGateWay )
//int main( int argc, char *argv[] )
{
	char flag = 0;
	int ret = 0;
	interface_st *pstInterPos = NULL;
	ipaddr_st *pstIpPos = NULL;

	rtnl_open( &rth );
	rtnl_request( &rth, RTM_GETLINK );
	ret = rtnl_dump( &rth, store_nlmsg, &g_stInterFaceList );
	if( ret< 0) {
		return APX_NET_OK;
	}

	rtnl_request( &rth, RTM_GETADDR );
	ret = rtnl_dump( &rth, store_nlmsg, ( void* )APX_NET_PRIVATE );
	if( ret< 0) {
		rtnl_close( &rth );
		free_list( &g_stInterFaceList );
		return APX_NET_OK;
	}

	if( list_empty( &g_stInterFaceList ) ) {
		rtnl_close( &rth );
		return APX_NET_INTER_ERR;
	}
	
	list_for_each_entry( pstInterPos, &g_stInterFaceList, stList ) {
		#if 0
		printf( "%d:  %s:  <%s>\n", pstInterPos->index, pstInterPos->name,
				( 1 == pstInterPos->state ) ? "UP" : "Down" );
		#endif		
		list_for_each_entry( pstIpPos, &pstInterPos->stIpList, stList ) {
			flag = 1;
		#if 0	
			printf( "	%s/%d %s\n", pstIpPos->ip, pstIpPos->prefix, pstIpPos->label );
		#endif
		}
	}

	rtnl_close( &rth );
	free_list( &g_stInterFaceList );
	if( 0 == flag ) {
		return APX_NET_IP_UNSET;
	}

	return dns_deteck( pGateWay  );
}

