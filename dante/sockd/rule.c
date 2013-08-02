/*
 * Copyright (c) 2010, 2011, 2012
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
 *  Gaustadalléen 21
 *  NO-0349 Oslo
 *  Norway
 *
 * any improvements or extensions that they make and grant Inferno Nettverk A/S
 * the rights to redistribute these changes.
 *
 */

#include "common.h"
#include "config_parse.h"

static const char rcsid[] =
"$Id: rule.c,v 1.321 2013/08/01 11:53:18 michaels Exp $";

#if HAVE_LIBWRAP
extern jmp_buf tcpd_buf;
int allow_severity, deny_severity;

static void
libwrapinit(int s, struct sockaddr_storage *local,
            struct sockaddr_storage *peer, struct request_info *request);
/*
 * Initializes "request" for later usage via libwrap.
 * "s" is the socket the connection from "peer" was accepted on, with
 * the local address of "s" being "local".
 */

static int
libwrap_hosts_access(struct request_info *request, 
                     const struct sockaddr_storage *peer);
/*
 * Perform libwrap hosts_access() check on the client "peer".
 */
#endif /* !HAVE_LIBWRAP */

static int
srchost_isok(const struct sockaddr_storage *peer, char *msg, size_t msgsize);
/*
 * Checks whether the connection/packet from "peer" is ok, according
 * to srchost-settings.  If the connection/packet is not ok, "msg" is filled
 * in with the reason why not.
 *
 * This function should be called after each rule check for a new
 * connection/packet.
 *
 * Returns:
 *      If connection is acceptable: true
 *      If connection is not acceptable: false
 */

static void
showlog(const log_t *log);
/*
 * shows what type of logging is specified in "log".
 */

static rule_t *
addrule(const rule_t *newrule, rule_t **rulebase, const objecttype_t ruletype);
/*
 * Appends a copy of "newrule" to "rulebase", setting sensible defaults where
 * appropriate.
 *
 * Returns a pointer to the added rule (not "newrule").
 */

static void
checkrule(const rule_t *rule);
/*
 * Check that the rule "rule" makes sense.
 */

static void
log_shmeminherit(const rule_t *from, const rule_t *to, const int type,
                 const int isoverride);
/*
 * Logs that shemem settings are inherited by rule "to" from rule "from",
 * or that rule "to" overrides settings from rule "from", depending on
 * whether isoverride is set or not.
 * "type" must be the SHMEM_ type in question.
 */

static void
log_inheritable(const char *prefix, const rule_t *rule,
                const unsigned long shmid, const char *settings,
                const int inheritable);
/*
 * logs whether the "settings" settings belonging to shmid "shmid" in
 * rule "rule" are inheritable or not, according to "inheritable".
 */


void
addclientrule(newrule)
   const rule_t *newrule;
{
   const char *function = "addclientrule()";
   rule_t *rule, ruletoadd;

   ruletoadd = *newrule; /* for const. */

   rule = addrule(&ruletoadd, &sockscf.crule, object_crule);

   checkrule(rule);

#if BAREFOOTD
   if (rule->state.protocol.udp) {
      /*
       * Only one level of acls, so we need to autogenerate the second level
       * ourselves as the first acl level can only handle tcp, but we
       * need to check each udp packet against rulespermit(), and for that,
       * a second level acl (socks-rule) is needed.
       *
       * In the tcp-case, we don't need any socks-rules, the client-rule
       * is enough as the endpoints get fixed at session-establishment
       * and we can just short-circuit the process as we knows the
       * session is allowed if it gets past the client-rule state.
       *
       * For udp, thins are similar, but on the reply-side things can
       * vary if sockscf.udpconnectdst is not set.  If it is not set,
       * it means we should not connect to the target, and that means
       * we will be able to receive reply-packets from addresses other
       * than the target address.  We want the client to be able to do 
       * that, if the Barefoot admin has explicitly allowed it by disabling 
       * sockscf.udpconnectdst.  That however also means that there is
       * no way to configure things so replies from only some targets
       * are allowed; if sockscf.udpconnectdst is not set, replies from
       * all targets will be allowed.  
       * The conclusion is thus that we do not need to generate any 
       * socks-rules for udp either; if the client rule permitted things,
       * any corresponding socks-rule would too.
       */
      struct sockaddr_storage sa;

      /*
       * so we know there may be udp traffic to bounce (may, since this may
       * be a sighup and the same rule being reloaded).
       */
      sockscf.state.alludpbounced = 0;

      if (addrindex_on_listenlist(sockscf.internal.addrc,
                                  sockscf.internal.addrv,
                                  ruleaddr2sockaddr(&rule->dst, &sa, SOCKS_UDP),
                                  SOCKS_UDP) == -1) {
         /*
          * add address to internal list also; need to listen for packets
          * from udp clients.
          */
         addinternal(&rule->dst, SOCKS_UDP);
      }
      else {
         slog(LOG_DEBUG,
              "%s: not adding address %s from rule #%lu to internal list; "
              "address already there",
              function,
              sockaddr2string(&sa, NULL, 0),
              (unsigned long)rule->number);

         rule->bounced = 1;
      }
   }
#endif /* BAREFOOTD */
}

#if HAVE_SOCKS_HOSTID
void
addhostidrule(newrule)
   const rule_t *newrule;
{
/*   const char *function = "addhostidrule()"; */
   rule_t *rule, ruletoadd;

   ruletoadd = *newrule; /* for const. */
   rule      = addrule(&ruletoadd, &sockscf.hrule, object_hrule);
   checkrule(rule);
}
#endif /* HAVE_SOCKS_HOSTID */


void
addsocksrule(newrule)
   const rule_t *newrule;
{
   rule_t *rule;

   rule = addrule(newrule, &sockscf.srule, object_srule);
   checkrule(rule);
}

linkedname_t *
addlinkedname(linkedname, name)
   linkedname_t **linkedname;
   const char *name;
{
   linkedname_t *user, *last;

   for (user = *linkedname, last = NULL; user != NULL; user = user->next)
      last = user;

   if ((user = malloc(sizeof(*user))) == NULL)
      return NULL;

   if ((user->name = strdup(name)) == NULL) {
      free(user);
      return NULL;
   }

   user->next = NULL;

   if (last == NULL)
      *linkedname = user;
   else
      last->next = user;

   return *linkedname;
}

void
freelinkedname(list)
   linkedname_t *list;
{

   while (list != NULL) {
      linkedname_t *next = list->next;

      free(list->name);
      free(list);

      list = next;
   }
}

void
showrule(_rule, ruletype)
   const rule_t *_rule;
   const objecttype_t ruletype;
{
   rule_t rule = *_rule; /* shmat()/shmdt() changes rule. */
   size_t bufused, i;
   char buf[256];

   slog(LOG_DEBUG, "%s-rule #%lu, line #%lu",
        objecttype2string(ruletype),
        (unsigned long)rule.number,
        (unsigned long)rule.linenumber);

   slog(LOG_DEBUG, "verdict: %s", verdict2string(rule.verdict));

   slog(LOG_DEBUG, "src: %s",
        ruleaddr2string(&rule.src, ADDRINFO_PORT | ADDRINFO_ATYPE, NULL, 0));

   slog(LOG_DEBUG, "dst: %s",
        ruleaddr2string(&rule.dst, ADDRINFO_PORT | ADDRINFO_ATYPE, NULL, 0));

#if HAVE_SOCKS_HOSTID
   if (rule.hostid.atype != SOCKS_ADDR_NOTSET)
      slog(LOG_DEBUG, "hostindex: %d, hostid: %s",
           rule.hostindex, ruleaddr2string(&rule.hostid, 0, NULL, 0));
#endif /* HAVE_SOCKS_HOSTID */

   for (i = 0; i < rule.socketoptionc; ++i)
      slog(LOG_DEBUG, "socketoption %s (%s side)",
           sockopt2string(&rule.socketoptionv[i], NULL, 0),
           rule.socketoptionv[i].isinternalside ?  "internal" : "external");

   switch (ruletype) {
      case object_crule:
#if BAREFOOTD
         slog(LOG_DEBUG, "bounce to: %s",
              ruleaddr2string(&rule.extra.bounceto, 
                               ADDRINFO_PORT | ADDRINFO_ATYPE,
                               NULL, 
                               0));
#endif /* BAREFOOTD */
         break;

#if HAVE_SOCKS_HOSTID
      case object_hrule:
         SASSERTX(rule.hostidoption_isset);
         slog(LOG_DEBUG, "hostindex: %d", rule.hostindex);
         break;
#endif /* HAVE_SOCKS_HOSTID */

#if HAVE_SOCKS_RULES
      case object_srule:
         break;
#endif /* HAVE_SOCKS_RULES */

      default:
         SERRX(ruletype);
   }

   /* only show if timeout differs from default. */
   if (memcmp(&rule.timeout, &sockscf.timeout, sizeof(rule.timeout)) != 0)
      showtimeout(&rule.timeout);

   if (rule.udprange.op == range)
      slog(LOG_DEBUG, "udp port range: %u - %u",
           ntohs(rule.udprange.start), ntohs(rule.udprange.end));

   if (rule.rdr_from.atype != SOCKS_ADDR_NOTSET)
      slog(LOG_DEBUG, "redirect from: %s",
           ruleaddr2string(&rule.rdr_from,
                           ADDRINFO_PORT | ADDRINFO_ATYPE,
                           NULL,
                           0));

   if (rule.rdr_to.atype != SOCKS_ADDR_NOTSET)
      slog(LOG_DEBUG, "redirect to: %s",
      ruleaddr2string(&rule.rdr_to, ADDRINFO_PORT | ADDRINFO_ATYPE, NULL, 0));

   sockd_shmat(&rule, SHMEM_ALL);
   if (rule.bw_shmid != 0) {
      SASSERTX(rule.bw != NULL);

      *buf    = NUL;
      bufused = 0;

      bufused += snprintf(&buf[bufused], sizeof(buf) - bufused,
                          "bw_shmid: %lu\n", (unsigned long)rule.bw_shmid);

      bufused += snprintf(&buf[bufused], sizeof(buf) - bufused,
                          "bandwidth limits inheritable: %s\n",
                           rule.bw_isinheritable ? "yes" : "no");

      bufused += snprintf(&buf[bufused], sizeof(buf) - bufused,
                          "max bandwidth allowed: %lu bytes/s\n",
                          (unsigned long)rule.bw->object.bw.maxbps);

      SASSERTX(bufused > 0);
      slog(LOG_DEBUG, "%s", buf);
   }

   if (rule.ss_shmid != 0) {
      SASSERT(rule.ss != NULL);

      *buf    = NUL;
      bufused = 0;

      bufused += snprintf(&buf[bufused], sizeof(buf) - bufused,
                          "ss_shmid: %lu\n", (unsigned long)rule.ss_shmid);

      bufused += snprintf(&buf[bufused], sizeof(buf) - bufused,
                          "sessions limits inheritable: %s\n",
                           rule.ss_isinheritable ? "yes" : "no");

      if (rule.ss->object.ss.max_isset)
         bufused += snprintf(&buf[bufused], sizeof(buf) - bufused,
                             "session.max: %lu\n",
                             (unsigned long)rule.ss->object.ss.max);

      if (rule.ss->object.ss.throttle_isset)
         bufused += snprintf(&buf[bufused], sizeof(buf) - bufused,
                            "session.throttle: %lu client%s per %lds\n",
                      (unsigned long)rule.ss->object.ss.throttle.limit.clients,
                      rule.ss->object.ss.throttle.limit.clients == 1 ? "" : "s",
                      (long)rule.ss->object.ss.throttle.limit.seconds);

      if (rule.ss->keystate.key != key_unset) {
         bufused += snprintf(&buf[bufused], sizeof(buf) - bufused,
                             "session.state.key: %s\n",
                             statekey2string(rule.ss->keystate.key));

         if (rule.ss->keystate.key == key_hostid) {
            SASSERTX(rule.hostidoption_isset);
            bufused += snprintf(&buf[bufused], sizeof(buf) - bufused,
                                "session.state.hostindex: %d\n",
                                rule.ss->keystate.keyinfo.hostindex);
         }

         if (rule.ss->object.ss.max_isset)
            bufused +=
            snprintf(&buf[bufused], sizeof(buf) - bufused,
                     "session.state.max : %lu\n",
                     (unsigned long)rule.ss->object.ss.max);

         if (rule.ss->object.ss.throttle_perstate_isset)
            bufused +=
            snprintf(&buf[bufused], sizeof(buf) - bufused,
                     "session.state.throttle: %lu clients per %lds\n",
                     (unsigned long)rule.ss->object.ss.throttle.limit.clients,
                     (long)rule.ss->object.ss.throttle.limit.seconds);
      }

      SASSERTX(bufused > 0);
      slog(LOG_DEBUG, "%s", buf);
   }
   sockd_shmdt(&rule, SHMEM_ALL);

   showlist(rule.user, "user: ");
   showlist(rule.group, "group: ");

#if HAVE_PAM
   slog(LOG_DEBUG, "pam.servicename: %s", rule.state.pamservicename);
#endif /* HAVE_PAM */

#if HAVE_BSDAUTH
   slog(LOG_DEBUG, "bsdauth.stylename: %s", rule.state.bsdauthstylename);
#endif /* HAVE_BSDAUTH */

#if HAVE_LDAP
   showlist(rule.ldapgroup, "ldap.group: ");
   if (rule.ldapgroup) {
      if (*rule.state.ldap.domain != NUL)
         slog(LOG_DEBUG, "ldap.domain: %s",rule.state.ldap.domain);

      slog(LOG_DEBUG, "ldap.auto.off: %s",
      rule.state.ldap.auto_off ? "yes" : "no");
#if HAVE_OPENLDAP
      slog(LOG_DEBUG, "ldap.debug: %d", rule.state.ldap.debug);
#endif
      slog(LOG_DEBUG, "ldap.keeprealm: %s",
      rule.state.ldap.keeprealm ? "yes" : "no");

      if (*rule.state.ldap.keytab != NUL)
         slog(LOG_DEBUG, "ldap.keytab: %s", rule.state.ldap.keytab);

      showlist(rule.state.ldap.ldapurl, "ldap.url: ");

      showlist(rule.state.ldap.ldapbasedn, "ldap.basedn: ");

      if (*rule.state.ldap.filter != NUL)
         slog(LOG_DEBUG, "ldap.filter: %s", rule.state.ldap.filter);

      if (*rule.state.ldap.filter_AD != NUL)
         slog(LOG_DEBUG, "ldap.filter.ad: %s", rule.state.ldap.filter_AD);

      if (*rule.state.ldap.attribute != NUL)
         slog(LOG_DEBUG, "ldap.attribute: %s", rule.state.ldap.attribute);

      if (*rule.state.ldap.attribute_AD != NUL)
         slog(LOG_DEBUG, "ldap.attribute.ad: %s",
         rule.state.ldap.attribute_AD);

      slog(LOG_DEBUG, "ldap.mdepth: %d", rule.state.ldap.mdepth);
      slog(LOG_DEBUG, "ldap.port: %d", rule.state.ldap.port);
      slog(LOG_DEBUG, "ldap.ssl: %s", rule.state.ldap.ssl ? "yes" : "no");
      slog(LOG_DEBUG, "ldap.certcheck: %s",
           rule.state.ldap.certcheck ? "yes" : "no");

      if (*rule.state.ldap.certfile != NUL)
         slog(LOG_DEBUG, "ldap.certfile: %s", rule.state.ldap.certfile);

      if (*rule.state.ldap.certpath != NUL)
         slog(LOG_DEBUG, "ldap.certpath: %s", rule.state.ldap.certpath);
   }
#endif /* HAVE_LDAP */

   showstate(&rule.state);
   showlog(&rule.log);

#if HAVE_LIBWRAP
   if (*rule.libwrap != NUL)
      slog(LOG_DEBUG, "libwrap: %s", rule.libwrap);
#endif /* HAVE_LIBWRAP */
}

