/*
 * Copyright (c) 1997, 1998, 1999
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
 *  N-0349 Oslo
 *  Norway
 *
 * any improvements or extensions that they make and grant Inferno Nettverk A/S
 * the rights to redistribute these changes.
 *
 */

#define WE_DONT_WANT_NO_SOCKADDR_ARG_UNION

#include "common.h"

#if SOCKSLIBRARY_DYNAMIC

#include "interposition.h"

static const char rcsid[] =
"$Id: interposition.c,v 1.48 1999/05/13 13:39:20 karls Exp $";

#undef accept
#undef bind
#undef bindresvport
#undef connect
#undef gethostbyname
#undef gethostbyname2
#undef getpeername
#undef getsockname
#undef read
#undef readv
#undef recv
#undef recvfrom
#undef recvmsg
#undef rresvport
#undef send
#undef sendmsg
#undef sendto
#undef write
#undef writev

#if NEED_DYNA_RTLD
#define DL_LAZY RTLD_LAZY
#endif  /* NEED_DYNA_RTLD */

static struct libsymbol_t libsymbolv[] = {
	{	SYMBOL_ACCEPT,						LIBRARY_ACCEPT },
	{	SYMBOL_BIND,						LIBRARY_BIND },
	{	SYMBOL_BINDRESVPORT,				LIBRARY_BINDRESVPORT },
	{	SYMBOL_CONNECT,					LIBRARY_CONNECT },
	{	SYMBOL_GETHOSTBYNAME,			LIBRARY_GETHOSTBYNAME },
	{	SYMBOL_GETHOSTBYNAME2,			LIBRARY_GETHOSTBYNAME2 },
	{	SYMBOL_GETPEERNAME,				LIBRARY_GETPEERNAME },
	{	SYMBOL_GETSOCKNAME,				LIBRARY_GETSOCKNAME },
	{	SYMBOL_READ,						LIBRARY_READ },
	{	SYMBOL_READV,						LIBRARY_READV },
	{	SYMBOL_RECV,						LIBRARY_RECV },
	{	SYMBOL_RECVMSG,					LIBRARY_RECVMSG },
	{	SYMBOL_RECVFROM,					LIBRARY_RECVFROM },
	{	SYMBOL_RRESVPORT,					LIBRARY_RRESVPORT },
	{	SYMBOL_SEND,						LIBRARY_SEND },
	{	SYMBOL_SENDMSG,					LIBRARY_SENDMSG },
	{	SYMBOL_SENDTO,						LIBRARY_SENDTO },
	{	SYMBOL_WRITE,						LIBRARY_WRITE },
	{	SYMBOL_WRITEV,						LIBRARY_WRITEV },
#if HAVE_EXTRA_OSF_SYMBOLS
	{	SYMBOL_EACCEPT,					LIBRARY_EACCEPT },
	{	SYMBOL_EGETPEERNAME,				LIBRARY_EGETPEERNAME },
	{  SYMBOL_EGETSOCKNAME,				LIBRARY_EGETSOCKNAME },
	{	SYMBOL_EREADV,						LIBRARY_EREADV },
	{	SYMBOL_ERECVFROM,					LIBRARY_ERECVFROM },
	{	SYMBOL_ERECVMSG,					LIBRARY_ERECVMSG },
	{  SYMBOL_ESENDMSG,					LIBRARY_ESENDMSG },
	{	SYMBOL_EWRITEV,					LIBRARY_EWRITEV },

	{	SYMBOL_NACCEPT,					LIBRARY_EACCEPT },
	{	SYMBOL_NGETPEERNAME,				LIBRARY_NGETPEERNAME },
	{	SYMBOL_NGETSOCKNAME,				LIBRARY_NGETSOCKNAME },
	{  SYMBOL_NRECVFROM,					LIBRARY_NRECVFROM },
	{  SYMBOL_NRECVMSG,					LIBRARY_NRECVMSG },
	{	SYMBOL_NSENDMSG,					LIBRARY_NSENDMSG },
#endif  /* HAVE_EXTRA_OSF_SYMBOLS */


};

