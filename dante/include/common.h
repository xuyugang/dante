/*
 * Copyright (c) 1997, 1998
 *      Inferno Nettverk A/S, Norway.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. The above copyright notice, this list of conditions and the following
 *    disclaimer must appear in all copies of the software, derivative works
 *    or modified versions, and any portions thereof, aswell as in all
 *    supporting documentation.
 * 2. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *      This product includes software developed by
 *      Inferno Nettverk A/S, Norway.
 * 3. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. 
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT 
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 * Inferno Nettverk A/S requests users of this software to return to
 * 
 *  Software Distribution Coordinator  or  sdc@inet.no
 *  Inferno Nettverk A/S
 *  Oslo Research Park
 *  Gaustadaléen 21
 *  N-0371 Oslo
 *  Norway
 * 
 * any improvements or extensions that they make and grant Inferno Nettverk A/S
 * the rights to redistribute these changes.
 *
 */

/* $Id: common.h,v 1.153 1999/02/26 21:41:47 karls Exp $ */

#ifndef _COMMON_H_
#define _COMMON_H_
#endif

#ifdef HAVE_CONFIG_H
#include "autoconf.h"
#endif  /* HAVE_CONFIG_H */

#include <sys/types.h>
#include <sys/time.h>
#ifdef HAVE_SYS_FILE_H
#include <sys/file.h>
#endif  /* HAVE_SYS_FILE_H */
#include <sys/resource.h>
/* XXX */
#ifdef NEED_UCBINCLUDE_SYS_IOCTL
#include "/usr/ucbinclude/sys/ioctl.h"
#else
#include <sys/ioctl.h>
#endif  /* NEED_UCBINCLUDE_SYS_IOCTL */
#include <sys/ipc.h>
#include <sys/sem.h>
/* XXX This is a hack. Avoid transparent sockaddr union used in linux
   to avoid the use of the union in the code. Mainly used in
   interposition.c*/
#ifdef WE_DONT_WANT_NO_SOCKADDR_ARG_UNION
#ifdef __GNUC__
#define __HAD_GNUC __GNUC__
#undef __GNUC__
#endif  /* __GNUC__ */
#endif  /* WE_DONT_WANT_NO_SOCKADDR_ARG_UNION */
/* XXXAnother hack, get cmsghdr type struct on solaris 2.6 */
#include <sys/socket.h>
#ifdef __HAD_GNUC
#define __GNUC__ __HAD_GNUC
#endif  /* __HAD_GNUC */
#ifdef NEED_SYS_SOCKIO_H
#include <sys/sockio.h>
#endif /* NEED_SYS_SOCKIO_H */
#include <sys/un.h>
#include <sys/stat.h>
#include <sys/uio.h>
#include <sys/wait.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include <assert.h>
#ifdef HAVE_CRYPT_H
#include <crypt.h>
#endif  /* HAVE_CRYPT_H */
#include <ctype.h>
#ifdef SOCKSLIBRARY_DYNAMIC
#include <dlfcn.h>
#endif  /* SOCKSLIBRARY_DYNAMIC */
#ifdef HAVE_VWARNX
#include <err.h>
#endif  /* HAVE_VWARNX */
#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <netdb.h>
#include <pwd.h>
#include <setjmp.h>
#include <signal.h>
#ifdef STDC_HEADERS
#include <stdarg.h>
#else
#include <varargs.h>
#endif  /* STDC_HEADERS */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <syslog.h>
#ifdef HAVE_LIBWRAP
#include <tcpd.h>
#endif  /* HAVE_LIBWRAP */
#include <time.h>
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif  /* HAVE_UNISTD_H */

#ifdef lint
extern const int lintnoloop_common_h;
#else
#define lintnoloop_common_h 0
#endif

/*XXX?*/
#ifndef _CONFIG_H_
#include "config.h"
#endif

#if 0 	/* BUGS */
#define SOCKS_TRYHARDER 	/* XXX should be configure option. */
#endif

#ifndef RLIMIT_OFILE
#define RLIMIT_OFILE RLIMIT_NOFILE
#endif /* !RLIMIT_OFILE */


#ifdef NEED_GETSOCKOPT_CAST
#define getsockopt(a,b,c,d,e) getsockopt((a),(b),(c),(char *)(d),(e))
#define setsockopt(a,b,c,d,e) setsockopt((a),(b),(c),(char *)(d),(e))
#endif  /* NEED_GETSOCKOPT_CAST */

#ifndef HAVE_BZERO
#define bzero(b, len) memset(b, 0, len)
#endif  /* !HAVE_BZERO */

#ifdef DEBUG

#ifndef DIAGNOSTIC
#define DIAGNOSTIC
#endif  /* !DIAGNOSTIC */

/*
 * solaris 2.5.1 and it's stream stuff is broken and puts the processes
 * into never-never land forever on half the sendmsg() calls if they
 * involve ancillary data.  (it seems to deadlock the processes.)
*/

#ifndef HAVE_SENDMSG_DEADLOCK
#define HAVE_SENDMSG_DEADLOCK
#endif

#ifndef HAVE_ACCEPTLOCK
#define HAVE_ACCEPTLOCK
#endif

#endif  /* DEBUG */


/* __P and __BEGIN_DECLS definitions taken from libtool manual */
#undef __BEGIN_DECLS
#undef __END_DECLS
#ifdef __cplusplus
# define __BEGIN_DECLS extern "C" {
# define __END_DECLS }
#else
# define __BEGIN_DECLS /* empty */
# define __END_DECLS /* empty */
#endif

#undef __P
#if defined (__STDC__) || defined (_AIX) \
        || (defined (__mips) && defined (_SYSTYPE_SVR4)) \
        || defined(WIN32) || defined(__cplusplus)
# define __P(protos) protos
#else
# define __P(protos) ()
#endif

#if SIZEOF_CHAR == 1
 typedef unsigned char ubits_8;
 typedef          char sbits_8;
#else
# error "no known 8 bits wide datatype"
#endif

#if SIZEOF_SHORT == 2
 typedef unsigned short ubits_16;
 typedef          short sbits_16;
#else
# if SIZEOF_INT == 2
  typedef unsigned int ubits_16;
  typedef          int sbits_16;
# else
#  error "no known 16 bits wide datatype"
# endif
#endif


#if SIZEOF_INT == 4
 typedef unsigned int ubits_32;
 typedef          int sbits_32;
#else
# if SIZEOF_SHORT == 4
   typedef unsigned short ubits_32;
   typedef          short sbits_32;
# else
#  if SIZEOF_LONG == 4
    typedef unsigned long ubits_32;
    typedef          long sbits_32;
#  else
#   error "no known 32 bits wide datatype"
#  endif /* SIZEOF_LONG == 4 */
# endif /* SIZEOF_SHORT == 4 */
#endif /* SIZEOF_INT == 4 */

#ifndef INADDR_NONE
# define INADDR_NONE (ubits_32) 0xffffffff
#endif  /* !INADDR_NONE */

#ifdef HAVE_LIMITS_H
# include <limits.h>
#endif  /* HAVE_LIMITS_H */

#ifdef HAVE_STRINGS_H
# include <strings.h>
#endif  /* HAVE_STRINGS_H */

#ifndef MAX
# define MAX(a,b) ((a) > (b) ? (a) : (b))
#endif /* !MAX */

#ifndef MIN
# define MIN(a,b) ((a) < (b) ? (a) : (b))
#endif /* !MIN */

#ifdef NEED_EXIT_FAILURE
/* XXX assumes EXIT_SUCCESS is undefined too */
#define EXIT_SUCCESS 0
#define EXIT_FAILURE 1
#endif /* NEED_EXIT_FAILURE */

#ifdef NEED_SA_RESTART
#define SA_RESTART SV_INTERRUPT
#endif  /* NEED_SA_RESTART */

#ifdef NEED_AF_LOCAL
#define AF_LOCAL AF_UNIX
#endif  /* NEED_AF_LOCAL */