int
rulespermit(s, peer, local, clientauth, srcauth, match, state,
            src, dst, dstmatched, msg, msgsize)
   int s;
   const struct sockaddr_storage *peer;
   const struct sockaddr_storage *local;
   const authmethod_t *clientauth;
   authmethod_t *srcauth;
   rule_t *match;
   connectionstate_t *state;
   const sockshost_t *src;
   const sockshost_t *dst;
   sockshost_t *dstmatched;
   char *msg;
   size_t msgsize;
{
   const char *function = "rulespermit()";
   static int init;
   static rule_t defrule;
   rule_t *rule;
   objecttype_t ruletype;
   authmethod_t oldauth;
   sockshost_t dstmatched_mem;
#if HAVE_LIBWRAP
   struct request_info libwraprequest;
   struct sockaddr_storage _local, _peer;
   unsigned char libwrapinited = 0;
#endif /* !HAVE_LIBWRAP */
   char srcstr[MAXSOCKSHOSTSTRING], dststr[MAXSOCKSHOSTSTRING],
        lstr[MAXSOCKADDRSTRING], pstr[MAXSOCKADDRSTRING];

   if (dstmatched == NULL)
      dstmatched = &dstmatched_mem;

   if (s != -1) {
      SASSERTX(local != NULL);
      SASSERTX(peer  != NULL);

#if HAVE_LIBWRAP /* libwrap wants non-const. */
      sockaddrcpy(&_local, local, salen(local->ss_family));
      sockaddrcpy(&_peer,  peer,  salen(peer->ss_family));
#endif /* !HAVE_LIBWRAP */
   }

   SASSERTX(srcauth != NULL);

   slog(LOG_DEBUG,
        "%s: %s -> %s, clientauth %s, srcauth %s, command %s, fd %d "
        "from %s, accepted on %s",
        function,
        src == NULL ? "N/A" : sockshost2string(src, srcstr, sizeof(srcstr)),
        dst == NULL ? "N/A" : sockshost2string(dst, dststr, sizeof(dststr)),
        clientauth == NULL ? "N/A" : method2string(clientauth->method),
        method2string(srcauth->method),
        command2string(state->command),
        s,
        peer  == NULL ? "N/A" : sockaddr2string(peer, pstr, sizeof(pstr)),
        local == NULL ? "N/A" : sockaddr2string(local, lstr, sizeof(lstr)));

   if (msgsize > 0)
      *msg = NUL;

   /* make a somewhat sensible default rule for entries with no match. */
   if (!init) {
      bzero(&defrule, sizeof(defrule));

      defrule.verdict      = VERDICT_BLOCK;
      defrule.src.atype    = SOCKS_ADDR_IPVANY;

      defrule.dst          = defrule.src;

#if BAREFOOTD
      defrule.extra.bounceto.atype = SOCKS_ADDR_IPV4;
#endif /* BAREFOOTD */

      if (sockscf.option.debug) {
         defrule.log.connect     = 1;
         defrule.log.disconnect  = 1;
         defrule.log.error       = 1;
         defrule.log.iooperation = 1;
      }

      memset(&defrule.state.command, UCHAR_MAX, sizeof(defrule.state.command));

      memset(&defrule.state.protocol,
             UCHAR_MAX,
             sizeof(defrule.state.protocol));

      memset(&defrule.state.proxyprotocol,
             UCHAR_MAX,
             sizeof(defrule.state.proxyprotocol));

      init = 1;
   }

   /*  
    * what rulebase to use.
    */
   switch (state->command) {
      case SOCKS_ACCEPT:
      case SOCKS_BOUNCETO:
         ruletype = object_crule;
         rule     = sockscf.crule;
         break;

#if HAVE_SOCKS_HOSTID
      case SOCKS_HOSTID:
         ruletype = object_hrule;
         rule     = sockscf.hrule;
         break;
#endif /* HAVE_SOCKS_HOSTID */

      default:
         ruletype = object_srule;
         rule     = sockscf.srule;
         break;
   }


#if HAVE_LIBWRAP
   if (s != -1  && sockscf.option.hosts_access) {
      libwrapinit(s, &_local, &_peer, &libwraprequest);
      libwrapinited = 1;

      if (libwrap_hosts_access(&libwraprequest, peer) == 0) {
         snprintf(msg, msgsize, "blocked by libwrap/tcp_wrappers");

         *match      = defrule;
         match->type = ruletype;

         return 0;
      }
   }
#endif /* HAVE_LIBWRAP */

   if (state->extension.bind && !sockscf.extension.bind) {
      snprintf(msg, msgsize, "client requested disabled Dante extension: bind");

      *match      = defrule;
      match->type = ruletype;

      return 0; /* will never succeed. */
   }

#if HAVE_SOCKS_HOSTID
   if (ruletype == object_crule) { /*
                                    * only need to check once; presumably 
                                    * hostids do not get added to the 
                                    * session by the client later.
                                    */
      if (s == -1)
         HOSTIDZERO(state);
      else {
         SASSERTX(peer != NULL);

         state->hostidc = getsockethostid(s,
                                          ELEMENTS(state->hostidv),
                                          state->hostidv);

         slog(LOG_DEBUG, "%s: retrieved %u hostids on connection from %s",
              function,
              (unsigned)state->hostidc,
              sockaddr2string(peer, NULL, 0));
      }
   }

   if (ruletype == object_hrule && state->hostidc == 0 ) {
      SASSERTX(sockscf.hrule != NULL);

      slog(LOG_DEBUG, 
           "%s: no hostid set on connection from client - can not match any "
           "%s, so ignoring %ss for this client as per spec",
           function, objecttype2string(ruletype), objecttype2string(ruletype));

      *match         = defrule;
      match->type    = ruletype;
      match->verdict = VERDICT_PASS;

      return 1;
   }
#endif /* HAVE_SOCKS_HOSTID */

   /*
    * let srcauth be unchanged from original unless we actually get a match.
    */
   for (oldauth = *srcauth;
   rule != NULL;
   rule = rule->next, *srcauth = oldauth) {
      size_t methodc;
      int *methodv;
      size_t i;

      slog(LOG_DEBUG,
           "%s: trying to match against %s-rule #%lu, verdict = %s",
           function,
           objecttype2string(ruletype),
           (unsigned long)rule->number,
           verdict2string(rule->verdict));

      if (!protocol_matches(state->protocol, &rule->state.protocol))
         continue;

      if (!command_matches(state->command, &rule->state.command))
         continue;

      /* current rule covers desired version? */
      if (ruletype == object_srule) { /* no version check for other rules. */
         if (!proxyprotocol_matches(state->proxyprotocol,
                                    &rule->state.proxyprotocol))
            continue;
      }

#if HAVE_SOCKS_HOSTID
      if (rule->hostid.atype != SOCKS_ADDR_NOTSET) {
         slog(LOG_DEBUG, "%s: rule %lu requires hostid to be present on the "
                         "connection, checking ...",
                         function,
                         (unsigned long)rule->number);

         if (!hostidmatches(state->hostidc,
                            state->hostidv,
                            rule->hostindex,
                            &rule->hostid,
                            ruletype,
                            rule->number))
            continue;
      }
#endif /* HAVE_SOCKS_HOSTID */

      /*
       * This is a little tricky.  For some commands we may not have
       * all info at time of (preliminary) rule checks.  What we want
       * to do if there is no (complete) address given is to see if
       * there's any chance at all the rules will permit this request
       * when the address (later) becomes available.  We therefore
       * continue to scan the rules until we either get a pass
       * (ignoring peer with missing info), or the default block is
       * triggered.
       * This is the case for e.g. bindreply and udp, where we will
       * have to call this function again when we get the addresses in
       * question.
       */

      if (src != NULL) {
#if HAVE_SOCKS_HOSTID
         if (ruletype == object_hrule) {
            slog(LOG_DEBUG, "%s: checking against hostids rather than "
                            "physical address %s",
                            function, sockshost2string(src, NULL, 0));

            if (!hostidmatches(state->hostidc,
                               state->hostidv,
                               rule->hostindex,
                               &rule->src,
                               ruletype,
                               rule->number))
               continue;
         }
         else /* hostid matches, check the remaining fields too. */
#endif /* HAVE_SOCKS_HOSTID */
            if (!addrmatch(&rule->src, src, NULL, state->protocol, 0))
               continue;
      }
      else
         if (rule->verdict == VERDICT_BLOCK)
            continue; /* don't have complete address. */

      if (dst != NULL) {
         if (!addrmatch(&rule->dst, dst, dstmatched, state->protocol, 0))
            continue;
      }
      else {
         if (rule->verdict == VERDICT_BLOCK) {
            /*
             * don't have a complete address tuple, so see if it's possible
             * to find a pass rule matching what info we have.
             * If we do, it's possible the rules will permit when we have the
             * complete address tuple and are ready to do i/o at some later
             * point.  If not, can return a block verdict now, as there is
             * no way things will pass later.
             */
            const int dstmatchesall
            = RULEADDR_MATCHES_ALL(&rule->dst, state->protocol);

            slog(LOG_DEBUG,
                 "%s: dst ruleaddr %s %s limited to certain addresses, so %s "
                 "say for sure now whether it will match all future target "
                 "addresses from this client",
                 function,
                 ruleaddr2string(&rule->dst, 
                                 ADDRINFO_PORT | ADDRINFO_ATYPE,
                                 NULL,
                                 0),
                 dstmatchesall ? "is not" : "is",
                 dstmatchesall ? "can"    : "can not");

            if (dstmatchesall) {
               SASSERTX(src != NULL);
               /*
                * this rule matches any dst and since src has
                * already been verified to match, this is the rule
                * that will match any dst from this src.
                */
            }
            else
               /*
                * this block-rule only matches some targets, other rules may
                * match other targets and let packets from the client through.
                */
               continue;
         }
      }

      /*
       * What methods does this rule require?
       * Clientrules require clientmethods, socksrules require socksmethods. 
       */
      switch (ruletype) {
         case object_crule:
#if HAVE_SOCKS_HOSTID
         case object_hrule:
#endif /* HAVE_SOCKS_HOSTID */
            methodv  = rule->state.cmethodv;
            methodc  = rule->state.cmethodc;
            break;

         case object_srule:
            methodv  = rule->state.smethodv;
            methodc  = rule->state.smethodc;
            break;

         default:
            SERRX(object_srule);
      }


      /*
       * Does this rule's authentication requirements match the current
       * authentication in use by the client?
       */
      if ( (state->command == SOCKS_BINDREPLY
         || state->command == SOCKS_UDPREPLY)
      && !sockscf.srchost.checkreplyauth) {
         /*
          * To be consistent, we should insist that the user specifies
          * authmethod none for replies, unless he really wants to use
          * an authmethod in these cases also, which is possible (e.g.
          * method rfc931, or ip-only based pam), though probably
          * extremely unlikely.
          *
          * That can make the config look weird though; if the
          * user e.g. wants all access to be password-authenticated,
          * and thus specifies method "uname" on the global
          * method-line, he can not do that without also adding method
          * "none", as the global method-line is a superset of the
          * methods specified in the individual rules.  That means he
          * then needs to set the method on a rule-by-rule basis;
          * "none" for replies, and "username" for all others.  With
          * more than a few rules, this can become quite a hassle, is
          * unexpected, and only needed because the server supports
          * this probably quite unusual and rarely used feature.
          *
          * We therefor try to be more user-friendly about this, so
          * unless "checkreplyauth" is set in "srchost:", assume
          * authmethod should not be checked for replies also.
          */
         srcauth->method = AUTHMETHOD_NONE; /* don't bother checking. */
      }
      else if (!methodisset(srcauth->method, methodv, methodc)) {
         /*
          * No.  There are however some methods which it's possible to get
          * a match on, even if above check failed by changing/upgrading "
          * the method.
          * E.g. if the client is using method NONE (or any other),
          * it might still be possible to change the authentication to
          * RFC931, or PAM.  Likewise, if the current method is
          * AUTHMETHOD_NOTSET, it can be "upgraded" to AUTHMETHOD_NONE.
          *
          * We therefore look at what methods this rule requires and see
          * if can match it with what the client _can_ provide, if we
          * do some more work to get that information.
          *
          * Currently these methods are: gssapi (only during negotiation),
          * AUTHMETHOD_NOTSET, rfc931, and pam.
          */

         /*
          * This variable only says if current client has provided the
          * necessary information to check it's access with one of the
          * methods required by the current rule.  It does *not* mean the
          * information was checked.  I.e. if it's AUTHMETHOD_RFC931,
          * and methodischeckable is set, it means we were able to retrieve
          * rfc931 info, but not that we checked the retrieved information
          * against the passsword database.
          * XXX would be nice to cache this, so we don't have to
          * copy memory around each time.
          */
         size_t methodischeckable = 0;

         for (i = 0; i < methodc; ++i) {
            if (sockscf.option.debug >= DEBUG_VERBOSE)
               slog(LOG_DEBUG, "%s: no match yet for method %s, command %s",
                    function,
                    method2string(methodv[i]),
                    command2string(state->command));

            switch (methodv[i]) {
               case AUTHMETHOD_NONE:
                  methodischeckable = 1; /* anything is good enough. */
                  break;

#if HAVE_LIBWRAP
               case AUTHMETHOD_RFC931: {
                  const char *evaleduser;

                  if (clientauth        != NULL
                  && clientauth->method == AUTHMETHOD_RFC931) {
                     slog(LOG_DEBUG, "%s: already have rfc931 name %s from "
                                     "clientauthentication done before, "
                                     "not doing lookup again",
                                     function,
                                     clientauth->mdata.rfc931.name);

                     srcauth->mdata.rfc931 = clientauth->mdata.rfc931;
                  }
                  else { /* need to do a tcp lookup. */
                     int errno_s = errno;

                     if (state->protocol != SOCKS_TCP) {
                        /*
                         * XXX should use tcp control-connection if
                         * it's a udpassociate request.
                         */
                        slog(LOG_DEBUG,
                             "%s: protocol is not tcp (is %s), so cannot "
                             "perform an ident/rfc931 lookup",
                             function, protocol2string(state->protocol));

                        break;
                     }

                     if (!libwrapinited) {
                        libwrapinit(s,
                                    &_local,
                                    &_peer,
                                    &libwraprequest);
                        libwrapinited = 1;
                     }

                     if (errno != 0 && errno != errno_s) {
                        slog(LOG_DEBUG,
                             "%s: libwrapinit() set errno to %d (%s)",
                             function, errno, strerror(errno));

                        errno = errno_s = 0;
                     }

                     SASSERTX(peer != NULL);

                     slog(LOG_DEBUG, "%s: trying to get rfc931 name for %s",
                          function, sockaddr2string(peer, NULL, 0));

                     if ((evaleduser = eval_user(&libwraprequest)) == NULL) {
                        slog(LOG_DEBUG, "%s: eval_user() failed: %s",
                             function, strerror(errno));

                        if (errno != 0 && errno != errno_s)
                           errno = 0;

                        break;
                     }

                     strncpy((char *)srcauth->mdata.rfc931.name,
                             evaleduser,
                             sizeof(srcauth->mdata.rfc931.name) - 1);

                     if (errno != 0 && errno != errno_s) {
                        slog(LOG_DEBUG,
                             "%s: eval_user() set errno to %d (%s)",
                             function, errno, strerror(errno));
                        errno = 0;
                     }

                     /* libwrap sets this if no identreply. */
                     if (strcmp((char *)srcauth->mdata.rfc931.name,
                     STRING_UNKNOWN) == 0) {
                        *srcauth->mdata.rfc931.name = NUL;
                        slog(LOG_DEBUG, "%s: no rfc931 name", function);
                     }
                     else if (srcauth->mdata.rfc931.name[
                            sizeof(srcauth->mdata.rfc931.name) - 1] != NUL) {
                        srcauth->mdata.rfc931.name[
                           sizeof(srcauth->mdata.rfc931.name) - 1] = NUL;

                        slog(LOG_INFO, "%s: rfc931 name \"%s...\" too long",
                             function, srcauth->mdata.rfc931.name);

                        *srcauth->mdata.rfc931.name = NUL; /* unusable. */
                     }
                     else
                        slog(LOG_DEBUG, "%s: rfc931 name gotten is \"%s\"",
                             function, srcauth->mdata.rfc931.name);
                  }

                  if (*srcauth->mdata.rfc931.name != NUL)
                     methodischeckable = 1;
                  break;
               }
#endif /* HAVE_LIBWRAP */

#if HAVE_PAM
               case AUTHMETHOD_PAM_ANY: 
               case AUTHMETHOD_PAM_ADDRESS: 
               case AUTHMETHOD_PAM_USERNAME: {
                  /*
                   * PAM can support username/password, just username,
                   * or neither username nor password (i.e. based on
                   * only ip address).
                   */
                  switch (srcauth->method) {
                     case AUTHMETHOD_UNAME: {
                        /* it's a union, make a copy first. */
                        const authmethod_uname_t uname = srcauth->mdata.uname;

                        /*
                         * similar enough, just copy name/password.
                         */

                        STRCPY_ASSERTSIZE(srcauth->mdata.pam.name, 
                                          (const char *)uname.name);

                        STRCPY_ASSERTSIZE(srcauth->mdata.pam.password,
                                          (const char *)uname.password);

                        methodischeckable = 1;
                        break;
                     }

#if HAVE_LIBWRAP
                     case AUTHMETHOD_RFC931: {
                          /* it's a union, make a copy first. */
                          const authmethod_rfc931_t rfc931
                          = srcauth->mdata.rfc931;

                         /*
                          * no password, but we can check for the username
                          * we got from ident, with an empty password.
                          */

                         STRCPY_ASSERTSIZE(srcauth->mdata.pam.name,
                                           (const char *)rfc931.name);

                        *srcauth->mdata.pam.password = NUL;

                        methodischeckable = 1;
                        break;
                     }
#endif /* HAVE_LIBWRAP */

                     case AUTHMETHOD_NOTSET:
                     case AUTHMETHOD_NONE:
                        /*
                         * PAM can also support no username/password.
                         */

                        *srcauth->mdata.pam.name     = NUL;
                        *srcauth->mdata.pam.password = NUL;

                        methodischeckable = 1;
                        break;
                  }

                  STRCPY_ASSERTSIZE(srcauth->mdata.pam.servicename,
                                    rule->state.pamservicename);
                  break;
               }
#endif /* HAVE_PAM */

#if HAVE_BSDAUTH
               case AUTHMETHOD_BSDAUTH: {
                  /*
                   * Requires username and password, but assume
                   * the password can be empty.
                   */

                  switch (srcauth->method) {
                     case AUTHMETHOD_UNAME: {
                        /* it's a union, make a copy first. */
                        const authmethod_uname_t uname
                        = srcauth->mdata.uname;

                        /*
                         * similar enough, just copy name/password.
                         */

                        STRCPY_ASSERTSIZE(srcauth->mdata.bsd.name,
                                          (const char *)uname.name);

                        STRCPY_ASSERTSIZE(srcauth->mdata.bsd.password,
                                          (const char *)uname.password);

                        methodischeckable = 1;
                        break;
                     }

#if HAVE_LIBWRAP
                     case AUTHMETHOD_RFC931: {
                          /* it's a union, make a copy first. */
                          const authmethod_rfc931_t rfc931
                          = srcauth->mdata.rfc931;

                         /*
                          * no password, but we can check for the username
                          * we got from ident, with an empty password.
                          */

                         STRCPY_ASSERTSIZE(srcauth->mdata.bsd.name,
                                           (const char *)rfc931.name);

                        *srcauth->mdata.bsd.password = NUL;

                        methodischeckable = 1;
                        break;
                     }
#endif /* HAVE_LIBWRAP */
                  }

                  STRCPY_ASSERTSIZE(srcauth->mdata.bsd.style,
                                    rule->state.bsdauthstylename);
                  break;
               }
#endif /* HAVE_BSDAUTH */


#if SOCKS_SERVER
#if HAVE_GSSAPI
               case AUTHMETHOD_GSSAPI: {
                  /*
                   * GSSAPI can only be checked/established during
                   * negotiation (command = SOCKS_ACCEPT).
                   * After that stage has completed, we either have
                   * it or we don't.
                   */
                  if (state->command != SOCKS_ACCEPT)
                     continue;

                  STRCPY_ASSERTSIZE(srcauth->mdata.gssapi.servicename,
                                    rule->state.gssapiservicename);

                  STRCPY_ASSERTSIZE(srcauth->mdata.gssapi.keytab,
                                    rule->state.gssapikeytab);

                  srcauth->mdata.gssapi.encryption.nec
                  = rule->state.gssapiencryption.nec;

                  srcauth->mdata.gssapi.encryption.clear
                  = rule->state.gssapiencryption.clear;

                  srcauth->mdata.gssapi.encryption.integrity
                  = rule->state.gssapiencryption.integrity;

                  srcauth->mdata.gssapi.encryption.confidentiality
                  = rule->state.gssapiencryption.confidentiality;

                  methodischeckable = 1;
                  break;
               }
#endif /* HAVE_GSSAPI */
#endif /* SOCKS_SERVR */
            }

            if (methodischeckable) {
               slog(LOG_DEBUG, "%s: changing authmethod from %d to %d",
                    function, srcauth->method, methodv[i]);

               srcauth->method = methodv[i]; 
               break;
            }
         }

         if (i == methodc) {
#if COVENANT
            /*
             * Respond to the client that it must provide proxy
             * authentication, which means we can go from no authentication
             * to "any" authentication, if the client goes on to provide
             * authentication later.
             */
            if (methodc > 0) { /* auth is required. */
               match->verdict                   = VERDICT_BLOCK;
               match->whyblock.missingproxyauth = 1;
               SHMEM_CLEAR(match, SHMEM_ALL, 1);

               SASSERTX(!SHMID_ISATTACHED(match));

               return 0;
            }
#else /* !COVENANT */
            /*
             * the methods of the current rule differs from what client can
             * provide us with.  Go to next rule.
             */
            continue;
#endif /* !COVENANT */
         }
      }

      SASSERTX(state->command == SOCKS_BINDREPLY
      ||       state->command == SOCKS_UDPREPLY
      ||       state->command == SOCKS_HOSTID
      ||       methodisset(srcauth->method, methodv, methodc));

      if (srcauth->method != AUTHMETHOD_NONE && rule->user != NULL) {
         /* rule requires user.  Covers current? */
         if (!usermatch(srcauth, rule->user)) {
            slog(LOG_DEBUG,
                 "%s: username \"%s\" did not match rule #%lu for %s",
                 function,
                 authname(srcauth) == NULL ? "<null>" : authname(srcauth),
                 (unsigned long)rule->number,
                 command2string(state->command));

            continue; /* no match. */
         }
      }

      if (srcauth->method != AUTHMETHOD_NONE && rule->group != NULL) {
         /* rule requires group.  Covers included? */
         if (!groupmatch(srcauth, rule->group)) {
            slog(LOG_DEBUG,
                 "%s: groupname \"%s\" did not match rule #%lu for %s",
                 function,
                 authname(srcauth) == NULL ? "<null>" : authname(srcauth),
                 (unsigned long)rule->number,
                 command2string(state->command));

            continue; /* no match. */
         }
      }

#if HAVE_LDAP
      /* rule requires a group, and covers current user? */
      if (srcauth->method != AUTHMETHOD_NONE && rule->ldapgroup != NULL) {
         if (!ldapgroupmatch(srcauth, (const rule_t *)rule))  {
               slog(LOG_DEBUG,
                    "%s: username \"%s\" did not match rule #%lu for %s",
                    function,
                    authname(srcauth) == NULL ? "<null>" : authname(srcauth),
                    (unsigned long)rule->number,
                    command2string(state->command));

               continue; /* no match. */
         }
      }
#endif

      if (!accesscheck(s, srcauth, peer, local, msg, msgsize)) {
         *match         = *rule;

         match->verdict = VERDICT_BLOCK;
         SHMEM_CLEAR(match, SHMEM_ALL, 1);

         return 0;
      }

      break;
   }

   if (rule == NULL) {
      char buf[1024];

      if (msg == NULL || msgsize == 0) {
         msg     = buf;
         msgsize = sizeof(buf);
      }

      snprintf(msg, msgsize, "no rules matched; using default block rule");

      *match       = defrule;
      match->type  = ruletype;
   }
   else {
      *match = *rule;

      slog(LOG_DEBUG, "%s: rule matched: %lu (%s), verdict %s",
           function,
           (unsigned long)match->number,
           objecttype2string(match->type),
           verdict2string(match->verdict));
   }

   /*
    * got our rule, now check connection.
    */
   if (match->verdict == VERDICT_PASS && peer != NULL)
      if (!srchost_isok(peer, msg, msgsize)) {
         match->verdict = VERDICT_BLOCK;
         SHMEM_CLEAR(match, SHMEM_ALL, 1);

         return 0;
      }

#if HAVE_LIBWRAP
   if (s != -1 && *match->libwrap != NUL) {
      char libwrapcmd[LIBWRAPBUF];
      int errno_pre, errno_post;

      if (!libwrapinited)
         libwrapinit(s, &_local, &_peer, &libwraprequest);

      /* libwrap modifies the passed buffer. */
      STRCPY_ASSERTSIZE(libwrapcmd, match->libwrap);

      /* Wietse Venema says something along the lines of: */
      if (setjmp(tcpd_buf) != 0) {
         sockd_priv(SOCKD_PRIV_LIBWRAP, PRIV_OFF);

         swarnx("%s: failed libwrap line: \"%s\"", function, libwrapcmd);

         match->verdict = VERDICT_BLOCK;
         SHMEM_CLEAR(match, SHMEM_ALL, 1);

         return 0;   /* something got screwed up. */
      }

      slog(LOG_DEBUG, "%s: executing libwrap command: \"%s\"",
           function, libwrapcmd);

      sockd_priv(SOCKD_PRIV_LIBWRAP, PRIV_ON);
      errno_pre = errno;
      errno = 0;
      process_options(libwrapcmd, &libwraprequest);
      errno_post = errno;
      sockd_priv(SOCKD_PRIV_LIBWRAP, PRIV_OFF);

      if (errno_post != 0)
         slog(LOG_INFO,
              "%s: processing of libwrap options set errno (%s)",
              function, strerror(errno_post));
      errno = errno_pre;

      if (match->verdict == VERDICT_BLOCK
      &&  strstr(match->libwrap, "banners ") != NULL) {
         /*
          * We don't want the kernel to RST this connection upon our
          * subsequent close(2) without us having sent the whole banner to
          * the client first.  But if the kernel wants to send RST while
          * we have data not yet sent, it will discard the data not yet
          * sent.  We therefor drain the data first, trying to make sure the
          * kernel does not discard the data in the outbuffer and sends a
          * RST when we close(2).  Note that this changes the RST to FIN.
          * See 2.17 in RFC 2525, "Failure to RST on close with data pending",
          * for more information about this.
          *
          * Note there is a race here, as the client could send us data
          * between our last read(2) call and us closing the session later, not
          * but much to do about that.
          */
          char buf[1024];
          ssize_t p;

          while ((p = read(s, buf, sizeof(buf))) > 0)
            slog(LOG_DEBUG, "%s: reading and discarding %ld bytes from blocked "
                            "connection so the banner is sent to the client",
                            function, (long)p);
      }
   }
#endif /* !HAVE_LIBWRAP */

   if (match->verdict == VERDICT_BLOCK)
      SHMEM_CLEAR(match, SHMEM_ALL, 1);
   else { 
      if (sockscf.monitor != NULL) {
         if (src != NULL) {
            const monitor_t *monitor;

            SASSERTX(match->mstats       == NULL);
            SASSERTX(match->mstats_shmid == 0);

            if ((monitor = monitormatch(src,
                                        dst,
                                        srcauth,
                                        state)) != NULL) {
               SASSERTX(monitor->mstats == NULL);

               slog(LOG_DEBUG, "%s: matched monitor #%lu, mstats_shmid = %lu",
                    function,
                    (unsigned long)monitor->number, 
                    (unsigned long)monitor->mstats_shmid);

               COPY_MONITORFIELDS(monitor, match);
            }
         }
      }
   }

   SASSERTX(!SHMID_ISATTACHED(match));

   return match->verdict == VERDICT_PASS;
}

