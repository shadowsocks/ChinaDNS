/*
 * Copyright (c) 1983, 1987, 1989
 *    The Regents of the University of California.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 4. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

/*
 * Portions Copyright (c) 1996-1999 by Internet Software Consortium.
 *
 * Permission to use, copy, modify, and distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND INTERNET SOFTWARE CONSORTIUM DISCLAIMS
 * ALL WARRANTIES WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL INTERNET SOFTWARE
 * CONSORTIUM BE LIABLE FOR ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL
 * DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR
 * PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS
 * ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS
 * SOFTWARE.
 */

/*
 *	@(#)resolv.h	8.1 (Berkeley) 6/2/93
 *	$BINDId: resolv.h,v 8.31 2000/03/30 20:16:50 vixie Exp $
 */

#ifndef RESOLV_H
#define RESOLV_H

#ifndef __MINGW32__
#  include <netinet/in.h>
#else
#  include <winsock2.h>
#  include <ws2tcpip.h>
#endif

#include <stdio.h>
#include "nameser.h"

struct res_sym {
	int	number;			/* Identifying number, like T_MX */
	char *name;			/* Its symbolic name, like "MX" */
	char *humanname;	/* Its fun name, like "mail exchanger" */
};

/*
 * Resolver flags (used to be discrete per-module statics ints).
 */
#define	RES_F_VC	0x00000001	/* socket is TCP */
#define	RES_F_CONN	0x00000002	/* socket is connected */
#define RES_F_EDNS0ERR	0x00000004	/* EDNS0 caused errors */

/* res_findzonecut() options */
#define	RES_EXHAUSTIVE	0x00000001	/* always do all queries */

/*
 * Resolver options (keep these in synch with res_debug.c, please)
 */
#define RES_INIT	0x00000001	/* address initialized */
#define RES_DEBUG	0x00000002	/* print debug messages */
#define RES_AAONLY	0x00000004	/* authoritative answers only (!IMPL)*/
#define RES_USEVC	0x00000008	/* use virtual circuit */
#define RES_PRIMARY	0x00000010	/* query primary server only (!IMPL) */
#define RES_IGNTC	0x00000020	/* ignore trucation errors */
#define RES_RECURSE	0x00000040	/* recursion desired */
#define RES_DEFNAMES	0x00000080	/* use default domain name */
#define RES_STAYOPEN	0x00000100	/* Keep TCP socket open */
#define RES_DNSRCH	0x00000200	/* search up local domain tree */
#define	RES_INSECURE1	0x00000400	/* type 1 security disabled */
#define	RES_INSECURE2	0x00000800	/* type 2 security disabled */
#define	RES_NOALIASES	0x00001000	/* shuts off HOSTALIASES feature */
#define	RES_USE_INET6	0x00002000	/* use/map IPv6 in gethostbyname() */
#define RES_ROTATE	0x00004000	/* rotate ns list after each query */
#define	RES_NOCHECKNAME	0x00008000	/* do not check names for sanity (!IMPL) */
#define	RES_KEEPTSIG	0x00010000	/* do not strip TSIG records */
#define	RES_BLAST	0x00020000	/* blast all recursive servers */
#define RES_USEBSTRING	0x00040000	/* IPv6 reverse lookup with byte
					   strings */
#define RES_NOIP6DOTINT	0x00080000	/* Do not use .ip6.int in IPv6
					   reverse lookup */
#define RES_USE_EDNS0	0x00100000	/* Use EDNS0.  */
#define RES_SNGLKUP	0x00200000	/* one outstanding request at a time */
#define RES_SNGLKUPREOP	0x00400000	/* -"-, but open new socket for each
					   request */
#define RES_USE_DNSSEC	0x00800000	/* use DNSSEC using OK bit in OPT */
#define RES_NOTLDQUERY	0x01000000	/* Do not look up unqualified name
					   as a TLD.  */

#define RES_DEFAULT	(RES_RECURSE|RES_DEFNAMES|RES_DNSRCH|RES_NOIP6DOTINT)

/*
 * Resolver "pfcode" values.  Used by dig.
 */
#define RES_PRF_STATS	0x00000001
#define RES_PRF_UPDATE	0x00000002
#define RES_PRF_CLASS   0x00000004
#define RES_PRF_CMD	0x00000008
#define RES_PRF_QUES	0x00000010
#define RES_PRF_ANS	0x00000020
#define RES_PRF_AUTH	0x00000040
#define RES_PRF_ADD	0x00000080
#define RES_PRF_HEAD1	0x00000100
#define RES_PRF_HEAD2	0x00000200
#define RES_PRF_TTLID	0x00000400
#define RES_PRF_HEADX	0x00000800
#define RES_PRF_QUERY	0x00001000
#define RES_PRF_REPLY	0x00002000
#define RES_PRF_INIT	0x00004000
/*			0x00008000	*/

void fp_nquery(const u_char *, int, FILE *);
void fp_query(const u_char *, FILE *);
const char *hostalias (const char *);
void p_query(const u_char *);
void res_close(void);
int res_init(void);
int res_isourserver(const struct sockaddr_in *);
int res_mkquery(int, const char *, int, int, const u_char *,
                int, const u_char *, u_char *, int);
int res_query(const char *, int, int, u_char *, int);
int res_querydomain(const char *, const char *, int, int,
                    u_char *, int);
int res_search(const char *, int, int, u_char *, int);
int res_send(const u_char *, int, u_char *, int);

int res_hnok(const char *);
int res_ownok(const char *);
int res_mailok(const char *);
int res_dnok(const char *);
int sym_ston(const struct res_sym *, const char *, int *);
const char *sym_ntos(const struct res_sym *, int, int *);
const char *sym_ntop(const struct res_sym *, int, int *);
int b64_ntop(u_char const *, size_t, char *, size_t);
int b64_pton(char const *, u_char *, size_t);
int loc_aton(const char *__ascii, u_char *__binary);
const char *loc_ntoa(const u_char *__binary, char *__ascii);
int dn_skipname(const u_char *, const u_char *);
void putlong(uint32_t, u_char *);
void putshort(uint16_t, u_char *);
const char *p_class(int);
const char *p_time(uint32_t);
const char *p_type(int);
const char *p_rcode(int);
const u_char *p_cdnname(const u_char *, const u_char *, int, FILE *);
const u_char *p_cdname(const u_char *, const u_char *, FILE *);
const u_char *p_fqnname(const u_char *__cp, const u_char *__msg,
                        int, char *, int);
const u_char *p_fqname(const u_char *, const u_char *, FILE *);
const char *p_option(u_long __option);
char *p_secstodate(u_long);
int dn_count_labels(const char *);
int dn_comp(const char *, u_char *, int, u_char **, u_char **);
int dn_expand(const u_char *, const u_char *, const u_char *,
              char *, int);
u_int res_randomid(void);
int res_nameinquery(const char *, int, int,
                    const u_char *, const u_char *);
int res_queriesmatch(const u_char *, const u_char *,
                     const u_char *, const u_char *);
const char *p_section(int __section, int __opcode);

#endif // RESOLV_H