#ifndef HAVE_LINUX_SOCKADDR_TYPE
#define __SOCKADDR_ARG struct sockaddr *
#define __CONST_SOCKADDR_ARG const struct sockaddr *
#endif  /* ! HAVE_LINUX_SOCKADDR_TYPE */

#ifdef HAVE_NOMALLOC_REALLOC
#define realloc(p,s) (((p) == NULL) ? (malloc(s)) : (realloc((p),(s))))
#endif  /* HAVE_NOMALLOC_REALLOC */

/* __CONCAT macro from anoncvs */
#ifndef __CONCAT
#if defined(__STDC__) || defined(__cplusplus)
#define __CONCAT(x,y)    x ## y
#else
#define __CONCAT(x,y)    x/**/y
#endif
#endif


/* global variables needed by everyone. */
extern struct config_t config;

	/*
	 * defines
	*/


/*
 * redefine system limits to match that of socks protocol. 
 * No need for these to be bigger than protocol allows, but they
 * _must_ be atleast as big as protocol allows.
*/

#ifdef 	MAXHOSTNAMELEN
#undef 	MAXHOSTNAMELEN
#endif
#define	MAXHOSTNAMELEN		(255 + 1)		/* socks5: 255, +1 for len. */

#ifdef	MAXNAMELEN
#undef 	MAXNAMELEN
#endif
#define 	MAXNAMELEN			(255 + 1)		/* socks5: 255, +1 for len. */

#ifdef	MAXPWLEN
#undef 	MAXPWLEN
#endif
#define 	MAXPWLEN				(255 + 1)		/* socks5: 255, +1 for len. */


/*									"255." "255." "255." "255" "," "65535" + NUL */
#define 	MAXSOCKADDRSTRING	 (4   +   4   + 4   +  3  + 1 +    5   + 1)

/*       											   "," + "65535" + NUL */
#define	MAXSOCKSHOSTSTRING (MAXHOSTNAMELEN + 1  +    5)

#define MAXRULEADDRSTRING	 (MAXSOCKSHOSTSTRING * 2)

#ifndef NUL
#define NUL '\0'
#endif

/*
 * We don't care whether it's called O_NONBLOCK, FNDELAY or whatever.
 * We just want to know whether the flags set give blocking or nonblocking
 * semantics.
*/
#ifndef FNDELAY
#define NONBLOCKING	(O_NONBLOCK | O_NDELAY)
#else
#define NONBLOCKING	(O_NONBLOCK | FNDELAY | O_NDELAY)
#endif

#define CONFIGTYPE_SERVER	1
#define CONFIGTYPE_CLIENT	2

#define PROTOCOL_TCPs		"tcp"
#define PROTOCOL_UDPs		"udp"
#define PROTOCOL_UNKNOWNs	"unknown"

#define LOGTYPE_SYSLOG		0x1
#define LOGTYPE_FILE			0x2

#define NOMEM 					"<memory exhausted>"


	/*
	 * macros
	*/


#define close(n)	closen(n)

#define select(nfds, readfds, writefds, exceptfds, timeout) \
		 selectn(nfds, readfds, writefds, exceptfds, timeout)

#define PORTRESERVED(port)	(ntohs((port)) == 0 ? \
	0 : ntohs((port)) < IPPORT_RESERVED ? 1 : 0)

#define ADDRISBOUND(addr) \
	((((struct sockaddr_in *)(&addr))->sin_addr.s_addr != htonl(INADDR_ANY)) \
	|| (((struct sockaddr_in *)(&addr))->sin_port != htons(0)))

#define ELEMENTS(array) (sizeof(array) / sizeof(array[0]))

#if UCHAR_MAX > 0xff
#define OCTETIFY(a) ((a) = ((a) & 0xff))
#else
#define OCTETIFY(a)	((a) = (a))
#endif
/*
 * Note that it's the argument that will be truncated, not just the
 * returnvalue.
*/


/*
 * macros to manipulate ancillary data depending on if were on sysv or bsd.
*/

/* allocate memory for data.  "size" is the amount of memory to allocate. */
#ifdef HAVE_CMSGHDR
#define CMSG_AALLOC(size) \
	union { \
		char cmsgmem[sizeof(struct cmsghdr) + (size)]; \
		struct cmsghdr align; \
	} cmsgmem; \
	struct cmsghdr *cmsg = &cmsgmem.align
#else /* !HAVE_CMSGHDR */
#define CMSG_AALLOC(size) \
	char cmsgmem[(size)]
#endif /* !HAVE_CMSGHDR */


/*
 * add a object to data.  "object" is the object to add to data at
 * offset "offset".
*/
#ifdef HAVE_CMSGHDR
#define CMSG_ADDOBJECT(object, offset) \
	do \
		memcpy(CMSG_DATA(cmsg) + (offset), &(object), sizeof(object)); \
	while (lintnoloop_common_h)
#else /* !HAVE_CMSGHDR */
#define CMSG_ADDOBJECT(object, offset) \
	do \
		memcpy(cmsgmem + (offset), &(object), sizeof((object))); \
	while (lintnoloop_common_h)
#endif /* !HAVE_CMSGHDR */


/*
 * get a object from data.  "object" is the object to get from data at
 * offset "offset".
*/
#ifdef HAVE_CMSGHDR
#define CMSG_GETOBJECT(object, offset) \
	do \
		memcpy(&(object), CMSG_DATA(cmsg) + (offset), sizeof((object))); \
	while (lintnoloop_common_h)
#else /* !HAVE_CMSGHDR */
#define CMSG_GETOBJECT(object, offset) \
	do \
		memcpy(&(object), cmsgmem + (offset), sizeof((object))); \
	while (lintnoloop_common_h)
#endif /* !HAVE_CMSGHDR */



/* set cmsg for sending */
#ifdef HAVE_CMSGHDR
#define CMSG_SETHDR_SEND(size) \
	do { \
		cmsg->cmsg_level		= SOL_SOCKET; \
		cmsg->cmsg_type		= SCM_RIGHTS; \
		cmsg->cmsg_len			= sizeof(struct cmsghdr) + (size); \
		\
		msg.msg_control		= (caddr_t)cmsg; \
		msg.msg_controllen	= cmsg->cmsg_len; \
	} while (lintnoloop_common_h)
#else /* !HAVE_CMSGHDR */
#define CMSG_SETHDR_SEND(size) \
	do { \
		msg.msg_accrights		= (caddr_t)cmsgmem; \
		msg.msg_accrightslen	= (size); \
	} while (lintnoloop_common_h)
#endif /* !HAVE_CMSGHDR */

/* set cmsg for receiving */
#ifdef HAVE_CMSGHDR
#define CMSG_SETHDR_RECV(size) \
	do { \
		msg.msg_control		= (caddr_t)cmsg; \
		msg.msg_controllen	= (size); \
	} while (lintnoloop_common_h)
#else /* !HAVE_CMSGHDR */
#define CMSG_SETHDR_RECV(size) \
	do { \
		msg.msg_accrights		= (caddr_t)cmsgmem; \
		msg.msg_accrightslen	= (size); \
	} while (lintnoloop_common_h)
#endif /* !HAVE_CMSGHDR */


/* returns length of controldata actually sent. */
#ifdef HAVE_CMSGHDR
#define CMSG_GETLEN(msg)	(msg.msg_controllen - sizeof(struct cmsghdr))
#else 
#define CMSG_GETLEN(msg)	(msg.msg_accrightslen)
#endif


#define INTERNAL_ERROR \
"an internal error was detected at %s:%d\nvalue = %ld, version = %s"

#define SASSERT(expression) 	\
do {									\
	if (!(expression))			\
		SERR(expression);			\
} while (lintnoloop_common_h)


#define SASSERTX(expression) 	\
do {									\
	if (!(expression))			\
		SERRX(expression);		\
} while (lintnoloop_common_h)


/*
 * wrappers around err()/errx()/warn()/warnx() for more consistent error
 * messages.
 * "failure" is the value that was wrong and which caused the internal error.
*/


