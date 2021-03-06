/*	$OpenBSD: if_pfsync.h,v 1.35 2008/06/29 08:42:15 mcbride Exp $	*/

/*
 * Copyright (c) 2001 Michael Shalayeff
 * All rights reserved.
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
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR OR HIS RELATIVES BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF MIND, USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING
 * IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
 * THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef _NET_IF_PFSYNC_H_
#define _NET_IF_PFSYNC_H_

#include <sys/ioccom.h>

/*
 * pfvar.h is required to get struct pf_addr.  Also kdump and other utilities
 * blindly include header files to try to get all the ioctl constants and
 * buildworld will fail without this.  We need a better way XXX
 */
#ifndef _NET_PFVAR_H_
#include "pfvar.h"
#endif

/* Reserved SPI numbers */
#define	SPI_LOCAL_USE		0
#define	SPI_RESERVED_MIN	1
#define	SPI_RESERVED_MAX	255


#define PFSYNC_ID_LEN	sizeof(u_int64_t)

#define PFSYNC_FLAG_COMPRESS 	0x01
#define PFSYNC_FLAG_STALE	0x02

struct pfsync_tdb {
	u_int32_t	spi;
	union sockaddr_union dst;
	u_int32_t	rpl;
	u_int64_t	cur_bytes;
	u_int8_t	sproto;
	u_int8_t	updates;
	u_int8_t	pad[2];
} __packed;

struct pfsync_state_upd {
	u_int32_t		id[2];
	struct pfsync_state_peer	src;
	struct pfsync_state_peer	dst;
	u_int32_t		creatorid;
	u_int32_t		expire;
	u_int8_t		timeout;
	u_int8_t		updates;
	u_int8_t		pad[6];
} __packed;

struct pfsync_state_del {
	u_int32_t		id[2];
	u_int32_t		creatorid;
	struct {
		u_int8_t	state;
	} src;
	struct {
		u_int8_t	state;
	} dst;
	u_int8_t		pad[2];
} __packed;

struct pfsync_state_upd_req {
	u_int32_t		id[2];
	u_int32_t		creatorid;
	u_int32_t		pad;
} __packed;

struct pfsync_state_clr {
	char			ifname[IFNAMSIZ];
	u_int32_t		creatorid;
	u_int32_t		pad;
} __packed;

struct pfsync_state_bus {
	u_int32_t		creatorid;
	u_int32_t		endtime;
	u_int8_t		status;
#define PFSYNC_BUS_START	1
#define PFSYNC_BUS_END		2
	u_int8_t		pad[7];
} __packed;

/*
 * Names for PFSYNC sysctl objects
 */
#define	PFSYNCCTL_STATS		1	/* PFSYNC stats */
#define	PFSYNCCTL_MAXID		2

#define	PFSYNCCTL_NAMES { \
	{ 0, 0 }, \
	{ "stats", CTLTYPE_STRUCT }, \
}

#ifdef _KERNEL

union sc_statep {
	struct pfsync_state	*s;
	struct pfsync_state_upd	*u;
	struct pfsync_state_del	*d;
	struct pfsync_state_clr	*c;
	struct pfsync_state_bus	*b;
	struct pfsync_state_upd_req	*r;
};

union sc_tdb_statep {
	struct pfsync_tdb	*t;
};

extern int	pfsync_sync_ok;

struct pfsync_softc {
	struct ifnet		 sc_if;
	struct ifnet		*sc_sync_ifp;

	struct ip_moptions	 sc_imo;
	struct callout		 sc_tmo;
	struct callout		 sc_tdb_tmo;
	struct callout		 sc_bulk_tmo;
	struct callout		 sc_bulkfail_tmo;
	struct in_addr		 sc_sync_peer;
	struct in_addr		 sc_sendaddr;
	struct mbuf		*sc_mbuf;	/* current cumulative mbuf */
	struct mbuf		*sc_mbuf_net;	/* current cumulative mbuf */
    	struct mbuf		*sc_mbuf_tdb;	/* dito for TDB updates */
	union sc_statep		 sc_statep;
	union sc_statep		 sc_statep_net;
	union sc_tdb_statep	 sc_statep_tdb;
	u_int32_t		 sc_ureq_received;
	u_int32_t		 sc_ureq_sent;
	struct pf_state		*sc_bulk_send_next;
	struct pf_state		*sc_bulk_terminator;
	int			 sc_bulk_send_cpu;
	int			 sc_bulk_terminator_cpu;
	int			 sc_bulk_tries;
	int			 sc_maxcount;	/* number of states in mtu */
	int			 sc_maxupdates;	/* number of updates/state */
	LIST_ENTRY(pfsync_softc) sc_next;
};

extern struct pfsync_softc	*pfsyncif;
#endif


