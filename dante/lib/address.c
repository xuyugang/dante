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

static const char rcsid[] =
"$Id: address.c,v 1.56 1999/02/26 21:29:25 karls Exp $";

#include "common.h"

static struct socksfd_t socksfdinit;
static int *dv;
static unsigned int dc;
static struct socksfd_t *socksfdv;
static unsigned int socksfdc;

struct socksfd_t *
socks_addaddr(clientfd, socksfd)
	unsigned int clientfd;
	struct socksfd_t *socksfd;
{
	const char *function = "socks_addaddr()";

#if 0 /* DEBUG */
	if (socksfd->state.command != -1 && !socksfd->state.system)
		slog(LOG_DEBUG, "%s: %d", function, clientfd);
#endif

	SASSERTX(socksfd->state.command 	== -1
	|| 	 socksfd->state.command 	== SOCKS_BIND
	|| 	 socksfd->state.command 	== SOCKS_CONNECT
	|| 	 socksfd->state.command 	== SOCKS_UDPASSOCIATE);

	if (socks_addfd(clientfd) != 0)
		serrx(1, NOMEM);

	if (socksfdc < dc) { /* init/reallocate */
		if (socksfdinit.control == 0) {	/* not initialized */
			socksfdinit.control = -1;
			/* other elements have ok default value. */
		}

		if ((socksfdv = (struct socksfd_t *)realloc(socksfdv,
		sizeof(*socksfdv) * dc)) == NULL)
			serrx(1, NOMEM);

		/* init new objects */
		while (socksfdc < dc)
			socksfdv[socksfdc++] = socksfdinit;
	}

	switch (socksfd->state.command) {
		case SOCKS_BIND:
#ifdef SOCKS_TRYHARDER
			if ((socksfd->state.lock = socks_mklock(SOCKS_LOCKFILE)) == -1)
				swarn("socks_mklock()");
#endif	
			break;
	}

	socksfdv[clientfd] = *socksfd;
	socksfdv[clientfd].allocated = 1;

	return &socksfdv[clientfd];
}


struct socksfd_t *
socks_getaddr(d)
	unsigned int d;
{
	if (!socks_isaddr(d))
		return NULL;

	return &socksfdv[d];
}

void
socks_rmaddr(d)
	unsigned int d;
{
	const char *function = "socks_rmaddr()";

#if 0 /* DEBUG */
	if (!socks_isaddr(d)
	|| (!socksfdv[d].state.command != -1 && !socksfdv[d].state.system))
		slog(LOG_DEBUG, "%s: %d", function, d);
#endif


	if (!socks_isaddr(d))
		return;

	socks_rmfd(d);

	switch (socksfdv[d].state.version) {
		case MSPROXY_V2:
			if (socksfdv[d].control != -1)
				close(socksfdv[d].control);
			break;

		case SOCKS_V4:
		case SOCKS_V5:
			if (!socksfdv[d].state.system)
				switch (socksfdv[d].state.command) {
					case SOCKS_BIND:
						if (d != socksfdv[d].control && socksfdv[d].control != -1)
							close(socksfdv[d].control);
						break;

					case SOCKS_CONNECT:
						break;

					case SOCKS_UDPASSOCIATE:
						if (socksfdv[d].control != -1)
							close(socksfdv[d].control);
						break;

					default:
						SERRX(socksfdv[d].state.command);
				}

			switch (socksfdv[d].state.command) {
				case SOCKS_BIND:
#ifdef SOCKS_TRYHARDER
					if (close(socksfdv[d].state.lock) != 0)
						swarn("socks_rmaddr()");
#endif	
					break;
			}
	}

	socksfdv[d] = socksfdinit;
}

int
socks_isaddr(d)
	unsigned int d;
{
	if (d < socksfdc && socksfdv[d].allocated)
		return 1;
	else
		return 0;
}