#define SERR(failure) 				\
do {										\
	SWARN(failure);					\
	abort();								\
} while (lintnoloop_common_h)

#define SERRX(failure) 				\
do {										\
	SWARNX(failure);					\
	abort();								\
} while (lintnoloop_common_h)



#define SWARN(failure) 		\
	swarn(INTERNAL_ERROR,	\
	__FILE__, __LINE__, 	(long int)(failure), rcsid)

#define SWARNX(failure) 	\
	swarnx(INTERNAL_ERROR,	\
	__FILE__, __LINE__, 	(long int)(failure), rcsid)



#define ERR(failure) 								\
do {														\
	warn(INTERNAL_ERROR, __FILE__, __LINE__, 	\
	(long int)(failure), rcsid);					\
	abort();												\
} while (p)

#define ERRX(failure) 								\
do {														\
	warnx(INTERNAL_ERROR, __FILE__, __LINE__, \
	(long int)(failure), rcsid);					\
	abort();												\
} while (lintnoloop_common_h)


#define WARN(failure) \
	warn(INTERNAL_ERROR, __FILE__, __LINE__, (long int)(failure), rcsid)

#define WARNX(failure) \
	warnx(INTERNAL_ERROR, __FILE__, __LINE__, (long int)(failure), rcsid)

#define ERRORMSG(failure) \
	error_msg(LOG_HIGH, INTERNAL_ERROR, __FILE__, __LINE__, (long int)(failure),\
	rcsid)


/* the size of a udp header "packet" (no padding) */
#define PACKETSIZE_UDP(packet) ( 											\
	sizeof((packet)->flag) + sizeof((packet)->frag) 					\
	+ sizeof((packet)->host.atype) + sizeof((packet)->host.port) 	\
	+ (ADDRESSIZE_V5(packet)))

	
/*
 * returns the length of the current address field in socks packet "packet".
 * "packet" can be one of pointer to response_t, request_t or udpheader_t. 
*/
#define ADDRESSIZE(packet) ( \
	  ((packet)->version == SOCKS_V4 ? \
	  (ADDRESSIZE_V4(packet)) : (ADDRESSIZE_V5(packet))))

/*
 *	version specifics
*/
#define ADDRESSIZE_V5(packet) ( 																\
  (packet)->host.atype == SOCKS_ADDR_IPV4 ? 												\
  sizeof((packet)->host.addr.ipv4) :(packet)->host.atype == SOCKS_ADDR_IPV6 ?	\
  sizeof((packet)->host.addr.ipv6) : (strlen((packet)->host.addr.domain) + 1))

#define ADDRESSIZE_V4(packet) ( \
	(packet)->atype == SOCKS_ADDR_IPV4 ? \
	sizeof((packet)->addr.ipv4) : (strlen((packet)->addr.host) + 1))


/*
 * This is for Rgethostbyname() support.  FAKEIP_START is the first
 * address in the range of "fake" ip addresses, FAKEIP_END is the last.
 * There can thus be FAKEIP_END - FAKEIP_START number of fake ip addresses' 
 * supported per program.  INADDR_ANY must not be within the range.
*/
#define FAKEIP_START 1	
#define FAKEIP_END	255


#define SOCKS_V4 		4
#define SOCKS_V5 		5
#define MSPROXY_V2	2

#define SOCKS_V4REPLY_VERSION 0

/* connection authentication METHOD values from rfc19228 */
#define AUTHMETHOD_NONE   	0x00
#define AUTHMETHOD_NONEs  	"none"
#define AUTHMETHOD_GSSAPI 	0x01
#define AUTHMETHOD_GSSAPIs "GSS-API"
#define AUTHMETHOD_UNAME  	0x02
#define AUTHMETHOD_UNAMEs  "username/password"


/* X'03' to X'7F' IANA ASSIGNED 						*/

/* X'80' to X'FE' RESERVED FOR PRIVATE METHODS	*/

#define AUTHMETHOD_NOACCEPT 0xff
#define AUTHMETHOD_NOACCEPTs "no acceptable method"

#define METHODS_MAX	255

/*
 *  Response commands/codes
*/
#define SOCKS_CONNECT				1
#define SOCKS_CONNECTs				"connect"
#define SOCKS_BIND					2
#define SOCKS_BINDs					"bind"
#define SOCKS_UDPASSOCIATE       3
#define SOCKS_UDPASSOCIATEs		"udpassociate"

/* pseudo commands */

#define SOCKS_COMMANDEND			0xff

#define SOCKS_BINDREPLY				(SOCKS_COMMANDEND + 1)
#define SOCKS_BINDREPLYs			"bindreply"

#define SOCKS_PURECONNECT			(SOCKS_BINDREPLY + 1)
#define SOCKS_PURECONNECTs			"connect"

/* misc stuff */
#define SOCKS_ACCEPT					(SOCKS_PURECONNECT + 1)
#define SOCKS_ACCEPTs				"accept"

#define SOCKS_DISCONNECT			(SOCKS_ACCEPT + 1)
#define SOCKS_DISCONNECTs			"disconnect"


/* address types */ 
#define SOCKS_ADDR_IPV4     	0x01
#define SOCKS_ADDR_DOMAIN	 	0x03
#define SOCKS_ADDR_IPV6       0x04

/* reply field values */
#define SOCKS_SUCCESS    		0x00
#define SOCKS_FAILURE    		0x01
#define SOCKS_NOTALLOWED 		0x02
#define SOCKS_NETUNREACH 		0x03
#define SOCKS_HOSTUNREACH 		0x04
#define SOCKS_CONNREFUSED 		0x05
#define SOCKS_TTLEXPIRED  		0x06
#define SOCKS_CMD_UNSUPP  		0x07
#define SOCKS_ADDR_UNSUPP 		0x08
#define SOCKS_INVALID_ADDRESS 0x09

/* version 4 codes. */
#define SOCKSV4_SUCCESS			90
#define SOCKSV4_FAIL				91
#define SOCKSV4_NO_IDENTD		92
#define SOCKSV4_BAD_ID			93




#define MSPROXY_PINGINTERVAL	(6 * 60)	

#define MSPROXY_SUCCESS			0
#define MSPROXY_FAILURE			1

#define MSPROXY_MINLENGTH		172

#define MSPROXY_WILL_CONNECT	0x00000100
#define MSPROXY_WILL_BIND		0x00010200

#define MSPROXY_ADDRINUSE			0x0701

/*
 * Server seems to ignore loworder bits of a 0x47?? command, so take them
 * for our own use.
*/
#define MSPROXY_HELLO				0x0500	/* packet 1 from client.				*/
#define MSPROXY_HELLO_ACK			0x1000	/* packet 1 from server. 				*/

#define MSPROXY_USERINFO			0x1000	/* packet 2 from client.				*/
#define MSPROXY_USERINFO_ACK		0x0400	/* packet 2 from server.				*/

#define MSPROXY_SOMETHING			0x4700	/* packet 3 from client.				*/
#define MSPROXY_SOMETHING_1_ACK	0x4714	/* packet 3 from server.				*/

#define MSPROXY_SOMETHING_2		0x4701	/* packet 4 from client.				*/
#define MSPROXY_SOMETHING_2_ACK	0x4715	/* packet 4 from server, high 8 bits
															seem to vary.							*/
#define MSPROXY_SOMETHING_2_ACK2	0x4716	/* could be this too... dunno.		*/

#define MSPROXY_RESOLVE				0x070d	/* resolve request from client.		*/
#define MSPROXY_RESOLVE_ACK		0x070f	/* resolved info from server.			*/

#define MSPROXY_BIND					0x0704	/* bind request.							*/
#define MSPROXY_BIND_ACK			0x0706	/* bind request accepted.				*/

#define MSPROXY_BIND2				0x0707	/* dunno.									*/
#define MSPROXY_BIND2_ACK			0x0708	/* dunno.									*/