int
command_matches(command, commands)
   const int command;
   const command_t *commands;
{
   switch (command) {
      /* client-rule commands. XXX set them to in commands for crule.*/
      case SOCKS_ACCEPT:
      case SOCKS_BOUNCETO:
         return 1;

      /* hostid: same as SOCKS_ACCEPT basically. */
      case SOCKS_HOSTID:
         return 1;

      /* socks-rule commands. */
      case SOCKS_BIND:
         if (commands->bind)
            return 1;
         else
            return 0;

      case SOCKS_CONNECT:
         if (commands->connect)
            return 1;
         else 
            return 0;

      case SOCKS_UDPASSOCIATE:
         if (commands->udpassociate)
            return 1;
         else
            return 0;

      /*
       * pseudo commands.
       */

      case SOCKS_BINDREPLY:
         if (commands->bindreply)
            return 1;
         else
            return 0;

      case SOCKS_UDPREPLY:
         if (commands->udpreply)
            return 1;
         return
            0;

      default:
         SERRX(command);
   }
}

int
protocol_matches(protocol, protocols)
   const int protocol;
   const protocol_t *protocols;
{
   switch (protocol) {
      case SOCKS_TCP:
         if (protocols->tcp)
            return 1;
         else
            return 0;

      case SOCKS_UDP:
         if (protocols->udp)
            return 1;
         else
            return 0;

      default:
         SERRX(protocol);
   }
}