__BEGIN_DECLS

static struct libsymbol_t *
libsymbol __P((const char *symbol));
/*
 * Finds the libsymbol_t that "symbol" is defined in.
*/

__END_DECLS

static struct libsymbol_t *
libsymbol(symbol)
	const char *symbol;
{
	const char *function = "libsymbol()";
	size_t i;

	for (i = 0; i < ELEMENTS(libsymbolv); ++i)
		if (strcmp(libsymbolv[i].symbol, symbol) == 0)
			return &libsymbolv[i];

	serrx(1, "%s: configuration error, can't find symbol %s", function, symbol);
	return NULL; /* please compiler. */
}


void *
symbolfunction(symbol)
	char *symbol;
{
	const char *function = "symbolfunction()";
	struct libsymbol_t *lib;

	lib = libsymbol(symbol);

	SASSERTX(lib != NULL);
	SASSERTX(lib->library != NULL);
	SASSERTX(strcmp(lib->symbol, symbol) == 0);

	if (lib->handle == NULL)
		if ((lib->handle = dlopen(lib->library, DL_LAZY)) == NULL)
			serrx(EXIT_FAILURE, "%s: %s: %s", function, lib->library, dlerror());

	if (lib->function == NULL)
		if ((lib->function = dlsym(lib->handle, symbol)) == NULL)
			serrx(EXIT_FAILURE, "%s: %s: %s", function, symbol, dlerror());

#if 0
	if (strcmp(symbol, SYMBOL_WRITE) != 0)
		slog(LOG_DEBUG, "found symbol %s in library %s\n",
		lib->symbol, lib->library);
#endif

	return lib->function;
}



	/* the real system calls. */

#if !HAVE_EXTRA_OSF_SYMBOLS
int
sys_accept(s, addr, addrlen)
	int s;
	__SOCKADDR_ARG addr;
	socklen_t *addrlen;
{
	int rc;
	int (*function)(int s, __SOCKADDR_ARG addr, socklen_t *addrlen);

	SYSCALL_START(s);
	function = symbolfunction(SYMBOL_ACCEPT);
	rc = function(s, addr, addrlen);
	SYSCALL_END(s);
	return rc;
}
#endif  /* !HAVE_EXTRA_OSF_SYMBOLS */

#if !HAVE_EXTRA_OSF_SYMBOLS
int
sys_bind(s, name, namelen)
	int s;
#if HAVE_FAULTY_BINDPROTO
	struct sockaddr *name;
#else
	__CONST_SOCKADDR_ARG name;
#endif  /* HAVE_FAULTY_BINDPROTO */
	socklen_t namelen;
{
	int rc;
	int (*function)(int s, __CONST_SOCKADDR_ARG name, socklen_t namelen);

	SYSCALL_START(s);
	function = symbolfunction(SYMBOL_BIND);
	rc = function(s, name, namelen);
	SYSCALL_END(s);
	return rc;
}
#endif /* !HAVE_EXTRA_OSF_SYMBOLS */

int
sys_bindresvport(sd, sin)
	int sd;
	struct sockaddr_in *sin;
{
	int rc;
	int (*function)(int sd, struct sockaddr_in *sin);

	SYSCALL_START(sd);
	function = symbolfunction(SYMBOL_BINDRESVPORT);
	rc = function(sd, sin);
	SYSCALL_END(sd);
	return rc;
}

#if !HAVE_EXTRA_OSF_SYMBOLS
int
sys_connect(s, name, namelen)
	int s;
#if HAVE_FAULTY_CONNECTPROTO
	struct sockaddr *name;
#else
	__CONST_SOCKADDR_ARG name;