#define MSPROXY_BIND2				0x0707	/* dunno.									*/
#define MSPROXY_BIND2_ACK			0x0708	/* dunno.									*/

#define MSPROXY_LISTEN				0x0406	/* listen() performed(?)				*/

#define MSPROXY_BINDINFO			0x0709	/* info about client server accepted*/

#define MSPROXY_BINDINFO_ACK		0x070a	/* we got the info(?)					*/

#define MSPROXY_CONNECT				0x071e	/* connect request.						*/
#define MSPROXY_CONNECT_ACK		0x0703	/* connect request accepted.			*/

#define MSPROXY_UDPASSOCIATE		0x0705	/* udp associate request.				*/
#define MSPROXY_UDPASSOCIATE_ACK	0x0706	/* udp associate request accepted.	*/

#define MSPROXY_CONNECTED			0x042c	/* client connected to server?		*/

#define MSPROXY_CONNREFUSED		0x4		/* low 12 bits seem to vary.			*/

#define MSPROXY_ADDRINUSE			0x0701	/* maybe...									*/

#define MSPROXY_SESSIONEND			0x251e	/* maybe...									*/


/* flag _bits_ */
#define SOCKS_INTERFACEREQUEST 	0x01
#define SOCKS_USECLIENTPORT		0x04

/* subcommands */
#define SOCKS_INTERFACEDATA		0x01


#define SOCKS_TCP			1
#define SOCKS_UDP			2

#define SOCKS_RECV		0
#define SOCKS_SEND		1

#if 0
/* where is this from? (michaels) */
#if defined(__alpha__)
typedef unsigned int u_int32;
#else
typedef unsigned long u_int32;
#endif
#endif

/* offsets into authentication packet */
#define AUTH_VERSION		0	/* version of method packet.								*/

/* request */
#define AUTH_NMETHODS	1	/* number of methods to offer.							*/
#define AUTH_METHODS		2	/* start of methods to offer.								*/

/* reply */
#define AUTH_VERSION		0	/* offset for version in reply.							*/
#define AUTH_METHOD		1	/* offset for selected method in reply.				*/

/* offsets into username/password negotiation packet */
#define UNAME_VERSION	0
#define UNAME_STATUS		1

/* XXX no ivp6 support currently. */
#define SOCKS_IPV6_ALEN 16

#if !defined(SOCKSLIBRARY_DYNAMIC)
#define sys_accept(s, addr, addrlen)		accept(s, addr, addrlen)
#define sys_bind(s, name, namelen)			bind(s, name, namelen)
#define sys_connect(s, name, namelen)		connect(s, name, namelen)
#define sys_bindresvport(sd, sin)			bindresvport(sd, sin)
#define sys_gethostbyname(name)				gethostbyname(name)
#define sys_gethostbyname2(name, af)		gethostbyname2(name, af)
#define sys_getpeername(s, name, namelen)	getpeername(s, name, namelen)
#define sys_getsockname(s, name, namelen)	getsockname(s, name, namelen)
#define sys_recvfrom(s, buf, len, flags, from, fromlen) \
		  recvfrom(s, buf, len, flags, from, fromlen)
#define sys_rresvport(port)					rresvport(port)
#define sys_sendto(s, msg, len, flags, to, tolen) \
		  sendto(s, msg, len, flags, to, tolen)
#endif

enum operator_t { none, eq, neq, ge, le, gt, lt, range };

struct compat_t {
	unsigned int	reuseaddr:1;		/* set SO_REUSEADDR?								*/
	unsigned int	sameport:1;			/* always try to use same port as client?	*/
	unsigned int	:0;
};


struct logtype_t {
	int				type;			/* type of logging (where to). 						*/
	FILE				**fpv;		/* if logging is to file, this is the open file.	*/
	int 				fpc;
	int				*fplockv;	/* locking of logfiles.									*/
};



/* extensions supported by us. */
struct extension_t {
	unsigned int 		bind:1;		/* use bind extension?							*/
	unsigned int			 :0;
};



/* the address part of a socks packet */
union socksaddr_t {
	struct in_addr ipv4;
	char 				ipv6[SOCKS_IPV6_ALEN];
  	char				domain[MAXHOSTNAMELEN]; /* _always_ stored as C string. */
}; 

/* the hostspecific part of misc. things */
struct sockshost_t {
	unsigned char 			atype;
	union socksaddr_t 	addr;
	in_port_t 				port;
};



struct msproxy_request_t {
	char						username[MAXNAMELEN];
	char						unknown[MAXNAMELEN];
	char 						executable[MAXNAMELEN];
	char 						clienthost[MAXHOSTNAMELEN];

	int32_t					clientid;			/* 1-4 										*/
	int32_t					magic25;				/* 5-8										*/
	int32_t					serverid;			/* 9-12										*/
	unsigned char			serverack;			/* 13: ack of last server packet		*/
	char						pad10[3];			/* 14-16										*/
	unsigned char			sequence;			/* 17: sequence # of this packet.	*/
	char						pad11[7];			/* 18-24										*/
	char						RWSP[4];				/* 25-28: 0x52,0x57,0x53,0x50			*/
	char						pad15[8];			/* 29-36										*/
	int16_t					command;				/* 37-38										*/

	/* packet specifics start at 39. */
	union {
		struct {
			char				pad1[18];			/* 39-56										*/
			int16_t			magic3;				/* 57-58										*/
			char           pad3[114];			/* 59-172									*/
			int16_t 			magic5;				/* 173-174: 0x4b, 0x00					*/
			char				pad5[2];				/* 175-176									*/
			int16_t			magic10;				/* 177-178: 0x14, 0x00					*/
			char				pad6[2];				/* 179-180									*/
			int16_t			magic15;				/* 181-182: 0x04, 0x00					*/
			char				pad10[6];			/* 183-188									*/
			int16_t			magic20;				/* 189-190: 0x57, 0x04					*/
			int16_t			magic25;				/* 191-192: 0x00, 0x04					*/
			int16_t			magic30;				/* 193-194: 0x01, 0x00					*/
			char				pad20[2];			/* 195-196: 0x4a, 0x02					*/
			int16_t			magic35;				/* 197-198: 0x4a, 0x02					*/
			char				pad30[10];			/* 199-208									*/
			int16_t			magic40;				/* 209-210: 0x30, 0x00					*/
			char				pad40[2];			/* 211-212									*/
			int16_t			magic45;				/* 213-214: 0x44, 0x00					*/
			char				pad45[2];			/* 215-216									*/
			int16_t			magic50;				/* 217-218: 0x39, 0x00					*/
			char				pad50[2];			/* 219-220									*/
		} _1;

		struct {
			char				pad1[18];			/* 39-56										*/
			int16_t			magic3;				/* 57-58										*/
			char           pad3[114];			/* 59-172									*/
			int16_t			magic5;				/* 173-174: 0x00, 0x4b					*/
			char				pad5[2];				/* 175-176									*/
			int16_t			magic10;				/* 177-178: 0x14, 0x00					*/
			char				pad10[2];			/* 179-180									*/
			int16_t			magic15;				/* 181-182: 0x04, 0x00					*/
			char				pad15[6];			/* 183-188									*/
			int16_t			magic20;				/* 189-190: 0x57, 0x04					*/
			int16_t			magic25;				/* 191-192: 0x00, 0x04					*/
			int16_t			magic30;				/* 193-194: 0x01, 0x00					*/
			char				pad20[2];			/* 195-196									*/
			int16_t			magic35;				/* 197-198: 0x04, 0x00					*/
			char				pad25[10];			/* 199-208									*/
			int16_t			magic50;				/* 209-210: 0x30, 0x00					*/
			char				pad50[2];			/* 211-212									*/
			int16_t			magic55;				/* 213-214: 0x44, 0x00					*/
			char				pad55[2];			/* 215-216									*/
			int16_t			magic60;				/* 217-218: 0x39, 0x00					*/
		} _2;