int
proxyprotocol_matches(protocol, protocols)
   const int protocol;
   const proxyprotocol_t *protocols;
{

   switch (protocol) {
      case PROXY_SOCKS_V4:
         if (protocols->socks_v4)
            return 1;
         else
            return 0;

      case PROXY_SOCKS_V5:
         if (protocols->socks_v5)
            return 1;
         else
            return 0;

      case PROXY_HTTP_10:
      case PROXY_HTTP_11:
         if (protocols->http)
            return 1;
         else
            return 0;

      default:
         SERRX(protocol);
   }
}

void
showlist(list, prefix)
   const linkedname_t *list;
   const char *prefix;
{
   char buf[10240];

   list2string(list, buf, sizeof(buf));
   if (strlen(buf) > 0)
      slog(LOG_DEBUG, "%s%s", prefix, buf);
}

int
rulerequires(rule, what)
   const rule_t *rule;
   const methodinfo_t what;
{
   const char *function = "rulerequires()";
   int rc;

   switch (what) {
      case username:
         if (rule->user       != NULL
         ||  rule->group      != NULL
#if HAVE_LDAP
         ||  rule->ldapgroup  != NULL
         ||  rule->ldapsettingsfromuser /* ldap also requires username. */
#endif /* HAVE_LDAP */
         )
            rc = 1;
         else
            rc =0;

         break;

      default:
         SERRX(what);
   }

   slog(LOG_DEBUG, "%s: %s: %lu requires %d: %s",
        function,
        objecttype2string(rule->type),
        (unsigned long)rule->number,
        (int)what,
        rc == 1 ? "yes" : "no");

   return rc;
}