int
socks_addrisok(s)
	unsigned int s;
{
	const char *function = "socks_addrisok()";
	int matched;
	sigset_t newmask, oldmask;

	/*
	 * block signals that might change socksfd.
	*/
	sigemptyset(&newmask);
	sigaddset(&newmask, SIGIO);
	sigaddset(&newmask, SIGCHLD);
	if (sigprocmask(SIG_BLOCK, &newmask, &oldmask) != 0) {
		swarn("%s: sigprocmask()", function);
		return -1;
	}

	matched = 1;
	do {
		struct socksfd_t *socksfd;
		struct sockaddr local;
		socklen_t locallen;

		locallen = sizeof(local);
		if (getsockname((int)s, &local, &locallen) != 0) {
				matched = 0;
				break;
		}

		socksfd = socks_getaddr(s);

		if (socksfd != NULL)
			if (sockaddrcmp(&local, &socksfd->local) != 0) {
				matched = 0;
				break;
			}

		/* check remote endpoint too? */
		
		if (socksfd == NULL) {	/* unknown descriptor, a dup? */
			struct socksfd_t newsocksfd;
			int duped;

			if ((duped = socks_addrmatch(&local, NULL, NULL)) >= 0) {
				socksfd
				= socksfddup(socks_getaddr((unsigned int)duped), &newsocksfd);

				if (socksfd == NULL) {
					matched = 0;
					break;
				}

				socks_addaddr(s, socksfd);
				break;
			}
			else {
				matched = 0;
				break;
			}
		}
	/* CONSTCOND */
	} while (0);

	if (sigprocmask(SIG_SETMASK, &oldmask, NULL) != 0) {
		swarn("%s: sigprocmask()", function);
		return -1;
	}

	return matched;
}

int 
socks_addrcontrol(local, remote)
	const struct sockaddr *local;
	const struct sockaddr *remote;
{
	int i;
	
	for (i = 0; i < socksfdc; ++i) {
		struct sockaddr localcontrol, remotecontrol;
		socklen_t len;

		if (!socks_isaddr((unsigned int)i))
			continue;
		
		if (local != NULL) {
			len = sizeof(localcontrol);
			if (getsockname(socksfdv[i].control, &localcontrol, &len) != 0)
				continue;

			if (sockaddrcmp(local, &localcontrol) != 0)
				continue;
		}

		if (remote != NULL) {
			len = sizeof(remotecontrol);
			if (getpeername(socksfdv[i].control, &remotecontrol, &len) != 0)
				continue;

			if (sockaddrcmp(remote, &remotecontrol) != 0)
				continue;
		}

		return i;
	}

	return -1;
}

int
socks_addrmatch(local, remote, state)
	const struct sockaddr *local;
	const struct sockaddr *remote;
	const struct socksstate_t *state;
{
	int i;

	for (i = 0; i < socksfdc; ++i) {
		if (!socks_isaddr((unsigned int)i))
			continue;

		/*
		 * only compare fields that have a valid value in request to compare
		 * against.
		*/

		if (local != NULL)
			if (sockaddrcmp(local, &socksfdv[i].local) != 0)
				continue;

		if (remote != NULL)
			if (sockaddrcmp(remote, &socksfdv[i].remote) != 0)
				continue;

		if (state != NULL) {
			if (state->version != -1)
				if (state->version != socksfdv[i].state.version)
					continue;

			if (state->command != -1)
				if (state->command != socksfdv[i].state.command)
					continue;
				
			if (state->inprogress != -1)
				if (state->inprogress != socksfdv[i].state.inprogress)
					continue;
			
			if (state->acceptpending != -1)
				if (state->acceptpending != socksfdv[i].state.acceptpending)
					continue;
		}

		return i;
	}

	return -1;
}


int
socks_addfd(d)
	unsigned int d;
{

	if (d >= dc)	{ /* init/reallocate */
		int *newfdv, newfdc;

		newfdc = MAX(d + 1, getdtablesize());
		if ((newfdv = (int *)realloc(dv, sizeof(*dv) * newfdc)) == NULL)
			return -1;
		dv = newfdv;

		/* init all to -1, a illegal value for a d. */
		while (dc < newfdc)
			dv[dc++] = -1;
	}
	
	dv[d] = d;

	return 0;
}

int
socks_isfd(d)
	unsigned int d;
{
	if (d >= dc || dv[d] == -1)	
		return 0;
	return 1;
}

void
socks_rmfd(d)
	unsigned int d;
{
	if (socks_isfd(d))
		dv[d] = -1;
}

struct socksfd_t *
socksfddup(old, new)
	const struct socksfd_t *old;
	struct socksfd_t *new;
{
	
	*new = *old;	/* init most stuff. */

	switch (old->state.command) {
		case SOCKS_BIND:
		case SOCKS_UDPASSOCIATE:
			if ((new->control = socketoptdup(old->control)) == -1)
				return NULL;
			break;

		case SOCKS_CONNECT:
			/* only descriptor for connect is the one client has. */
			break;

		default:
			SERRX(old->state.command);
	}

	return new;
}