		struct {
			char				pad1[4];				/* 39-42										*/
			int16_t			magic2;				/* 43-44										*/
			char				pad10[12];			/* 45-56										*/
			in_addr_t		bindaddr;			/* 57-60: address to bind.				*/
			in_port_t		bindport;			/* 61-62: port to bind.					*/
			char           pad15[2];			/* 63-64										*/
			int16_t			magic3;				/* 65-66										*/
			in_port_t		boundport;			/* 67-68										*/
			char           pad20[104];			/* 69-172									*/
			char				NTLMSSP[sizeof("NTLMSSP")];	/* 173-180: "NTLMSSP"	*/
			int16_t			magic5;				/* 181-182: 0x01, 0x00					*/
			char				pad25[2];			/* 183-184									*/
			int16_t			magic10;				/* 185-186: 0x96, 0x82					*/
			int16_t			magic15;				/* 187-188: 0x08, 0x00					*/
			int16_t			magic20;				/* 189-190: 0x28, 0x00					*/
			char				pad30[2];			/* 191-192									*/
			int16_t			magic25;				/* 193-194: 0x96, 0x82					*/
			int16_t			magic30;				/* 195-196: 0x01, 0x00					*/
			char				pad40[12];			/* 197-208									*/
			int16_t			magic50;				/* 209-210: 0x30, 0x00					*/
			char				pad50[6];			/* 211-216									*/
			int16_t			magic55;				/* 217-218: 0x30, 0x00					*/
			char				pad55[2];			/* 219-220									*/
		} _3;

		struct {
			char				pad1[4];				/* 39-42										*/
			int16_t			magic1;				/* 43-44										*/
			int32_t			magic2;				/* 45-48 									*/
			char				pad2[8];				/* 49-56										*/
			int16_t			magic3;				/* 57-58										*/
			char				pad3[6];				/* 59-64										*/
			int16_t			magic4;				/* 65-66										*/
			in_port_t		boundport;			/* 67-68										*/
			char           pad4[104];			/* 69-172									*/
			char				NTLMSSP[sizeof("NTLMSSP")];	/* 173-180: "NTLMSSP"	*/
			int16_t			magic5;				/* 181-182: 0x03, 0x00					*/
			char				pad5[2];				/* 183-184									*/
			int16_t			magic10;				/* 185-186: 0x18, 0x00					*/
			int16_t			magic15;				/* 187-188: 0x18, 0x00					*/
			int16_t			magic20;				/* 189-190: 0x49, 0x00					*/
			char				pad10[6];			/* 191-196									*/
			int16_t			magic30;				/* 197-198: 0x61, 0x00					*/
			char				pad15[2];			/* 199-200									*/
			int16_t			magic35;				/* 201-202: 0x08, 0x00					*/
			int16_t			magic40;				/* 203-204: 0x08, 0x00					*/
			int16_t			magic45;				/* 205-206: 0x34, 0x00					*/
			char				pad20[2];			/* 207-208									*/
			int16_t			magic50;				/* 209-210: 0x07, 0x00					*/
			int16_t			magic55;				/* 211-212: 0x07, 0x00					*/
			int16_t			magic60;				/* 213-214: 0x3c, 0x00					*/
			char				pad25[2];			/* 215-216									*/
			int16_t			magic65;				/* 217-218: 0x06, 0x00					*/
			int16_t			magic70;				/* 219-220: 0x06, 0x00					*/
			int16_t			magic75;				/* 221-222: 0x43, 0x00					*/
		} _4;
		
		struct {
			unsigned char	hostlength;			/* length of host, including NUL.	*/
			char				pad1[17];			/* 39-56										*/
			char				*host;				/* 57-...									*/
		} resolve;

		struct {
			int16_t			magic1;				/* 39-40										*/
			char				pad1[4];				/* 41-45										*/
			int32_t			magic3;				/* 45-48 									*/
			char				pad5[8];				/* 48-56										*/
			int16_t			magic6;				/* 57-58: 0x0200							*/
			in_port_t		destport;			/* 59-60										*/
			in_addr_t		destaddr;			/* 61-64										*/
			char				pad10[4];			/* 65-68										*/
			int16_t			magic10;				/* 69-70										*/
			char				pad15[2];			/* 71-72										*/
			in_port_t		srcport;				/* 73-74: port client connects from	*/
			char				pad20[82];			/* 75-156									*/
		} _5;

		struct {
			int16_t			magic1;				/* 39-40 									*/
			char				pad5[2];				/* 41-42										*/
			int16_t			magic5;				/* 43-44 									*/
			int32_t			magic10;				/* 45-48 									*/
			char				pad10[2];			/* 49-50										*/
			int16_t			magic15;				/* 51-52 									*/
			int32_t			magic16;				/* 53-56										*/
			int16_t			magic20;				/* 57-58 									*/
			in_port_t		clientport;			/* 59-60: forwarded port.				*/
			in_addr_t		clientaddr;			/* 61-64: forwarded address.			*/
			int32_t			magic30;				/* 65-68										*/
			int32_t			magic35;				/* 69-72										*/
			in_port_t		serverport;			/* 73-74: port server will connect
														 *	       to us from.
														*/
			in_port_t		srcport;				/* 75-76: connect request; port used
														 * 		 on client behalf.
														*/
			in_port_t		boundport;			/* 77-78: bind request; port used
														 * 		 on client behalf.
														*/
			in_addr_t		boundaddr;			/* 79-82: addr used on client behalf*/
			char				pad30[90];			/* 83-172									*/
		} _6;

	} packet;
};

struct msproxy_response_t {
	int32_t					packetid;			/* 1-4 										*/
	int32_t					magic5;				/* 5-8										*/
	int32_t              serverid;			/* 9-12										*/
	char						clientack;			/* 13: ack of last client packet.	*/
	char						pad5[3];				/* 14-16										*/
	unsigned char			sequence;			/* 17: sequence # of this packet.	*/
	char						pad10[7];			/* 18-24										*/
	char						RWSP[4];				/* 25-28: 0x52,0x57,0x53,0x50			*/
	char						pad15[8];			/* 29-36										*/
	int16_t					command;				/* 37-38										*/

	union {
		struct {
			char				pad5[18];			/* 39-56										*/
			int16_t			magic20;				/* 57-58: 0x02, 0x00						*/
			char				pad10[6];			/* 59-64										*/
			int16_t			magic30;				/* 65-66: 0x74, 0x01						*/
			char				pad15[2];			/* 67-68										*/
			int16_t			magic35;				/* 69-70: 0x0c, 0x00						*/
			char				pad20[6];			/* 71-76										*/
			int16_t			magic50;				/* 77-78: 0x04, 0x00						*/
			char				pad30[6];			/* 79-84										*/
			int16_t			magic60;				/* 85-86: 0x65, 0x05						*/
			char				pad35[2];			/* 87-88										*/
			int16_t			magic65;				/* 89-90: 0x02, 0x00						*/
			char				pad40[8];			/* 91-98										*/
			in_port_t		udpport;				/* 99-100									*/
			in_addr_t		udpaddr;				/* 101-104									*/
		} _1;

		struct {
			char				pad5[18];			/* 39-56										*/
			int16_t			magic5;				/* 57-58: 0x01, 0x00						*/
		} _2;
		
		struct {
			char				pad1[6];				/* 39-44										*/
			int32_t			magic10;				/* 45-48										*/
			char				pad3[10];			/* 49-58										*/
			in_port_t		boundport;			/* 59-60: port server bound for us.	*/
			in_addr_t		boundaddr;			/* 61-64: addr server bound for us.	*/
			char				pad10[4];			/* 65-68										*/
			int16_t			magic15;				/* 69-70										*/
			char				pad15[102];			/* 70-172									*/
			char				NTLMSSP[sizeof("NTLMSSP")];	/* 173-180: "NTLMSSP"	*/
			int16_t			magic50;				/* 181-182: 0x02, 0x00					*/
			char				pad50[2];			/* 183-184									*/
			int16_t			magic55;				/* 185-186: 0x08, 0x00					*/
			int16_t			magic60;				/* 187-188: 0x08, 0x00					*/
			int16_t			magic65;				/* 189-190: 0x28, 0x00					*/
			char				pad60[2];			/* 191-192									*/
			int16_t			magic70;				/* 193-194: 0x96, 0x82					*/
			int16_t			magic75;				/* 195-196: 0x01, 0x00					*/
			char				pad70[16];			/* 197-212									*/
			char				ntdomain[257];		/* 213-EOP									*/
		} _3;