static int
srchost_isok(peer, msg, msgsize)
   const struct sockaddr_storage *peer;
   char *msg;
   size_t msgsize;
{
   const char *function = "srchost_isok()";
   char hostname[MAXHOSTNAMELEN], vishname[sizeof(hostname) * 4];
   int rc;

   if (!sockscf.srchost.nodnsmismatch 
   &&  !sockscf.srchost.nodnsunknown)
      return 1;

   rc = cgetnameinfo(TOCSA(peer), 
                     salen(peer->ss_family),
                     hostname,
                     sizeof(hostname),
                     NULL,
                     0,
                     NI_NAMEREQD);

   if (rc != 0) {
      log_reversemapfailed(peer, INTERNALIF, rc);

      snprintf(msg, msgsize, "no dns entry found for srchost %s",
               sockaddr2string(peer, NULL, 0));

      if (sockscf.srchost.nodnsunknown)
         return 0;

      /* 
       * if there is no dns-entry for this address, there can be
       * no mismatch either.  
       */
      SASSERTX(sockscf.srchost.nodnsmismatch);
      return 1;
   }

   slog(LOG_DEBUG, "%s: %s has a dns entry: %s",
        function,
        sockaddr2string(peer, NULL, 0), 
        str2vis(hostname, strlen(hostname), vishname, sizeof(vishname)));

   if (sockscf.srchost.nodnsmismatch) {
      /*
       * Check if the reversemapped hostname maps back to the 
       * ipaddress.
       */
      ruleaddr_t ruleaddr;
      sockshost_t resolvedhost;

      sockaddr2ruleaddr(peer, &ruleaddr);
      ruleaddr.operator  = none;

      if (strlen(hostname) >= sizeof(resolvedhost.addr.domain)) {
         swarnx("%s: ipaddress %s resolved to a hostname (%s) which is too "
                "long.  %lu is the defined maximum",
                function,
                sockaddr2string2(peer, 0, NULL, 0),
                vishname,
                (unsigned long)sizeof(resolvedhost.addr.domain) - 1);

         return 0;
      }

      resolvedhost.atype = SOCKS_ADDR_DOMAIN;
      strcpy(resolvedhost.addr.domain, hostname);
      resolvedhost.port  = htons(0);

      if (!addrmatch(&ruleaddr, &resolvedhost, NULL, SOCKS_TCP, 0)) {
         snprintf(msg, msgsize,
                  "DNS IP/hostname mismatch.  %s does not match "
                  "reversemapped addresses for hostname \"%s\"",
                  sockaddr2string2(peer, 0, NULL, 0), vishname);

         return 0;
      }
   }

   return 1;
}

#if HAVE_LIBWRAP
static void
libwrapinit(s, local, peer, request)
   int s;
   struct sockaddr_storage *local;
   struct sockaddr_storage *peer;
   struct request_info *request;
{
   const char *function = "libwrapinit()";
   const int errno_s = errno;
   char hostname[MAXHOSTNAMELEN],
        localstr[MAXSOCKADDRSTRING], peerstr[MAXSOCKADDRSTRING];
   int rc;

   slog(LOG_DEBUG, 
        "%s: initing libwrap using fd %d for local %s, peer %s",
        function,
        s,
        sockaddr2string(local, localstr, sizeof(localstr)),
        sockaddr2string(peer, peerstr, sizeof(peerstr)));

   rc = cgetnameinfo(TOSA(local), 
                     salen(local->ss_family),
                     hostname,
                     sizeof(hostname),
                     NULL,
                     0, 
                     NI_NAMEREQD);

   request_init(request,
                RQ_FILE, s,
                RQ_DAEMON, __progname,
                RQ_CLIENT_SIN, peer,
                RQ_SERVER_SIN, local,
                RQ_SERVER_NAME, rc == 0 ? hostname : "",
                0);

   rc = cgetnameinfo(TOSA(peer), 
                     salen(peer->ss_family),
                     hostname,
                     sizeof(hostname),
                     NULL,
                     0, 
                     NI_NAMEREQD);

   request_set(request,
               RQ_CLIENT_NAME, rc == 0 ? hostname : "",
               0);

   /* apparently some obscure libwrap bug requires this call. */
   sock_methods(request);

   errno = errno_s;
}

static int
libwrap_hosts_access(request, peer)
   struct request_info *request;
   const struct sockaddr_storage *peer;
{
   const char *function = "libwrap_hosts_access()";
   int allow;

   sockd_priv(SOCKD_PRIV_LIBWRAP, PRIV_ON);
   allow = hosts_access(request) != 0;
   sockd_priv(SOCKD_PRIV_LIBWRAP, PRIV_OFF);

   slog(LOG_DEBUG, "%s: libwrap/tcp_wrappers hosts_access() %s address %s",
        function,
        allow ? "allows" : "denies", 
        sockaddr2string(peer, NULL, 0));

   if (allow)
      return 1;

   return 0;
}
#endif /* HAVE_LIBWRAP */