#endif  /* HAVE_FAULTY_CONNECTPROTO */
	socklen_t namelen;
{
	int rc;
	int (*function)(int s, __CONST_SOCKADDR_ARG name, socklen_t namelen);

	SYSCALL_START(s);
	function = symbolfunction(SYMBOL_CONNECT);
	rc = function(s, name, namelen);
	SYSCALL_END(s);
	return rc;
}
#endif /* !HAVE_EXTRA_OSF_SYMBOLS */

struct hostent *
sys_gethostbyname(name)
	const char *name;
{
	struct hostent *(*function)(const char *name);

	function = symbolfunction(SYMBOL_GETHOSTBYNAME);
	return function(name);
}

struct hostent *
sys_gethostbyname2(name, af)
	const char *name;
	int af;
{
	struct hostent *(*function)(const char *name, int af);

	function = symbolfunction(SYMBOL_GETHOSTBYNAME2);
	return function(name, af);
}

#if !HAVE_EXTRA_OSF_SYMBOLS
int
sys_getpeername(s, name, namelen)
	int s;
	__SOCKADDR_ARG name;
	socklen_t *namelen;
{
	int rc;
	int (*function)(int s, const __SOCKADDR_ARG name, socklen_t *namelen);

	SYSCALL_START(s);
	function = symbolfunction(SYMBOL_GETPEERNAME);
	rc = function(s, name, namelen);
	SYSCALL_END(s);
	return rc;
}
#endif /* ! HAVE_EXTRA_OSF_SYMBOLS */

#if !HAVE_EXTRA_OSF_SYMBOLS
int
sys_getsockname(s, name, namelen)
	int s;
	__SOCKADDR_ARG name;
	socklen_t *namelen;
{
	int rc;
	int (*function)(int s, const __SOCKADDR_ARG name, socklen_t *namelen);

	SYSCALL_START(s);
	function = symbolfunction(SYMBOL_GETSOCKNAME);
	rc = function(s, name, namelen);
	SYSCALL_END(s);
	return rc;
}
#endif /* HAVE_FAULTY_CONNECTPROTO */

ssize_t
sys_read(d, buf, nbytes)
	int d;
	void *buf;
	size_t nbytes;
{
	ssize_t rc;
	int (*function)(int d, void *buf, size_t nbutes);

	SYSCALL_START(d);
	function = symbolfunction(SYMBOL_READ);
	rc = function(d, buf, nbytes);
	SYSCALL_END(d);
	return rc;
}

#if !HAVE_EXTRA_OSF_SYMBOLS
ssize_t
sys_readv(d, iov, iovcnt)
	int d;
#if HAVE_FAULTY_READVPROTO
	struct iovec *iov;
#else
	const struct iovec *iov;
#endif
	int iovcnt;
{
	ssize_t rc;
	int (*function)(int d, const struct iovec *iov, int iovcnt);

	SYSCALL_START(d);
	function = symbolfunction(SYMBOL_READV);
	rc = function(d, iov, iovcnt);
	SYSCALL_END(d);
	return rc;
}
#endif /* HAVE_EXTRA_OSF_SYMBOLS */

ssize_t
sys_recv(s, buf, len, flags)
	int s;
/* XXX rename */
#if HAVE_RECVFROM_CHAR
	char *buf;
	int len;
#else
	void *buf;
	size_t len;
#endif
	int flags;
{
	ssize_t rc;
	int (*function)(int s, void *buf, size_t len, int flags);

	SYSCALL_START(s);
	function = symbolfunction(SYMBOL_RECV);
	rc = function(s, buf, len, flags);
	SYSCALL_END(s);
	return rc;
}

#if !HAVE_EXTRA_OSF_SYMBOLS
int
sys_recvfrom(s, buf, len, flags, from, fromlen)
	int s;
#if HAVE_RECVFROM_CHAR
	char *buf;
	int len;
#else
	void *buf;
	size_t len;
#endif  /* HAVE_RECVFROM_CHAR */
	int flags;
#if HAVE_RECVFROM_CHAR
	struct sockaddr *from;
	int *fromlen;
#else
	__SOCKADDR_ARG from;
	socklen_t *fromlen;