		struct {
			char				pad5[134];			/* 39-172									*/
		} _4;

		struct {
			unsigned char	addroffset;			/* 39: weird, probably wrong.			*/
			char				pad5[13];			/* 40-52										*/
			in_addr_t		hostaddr;			/* ?-?+4										*/
		} resolve;

		struct {
			int16_t			magic1;				/* 39-40 									*/
			char				pad5[18];			/* 41-58										*/
			in_port_t		clientport;			/* 59-60: forwarded port.				*/
			in_addr_t		clientaddr;			/* 61-64: forwarded address.			*/
			int32_t			magic10;				/* 65-68										*/
			int32_t			magic15;				/* 69-72										*/
			in_port_t		serverport;			/* 73-74: port server will connect
														 *	       to us from.
														*/
			in_port_t		srcport;				/* 75-76: connect request; port used
														 * 		 on client behalf.
														*/
			in_port_t		boundport;			/* 77-78: bind request; port used
														 * 		 on client behalf.
														*/
			in_addr_t		boundaddr;			/* 79-82: addr used on client behalf*/
			char				pad10[90];			/* 83-172									*/
		} _5;
	} packet;
};

struct request_t {
	unsigned char 			version; 
	unsigned char 			command;
	unsigned char 			flag; 
	struct sockshost_t	host;
	struct authmethod_t	*auth;	/* pointer to level above. */
};


struct response_t {
	unsigned char 			version; 
	unsigned char 			reply;
	unsigned char 			flag; 
	struct sockshost_t	host;
	struct authmethod_t	*auth;	/* pointer to level above. */
};

/* encapsulation for udp packets. */
struct udpheader_t {
	unsigned char 			flag[2];
	unsigned char 			frag;
	struct sockshost_t	host;
};


/* interface request packet. */
struct interfacerequest_t {
	unsigned char 			rsv;
	unsigned char 			sub;
	unsigned char 			flag;
	struct sockshost_t	host;
};


/* username */
struct uname_t {
	unsigned char	version;
	char				name[MAXNAMELEN];
	char				password[MAXPWLEN];
};


/* this must be big enough to hold a complete method request. */
struct authmethod_t {
	unsigned char 			method;
	union {
		struct uname_t		uname;
	} mdata;
};


struct method_t {
	unsigned int	none:1;
	unsigned int	uname:1;
	unsigned int	:0;
};


struct protocol_t {
	unsigned int	tcp:1;
	unsigned int	udp:1;
	unsigned int	:0;
};


struct command_t {
	unsigned int	bind:1;
	unsigned int	connect:1;
	unsigned int	udpassociate:1;

	/* not real commands as per standard, but they can have their use. */
	unsigned int	bindreply:1;		/* allow a reply to bind command.	*/
	unsigned int	:0;
};


struct proxyprotocol_t {
	unsigned int			socks_v4:1;
	unsigned int			socks_v5:1;
	unsigned int			msproxy_v2:1;
	unsigned int			:0;
};


struct serverstate_t {
	struct command_t			command;
	struct extension_t		extension;
	struct protocol_t			protocol;
	char							methodv[METHODS_MAX];	/* methods to offer.			*/
	unsigned	char				methodc;						/* number of methods set.	*/
	struct proxyprotocol_t	proxyprotocol;
};


struct msproxy_state_t {
	struct sockaddr_in		controladdr;	/* udp address of proxyserver.		*/
	int32_t						magic25;
	int32_t						bindid;		
	int32_t						clientid;			
	int32_t						serverid;			
	unsigned char				seq_recv;		/* seq number of last packet recv.	*/
	unsigned char				seq_sent;		/* seq number of last packet sent.	*/
};


struct gateway_t {
	struct sockshost_t			host;
	struct serverstate_t			state;
};

/* values in parentheses designate "don't care" values.	*/
struct socksstate_t {
	int							acceptpending;	/* a accept pending?		(-1)			*/
	struct authmethod_t		auth;				/* authentication in use.				*/
	int							command;			/* command (-1)							*/
	int							err;				/* if request failed, errno. 			*/
	int 							inprogress;		/* operation in progress? (-1)	  	*/
#ifdef SOCKS_TRYHARDER
	int							lock;				/* some calls require a lock.			*/
#endif
	struct msproxy_state_t 	msproxy;			/* if msproxy, msproxy state.			*/
	struct protocol_t			protocol;		/* protocol in use.						*/
	unsigned int				udpconnect:1;	/* connected udp socket?				*/
	int							system;			/* don't check, use system call.		*/
	int 							version;			/* version 									*/
};


struct ruleaddress_t {
	char						atype;
	union {

		char					domain[MAXHOSTNAMELEN];
		
		struct {
			struct in_addr	ip;
			struct in_addr	mask;
		} ipv4;

	} addr;

	struct {
		in_port_t 			tcp;			/* tcp portstart or field to operator on.	*/
		in_port_t 			udp;			/* udp portstart or field to operator on.	*/
	} port;
	in_port_t 				portend;		/* only used if operator is range.			*/
	enum operator_t		operator;	/* operator to compare ports via.			*/
};

struct route_t {
	int							number;		/* routenumber.								*/

	struct {
		unsigned int			bad:1;		/* route is bad?								*/
		unsigned int			direct:1;	/* direct connection, no proxy.			*/
	} state;


	struct ruleaddress_t		src;
	struct ruleaddress_t		dst;
	struct gateway_t			gw;

	struct route_t				*next;		/* next route in list.						*/
};


struct socks_t {
	unsigned char 				version;
									/*
							 		 *	Negotiated version.  Each request and
							 		 *	response will also contain a version number, that 
							 		 *	is the version number given for that particular
							 		 *	packet and should be checked to make sure it is
							 		 *  the same as the negotiated version.
										*/

  	struct request_t 				req;
  	struct response_t 			res;
	struct authmethod_t			auth;
	struct gateway_t				gw;
	struct socksstate_t			state;
};



enum portcmp { e_lt, e_gt, e_eq, e_neq, e_le, e_ge, e_nil };
typedef enum portcmp Portcmp;



/*
 * for use in generic functions that take either reply or request
 * packets, include a field indicating what it is.
*/
#define SOCKS_REQUEST 	0x1
#define SOCKS_RESPONSE	0x2

struct socksfd_t {
	unsigned int			allocated:1;/* allocated?										*/
	int 						control;		/* control connection to server.				*/
	struct socksstate_t 	state;		/* state of this connection.		 			*/
	struct sockaddr 		local;		/* local address of data connection.		*/
	struct sockaddr 		server;		/* remote address of data connection.		*/
	struct sockaddr 		remote;		/* address server is using on our behalf.	*/
	struct sockaddr		reply;		/* address to expect reply from.				*/

	/* XXX union this. */
	struct sockaddr		accepted;	/* address server accepted for us.	 		*/
	struct sockaddr		connected;	/* address server connected to for us.		*/

	struct route_t 		*route;
};

__BEGIN_DECLS

/*
 * versions of BSD's error functions that log via slog() instead.
*/

#ifdef STDC_HEADERS
void serr(int eval, const char *fmt, ...);
#else
void serr();
#endif  /* STDC_HEADERS */

#ifdef STDC_HEADERS
void serrx(int eval, const char *fmt, ...);
#else
void serrx();
#endif  /* STDC_HEADERS */

#ifdef STDC_HEADERS
void swarn(const char *fmt, ...);
#else
void swarn();
#endif  /* STDC_HEADERS */