static rule_t *
addrule(newrule, rulebase, ruletype)
   const rule_t *newrule;
   rule_t **rulebase;
   const objecttype_t ruletype;
{
   const char *function = "addrule()";
   serverstate_t zstate;
   rule_t *rule;
   size_t i;

   bzero(&zstate, sizeof(zstate));

   if ((rule = malloc(sizeof(*rule))) == NULL)
      serr("%s: could not allocate %lu bytes for a rule",
           function, (unsigned long)sizeof(*rule));

   *rule = *newrule;

   rule->type = ruletype;

   /*
    * try to set values not set to a sensible default.
    */

   if (sockscf.option.debug) {
      rule->log.connect       = 1;
      rule->log.disconnect    = 1;
      rule->log.error         = 1;
      rule->log.iooperation   = 1;
      /* rule->log.tcpinfo       = 1; */
   }
   /* else; don't touch logging, no logging is ok. */

   switch (ruletype) {
      case object_srule:
         if (rule->timeout.tcpio != 0)
            if (rule->timeout.tcp_fin_wait != 0
            &&  rule->timeout.tcp_fin_wait > rule->timeout.tcpio) {
               yywarnx("%s: it does not make sense to let the tcp-fin-wait "
                       "timeout be longer than the tcp i/o timeout, so "
                       "reducing tcp-fin-wait for this rule from %ld to %ld",
                       function,
                       (long)rule->timeout.tcp_fin_wait,
                       (long)rule->timeout.tcpio);

               rule->timeout.tcp_fin_wait = rule->timeout.tcpio;
            }
         break;

      default:
         break;
   }

   setcommandprotocol(rule->type, &rule->state.command, &rule->state.protocol);

   /*
    * If no clientmethod is set, set AUTHMETHOD_NONE.  Avoids users
    * having to always manually set it since there are no cases where
    * it makes sense to not have at least one clientmethod set.
    * Doesn't make much sense to not have method set either, but require
    * user to set that to be more sure that is set correctly.
    */
   if (sockscf.cmethodc == 0)
      sockscf.cmethodv[sockscf.cmethodc++] = AUTHMETHOD_NONE;

   /*
    * If no clientmethod is set in this rule, set all from the global method 
    * line that make sense for this particular rule.
    */
   if (rule->state.cmethodc == 0) {
      for (i = 0; i < sockscf.cmethodc; ++i)
         rule->state.cmethodv[rule->state.cmethodc++] = sockscf.cmethodv[i];
   }
   else {
      for (i = 0; i < rule->state.cmethodc; ++i)
         if (!methodisset(rule->state.cmethodv[i], 
                          sockscf.cmethodv, 
                          sockscf.cmethodc))
            yyerrorx("clientmethod \"%s\" is set in the rule, but not in the "
                     "global clientmethod specification (containing \"%s\').  "
                     "Only methods that are part of the global method "
                     "specification can be part of the methods set in a local "
                     "rule",
                     method2string(rule->state.cmethodv[i]),
                     methods2string(sockscf.cmethodc, 
                                    sockscf.cmethodv, 
                                    NULL, 
                                    0));
   }


   /*
    * need to do this check before adding methods ourselves, as if user 
    * explicitly sets a method that will not work with all commands the 
    * rule supports, we want to error out.  We are less strict if he 
    * does not and it's a method we add ourselves when it make sense.
    */
   if (isreplycommandonly(&rule->state.command)) {
      for (i = 0; i < rule->state.cmethodc; ++i) {
         if ((   rule->state.command.udpreply
             && !methodworkswith(rule->state.smethodv[i], udpreplies))
         || (   rule->state.command.bindreply
             && !methodworkswith(rule->state.smethodv[i], tcpreplies)))
            yyerrorx("%s #%lu specifies method %s, but this "
                     "method can not be provided by %sreplies",
                     objecttype2string(rule->type),
                     (unsigned long)rule->number,
                     method2string(rule->state.smethodv[i]),
                     rule->state.command.udpreply ? "udp" : "");
      }
   }

   /*
    * If no socksmethod is set in this rule, set all from the global method 
    * line.  Note that for the client-rule there isn't really many checks we 
    * can do on what socksmethods to set (e.g., do they support what is 
    * required by the socks-rule?), as we won't know what is required until 
    * we know what the matching socks-rule will be for the clients request.
    */
   if (rule->state.smethodc == 0) {
      for (i = 0; i < sockscf.smethodc; ++i) {
         switch (rule->type) {
            case object_crule:

#if HAVE_SOCKS_HOSTID
            case object_hrule:
#endif /* HAVE_SOCKS_HOSTID */

               rule->state.smethodv[rule->state.smethodc++] 
               = sockscf.smethodv[i];
               break;

#if HAVE_SOCKS_RULES
            case object_srule:
               if (isreplycommandonly(&rule->state.command)) {
                  if (rule->state.command.udpreply
                  && !methodworkswith(sockscf.smethodv[i], udpreplies))
                     continue;

                  if (rule->state.command.bindreply
                  &&  !methodworkswith(sockscf.smethodv[i], tcpreplies))
                     continue;
               }

               if (rulerequires(rule, username))
                  if (!methodcanprovide(sockscf.smethodv[i], username))
                     continue;

               rule->state.smethodv[rule->state.smethodc++] 
               = sockscf.smethodv[i];

               break;
#endif /* HAVE_SOCKS_RULES */

            default:
               SERRX(rule->type);
         }

      }
   }
   else {
      /*
       * socksmethods set by user.  Make sure they are also part of the
       * global socksmethods set.
       */
      for (i = 0; i < rule->state.smethodc; ++i)
         if (!methodisset(rule->state.smethodv[i], 
                          sockscf.smethodv, 
                          sockscf.smethodc))
            yyerrorx("method \"%s\" is set in the rule, but not in the global "
                     "socksmethod specification (containing \"%s\').  "
                     "Only methods that are part of the global method "
                     "specification can be part of the methods set in a local "
                     "rule",
                     method2string(rule->state.smethodv[i]),
                     methods2string(sockscf.smethodc, 
                                    sockscf.smethodv, 
                                    NULL, 
                                    0));
   }

   /* if no proxy protocol set, set appropriate. */
   if (memcmp(&zstate.proxyprotocol, &rule->state.proxyprotocol,
   sizeof(zstate.proxyprotocol)) == 0) {
#if SOCKS_SERVER
      rule->state.proxyprotocol.socks_v4 = 1;
      rule->state.proxyprotocol.socks_v5 = 1;

#elif BAREFOOTD /* !SOCKS_SERVER */
      rule->state.proxyprotocol.socks_v5 = 1;

#elif COVENANT
      rule->state.proxyprotocol.http     = 1;
#endif /* COVENANT */
   }

#if BAREFOOTD
   SASSERTX(rule->type != object_srule);

   if (rule->verdict == VERDICT_PASS
   &&  ( (rule->extra.bounceto.port.tcp == htons(0)) 
      || (rule->extra.bounceto.port.udp == htons(0)))) {
      /*
       * If no port-number is given for the bounce-to address, default to 
       * using the same port as the port we accept the client on.  Only do 
       * this if it's a pass rule however.  In the block case, we don't care 
       * what the bounce-to address is as we will never connect there, so 
       * prefer it to be 0.0.0.0, port 0, simply for cosmetic reasons.
       */
      if (rule->state.protocol.tcp 
      && rule->extra.bounceto.port.tcp == htons(0)) {
         rule->extra.bounceto.port.tcp = rule->dst.port.tcp;

         slog(LOG_DEBUG, 
              "%s: no bounce-to port given for protocol %s in %s #%lu.  "
              "Using dst port %u", 
              function, 
              protocol2string(SOCKS_TCP),
              objecttype2string(rule->type),
              (unsigned long)rule->number,
              ntohs(rule->dst.port.tcp));
      }

      if (rule->state.protocol.udp 
      && rule->extra.bounceto.port.udp == htons(0)) {
         rule->extra.bounceto.port.udp = rule->dst.port.udp;

         slog(LOG_DEBUG, 
              "%s: no bounce-to port given for protocol %s in %s #%lu.  "
              "Using dst port %u", 
              function, 
              protocol2string(SOCKS_UDP),
              objecttype2string(rule->type),
              (unsigned long)rule->number,
              ntohs(rule->dst.port.udp));
      }
   }
#endif /* BAREFOOTD */


   /*
    * Set default values for some authentication-methods, if none
    * set.  Note that this needs to be set regardless of what the
    * method set in the rule is, as checkconfig() might add methods
    * to the rules as part of it's operation.  This happens e.g. when
    * adding default methods to the global clientmethod line, if appropriate,
    * and then adding the same methods to the client-rules, if the rules
    * do not already have a method.
    */

#if HAVE_PAM
   if (*rule->state.pamservicename == NUL) /* set to default. */
      STRCPY_ASSERTSIZE(rule->state.pamservicename, DEFAULT_PAMSERVICENAME);
#endif /* HAVE_PAM */

#if HAVE_BSDAUTH
   if (*rule->state.bsdauthstylename == NUL) /* set to default. */
      STRCPY_ASSERTSIZE(rule->state.bsdauthstylename, DEFAULT_BSDAUTHSTYLENAME);
#endif /* HAVE_BSDAUTH */

#if HAVE_GSSAPI
   if (*rule->state.gssapiservicename == NUL) /* set to default. */
      STRCPY_ASSERTSIZE(rule->state.gssapiservicename,
                       DEFAULT_GSSAPISERVICENAME);

   if (*rule->state.gssapikeytab == NUL) /* set to default. */
      STRCPY_ASSERTSIZE(rule->state.gssapikeytab, DEFAULT_GSSAPIKEYTAB);

   /*
    * can't do memcmp since we don't want to include
    * gssapiencryption.nec in the compare.
    */
   if (rule->state.gssapiencryption.clear           == 0
   &&  rule->state.gssapiencryption.integrity       == 0
   &&  rule->state.gssapiencryption.confidentiality == 0
   &&  rule->state.gssapiencryption.permessage      == 0) {
      rule->state.gssapiencryption.integrity      = 1;
      rule->state.gssapiencryption.confidentiality= 1;
      rule->state.gssapiencryption.permessage     = 0;
   }
#endif /* HAVE_GSSAPI */

#if HAVE_LDAP
   if (*rule->state.ldap.keytab == NUL) /* set to default. */
      STRCPY_ASSERTSIZE(rule->state.ldap.keytab, DEFAULT_GSSAPIKEYTAB);
   else
      rule->ldapsettingsfromuser = 1;

   if (*rule->state.ldap.filter == NUL) /* set to default. */
      STRCPY_ASSERTSIZE(rule->state.ldap.filter, DEFAULT_LDAP_FILTER);
   else
      rule->ldapsettingsfromuser = 1;

   if (*rule->state.ldap.filter_AD == NUL) /* set to default. */
      STRCPY_ASSERTSIZE(rule->state.ldap.filter_AD, DEFAULT_LDAP_FILTER_AD);
   else
      rule->ldapsettingsfromuser = 1;

   if (*rule->state.ldap.attribute == NUL) /* set to default. */
      STRCPY_ASSERTSIZE(rule->state.ldap.attribute, DEFAULT_LDAP_ATTRIBUTE);
   else
      rule->ldapsettingsfromuser = 1;

   if (*rule->state.ldap.attribute_AD == NUL) /* set to default. */
      STRCPY_ASSERTSIZE(rule->state.ldap.attribute_AD, 
                       DEFAULT_LDAP_ATTRIBUTE_AD);
   else
      rule->ldapsettingsfromuser = 1;

   if (*rule->state.ldap.certfile == NUL) /* set to default. */
      STRCPY_ASSERTSIZE(rule->state.ldap.certfile, DEFAULT_LDAP_CACERTFILE);
   else
      rule->ldapsettingsfromuser = 1;

   if (*rule->state.ldap.certpath == NUL) /* set to default. */
      STRCPY_ASSERTSIZE(rule->state.ldap.certpath, DEFAULT_LDAP_CERTDBPATH);
   else
      rule->ldapsettingsfromuser = 1;

   if (rule->state.ldap.port == 0) /* set to default */
      rule->state.ldap.port = SOCKD_EXPLICIT_LDAP_PORT;
   else
      rule->ldapsettingsfromuser = 1;

   if (rule->state.ldap.portssl == 0) /* set to default */
      rule->state.ldap.portssl = SOCKD_EXPLICIT_LDAPS_PORT;
   else
      rule->ldapsettingsfromuser = 1;
#endif /* HAVE_LDAP */

   if (*rulebase == NULL) {
      *rulebase           = rule;
      (*rulebase)->number = 1;
   }
   else { /* append this rule to the end of our list. */
      rule_t *lastrule;

      lastrule = *rulebase;
      while (lastrule->next != NULL)
         lastrule = lastrule->next;

      rule->number = lastrule->number + 1;
      lastrule->next = rule;
   }

   INIT_MSTATES(rule, rule->type, rule->number);
   rule->next = NULL;

   return rule;
}