#endif  /* HAVE_RECVFROM_CHAR */
{
	int rc;
	int (*function)(int s, void *buf, size_t len, int flags,
					    __SOCKADDR_ARG from, socklen_t *fromlen);

	SYSCALL_START(s);
	function = symbolfunction(SYMBOL_RECVFROM);
	rc = function(s, buf, len, flags, from, fromlen);
	SYSCALL_END(s);
	return rc;
}
#endif /* HAVE_EXTRA_OSF_SYMBOLS */

#if !HAVE_EXTRA_OSF_SYMBOLS
ssize_t
sys_recvmsg(s, msg, flags)
	int s;
	struct msghdr *msg;
	int flags;
{
	ssize_t rc;
	int (*function)(int s, struct msghdr *msg, int flags);

	SYSCALL_START(s);
	function = symbolfunction(SYMBOL_RECVMSG);
	rc = function(s, msg, flags);
	SYSCALL_END(s);
	return rc;
}
#endif /* HAVE_EXTRA_OSF_SYMBOLS */

int
sys_rresvport(port)
	int *port;
{
	int (*function)(int *port);

	function = symbolfunction(SYMBOL_RRESVPORT);
	return function(port);
}

ssize_t
sys_send(s, msg, len, flags)
	int s;
#if HAVE_RECVFROM_CHAR
	const char *msg;
	int len;
#else
	const void *msg;
	size_t len;
#endif
	int flags;
{
	ssize_t rc;
	int (*function)(int s, const void *msg, size_t len, int flags);

	SYSCALL_START(s);
	function = symbolfunction(SYMBOL_SEND);
	rc = function(s, msg, len, flags);
	SYSCALL_END(s);
	return rc;
}

#if !HAVE_EXTRA_OSF_SYMBOLS
ssize_t
sys_sendmsg(s, msg, flags)
	int s;
	const struct msghdr *msg;
	int flags;
{
	ssize_t rc;
	int (*function)(int s, const struct msghdr *msg, int flags);

	SYSCALL_START(s);
	function = symbolfunction(SYMBOL_SENDMSG);
	rc = function(s, msg, flags);
	SYSCALL_END(s);
	return rc;
}
#endif /* HAVE_EXTRA_OSF_SYMBOLS */

#if !HAVE_EXTRA_OSF_SYMBOLS
ssize_t
sys_sendto(s, msg, len, flags, to, tolen)
	int s;
#if HAVE_SENDTO_ALT
	const char *msg;
	int len;
#else
	const void *msg;
	size_t len;
#endif  /* HAVE_SENDTO_ALT */
	int flags;
	__CONST_SOCKADDR_ARG to;
	socklen_t tolen;
{
	ssize_t rc;
	int (*function)(int s, const void *msg, size_t len, int flags,
					    __CONST_SOCKADDR_ARG to, socklen_t tolen);

	SYSCALL_START(s);
	function = symbolfunction(SYMBOL_SENDTO);
	rc = function(s, msg, len, flags, to, tolen);
	SYSCALL_END(s);
	return rc;
}
#endif /* !HAVE_EXTRA_OSF_SYMBOLS */

ssize_t
sys_write(d, buf, nbytes)
	int d;
	const void *buf;
	size_t nbytes;
{
	ssize_t rc;
	int (*function)(int d, const void *buf, size_t nbutes);

	SYSCALL_START(d);
	function = symbolfunction(SYMBOL_WRITE);
	rc = function(d, buf, nbytes);
	SYSCALL_END(d);
	return rc;
}

#if !HAVE_EXTRA_OSF_SYMBOLS
ssize_t
sys_writev(d, iov, iovcnt)
	int d;
	const struct iovec *iov;
	int iovcnt;
{
	ssize_t rc;
	int (*function)(int d, const struct iovec *buf, int iovcnt);

	SYSCALL_START(d);
	function = symbolfunction(SYMBOL_WRITEV);
	rc = function(d, iov, iovcnt);
	SYSCALL_END(d);
	return rc;
}
#endif /* HAVE_EXTRA_OSF_SYMBOLS */

	/*
	 * the interpositioned functions.
	*/