#ifdef STDC_HEADERS
void swarnx(const char *fmt, ...);
#else
void swarnx();
#endif  /* STDC_HEADERS */


struct udpheader_t *
sockaddr2udpheader __P((const struct sockaddr *to, struct udpheader_t *header));
/*
 * Writes a udpheader representation of "to" to "header".
 * Returns a pointer to "header".
*/

char *
udpheader_add __P((const struct sockaddr *to, const char *msg, size_t *len));
/*
 * Prefixes the udpheader_t version of "to" to "msg", which is of 
 * length "len".
 * Upon return "len" gives the length of the returned "msg".
 *	Returns:
 *		On success: the new string.
 *		On failure: NULL.
*/

struct udpheader_t *
string2udpheader __P((const char *data, size_t len,
							 struct udpheader_t *header));
/*
 * Converts "data" to udpheader_t representation. 
 * "len" is length of "data". 
 * "data" is assumed to be in network order.
 * Returns:
 * 	On success: pointer to a udpheader_t in static memory.
 *		On failure: NULL ("data" is not a complete udppacket).
*/


const char *
socks_packet2string __P((const void *packet, int type));
/*
 * debug function; dumps socks packet content
 * "packet" is a socks packet, "type" indicates it's type.
 * Returns:
 * 	On success: 0
 *		On failure: -1
 */

int
socks_socketisbound __P((int s));
/*
 * Returns:
 * 	If "s" is bound: 1
 *		If "s" is not bound: 0
 *		If "s" is not socket or error occured determining if bound: -1
*/

int
fdisopen __P((int fd));
/*
 * returns 1 if the filedescriptor "fd" currently references a open object.
 * returns 0 otherwise.
*/

int
socks_logmatch(int d, const struct logtype_t *log);
/*
 * Returns true if "d" is a descriptor matching any descriptor in "log".
 * Returns false otherwise.
*/

char *
sockaddr2string __P((const struct sockaddr *address, char *string, size_t len));
/*
 * Returns the ip address and port in "address" on string form.
 * "address" is assumed to be on network form and it will be
 * converted to host form before written to "string".
 * "len" gives length of the NUL terminated string.
 * Returns: "string".
*/


struct sockaddr *
sockshost2sockaddr __P((const struct sockshost_t *shost,
								struct sockaddr *addr));
/*
 * Converts the sockhost_t "shost" to a sockaddr struct and stores it 
 * in "addr".
 * Returns: "addr".
*/

struct sockshost_t *
sockaddr2sockshost __P((const struct sockaddr *addr, struct sockshost_t *host));
/*
 * Converts the sockaddr struct "shost" to a sockshost_t struct and stores it 
 * in "host".
 * Returns: "host".
*/

struct sockshost_t *
ruleaddress2sockshost __P((const struct ruleaddress_t *address,
									struct sockshost_t *host, int protocol));
/*
 * Converts the ruleaddress_t "address" to a sockshost_t struct and stores it 
 * in "host".
 * Returns: "host".
*/

struct ruleaddress_t *
sockshost2ruleaddress __P((const struct sockshost_t *host,
									struct ruleaddress_t *addr));
/*
 * Converts the sockshost_t "host" to a ruleaddress_t struct and stores it 
 * in "addr".
 * Returns: "addr".
*/

struct ruleaddress_t *
sockaddr2ruleaddress(const struct sockaddr *addr,
							struct ruleaddress_t *ruleaddr);
/*
 * Converts the struct sockaddr "addr" to a ruleaddress_t struct and stores it 
 * in "ruleaddr".
 * Returns: "addr".
*/

int 
sockatmark __P((int s));
/* 
 * If "s" is at oob mark, return 1, otherwise 0.
 * Returns -1 if a error occurred.
*/

ssize_t
recvmsgn __P((int s, struct msghdr *msg, int flags, size_t len));
/*
 * Like recvmsg(), but tries to read until "len" has been read.
 * BUGS:
 *   Assumes msg->msg_iov[n] are laid out next to each others.
*/

ssize_t
readn __P((int, void *, size_t));
/*
 * Like read() but retries. 
*/

ssize_t
writen __P((int, const void *, size_t));
/*
 * like write() but retries.
*/

int
closen __P((int));
/*
 * Wrapper around close().  Retries on EINTR.
*/

int
selectn __P((int, fd_set *, fd_set *, fd_set *, struct timeval *));
/* 
 * Wrapper around select().  Retries on EINTR.
*/

int
acceptn __P((int, struct sockaddr *, socklen_t *));
/*
 * Wrapper around accept().  Retries on EINTR.
*/


char *
sockshost2string __P((const struct sockshost_t *host, char *string,
							 size_t len));
/*
 * Writes "host" out as a string.  The string is written to "string",
 * which is of length "len", including NUL termination.
 * Returns: "string".
*/

const char *
strcheck __P((const char *string));
/* 
 * Checks "string".  If it is NULL, returns a string indicating memory
 * exhausted, if not, returns the same string it was passed.
*/

const char *
command2string __P((int command));
/*
 * Returns a printable representation of the socks command "command". 
 * Can't fail.
*/

const char *
method2string __P((int method));
/*
 * Returns a printable representation of the authmethod "method".
 * Can't fail.
*/



char *
sockshost2mem __P((const struct sockshost_t *host, char *mem, int version));
/*
 * Writes "host" out to "mem".  The caller must make sure "mem"
 * is big enough to hold the contents of "host".
 * "version" gives the socks version "host" is to be written out in.
 * Returns a pointer to one element past the last byte written to "mem".
*/

const char *
mem2sockshost __P((struct sockshost_t *host, const char *mem, size_t len,
						 int version));
/*
 * Writes "mem", which is assumed to be a sockshost string 
 * of version "version" in network order, out to "host".
 * Returns:
 *		On success: pointer to one element past last byte used of mem
 *						and fills in "host" appropriately.
 *		On failure: NULL ("mem" is not a valid sockshost.)
*/
#ifdef STDC_HEADERS
void slog(int priority, const char *message, ...);
#else
void slog();
#endif  /* STDC_HEADERS */
/*
 * Logs message "message" at priority "priority" to previously configured
 * outputdevice.
 * Checks settings and ignores message if it's of to low a priority.
*/

void vslog __P((int priority, const char *message, va_list ap));
/*
 * Same as slog() but assumes varargs/stdargs have already processed 
 * the arguments.
*/

int
readconfig __P((FILE *fp));
/*
 * Parses the config in the open file "fp" from current offset.
 * Returns:
 *		On success: 0.
 *		On failure: -1.  If failure is in config, function exits.
*/


int
addressmatch __P((const struct ruleaddress_t *rule, 
			 			const struct sockshost_t *address, int protocol,
			 			int ipalias));
/*
 * Tries to match "address" against "rule".  "address" is resolved
 * if necessary.  "address" supports the wildcard INADDR_ANY and port of 0.
 * "protocol" is the protocol to compare under.
 * If "ipalias" is true, the function will try to match any ip alias
 * "address"'s might have if appropriate, this can be useful to match
 * multihomed hosts where the client requests e.g a bind connection.
 * Returns true if "rule" matched "address". 

*/


int
socks_connect __P((int s, const struct sockshost_t *host));
/*
 * Tries to connect to "host".  If "host"'s address is not a ip address,
 * the function also tries to connect to any alias for "host"'s
 * name.  The connection is done using the open descriptor "s".
 * Returns:
 *		On success: 0
 *		On failure: -1 
*/

struct route_t *
socks_connectroute __P((int s, struct socks_t *packet,
						 		const struct sockshost_t *src,
						 		const struct sockshost_t *dst));