static void
checkrule(rule)
   const rule_t *rule;
{
   const char *function = "checkrule()";
   ruleaddr_t ruleaddr;
   const int *methodv[]      = { 
                                 rule->state.cmethodv,
                                 rule->state.smethodv 
                               };
   const size_t methodc[]    = {
                                 rule->state.cmethodc,
                                 rule->state.smethodc
                               };
   const objecttype_t type[] = { 
                                 object_crule,
                                 object_srule
                               };

   size_t i, methodi;

   if (rule->src.atype == SOCKS_ADDR_IFNAME) {
      struct sockaddr_storage addr, mask;

      if (ifname2sockaddr(rule->src.addr.ifname, 0, &addr, &mask) == NULL)
         yyerror("no ip address found on interface %s", rule->src.addr.ifname);
   }

   if (rule->dst.atype == SOCKS_ADDR_IFNAME) {
      struct sockaddr_storage addr, mask;

      if (ifname2sockaddr(rule->dst.addr.ifname, 0, &addr, &mask)
      == NULL)
         yyerror("no ip address found on interface %s", rule->dst.addr.ifname);
   }


   if (rule->ss != NULL) {
      SASSERTX(rule->ss->type == SHMEM_SS);

      if (rule->ss->keystate.key == key_unset) {
         if (rule->ss->object.ss.max_perstate_isset)
            yyerrorx("state.session.max is set, but session.state.key is not");

         if (rule->ss->object.ss.throttle_perstate_isset)
            yyerrorx("session.state.throttle is set, but session.state.key "
                     "is not");
      }
      else {
         if (!rule->ss->object.ss.max_perstate_isset
         &&  !rule->ss->object.ss.throttle_perstate_isset)
            yyerrorx("session state key is set (%d), but no value has been "
                     "given to state.max or state.throttle",
                     (int)rule->ss->keystate.key);

         switch (rule->ss->keystate.key) {
            case key_hostid:
               SASSERTX(rule->hostidoption_isset);

#if 0 /* XXX more thought needed. */
               if (rule->state.protocol.udp)
                  yyerrorx("hostids are not available on sessions using UDP");
#endif /* 0  */

               if (rule->ss->keystate.keyinfo.hostindex < 1)
                  yyerrorx("illegal hostindex value %d%s",
                           (int)rule->ss->keystate.keyinfo.hostindex,
                           rule->ss->keystate.keyinfo.hostindex == 0 ?
                              "/any" : "");
               break;

            case key_from:
               if (rule->ss->keystate.keyinfo.hostindex != 0)
                  yyerrorx("it does not make sense to set a hostindex value "
                           "for state key %s.  The hostindex keyword "
                           "is only usable with hostid keys",
                           statekey2string(rule->ss->keystate.key));
               break;

            default:
               SERRX(rule->ss->keystate.key);
         }

         if (rule->ss->object.ss.max_isset
         && (rule->ss->object.ss.max_perstate
             > rule->ss->object.ss.max))
            yyerrorx("it does not make sense to have a larger value for "
                     "session.state.max (%ld) than the combined, state-less, "
                     "session.max value (%ld).  Configuration error?",
                     (unsigned long)rule->ss->object.ss.max_perstate,
                     (unsigned long)rule->ss->object.ss.max);

         if (rule->ss->object.ss.throttle_perstate_isset
         && rule->ss->object.ss.throttle_isset) {
            if ((rule->ss->object.ss.throttle_perstate.limit.seconds
              <= rule->ss->object.ss.throttle.limit.seconds)
            &&  (rule->ss->object.ss.throttle_perstate.limit.clients
              >  rule->ss->object.ss.throttle.limit.clients))
            yyerrorx("it does not make sense to have a more strict limit for "
                     "the combined, state-less, session.throttle limit "
                     "(%lu/%ld), than for the per %s state "
                     "session.state.throttle (%lu/%ld).  Configuration error?",
                     (unsigned long)rule->ss->object.ss.throttle.limit.clients,
                     (long)rule->ss->object.ss.throttle.limit.seconds,
                     statekey2string(rule->ss->keystate.key),
            (unsigned long)rule->ss->object.ss.throttle_perstate.limit.clients,
            (long)rule->ss->object.ss.throttle_perstate.limit.seconds);
         }
      }
   }

   if (rule->bw != NULL) {
      SASSERTX(rule->bw->type == SHMEM_BW);

      if (!rule->bw->object.bw.maxbps_isset)
         yyerrorx("no value for maxbps has been given");
   }

#if 0
   /*
    * XXX
    * Needs more thought.  Would it be more meaningfull to consider 
    * hostids provided as long as any part of the session (i.e., the
    * TCP connectol connectio) has provded it?
    */

   if (rule->state.protocol.udp) {
      if (rule->hostidoption_isset)
         yyerrorx("hostids are not available on sessions using UDP");
   }
#endif /* 0 */


   if (rule->src.atype == SOCKS_ADDR_IPVANY) {
      SASSERTX(rule->src.addr.ipvany.ip.s_addr   == htonl(0));
      SASSERTX(rule->src.addr.ipvany.mask.s_addr == htonl(0));
   }

   if (rule->dst.atype == SOCKS_ADDR_IPVANY) {
      SASSERTX(rule->dst.addr.ipvany.ip.s_addr   == htonl(0));
      SASSERTX(rule->dst.addr.ipvany.mask.s_addr == htonl(0));
   }

#if BAREFOOTD
   SASSERTX(rule->type != object_srule);

   if (rule->type == object_crule) {
      if (rule->dst.atype == SOCKS_ADDR_IPV4)
        SASSERTX(rule->dst.addr.ipv4.mask.s_addr == htonl(IPV4_FULLNETMASK));
      else if (rule->dst.atype == SOCKS_ADDR_IPV6)
        SASSERTX(rule->dst.addr.ipv6.maskbits    == IPV6_NETMASKBITS);
   }
#endif /* BAREFOOTD */

   for (methodi = 0; methodi < ELEMENTS(methodv); ++methodi) {
      for (i = 0; i < methodc[methodi]; ++i) {
         if (!methodisvalid(methodv[methodi][i], type[methodi]))
            yyerrorx("method %s is not valid for %ss",
                     method2string(methodv[methodi][i]),
                     objecttype2string(type[methodi]));
      }
   }

   switch (rule->type) {
      case object_crule:
#if HAVE_SOCKS_HOSTID
      case object_hrule:
#endif /* HAVE_SOCKS_HOSTID */

         /* 
          * we always set AUTHMETHOD_NONE ourselves if the user does not 
          * set anything. 
          */
         SASSERTX(rule->state.cmethodc > 0);
         break;

#if HAVE_SOCKS_RULES
      case object_srule:
         if (rule->state.smethodc == 0) {
            if (isreplycommandonly(&rule->state.command)
            &&  !sockscf.srchost.checkreplyauth) {
               /* 
                * don't require user to specify a method for reply-only 
                * rules as normally noting can be gotten from there.
                */
               ; 
            }
            else
               yywarnx("%s: %s #%lu sets no socks authentication methods, not "
                       "even the method \"none\".  This means the client will "
                       "never be permitted.  Perhaps this is not intended?",
                       function,
                       objecttype2string(rule->type),
                       (unsigned long)rule->number);
         }

         break;
#endif /* HAVE_SOCKS_RULES */


      default:
         SERRX(rule->type);
   }

   if (rulerequires(rule, username)) {
      const int *methodv;
      size_t methodc;
      
      switch (rule->type) {
         case object_crule:
#if HAVE_SOCKS_HOSTID
         case object_hrule:
#endif /* HAVE_SOCKS_HOSTID */
            methodv = rule->state.cmethodv;
            methodc = rule->state.cmethodc;
            break;

         case object_srule:
            methodv = rule->state.smethodv;
            methodc = rule->state.smethodc;
            break;

         default:
            SERRX(object_srule);
      }


      if (rule->state.command.udpreply)
         if (isreplycommandonly(&rule->state.command)
         || sockscf.srchost.checkreplyauth)
            yyerrorx("udp replies can not provide any usernames, yet the "
                     "settings in this rule require it");

      /* 
       * also check that all methods given in the rule provide usernames.
       */

      for (i = 0; i < methodc; ++i) {
         if (!methodcanprovide(methodv[i], username))
            yyerrorx("the settings in this %s requires the client to "
                     "provide a user/group-name in some way, but the "
                     "method \"%s\", which is implicitly or explicitly "
                     "specified in this rule, cannot be depended on to "
                     "provide this information",
                     objecttype2string(rule->type),
                     method2string(methodv[i]));
      }
   }

   if (rule->rdr_from.atype != SOCKS_ADDR_NOTSET) {
      switch (rule->rdr_from.atype) {
         case SOCKS_ADDR_IPV4:
         case SOCKS_ADDR_IPV6:
         case SOCKS_ADDR_IFNAME:
         case SOCKS_ADDR_DOMAIN:
            break;

         default:
            yyerrorx("redirect from address can not be a %s",
                     atype2string(rule->rdr_from.atype));
      }

      ruleaddr          = rule->rdr_from;
      ruleaddr.port.tcp = htons(0); /* any port is good for testing. */

      if (!addrisbindable(&ruleaddr)) {
         char addr[MAXRULEADDRSTRING];

         yyerrorx("address %s in rule #%lu is not bindable",
                  ruleaddr2string(&ruleaddr, 0, addr, sizeof(addr)),
                  (unsigned long)rule->number);
      }
   }

   if (rule->rdr_to.atype != SOCKS_ADDR_NOTSET) {
      switch (rule->rdr_to.atype) {
         case SOCKS_ADDR_IPV4:
         case SOCKS_ADDR_IPV6:
         case SOCKS_ADDR_DOMAIN:
            break;

         default:
            yyerrorx("redirect to a %s-type address is not supported "
                     "(or meaningful?)",
                     atype2string(rule->rdr_to.atype));
      }
   }

   /* check socket options and warn if something does not look right. */
   if (rule->socketoptionv != NULL
#if !HAVE_SOCKS_RULES
   && rule->type != object_srule /*
                                  * socks-rules are autogenerated based on
                                  * client-rule and will contain most of the ,
                                  * same, so avoid duplicate warnings by only 
                                  * checking the options when checking the 
                                  * crule.
                                  */
#endif /* !HAVE_SOCKS_RULES */
   ) {
      for (i = 0; i < rule->socketoptionc; ++i) {
         if (rule->type != object_srule) {
#if HAVE_SOCKS_RULES
            if (!rule->socketoptionv[i].isinternalside)
               yyerrorx("can not set socket options for the external side in "
                        "a %s, as there is no external side set up "
                        "yet at this point.  Needs to be set in the "
                        "corresponding socks-rule if necessary",
                        objecttype2string(rule->type));
#else
            /* reusing the client-rule as a srule later. */
#endif
         }

         if (rule->socketoptionv[i].info == NULL)
            continue;
      }
   }
}

static void
showlog(log)
   const log_t *log;
{
   char buf[1024];

   slog(LOG_DEBUG, "log: %s", logs2string(log, buf, sizeof(buf)));
}

int
rule_inheritoruse(from, cinfo_from, to, cinfo_to, sidesconnected, emsg, emsglen)
   struct rule_t *from;
   const clientinfo_t *cinfo_from;
   struct rule_t *to;
   const clientinfo_t *cinfo_to;
   const size_t sidesconnected;
   char *emsg;
   const size_t emsglen;
{
   const char *function = "rule_inheritoruse()";
   char fromstr[512], tostr[sizeof(fromstr)];
   int rc, use_to, unuse_from,
       to_attached = SHMEM_NONE, from_attached = SHMEM_NONE;

#define SET_USEUNUSE(failedalready, _from, _cinfo_from, _unuse_from,           \
                     _to, _cinfo_to, _use_to,                                  \
                     inheritablefield,  idfield, type)                         \
do {                                                                           \
   (_unuse_from) = (_use_to) = 0;                                              \
                                                                               \
   if ((failedalready)) {                                                      \
      if ((_from)->idfield != 0)                                               \
         (_unuse_from) = 1;                                                    \
                                                                               \
      slog(LOG_DEBUG, "%s: unuse_from? %s  use_to? %s",                        \
           function, (_unuse_from) ? "yes" : "no", (_use_to) ? "yes" : "no");  \
                                                                               \
      break;                                                                   \
   }                                                                           \
                                                                               \
   if ((_from)->idfield == 0) {                                                \
      if ((_to)->idfield != 0)                                                 \
         (_use_to) = 1;                                                        \
   }                                                                           \
   else {                                                                      \
      if ((_from)->inheritablefield) {                                         \
         if (memcmp((_cinfo_from), (_cinfo_to), sizeof(*(_cinfo_to))) == 0     \
         && ((_to)->idfield == 0  || (_to)->idfield == (_from)->idfield)) {    \
            log_shmeminherit((_from), (_to), (type), (_to)->idfield != 0);     \
            SHMEM_MOVE((_from), (_to), (type));                                \
         }                                                                     \
         else {                                                                \
            (_unuse_from) = 1;                                                 \
            log_shmeminherit((_from), (_to), (type), 1);                       \
            (_use_to) = 1;                                                     \
         }                                                                     \
      }                                                                        \
      else {                                                                   \
         (_unuse_from) = 1;                                                    \
                                                                               \
         if ((_to)->idfield != 0)                                              \
            (_use_to) = 1;                                                     \
      }                                                                        \
   }                                                                           \
                                                                               \
   slog(LOG_DEBUG, "%s: unuse_from? %s  use_to? %s",                           \
        function, (_unuse_from) ? "yes" : "no", (_use_to) ? "yes" : "no");     \
} while (/* CONSTCOND */ 0)