#if !HAVE_EXTRA_OSF_SYMBOLS
int
accept(s, addr, addrlen)
	int s;
	__SOCKADDR_ARG addr;
	socklen_t *addrlen;
{
	if (ISSYSCALL(s))
		return sys_accept(s, addr, addrlen);
	return Raccept(s, addr, addrlen);
}
#endif /* HAVE_EXTRA_OSF_SYMBOLS */

int
bind(s, name, namelen)
	int s;
#if HAVE_FAULTY_BINDPROTO
	struct sockaddr *name;
#else
	__CONST_SOCKADDR_ARG name;
#endif  /* HAVE_FAULTY_BINDPROTO */
	socklen_t namelen;
{
	if (ISSYSCALL(s))
		return sys_bind(s, name, namelen);
	return Rbind(s, name, namelen);
}

int
bindresvport(sd, sin)
	int sd;
	struct sockaddr_in *sin;
{
	if (ISSYSCALL(sd))
		return sys_bindresvport(sd, sin);
	return Rbindresvport(sd, sin);
}

#if !HAVE_EXTRA_OSF_SYMBOLS
int
connect(s, name, namelen)
	int s;
#if HAVE_FAULTY_CONNECTPROTO
	struct sockaddr *name;
#else
	__CONST_SOCKADDR_ARG name;
#endif  /* HAVE_FAULTY_CONNECTPROTO */
	socklen_t namelen;
{
	if (ISSYSCALL(s))
		return sys_connect(s, name, namelen);
	return Rconnect(s, name, namelen);
}
#endif /* HAVE_EXTRA_OSF_SYMBOLS */

struct hostent *
gethostbyname(name)
	const char *name;
{
	return Rgethostbyname(name);
}

struct hostent *
gethostbyname2(name, af)
	const char *name;
	int af;
{
	return Rgethostbyname2(name, af);
}


#if !HAVE_EXTRA_OSF_SYMBOLS
int
getpeername(s, name, namelen)
	int s;
	__SOCKADDR_ARG name;
	socklen_t *namelen;
{
	if (ISSYSCALL(s))
		return sys_getpeername(s, name, namelen);
	return Rgetpeername(s, name, namelen);
}
#endif /* HAVE_EXTRA_OSF_SYMBOLS */


#if !HAVE_EXTRA_OSF_SYMBOLS
int
getsockname(s, name, namelen)
	int s;
	__SOCKADDR_ARG name;
	socklen_t *namelen;
{
	if (ISSYSCALL(s))
		return sys_getpeername(s, name, namelen);
	return Rgetsockname(s, name, namelen);
}
#endif /* HAVE_EXTRA_OSF_SYMBOLS */

ssize_t
read(d, buf, nbytes)
	int d;
	void *buf;
	size_t nbytes;
{
	if (ISSYSCALL(d))
		return sys_read(d, buf, nbytes);
	return Rread(d, buf, nbytes);
}

#if !HAVE_EXTRA_OSF_SYMBOLS
ssize_t
readv(d, iov, iovcnt)
	int d;
#if HAVE_FAULTY_READVPROTO
	struct iovec *iov;
#else
	const struct iovec *iov;
#endif  /* HAVE_FAULTY_READVPROTO */
	int iovcnt;
{
	if (ISSYSCALL(d))
		return sys_readv(d, iov, iovcnt);
	return Rreadv(d, iov, iovcnt);
}
#endif /* HAVE_EXTRA_OSF_SYMBOLS */

ssize_t
recv(s, msg, len, flags)
	int s;
#if HAVE_RECVFROM_CHAR
	char *msg;
#else
	void *msg;
#endif
#if HAVE_RECVFROM_CHAR || HAVE_RECV_LEN_INT
	int len;