/*
 * Finds a route from "src" to "dst" and connects to it "s".
 * If any of the arguments is NULL, that argument is ignored.
 *
 * The route used may take into account the contents of "packet->req",
 * which is assumed to be the packet that will be sent to a socksserver,
 * so it is recommended that it's contents be as conservative as possible.
 * 
 * When it has successfully connected to a gateway it will set 
 * the packet->method members to point to the methods the gateway 
 * should be offered.
 *
 * Returns:
 *		On success: the route that was used.
 *		On failure: NULL.  If errno is 0, the reason for failure was
 *   					that no route was found.
*/


struct request_t *
socks_requestpolish __P((struct request_t *req, const struct sockshost_t *src,
							   const struct sockshost_t *dst));
/* 
 * Tries to "polish" the request "req" so that a later socks_getroute()
 * will succeed.
 * Returns:
 *		On success: "req".
 *		On failure: NULL.
*/

void
showstate __P((const struct serverstate_t *state));
/*
 * Shows "state".
*/


struct route_t *
addroute __P((const struct route_t *route));
/*
 * Appends a copy of "route" to our list of routes.
 * Returns a pointer to the added route. 
*/

void
showroute __P((const struct route_t *route));
/*
 * prints the route "route".
*/


struct route_t *
socks_getroute __P((const struct request_t *req, const struct sockshost_t *src,
						  const struct sockshost_t *dst));
/*
 * Tries to find a  route to be used for a connection going from
 * "src" to "dst".
 * If any of the arguments is NULL, that argument is ignored.
 *
 * The route used may take into account the contents of "req", which is
 * assumed to be the packet that will be sent to a socksserver, so it is
 * recommended that it's contents be as conservative as possible.
 *
 * Returns:
 *		On success: pointer to route that should be used.
 *		On failure: NULL (no socks route found).
*/

const char *
ruleaddress2string __P((const struct ruleaddress_t *rule, char *string,
								size_t len));
/*
 * Writes "rule" out as a string.  The string is written to "string",
 * which is of length "len", including NUL termination.
 * Returns: "string".
*/


int
sockscode __P((int version, int code));
/*
 * Maps the socks replycode "code", which is in non-version specific format,
 * to the equivalent replycode in version "version".
*/

int
errno2reply __P((int errnum, int version));
/*
 * Returns the socks version "version" reply code for a error of type "errno".
*/

enum operator_t
string2operator __P((const char *operator));
/*
 * Returns the enum for the string representation of a operator.
 * Can't fail.
*/

const char *
operator2string __P((enum operator_t operator));
/*
 * Returns the string representation of the operator.
 * Can't fail.
*/

char *
str2vis(const char *string, size_t len);
/* 
 * Visually encodes exactly "len" chars of "string".
 * Returns:
 *		On success: the visually encoded string, to be free()'ed by caller.
 * 	On failure: NULL.  (out of memory).
*/

in_addr_t
socks_addfakeip __P((const char *name));
/*
 * Adds "name" to a internal table indexed by (fake)ip addresses.
 * Returns:
 *		On success: "name"'s index.
 *		On failure:	INADDR_NONE
*/

const char *
socks_getfakehost __P((in_addr_t addr));
/*
 * If "addr" is a "fake" (non-resolved) addr, it returns the name
 * corresponding to it.
 * Else, NULL is returned.
*/

int
socks_getfakeip __P((const char *host, struct in_addr *addr));
/*
 * If "host" has a fake address entry, the address is written into
 * "addr".
 * Returns: 
 * 	If a fake address exits: 1
 *		Else: 0
*/

int
sockaddrcmp __P((const struct sockaddr *a, const struct sockaddr *b));
/*
 * Compares the address in "a" against "b", returning 0 if they are
 * identical, something else otherwise.
*/

fd_set *
fdsetop(int nfds, const fd_set *a, const fd_set *b, int op);
/*
 * Performs operation on descriptor sets.
 * "nfds" is the number of descriptors to perform "op" on in the sets
 * "a" and "b".
 * "op" is the operation to be performed on the descriptor sets and
 * can take on the value of standard C bitwise operators. 
 * Returns a pointer to the set that is the result of doing "a" "op" "b".
 * The memory used is static.
 * BUGS:
 * 	Only operator currently supported is XOR ('^').
*/

int
methodisset(int method, const char *methodv, size_t methodc);
/*
 * Returns true if the method "method" is set in "methodv", false otherwise.
 * "methodc" is the length of "methodv".
*/

int
socketoptdup(int s);
/*
 * Duplicates the settings of "s" and returns a new socket with the
 * same settings.
 * Returns:
 *		On success: the descriptor for the new socket
 *		On failure: -1
*/

int 
socks_mklock(const char *template);
/*
 * Creates a filedescriptor that can be used with socks_lock() and
 * socks_unlock().
 * Returns:
 *		On success: filedescriptor
 *		On failure: -1
*/

int
socks_lock(int fd, int type, int timeout);
/*
 * Looks the filedescriptor "fd".
 * "type" is the type of lock; F_RDLCK or F_WRLCK.
 * "timeout" is how long to wait for lock, a negative value means forever.
 * Returns:
 *		On success: 0
 *		On error  : -1
*/

int 
socks_unlock(int fd, int timeout);
/*
 * Unlocks the filedescriptor "fd".
 * "timeout" is how long to wait for successful unlock.
 * Returns:
 *		On success: 0
 *		On error  : -1
*/ 


ssize_t
strnlen(const char *s, size_t len);
/*
 * Returns the number of characters that precede the terminating NUL
 * character, or (size_t)-1 if no terminating NUL character is found
 * within "len" characters.
*/

#if defined(DEBUG) || defined(HAVE_SOLARIS_BUGS)

int
freedescriptors __P((const char *message));
/*
 * Returns the number on unallocated descriptors.
*/

#endif /* DEBUG) || HAVE_SOLARIS_BUGS */


#ifdef DEBUG

int
fd_isset __P((int fd, fd_set *fdset));
/* function version of FD_ISSET() */

#endif /* DEBUG */

/* replacements */

#ifndef HAVE_VWARNX
void vwarnx __P((const char *, va_list ap));
#endif  /* !HAVE_VWARNX */

#ifndef HAVE_DAEMON
int daemon __P((int, int));
#endif  /* !HAVE_DAEMON */

#ifndef HAVE_GETDTABLESIZE
/* return upper limit on per-process open file descriptors */
int getdtablesize __P((void));
#endif  /* !HAVE_GETDTABLESIZE */

#ifndef HAVE_SNPRINTF
# ifdef STDC_HEADERS
int snprintf __P((char *, size_t, char const *, ...));
# else
int snprintf ();
# endif /* STDC_HEADERS */
int vsnprintf __P((char *, size_t, const char *, char *));
#endif /* !HAVE_SNPRINTF */

#ifndef HAVE_SETPROCTITLE
#ifdef STDC_HEADERS
void setproctitle(const char *fmt, ...);
#else
void setproctitle();
#endif  /* STDC_HEADERS */
#endif  /* !HAVE_SETPROCTITLE */

#ifndef HAVE_SOCKATMARK
int sockatmark __P((int));
#endif  /* !HAVE_SOCKATMARK */

#ifndef HAVE_ATON
int inet_aton __P((register const char *cp, struct in_addr *addr));
#endif  /* ! HAVE_ATON */

#ifndef HAVE_GETDTABLESIZE
int getdtablesize __P((void));
#endif  /* ! HAVE_GETDTABLESIZE */

#ifndef HAVE_STRERROR
char *__strerror __P((int, char *));
char *strerror __P((int));
const char *hstrerror __P((int));
#endif  /* !HAVE_STRERROR */

#ifndef HAVE_MEMMOVE
void * memmove __P((void *, const void *, register size_t));
#endif  /* !HAVE_MEMMOVE */

#ifndef HAVE_INET_PTON
int inet_pton __P((int af, const char *src, void *dst));
#endif

#ifndef HAVE_ISSETUGID
int issetugid __P((void));
#endif  /* !HAVE_ISSETUGID */


__END_DECLS

/* XXX */
#ifdef SOCKS_CLIENT
#include "socks.h"
#else
#include "sockd.h"
#endif  /* SOCKS_CLIENT */