#define ATTACH_IF_NEEDED(_rule, _attr, _type, _rule_attached, _rc)             \
do {                                                                           \
   if ((_rule)->_attr == NULL) {                                               \
      if (sockd_shmat((_rule), (_type)) == 0)                                  \
         *(_rule_attached) |= (_type);                                         \
      else                                                                     \
         *(_rc) = -1;                                                          \
   }                                                                           \
} while (/* CONSTCOND */ 0)


   snprintf(fromstr, sizeof(fromstr),
            "from-rule before (cinfo: %s)",
            clientinfo2string(cinfo_from, NULL, 0));

   snprintf(tostr, sizeof(tostr),
            "to-rule before (cinfo: %s)",
            clientinfo2string(cinfo_to, NULL, 0));

   log_ruleinfo_shmid(from, function, fromstr);
   log_ruleinfo_shmid(to, function, tostr);

   rc = 0;

   /*
    * Session-settings to inherit?  Check this first since that's the one
    * that can fail.
    */

   log_inheritable(function,
                   from,
                   from->ss_shmid,
                   "session",
                   from->ss_shmid == 0 ? 0 : from->ss_isinheritable);

   SET_USEUNUSE(rc == -1,
                from,
                cinfo_from,
                unuse_from,
                to,
                cinfo_to,
                use_to,
                ss_isinheritable,
                ss_shmid,
                SHMEM_SS);

   if (unuse_from) {
      ATTACH_IF_NEEDED(from, ss, SHMEM_SS, &from_attached, &rc);

      if (from->ss != NULL)
         session_unuse(from->ss, cinfo_from, sockscf.shmemfd);
   }

   if (use_to) {
      ATTACH_IF_NEEDED(to, ss, SHMEM_SS, &to_attached, &rc);

      if (to->ss != NULL) {
         if (!session_use(to->ss, cinfo_to, sockscf.shmemfd, emsg, emsglen)) {
            SHMEM_CLEAR(to, SHMEM_SS, 1); /* could not be used. */
            rc = -1;
         }
      }
   }

   /*
    * BW-settings to inherit?
    */

   log_inheritable(function,
                   from,
                   from->bw_shmid,
                   "bandwidth",
                   from->bw_shmid == 0 ? 0 : from->bw_isinheritable);

   SET_USEUNUSE(rc == -1,
                from,
                cinfo_from,
                unuse_from,
                to,
                cinfo_to,
                use_to,
                bw_isinheritable,
                bw_shmid,
                SHMEM_BW);

   if (unuse_from) {
      ATTACH_IF_NEEDED(from, bw, SHMEM_BW, &from_attached, &rc);

      if (from->bw != NULL)
         bw_unuse(from->bw, cinfo_from, sockscf.shmemfd);
   }

   if (use_to) {
      ATTACH_IF_NEEDED(to, bw, SHMEM_BW, &to_attached, &rc);

      if (to->bw != NULL)
         bw_use(to->bw, cinfo_to, sockscf.shmemfd);
   }


   if (rc == 0) /* only inherit alarms on success. */
      /* 
       * Do this before we potentially clear fields in the code below.
       */
      alarm_inherit(from, cinfo_from, to, cinfo_to, sidesconnected);


   /*
    * Monitor-settings to inherit?
    */

   log_inheritable(function,
                   from,
                   from->mstats_shmid,
                   "mstats",
                   from->mstats_shmid == 0 ? 0 : from->mstats_isinheritable);

   SET_USEUNUSE(rc == -1,
                from,
                cinfo_from,
                unuse_from,
                to,
                cinfo_to,
                use_to,
                mstats_isinheritable,
                mstats_shmid,
                SHMEM_MONITOR);

   if (unuse_from) {
      ATTACH_IF_NEEDED(from, mstats, SHMEM_MONITOR, &from_attached, &rc);

      if (from->mstats != NULL)
         monitor_unuse(from->mstats, cinfo_from, sockscf.shmemfd);
   }

   if (use_to) {
      ATTACH_IF_NEEDED(to, mstats, SHMEM_MONITOR, &to_attached, &rc);

      if (to->mstats != NULL)
         monitor_use(to->mstats, cinfo_to, sockscf.shmemfd);
   }

   /*
    * Redirect settings to inherit?
    * Like the other settings, we inherit all or nothing.
    */
   if (rc != -1) {
      if (from->rdr_from.atype != SOCKS_ADDR_NOTSET
      ||  from->rdr_to.atype   != SOCKS_ADDR_NOTSET) {
         if (to->rdr_from.atype == SOCKS_ADDR_NOTSET
         &&  to->rdr_to.atype   == SOCKS_ADDR_NOTSET) {
            slog(LOG_DEBUG,
                 "%s: %s #%lu inherits redirect settings from %s #%lu",
                 function,
                 objecttype2string(to->type),
                 (unsigned long)to->number,
                 objecttype2string(from->type),
                 (unsigned long)from->number);

            to->rdr_from = from->rdr_from;
            to->rdr_to   = from->rdr_to;
         }
         else {
            slog(LOG_DEBUG,
                 "%s: %s #%lu overrides redirect settings of %s #%lu",
                 function,
                 objecttype2string(to->type),
                 (unsigned long)to->number,
                 objecttype2string(from->type),
                 (unsigned long)from->number);

            bzero(&from->rdr_from, sizeof(from->rdr_from));
            bzero(&from->rdr_to, sizeof(from->rdr_to));
         }
      }
      /* else; nothing to inherit. */
   }

   if (from_attached != SHMEM_NONE)
      sockd_shmdt(from, from_attached);

   if (to_attached != SHMEM_NONE)
      sockd_shmdt(to, to_attached);

   /*
    * what should be inherited has been inherited; no need to reference
    * any limits in "from" ever again.
    */
   SHMEM_CLEAR(from, SHMEM_ALL, 1);

   if (rc == -1) {
      /*
       * Did not use anything in to.
       */
      SASSERTX(to->bw     == NULL);
      SASSERTX(to->mstats == NULL);
      SASSERTX(to->ss     == NULL);

      SHMEM_CLEAR(to, SHMEM_ALL, 1);

      to->verdict = VERDICT_BLOCK;
   }

   log_ruleinfo_shmid(from, function, "from-rule after");
   log_ruleinfo_shmid(to, function, "to-rule after");

   return rc;
}

static void
log_shmeminherit(from, to, type, isoverride)
   const rule_t *from;
   const rule_t *to;
   const int type;
   const int isoverride;
{
   const char *function = "log_shmeminherit()";
   const char *settings;
   long shmid_from, shmid_to;

   if (type == SHMEM_BW) {
      settings   = "bandwidth";
      shmid_from = from->bw_shmid;
      shmid_to   = to->bw_shmid;
   }
   else if (type == SHMEM_MONITOR) {
      settings   = "monitor";
      shmid_from = from->mstats_shmid;
      shmid_to   = to->mstats_shmid;
   }
   else if (type == SHMEM_SS) {
      settings   = "session";
      shmid_from = from->ss_shmid;
      shmid_to   = to->ss_shmid;
   }
   else
      SERRX(type);

   if (isoverride)
      slog(LOG_DEBUG, "%s: %s #%lu with shmid %ld overrides %s "
                      "settings of %s #%lu with shmid %ld",
                      function,
                      objecttype2string(to->type),
                      (unsigned long)to->number,
                      shmid_to,
                      settings,
                      objecttype2string(from->type),
                      (unsigned long)from->number,
                      shmid_from);
   else
      slog(LOG_DEBUG,
           "%s: %s #%lu inherits %s settings from %s #%lu using shmid %ld",
           function,
           objecttype2string(to->type),
           (unsigned long)to->number,
           settings,
           objecttype2string(from->type),
           (unsigned long)from->number,
           shmid_from);
}

static void
log_inheritable(prefix, rule, shmid, settings, inheritable)
   const char *prefix;
   const rule_t *rule;
   const unsigned long shmid;
   const char *settings;
   const int inheritable;
{

   if (shmid == 0)
      slog(LOG_DEBUG,
           "%s: no %s settings in %s #%lu to consider for inheritance",
           prefix,
           settings,
           objecttype2string(rule->type),
           (unsigned long)rule->number);
   else
      slog(LOG_DEBUG,
           "%s: %s settings (shmid %ld) in %s #%lu marked as %sinheritable",
           prefix,
           settings,
           shmid,
           objecttype2string(rule->type),
           (unsigned long)rule->number,
           inheritable ? "" : "not ");
}

void
setcommandprotocol(type, commands, protocols)
   const objecttype_t type;
   command_t  *commands;
   protocol_t *protocols;
{
   serverstate_t zstate;

   /*
    * protocol and commands are two sides of the same coin.
    * If only protocol is set, set all commands that can apply to that
    * protocol.
    * If only command is set, set all protocols that can apply to that
    * command.
    * If none are set, set all protocols and commands.
    * If both are set, don't touch; user has explicitly set what he wants.
    */

   bzero(&zstate, sizeof(zstate));

   if (memcmp(&zstate.protocol, protocols, sizeof(zstate.protocol)) != 0
   && memcmp(&zstate.command, commands, sizeof(zstate.command))     == 0) {
      /*
       * only protocol is set.  Add all applicable commands. 
       */

      if (type == object_srule || type == object_monitor) {
         if (protocols->tcp) {
            /*
             * All commands, except the udp-spesific.
             */
            memset(commands, UCHAR_MAX, sizeof(*commands));
            commands->udpassociate = 0;
            commands->udpreply     = 0;
         }

         if (protocols->udp) {
            commands->udpassociate = 1;
            commands->udpreply     = 1;
         }
      }
   }
   else if (memcmp(&zstate.command, commands, sizeof(zstate.command)) != 0
   && memcmp(&zstate.protocol, protocols, sizeof(zstate.protocol))    == 0) {
      /* 
       * only command is set.  Add all applicable protocols.
       */

      if (type == object_srule || type == object_monitor) {
         if (commands->bind
         ||  commands->bindreply
         ||  commands->connect)
            protocols->tcp = 1;

         if (commands->udpassociate
         ||  commands->udpreply
         ||  commands->connect)
            protocols->udp = 1;
      }
      else {
#if HAVE_NEGOTIATE_PHASE
         protocols->tcp = 1;
#else /* !HAVE_NEGOTIATE_PHASE. */
         memset(protocols, UCHAR_MAX, sizeof(*protocols));
#endif /* !HAVE_NEGOTIATE_PHASE. */
      }
   }
   else if (memcmp(&zstate.command, commands, sizeof(zstate.command)) == 0
   && memcmp(&zstate.protocol, protocols, sizeof(zstate.protocol))    == 0) {
      /*
       * nothing is set.  Set all. 
       */

      if (type == object_srule || type == object_monitor) {
         memset(protocols, UCHAR_MAX, sizeof(*protocols));
         memset(commands, UCHAR_MAX, sizeof(*commands));
      }
      else {
#if HAVE_NEGOTIATE_PHASE
         protocols->tcp = 1;
#else /* !HAVE_NEGOTIATE_PHASE. */
         memset(protocols, UCHAR_MAX, sizeof(*protocols));
#endif /* !HAVE_NEGOTIATE_PHASE. */
      }
   }
   else { 
      /* 
       * both are set.  Don't touch, but check it makes sense.
       */

      SASSERTX(memcmp(&zstate.command, commands, sizeof(*commands))    != 0);
      SASSERTX(memcmp(&zstate.protocol, protocols, sizeof(*protocols)) != 0);

      if ((commands->udpassociate || commands->udpreply)
      &&  !protocols->udp)
         yywarnx("%s specifies UDP-based commands, but does not enable the "
                 "UDP protocol.  This is probably not what was intended",
                 objecttype2string(type));

      if ((commands->bind || commands->bindreply || commands->connect)
      &&  !protocols->tcp)
         yywarnx("%s specifies TCP-based commands, but does not enable the "
                 "TCP protocol.  This is probably not what was intended",
                 objecttype2string(type));
   }
}

#if HAVE_SOCKS_HOSTID
int
hostidmatches(hostidc, hostidv, hostindex, addr, ruletype, number)
   const size_t hostidc;
   const struct in_addr *hostidv;
   const unsigned char hostindex;
   const ruleaddr_t *addr;
   const objecttype_t ruletype;
   const size_t number;
{
   const char *function = "hostidmatches()";
   sockshost_t hostid;

   hostid.atype = SOCKS_ADDR_IPV4;
   hostid.port  = htons(0);

   if (hostindex == 0) {
      size_t i;

      slog(LOG_DEBUG, "%s: %s #%lu specifies checking against all %u hostids",
           function,
           objecttype2string(ruletype),
           (unsigned long)number,
           (unsigned)hostidc);

      for (i = 0; i < hostidc; ++i) {
         hostid.addr.ipv4 = hostidv[i];
         if (addrmatch(addr, &hostid, NULL, SOCKS_TCP, 0))
            return 1;
      }

      return 0;
   }
   else { /* check a specific hostid index only. */
      if (hostindex > hostidc) {
         slog(LOG_DEBUG, 
              "%s: %s #%lu specifies checking against hostid index %d, "
              "but the number of hostids set on the connection is %lu, "
              "so it can not match",
              function,
              objecttype2string(ruletype),
              (unsigned long)number,
              hostindex,
              (unsigned long)hostidc);

         return 0;
      }

      SASSERTX(hostindex <= hostidc);

      hostid.addr.ipv4 = hostidv[hostindex - 1];
      slog(LOG_DEBUG, "%s: checking against hostid address %s",
                      function, sockshost2string(&hostid, NULL, 0));

      return addrmatch(addr, &hostid, NULL, SOCKS_TCP, 0);
   }
}
#endif /* HAVE_SOCKS_HOSTID */