#else
	size_t len;
#endif
	int flags;
{
	if (ISSYSCALL(s))
		return sys_recv(s, msg, len, flags);
	return Rrecv(s, msg, len, flags);
}

#if !HAVE_EXTRA_OSF_SYMBOLS
#ifdef HAVE_DEC_PROTO
ssize_t
#else
int
#endif
recvfrom(s, buf, len, flags, from, fromlen)
	int s;
#if HAVE_RECVFROM_CHAR
	char *buf;
#else
	void *buf;
#endif  /* HAVE_RECVFROM_CHAR */
#if HAVE_RECVFROM_CHAR || HAVE_RECV_LEN_INT
	int len;
#else
	size_t len;
#endif
	int flags;
	__SOCKADDR_ARG from;
	socklen_t *fromlen;
{
	if (ISSYSCALL(s))
		return sys_recvfrom(s, buf, len, flags, from, fromlen);
	return Rrecvfrom(s, buf, len, flags, from, fromlen);
}
#endif /* HAVE_EXTRA_OSF_SYMBOLS */

#if !HAVE_EXTRA_OSF_SYMBOLS
ssize_t
recvmsg(s, msg, flags)
	int s;
	struct msghdr *msg;
	int flags;
{
	if (ISSYSCALL(s))
		return sys_recvmsg(s, msg, flags);
	return Rrecvmsg(s, msg, flags);
}
#endif /* HAVE_EXTRA_OSF_SYMBOLS */

int
rresvport(port)
	int *port;
{
	return Rrresvport(port);
}

ssize_t
write(d, buf, nbytes)
	int d;
	const void *buf;
	size_t nbytes;
{
	if (ISSYSCALL(d))
		return sys_write(d, buf, nbytes);
	return Rwrite(d, buf, nbytes);
}

#if !HAVE_EXTRA_OSF_SYMBOLS
ssize_t
writev(d, iov, iovcnt)
	int d;
	const struct iovec *iov;
	int iovcnt;
{
	if (ISSYSCALL(d))
		return sys_writev(d, iov, iovcnt);
	return Rwritev(d, iov, iovcnt);
}
#endif /* HAVE_EXTRA_OSF_SYMBOLS */

ssize_t
send(s, msg, len, flags)
	int s;
#if HAVE_RECVFROM_CHAR
	const char *msg;
#else
	const void *msg;
#endif  /* HAVE_RECVFROM_CHAR */
#if HAVE_RECVFROM_CHAR || HAVE_RECV_LEN_INT
	int len;
#else
	size_t len;
#endif
	int flags;
{
	if (ISSYSCALL(s))
		return sys_send(s, msg, len, flags);
	return Rsend(s, msg, len, flags);
}

#if !HAVE_EXTRA_OSF_SYMBOLS
ssize_t
sendmsg(s, msg, flags)
	int s;
	const struct msghdr *msg;
	int flags;
{
	if (ISSYSCALL(s))
		return sys_sendmsg(s, msg, flags);
	return Rsendmsg(s, msg, flags);
}
#endif /* HAVE_EXTRA_OSF_SYMBOLS */

#if !HAVE_EXTRA_OSF_SYMBOLS
ssize_t
sendto(s, msg, len, flags, to, tolen)
	int s;
#if HAVE_SENDTO_ALT
	const char *msg;
#else
	const void *msg;
#endif  /* HAVE_SENDTO_ALT */
#if HAVE_SENDTO_ALT || HAVE_RECV_LEN_INT
	int len;
#else
	size_t len;
#endif
	int flags;
	__CONST_SOCKADDR_ARG to;
	socklen_t tolen;
{
	if (ISSYSCALL(s))
		return sys_sendto(s, msg, len, flags, to, tolen);
	return Rsendto(s, msg, len, flags, to, tolen);
}
#endif /* !HAVE_EXTRA_OSF_SYMBOLS */


#endif /* SOCKSLIBRARY_DYNAMIC */