struct pfsync_header {
	u_int8_t version;
#define	PFSYNC_VERSION	4
	u_int8_t af;
	u_int8_t action;
#define	PFSYNC_ACT_CLR		0	/* clear all states */
#define	PFSYNC_ACT_INS		1	/* insert state */
#define	PFSYNC_ACT_UPD		2	/* update state */
#define	PFSYNC_ACT_DEL		3	/* delete state */
#define	PFSYNC_ACT_UPD_C	4	/* "compressed" state update */
#define	PFSYNC_ACT_DEL_C	5	/* "compressed" state delete */
#define	PFSYNC_ACT_INS_F	6	/* insert fragment */
#define	PFSYNC_ACT_DEL_F	7	/* delete fragments */
#define	PFSYNC_ACT_UREQ		8	/* request "uncompressed" state */
#define PFSYNC_ACT_BUS		9	/* Bulk Update Status */
#define PFSYNC_ACT_TDB_UPD	10	/* TDB replay counter update */
#define	PFSYNC_ACT_MAX		11
	u_int8_t count;
	u_int8_t pf_chksum[PF_MD5_DIGEST_LENGTH];
} __packed;

#define PFSYNC_BULKPACKETS	1	/* # of packets per timeout */
#define PFSYNC_MAX_BULKTRIES	12
#define PFSYNC_HDRLEN	sizeof(struct pfsync_header)
#define	PFSYNC_ACTIONS \
	"CLR ST", "INS ST", "UPD ST", "DEL ST", \
	"UPD ST COMP", "DEL ST COMP", "INS FR", "DEL FR", \
	"UPD REQ", "BLK UPD STAT", "TDB UPD"

#define PFSYNC_DFLTTL		255

struct pfsyncstats {
	u_int64_t	pfsyncs_ipackets;	/* total input packets, IPv4 */
	u_int64_t	pfsyncs_ipackets6;	/* total input packets, IPv6 */
	u_int64_t	pfsyncs_badif;		/* not the right interface */
	u_int64_t	pfsyncs_badttl;		/* TTL is not PFSYNC_DFLTTL */
	u_int64_t	pfsyncs_hdrops;		/* packets shorter than hdr */
	u_int64_t	pfsyncs_badver;		/* bad (incl unsupp) version */
	u_int64_t	pfsyncs_badact;		/* bad action */
	u_int64_t	pfsyncs_badlen;		/* data length does not match */
	u_int64_t	pfsyncs_badauth;	/* bad authentication */
	u_int64_t	pfsyncs_stale;		/* stale state */
	u_int64_t	pfsyncs_badval;		/* bad values */
	u_int64_t	pfsyncs_badstate;	/* insert/lookup failed */

	u_int64_t	pfsyncs_opackets;	/* total output packets, IPv4 */
	u_int64_t	pfsyncs_opackets6;	/* total output packets, IPv6 */
	u_int64_t	pfsyncs_onomem;		/* no memory for an mbuf */
	u_int64_t	pfsyncs_oerrors;	/* ip output error */
};

/*
 * Configuration structure for SIOCSETPFSYNC SIOCGETPFSYNC
 */
struct pfsyncreq {
	char		 pfsyncr_syncdev[IFNAMSIZ];
	struct in_addr	 pfsyncr_syncpeer;
	int		 pfsyncr_maxupdates;
	int		 pfsyncr_authlevel;
};
#define SIOCSETPFSYNC	_IOW('i', 247, struct ifreq)
#define SIOCGETPFSYNC	_IOWR('i', 248, struct ifreq)

#ifdef _KERNEL
void			pfsync_input(struct mbuf *, ...);
int			pfsync_clear_states(u_int32_t, char *);
int			pfsync_pack_state(u_int8_t, struct pf_state *, int);
int			pfsync_sysctl(int *, u_int,  void *, size_t *,
			    void *, size_t);
void			pfsync_state_export(struct pfsync_state *,
			    struct pf_state *);

#define	PFSYNC_SI_IOCTL		0x01
#define	PFSYNC_SI_CKSUM		0x02
int			pfsync_state_import(struct pfsync_state *, u_int8_t);

#define pfsync_insert_state(st)	do {				\
	if ((st->rule.ptr->rule_flag & PFRULE_NOSYNC) ||	\
	    (st->key[PF_SK_WIRE]->proto == IPPROTO_PFSYNC))	\
		st->sync_flags |= PFSTATE_NOSYNC;		\
	else if (!st->sync_flags)				\
		pfsync_pack_state(PFSYNC_ACT_INS, (st), 	\
		    PFSYNC_FLAG_COMPRESS);			\
	st->sync_flags &= ~PFSTATE_FROMSYNC;			\
} while (0)
#define pfsync_update_state(st) do {				\
	if (!(st)->sync_flags)					\
		pfsync_pack_state(PFSYNC_ACT_UPD, (st), 	\
		    PFSYNC_FLAG_COMPRESS);			\
	(st)->sync_flags &= ~PFSTATE_FROMSYNC;			\
} while (0)
#define pfsync_delete_state(st) do {				\
	if (!(st)->sync_flags)					\
		pfsync_pack_state(PFSYNC_ACT_DEL, (st),		\
		    PFSYNC_FLAG_COMPRESS);			\
} while (0)
#endif

#endif /* _NET_IF_PFSYNC_H_ */
