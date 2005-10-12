/*
 * Copyright (c) 2003,2004 The DragonFly Project.  All rights reserved.
 * 
 * This code is derived from software contributed to The DragonFly Project
 * by Matthew Dillon <dillon@backplane.com>
 * 
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in
 *    the documentation and/or other materials provided with the
 *    distribution.
 * 3. Neither the name of The DragonFly Project nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific, prior written permission.
 * 
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 * FOR A PARTICULAR PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE
 * COPYRIGHT HOLDERS OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED
 * AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT
 * OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 * 
 * Copyright (c) 1997, 1998 Poul-Henning Kamp <phk@FreeBSD.org>
 * Copyright (c) 1982, 1986, 1991, 1993
 *	The Regents of the University of California.  All rights reserved.
 * (c) UNIX System Laboratories, Inc.
 * All or some portions of this file are derived from material licensed
 * to the University of California by American Telephone and Telegraph
 * Co. or Unix System Laboratories, Inc. and are reproduced herein with
 * the permission of UNIX System Laboratories, Inc.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *	This product includes software developed by the University of
 *	California, Berkeley and its contributors.
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
 *
 *	@(#)kern_clock.c	8.5 (Berkeley) 1/21/94
 * $FreeBSD: src/sys/kern/kern_clock.c,v 1.105.2.10 2002/10/17 13:19:40 maxim Exp $
 * $DragonFly: src/sys/kern/kern_clock.c,v 1.48 2005/10/12 05:17:38 sephe Exp $
 */

#include "opt_ntp.h"

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/callout.h>
#include <sys/kernel.h>
#include <sys/kinfo.h>
#include <sys/proc.h>
#include <sys/malloc.h>
#include <sys/resourcevar.h>
#include <sys/signalvar.h>
#include <sys/timex.h>
#include <sys/timepps.h>
#include <vm/vm.h>
#include <sys/lock.h>
#include <vm/pmap.h>
#include <vm/vm_map.h>
#include <sys/sysctl.h>
#include <sys/thread2.h>

#include <machine/cpu.h>
#include <machine/limits.h>
#include <machine/smp.h>

#ifdef GPROF
#include <sys/gmon.h>
#endif

#ifdef DEVICE_POLLING
#ifndef DEVICE_POLLING_FREQ_MAX
#define DEVICE_POLLING_FREQ_MAX		20000
#endif
#define DEVICE_POLLING_FREQ_DEFAULT	1000

extern void init_device_poll(void);
extern void hardclock_device_poll(void);

static int sysctl_pollhz(SYSCTL_HANDLER_ARGS);

static struct systimer gd0_pollclock;
static int pollhz = DEVICE_POLLING_FREQ_DEFAULT;

TUNABLE_INT("kern.polling.pollhz", &pollhz);
SYSCTL_DECL(_kern_polling);
SYSCTL_PROC(_kern_polling, OID_AUTO, pollhz, CTLTYPE_INT | CTLFLAG_RW,
	    0, 0, sysctl_pollhz, "I", "Device polling frequency");

static int
sysctl_pollhz(SYSCTL_HANDLER_ARGS)
{
	int error, phz;

	crit_enter();
	phz = pollhz;
	crit_exit();

	error = sysctl_handle_int(oidp, &phz, 0, req);
	if (error || req->newptr == NULL)
		return error;
	if (phz <= 0)
		return EINVAL;
	else if (phz > DEVICE_POLLING_FREQ_MAX)
		phz = DEVICE_POLLING_FREQ_MAX;

	crit_enter();
	pollhz = phz;
	gd0_pollclock.periodic = sys_cputimer->fromhz(phz);
	crit_exit();
	return 0;
}
#endif /* DEVICE_POLLING */

static void initclocks (void *dummy);
SYSINIT(clocks, SI_SUB_CLOCKS, SI_ORDER_FIRST, initclocks, NULL)

/*
 * Some of these don't belong here, but it's easiest to concentrate them.
 * Note that cpu_time counts in microseconds, but most userland programs
 * just compare relative times against the total by delta.
 */
struct kinfo_cputime cputime_percpu[MAXCPU];
#ifdef SMP
static int
sysctl_cputime(SYSCTL_HANDLER_ARGS)
{
	int cpu, error = 0;
	size_t size = sizeof(struct kinfo_cputime);

	for (cpu = 0; cpu < ncpus; ++cpu) {
		if ((error = SYSCTL_OUT(req, &cputime_percpu[cpu], size)))
			break;
	}

	return (error);
}
SYSCTL_PROC(_kern, OID_AUTO, cputime, (CTLTYPE_OPAQUE|CTLFLAG_RD), 0, 0,
	sysctl_cputime, "S,kinfo_cputime", "CPU time statistics");
#else
SYSCTL_STRUCT(_kern, OID_AUTO, cputime, CTLFLAG_RD, &cpu_time, kinfo_cputime,
    "CPU time statistics");
#endif

/*
 * boottime is used to calculate the 'real' uptime.  Do not confuse this with
 * microuptime().  microtime() is not drift compensated.  The real uptime
 * with compensation is nanotime() - bootime.  boottime is recalculated
 * whenever the real time is set based on the compensated elapsed time
 * in seconds (gd->gd_time_seconds).
 *
 * The gd_time_seconds and gd_cpuclock_base fields remain fairly monotonic.
 * Slight adjustments to gd_cpuclock_base are made to phase-lock it to
 * the real time.
 */
struct timespec boottime;	/* boot time (realtime) for reference only */
time_t time_second;		/* read-only 'passive' uptime in seconds */

/*
 * basetime is used to calculate the compensated real time of day.  The
 * basetime can be modified on a per-tick basis by the adjtime(), 
 * ntp_adjtime(), and sysctl-based time correction APIs.
 *
 * Note that frequency corrections can also be made by adjusting
 * gd_cpuclock_base.
 *
 * basetime is a tail-chasing FIFO, updated only by cpu #0.  The FIFO is
 * used on both SMP and UP systems to avoid MP races between cpu's and
 * interrupt races on UP systems.
 */
#define BASETIME_ARYSIZE	16
#define BASETIME_ARYMASK	(BASETIME_ARYSIZE - 1)
static struct timespec basetime[BASETIME_ARYSIZE];
static volatile int basetime_index;

static int
sysctl_get_basetime(SYSCTL_HANDLER_ARGS)
{
	struct timespec *bt;
	int error;
	int index;

	/*
	 * Because basetime data and index may be updated by another cpu,
	 * a load fence is required to ensure that the data we read has
	 * not been speculatively read relative to a possibly updated index.
	 */
	index = basetime_index;
	cpu_lfence();
	bt = &basetime[index];
	error = SYSCTL_OUT(req, bt, sizeof(*bt));
	return (error);
}

SYSCTL_STRUCT(_kern, KERN_BOOTTIME, boottime, CTLFLAG_RD,
    &boottime, timespec, "System boottime");
SYSCTL_PROC(_kern, OID_AUTO, basetime, CTLTYPE_STRUCT|CTLFLAG_RD, 0, 0,
    sysctl_get_basetime, "S,timespec", "System basetime");

static void hardclock(systimer_t info, struct intrframe *frame);
static void statclock(systimer_t info, struct intrframe *frame);
static void schedclock(systimer_t info, struct intrframe *frame);
#ifdef DEVICE_POLLING
static void pollclock(systimer_t info, struct intrframe *frame);
#endif
static void getnanotime_nbt(struct timespec *nbt, struct timespec *tsp);

int	ticks;			/* system master ticks at hz */
int	clocks_running;		/* tsleep/timeout clocks operational */
int64_t	nsec_adj;		/* ntpd per-tick adjustment in nsec << 32 */
int64_t	nsec_acc;		/* accumulator */

/* NTPD time correction fields */
int64_t	ntp_tick_permanent;	/* per-tick adjustment in nsec << 32 */
int64_t	ntp_tick_acc;		/* accumulator for per-tick adjustment */
int64_t	ntp_delta;		/* one-time correction in nsec */
int64_t ntp_big_delta = 1000000000;
int32_t	ntp_tick_delta;		/* current adjustment rate */
int32_t	ntp_default_tick_delta;	/* adjustment rate for ntp_delta */
time_t	ntp_leap_second;	/* time of next leap second */
int	ntp_leap_insert;	/* whether to insert or remove a second */

/*
 * Finish initializing clock frequencies and start all clocks running.
 */
/* ARGSUSED*/
static void
initclocks(void *dummy)
{
	cpu_initclocks();
#ifdef DEVICE_POLLING
	init_device_poll();
#endif
	/*psratio = profhz / stathz;*/
	initclocks_pcpu();
	clocks_running = 1;
}

/*
 * Called on a per-cpu basis
 */
void
initclocks_pcpu(void)
{
	struct globaldata *gd = mycpu;

	crit_enter();
	if (gd->gd_cpuid == 0) {
	    gd->gd_time_seconds = 1;
	    gd->gd_cpuclock_base = sys_cputimer->count();

#ifdef DEVICE_POLLING
	    if (pollhz <= 0)
		pollhz = DEVICE_POLLING_FREQ_DEFAULT;
	    else if (pollhz > DEVICE_POLLING_FREQ_MAX)
		pollhz = DEVICE_POLLING_FREQ_MAX;

	    systimer_init_periodic_nq(&gd0_pollclock, pollclock, NULL, pollhz);
#endif
	} else {
	    /* XXX */
	    gd->gd_time_seconds = globaldata_find(0)->gd_time_seconds;
	    gd->gd_cpuclock_base = globaldata_find(0)->gd_cpuclock_base;
	}

	/*
	 * Use a non-queued periodic systimer to prevent multiple ticks from
	 * building up if the sysclock jumps forward (8254 gets reset).  The
	 * sysclock will never jump backwards.  Our time sync is based on
	 * the actual sysclock, not the ticks count.
	 */
	systimer_init_periodic_nq(&gd->gd_hardclock, hardclock, NULL, hz);
	systimer_init_periodic_nq(&gd->gd_statclock, statclock, NULL, stathz);
	/* XXX correct the frequency for scheduler / estcpu tests */
	systimer_init_periodic_nq(&gd->gd_schedclock, schedclock, 
				NULL, ESTCPUFREQ); 
	crit_exit();
}

/*
 * This sets the current real time of day.  Timespecs are in seconds and
 * nanoseconds.  We do not mess with gd_time_seconds and gd_cpuclock_base,
 * instead we adjust basetime so basetime + gd_* results in the current
 * time of day.  This way the gd_* fields are guarenteed to represent
 * a monotonically increasing 'uptime' value.
 *
 * When set_timeofday() is called from userland, the system call forces it
 * onto cpu #0 since only cpu #0 can update basetime_index.
 */
void
set_timeofday(struct timespec *ts)
{
	struct timespec *nbt;
	int ni;

	/*
	 * XXX SMP / non-atomic basetime updates
	 */
	crit_enter();
	ni = (basetime_index + 1) & BASETIME_ARYMASK;
	nbt = &basetime[ni];
	nanouptime(nbt);
	nbt->tv_sec = ts->tv_sec - nbt->tv_sec;
	nbt->tv_nsec = ts->tv_nsec - nbt->tv_nsec;
	if (nbt->tv_nsec < 0) {
	    nbt->tv_nsec += 1000000000;
	    --nbt->tv_sec;
	}

	/*
	 * Note that basetime diverges from boottime as the clock drift is
	 * compensated for, so we cannot do away with boottime.  When setting
	 * the absolute time of day the drift is 0 (for an instant) and we
	 * can simply assign boottime to basetime.  
	 *
	 * Note that nanouptime() is based on gd_time_seconds which is drift
	 * compensated up to a point (it is guarenteed to remain monotonically
	 * increasing).  gd_time_seconds is thus our best uptime guess and
	 * suitable for use in the boottime calculation.  It is already taken
	 * into account in the basetime calculation above.
	 */
	boottime.tv_sec = nbt->tv_sec;
	ntp_delta = 0;

	/*
	 * We now have a new basetime, make sure all other cpus have it,
	 * then update the index.
	 */
	cpu_sfence();
	basetime_index = ni;

	crit_exit();
}
	
/*
 * Each cpu has its own hardclock, but we only increments ticks and softticks
 * on cpu #0.
 *
 * NOTE! systimer! the MP lock might not be held here.  We can only safely
 * manipulate objects owned by the current cpu.
 */
static void
hardclock(systimer_t info, struct intrframe *frame)
{
	sysclock_t cputicks;
	struct proc *p;
	struct pstats *pstats;
	struct globaldata *gd = mycpu;

	/*
	 * Realtime updates are per-cpu.  Note that timer corrections as
	 * returned by microtime() and friends make an additional adjustment
	 * using a system-wise 'basetime', but the running time is always
	 * taken from the per-cpu globaldata area.  Since the same clock
	 * is distributing (XXX SMP) to all cpus, the per-cpu timebases
	 * stay in synch.
	 *
	 * Note that we never allow info->time (aka gd->gd_hardclock.time)
	 * to reverse index gd_cpuclock_base, but that it is possible for
	 * it to temporarily get behind in the seconds if something in the
	 * system locks interrupts for a long period of time.  Since periodic
	 * timers count events, though everything should resynch again
	 * immediately.
	 */
	cputicks = info->time - gd->gd_cpuclock_base;
	if (cputicks >= sys_cputimer->freq) {
		++gd->gd_time_seconds;
		gd->gd_cpuclock_base += sys_cputimer->freq;
	}

	/*
	 * The system-wide ticks counter and NTP related timedelta/tickdelta
	 * adjustments only occur on cpu #0.  NTP adjustments are accomplished
	 * by updating basetime.
	 */
	if (gd->gd_cpuid == 0) {
	    struct timespec *nbt;
	    struct timespec nts;
	    int leap;
	    int ni;

	    ++ticks;

#if 0
	    if (tco->tc_poll_pps) 
		tco->tc_poll_pps(tco);
#endif

	    /*
	     * Calculate the new basetime index.  We are in a critical section
	     * on cpu #0 and can safely play with basetime_index.  Start
	     * with the current basetime and then make adjustments.
	     */
	    ni = (basetime_index + 1) & BASETIME_ARYMASK;
	    nbt = &basetime[ni];
	    *nbt = basetime[basetime_index];

	    /*
	     * Apply adjtime corrections.  (adjtime() API)
	     *
	     * adjtime() only runs on cpu #0 so our critical section is
	     * sufficient to access these variables.
	     */
	    if (ntp_delta != 0) {
		nbt->tv_nsec += ntp_tick_delta;
		ntp_delta -= ntp_tick_delta;
		if ((ntp_delta > 0 && ntp_delta < ntp_tick_delta) ||
		    (ntp_delta < 0 && ntp_delta > ntp_tick_delta)) {
			ntp_tick_delta = ntp_delta;
 		}
 	    }

	    /*
	     * Apply permanent frequency corrections.  (sysctl API)
	     */
	    if (ntp_tick_permanent != 0) {
		ntp_tick_acc += ntp_tick_permanent;
		if (ntp_tick_acc >= (1LL << 32)) {
		    nbt->tv_nsec += ntp_tick_acc >> 32;
		    ntp_tick_acc -= (ntp_tick_acc >> 32) << 32;
		} else if (ntp_tick_acc <= -(1LL << 32)) {
		    /* Negate ntp_tick_acc to avoid shifting the sign bit. */
		    nbt->tv_nsec -= (-ntp_tick_acc) >> 32;
		    ntp_tick_acc += ((-ntp_tick_acc) >> 32) << 32;
		}
 	    }

	    if (nbt->tv_nsec >= 1000000000) {
		    nbt->tv_sec++;
		    nbt->tv_nsec -= 1000000000;
	    } else if (nbt->tv_nsec < 0) {
		    nbt->tv_sec--;
		    nbt->tv_nsec += 1000000000;
	    }

	    /*
	     * Another per-tick compensation.  (for ntp_adjtime() API)
	     */
	    if (nsec_adj != 0) {
		nsec_acc += nsec_adj;
		if (nsec_acc >= 0x100000000LL) {
		    nbt->tv_nsec += nsec_acc >> 32;
		    nsec_acc = (nsec_acc & 0xFFFFFFFFLL);
		} else if (nsec_acc <= -0x100000000LL) {
		    nbt->tv_nsec -= -nsec_acc >> 32;
		    nsec_acc = -(-nsec_acc & 0xFFFFFFFFLL);
		}
		if (nbt->tv_nsec >= 1000000000) {
		    nbt->tv_nsec -= 1000000000;
		    ++nbt->tv_sec;
		} else if (nbt->tv_nsec < 0) {
		    nbt->tv_nsec += 1000000000;
		    --nbt->tv_sec;
		}
	    }

	    /************************************************************
	     *			LEAP SECOND CORRECTION			*
	     ************************************************************
	     *
	     * Taking into account all the corrections made above, figure
	     * out the new real time.  If the seconds field has changed
	     * then apply any pending leap-second corrections.
	     */
	    getnanotime_nbt(nbt, &nts);

	    if (time_second != nts.tv_sec) {
		/*
		 * Apply leap second (sysctl API).  Adjust nts for changes
		 * so we do not have to call getnanotime_nbt again.
		 */
		if (ntp_leap_second) {
		    if (ntp_leap_second == nts.tv_sec) {
			if (ntp_leap_insert) {
			    nbt->tv_sec++;
			    nts.tv_sec++;
			} else {
			    nbt->tv_sec--;
			    nts.tv_sec--;
			}
			ntp_leap_second--;
		    }
		}

		/*
		 * Apply leap second (ntp_adjtime() API), calculate a new
		 * nsec_adj field.  ntp_update_second() returns nsec_adj
		 * as a per-second value but we need it as a per-tick value.
		 */
		leap = ntp_update_second(time_second, &nsec_adj);
		nsec_adj /= hz;
		nbt->tv_sec += leap;
		nts.tv_sec += leap;

		/*
		 * Update the time_second 'approximate time' global.
		 */
		time_second = nts.tv_sec;
	    }

	    /*
	     * Finally, our new basetime is ready to go live!
	     */
	    cpu_sfence();
	    basetime_index = ni;
	}

	/*
	 * softticks are handled for all cpus
	 */
	hardclock_softtick(gd);

	/*
	 * ITimer handling is per-tick, per-cpu.  I don't think psignal()
	 * is mpsafe on curproc, so XXX get the mplock.
	 */
	if ((p = curproc) != NULL && try_mplock()) {
		pstats = p->p_stats;
		if (frame && CLKF_USERMODE(frame) &&
		    timevalisset(&p->p_timer[ITIMER_VIRTUAL].it_value) &&
		    itimerdecr(&p->p_timer[ITIMER_VIRTUAL], tick) == 0)
			psignal(p, SIGVTALRM);
		if (timevalisset(&p->p_timer[ITIMER_PROF].it_value) &&
		    itimerdecr(&p->p_timer[ITIMER_PROF], tick) == 0)
			psignal(p, SIGPROF);
		rel_mplock();
	}
	setdelayed();
}

/*
 * The statistics clock typically runs at a 125Hz rate, and is intended
 * to be frequency offset from the hardclock (typ 100Hz).  It is per-cpu.
 *
 * NOTE! systimer! the MP lock might not be held here.  We can only safely
 * manipulate objects owned by the current cpu.
 *
 * The stats clock is responsible for grabbing a profiling sample.
 * Most of the statistics are only used by user-level statistics programs.
 * The main exceptions are p->p_uticks, p->p_sticks, p->p_iticks, and
 * p->p_estcpu.
 *
 * Like the other clocks, the stat clock is called from what is effectively
 * a fast interrupt, so the context should be the thread/process that got
 * interrupted.
 */
static void
statclock(systimer_t info, struct intrframe *frame)
{
#ifdef GPROF
	struct gmonparam *g;
	int i;
#endif
	thread_t td;
	struct proc *p;
	int bump;
	struct timeval tv;
	struct timeval *stv;

	/*
	 * How big was our timeslice relative to the last time?
	 */
	microuptime(&tv);	/* mpsafe */
	stv = &mycpu->gd_stattv;
	if (stv->tv_sec == 0) {
	    bump = 1;
	} else {
	    bump = tv.tv_usec - stv->tv_usec +
		(tv.tv_sec - stv->tv_sec) * 1000000;
	    if (bump < 0)
		bump = 0;
	    if (bump > 1000000)
		bump = 1000000;
	}
	*stv = tv;

	td = curthread;
	p = td->td_proc;

	if (frame && CLKF_USERMODE(frame)) {
		/*
		 * Came from userland, handle user time and deal with
		 * possible process.
		 */
		if (p && (p->p_flag & P_PROFIL))
			addupc_intr(p, CLKF_PC(frame), 1);
		td->td_uticks += bump;

		/*
		 * Charge the time as appropriate
		 */
		if (p && p->p_nice > NZERO)
			cpu_time.cp_nice += bump;
		else
			cpu_time.cp_user += bump;
	} else {
#ifdef GPROF
		/*
		 * Kernel statistics are just like addupc_intr, only easier.
		 */
		g = &_gmonparam;
		if (g->state == GMON_PROF_ON && frame) {
			i = CLKF_PC(frame) - g->lowpc;
			if (i < g->textsize) {
				i /= HISTFRACTION * sizeof(*g->kcount);
				g->kcount[i]++;
			}
		}
#endif
		/*
		 * Came from kernel mode, so we were:
		 * - handling an interrupt,
		 * - doing syscall or trap work on behalf of the current
		 *   user process, or
		 * - spinning in the idle loop.
		 * Whichever it is, charge the time as appropriate.
		 * Note that we charge interrupts to the current process,
		 * regardless of whether they are ``for'' that process,
		 * so that we know how much of its real time was spent
		 * in ``non-process'' (i.e., interrupt) work.
		 *
		 * XXX assume system if frame is NULL.  A NULL frame 
		 * can occur if ipi processing is done from a crit_exit().
		 */
		if (frame && CLKF_INTR(frame))
			td->td_iticks += bump;
		else
			td->td_sticks += bump;

		if (frame && CLKF_INTR(frame)) {
			cpu_time.cp_intr += bump;
		} else {
			if (td == &mycpu->gd_idlethread)
				cpu_time.cp_idle += bump;
			else
				cpu_time.cp_sys += bump;
		}
	}
}

/*
 * The scheduler clock typically runs at a 50Hz rate.  NOTE! systimer,
 * the MP lock might not be held.  We can safely manipulate parts of curproc
 * but that's about it.
 *
 * Each cpu has its own scheduler clock.
 */
static void
schedclock(systimer_t info, struct intrframe *frame)
{
	struct lwp *lp;
	struct pstats *pstats;
	struct rusage *ru;
	struct vmspace *vm;
	long rss;

	if ((lp = lwkt_preempted_proc()) != NULL) {
		/*
		 * Account for cpu time used and hit the scheduler.  Note
		 * that this call MUST BE MP SAFE, and the BGL IS NOT HELD
		 * HERE.
		 */
		++lp->lwp_cpticks;
		/*
		 * XXX I think accessing lwp_proc's p_usched is
		 * reasonably MP safe.  This needs to be revisited
		 * when we have pluggable schedulers.
		 */
		lp->lwp_proc->p_usched->schedulerclock(lp, info->periodic, info->time);
	}
	if ((lp = curthread->td_lwp) != NULL) {
		/*
		 * Update resource usage integrals and maximums.
		 */
		if ((pstats = lp->lwp_stats) != NULL &&
		    (ru = &pstats->p_ru) != NULL &&
		    (vm = lp->lwp_proc->p_vmspace) != NULL) {
			ru->ru_ixrss += pgtok(vm->vm_tsize);
			ru->ru_idrss += pgtok(vm->vm_dsize);
			ru->ru_isrss += pgtok(vm->vm_ssize);
			rss = pgtok(vmspace_resident_count(vm));
			if (ru->ru_maxrss < rss)
				ru->ru_maxrss = rss;
		}
	}
}

#ifdef DEVICE_POLLING
static void
pollclock(systimer_t info __unused, struct intrframe *frame __unused)
{
	hardclock_device_poll();	/* mpsafe, short and quick */
}
#endif

/*
 * Compute number of ticks for the specified amount of time.  The 
 * return value is intended to be used in a clock interrupt timed
 * operation and guarenteed to meet or exceed the requested time.
 * If the representation overflows, return INT_MAX.  The minimum return
 * value is 1 ticks and the function will average the calculation up.
 * If any value greater then 0 microseconds is supplied, a value
 * of at least 2 will be returned to ensure that a near-term clock
 * interrupt does not cause the timeout to occur (degenerately) early.
 *
 * Note that limit checks must take into account microseconds, which is
 * done simply by using the smaller signed long maximum instead of
 * the unsigned long maximum.
 *
 * If ints have 32 bits, then the maximum value for any timeout in
 * 10ms ticks is 248 days.
 */
int
tvtohz_high(struct timeval *tv)
{
	int ticks;
	long sec, usec;

	sec = tv->tv_sec;
	usec = tv->tv_usec;
	if (usec < 0) {
		sec--;
		usec += 1000000;
	}
	if (sec < 0) {
#ifdef DIAGNOSTIC
		if (usec > 0) {
			sec++;
			usec -= 1000000;
		}
		printf("tvotohz: negative time difference %ld sec %ld usec\n",
		       sec, usec);
#endif
		ticks = 1;
	} else if (sec <= INT_MAX / hz) {
		ticks = (int)(sec * hz + 
			    ((u_long)usec + (tick - 1)) / tick) + 1;
	} else {
		ticks = INT_MAX;
	}
	return (ticks);
}

/*
 * Compute number of ticks for the specified amount of time, erroring on
 * the side of it being too low to ensure that sleeping the returned number
 * of ticks will not result in a late return.
 *
 * The supplied timeval may not be negative and should be normalized.  A
 * return value of 0 is possible if the timeval converts to less then
 * 1 tick.
 *
 * If ints have 32 bits, then the maximum value for any timeout in
 * 10ms ticks is 248 days.
 */
int
tvtohz_low(struct timeval *tv)
{
	int ticks;
	long sec;

	sec = tv->tv_sec;
	if (sec <= INT_MAX / hz)
		ticks = (int)(sec * hz + (u_long)tv->tv_usec / tick);
	else
		ticks = INT_MAX;
	return (ticks);
}


/*
 * Start profiling on a process.
 *
 * Kernel profiling passes proc0 which never exits and hence
 * keeps the profile clock running constantly.
 */
void
startprofclock(struct proc *p)
{
	if ((p->p_flag & P_PROFIL) == 0) {
		p->p_flag |= P_PROFIL;
#if 0	/* XXX */
		if (++profprocs == 1 && stathz != 0) {
			crit_enter();
			psdiv = psratio;
			setstatclockrate(profhz);
			crit_exit();
		}
#endif
	}
}

/*
 * Stop profiling on a process.
 */
void
stopprofclock(struct proc *p)
{
	if (p->p_flag & P_PROFIL) {
		p->p_flag &= ~P_PROFIL;
#if 0	/* XXX */
		if (--profprocs == 0 && stathz != 0) {
			crit_enter();
			psdiv = 1;
			setstatclockrate(stathz);
			crit_exit();
		}
#endif
	}
}

/*
 * Return information about system clocks.
 */
static int
sysctl_kern_clockrate(SYSCTL_HANDLER_ARGS)
{
	struct kinfo_clockinfo clkinfo;
	/*
	 * Construct clockinfo structure.
	 */
	clkinfo.ci_hz = hz;
	clkinfo.ci_tick = tick;
	clkinfo.ci_tickadj = ntp_default_tick_delta / 1000;
	clkinfo.ci_profhz = profhz;
	clkinfo.ci_stathz = stathz ? stathz : hz;
	return (sysctl_handle_opaque(oidp, &clkinfo, sizeof clkinfo, req));
}

SYSCTL_PROC(_kern, KERN_CLOCKRATE, clockrate, CTLTYPE_STRUCT|CTLFLAG_RD,
	0, 0, sysctl_kern_clockrate, "S,clockinfo","");

/*
 * We have eight functions for looking at the clock, four for
 * microseconds and four for nanoseconds.  For each there is fast
 * but less precise version "get{nano|micro}[up]time" which will
 * return a time which is up to 1/HZ previous to the call, whereas
 * the raw version "{nano|micro}[up]time" will return a timestamp
 * which is as precise as possible.  The "up" variants return the
 * time relative to system boot, these are well suited for time
 * interval measurements.
 *
 * Each cpu independantly maintains the current time of day, so all
 * we need to do to protect ourselves from changes is to do a loop
 * check on the seconds field changing out from under us.
 *
 * The system timer maintains a 32 bit count and due to various issues
 * it is possible for the calculated delta to occassionally exceed
 * sys_cputimer->freq.  If this occurs the sys_cputimer->freq64_nsec
 * multiplication can easily overflow, so we deal with the case.  For
 * uniformity we deal with the case in the usec case too.
 */
void
getmicrouptime(struct timeval *tvp)
{
	struct globaldata *gd = mycpu;
	sysclock_t delta;

	do {
		tvp->tv_sec = gd->gd_time_seconds;
		delta = gd->gd_hardclock.time - gd->gd_cpuclock_base;
	} while (tvp->tv_sec != gd->gd_time_seconds);

	if (delta >= sys_cputimer->freq) {
		tvp->tv_sec += delta / sys_cputimer->freq;
		delta %= sys_cputimer->freq;
	}
	tvp->tv_usec = (sys_cputimer->freq64_usec * delta) >> 32;
	if (tvp->tv_usec >= 1000000) {
		tvp->tv_usec -= 1000000;
		++tvp->tv_sec;
	}
}

void
getnanouptime(struct timespec *tsp)
{
	struct globaldata *gd = mycpu;
	sysclock_t delta;

	do {
		tsp->tv_sec = gd->gd_time_seconds;
		delta = gd->gd_hardclock.time - gd->gd_cpuclock_base;
	} while (tsp->tv_sec != gd->gd_time_seconds);

	if (delta >= sys_cputimer->freq) {
		tsp->tv_sec += delta / sys_cputimer->freq;
		delta %= sys_cputimer->freq;
	}
	tsp->tv_nsec = (sys_cputimer->freq64_nsec * delta) >> 32;
}

void
microuptime(struct timeval *tvp)
{
	struct globaldata *gd = mycpu;
	sysclock_t delta;

	do {
		tvp->tv_sec = gd->gd_time_seconds;
		delta = sys_cputimer->count() - gd->gd_cpuclock_base;
	} while (tvp->tv_sec != gd->gd_time_seconds);

	if (delta >= sys_cputimer->freq) {
		tvp->tv_sec += delta / sys_cputimer->freq;
		delta %= sys_cputimer->freq;
	}
	tvp->tv_usec = (sys_cputimer->freq64_usec * delta) >> 32;
}

void
nanouptime(struct timespec *tsp)
{
	struct globaldata *gd = mycpu;
	sysclock_t delta;

	do {
		tsp->tv_sec = gd->gd_time_seconds;
		delta = sys_cputimer->count() - gd->gd_cpuclock_base;
	} while (tsp->tv_sec != gd->gd_time_seconds);

	if (delta >= sys_cputimer->freq) {
		tsp->tv_sec += delta / sys_cputimer->freq;
		delta %= sys_cputimer->freq;
	}
	tsp->tv_nsec = (sys_cputimer->freq64_nsec * delta) >> 32;
}

/*
 * realtime routines
 */

void
getmicrotime(struct timeval *tvp)
{
	struct globaldata *gd = mycpu;
	struct timespec *bt;
	sysclock_t delta;

	do {
		tvp->tv_sec = gd->gd_time_seconds;
		delta = gd->gd_hardclock.time - gd->gd_cpuclock_base;
	} while (tvp->tv_sec != gd->gd_time_seconds);

	if (delta >= sys_cputimer->freq) {
		tvp->tv_sec += delta / sys_cputimer->freq;
		delta %= sys_cputimer->freq;
	}
	tvp->tv_usec = (sys_cputimer->freq64_usec * delta) >> 32;

	bt = &basetime[basetime_index];
	tvp->tv_sec += bt->tv_sec;
	tvp->tv_usec += bt->tv_nsec / 1000;
	while (tvp->tv_usec >= 1000000) {
		tvp->tv_usec -= 1000000;
		++tvp->tv_sec;
	}
}

void
getnanotime(struct timespec *tsp)
{
	struct globaldata *gd = mycpu;
	struct timespec *bt;
	sysclock_t delta;

	do {
		tsp->tv_sec = gd->gd_time_seconds;
		delta = gd->gd_hardclock.time - gd->gd_cpuclock_base;
	} while (tsp->tv_sec != gd->gd_time_seconds);

	if (delta >= sys_cputimer->freq) {
		tsp->tv_sec += delta / sys_cputimer->freq;
		delta %= sys_cputimer->freq;
	}
	tsp->tv_nsec = (sys_cputimer->freq64_nsec * delta) >> 32;

	bt = &basetime[basetime_index];
	tsp->tv_sec += bt->tv_sec;
	tsp->tv_nsec += bt->tv_nsec;
	while (tsp->tv_nsec >= 1000000000) {
		tsp->tv_nsec -= 1000000000;
		++tsp->tv_sec;
	}
}

static void
getnanotime_nbt(struct timespec *nbt, struct timespec *tsp)
{
	struct globaldata *gd = mycpu;
	sysclock_t delta;

	do {
		tsp->tv_sec = gd->gd_time_seconds;
		delta = gd->gd_hardclock.time - gd->gd_cpuclock_base;
	} while (tsp->tv_sec != gd->gd_time_seconds);

	if (delta >= sys_cputimer->freq) {
		tsp->tv_sec += delta / sys_cputimer->freq;
		delta %= sys_cputimer->freq;
	}
	tsp->tv_nsec = (sys_cputimer->freq64_nsec * delta) >> 32;

	tsp->tv_sec += nbt->tv_sec;
	tsp->tv_nsec += nbt->tv_nsec;
	while (tsp->tv_nsec >= 1000000000) {
		tsp->tv_nsec -= 1000000000;
		++tsp->tv_sec;
	}
}


void
microtime(struct timeval *tvp)
{
	struct globaldata *gd = mycpu;
	struct timespec *bt;
	sysclock_t delta;

	do {
		tvp->tv_sec = gd->gd_time_seconds;
		delta = sys_cputimer->count() - gd->gd_cpuclock_base;
	} while (tvp->tv_sec != gd->gd_time_seconds);

	if (delta >= sys_cputimer->freq) {
		tvp->tv_sec += delta / sys_cputimer->freq;
		delta %= sys_cputimer->freq;
	}
	tvp->tv_usec = (sys_cputimer->freq64_usec * delta) >> 32;

	bt = &basetime[basetime_index];
	tvp->tv_sec += bt->tv_sec;
	tvp->tv_usec += bt->tv_nsec / 1000;
	while (tvp->tv_usec >= 1000000) {
		tvp->tv_usec -= 1000000;
		++tvp->tv_sec;
	}
}

void
nanotime(struct timespec *tsp)
{
	struct globaldata *gd = mycpu;
	struct timespec *bt;
	sysclock_t delta;

	do {
		tsp->tv_sec = gd->gd_time_seconds;
		delta = sys_cputimer->count() - gd->gd_cpuclock_base;
	} while (tsp->tv_sec != gd->gd_time_seconds);

	if (delta >= sys_cputimer->freq) {
		tsp->tv_sec += delta / sys_cputimer->freq;
		delta %= sys_cputimer->freq;
	}
	tsp->tv_nsec = (sys_cputimer->freq64_nsec * delta) >> 32;

	bt = &basetime[basetime_index];
	tsp->tv_sec += bt->tv_sec;
	tsp->tv_nsec += bt->tv_nsec;
	while (tsp->tv_nsec >= 1000000000) {
		tsp->tv_nsec -= 1000000000;
		++tsp->tv_sec;
	}
}

/*
 * note: this is not exactly synchronized with real time.  To do that we
 * would have to do what microtime does and check for a nanoseconds overflow.
 */
time_t
get_approximate_time_t(void)
{
	struct globaldata *gd = mycpu;
	struct timespec *bt;

	bt = &basetime[basetime_index];
	return(gd->gd_time_seconds + bt->tv_sec);
}

int
pps_ioctl(u_long cmd, caddr_t data, struct pps_state *pps)
{
	pps_params_t *app;
	struct pps_fetch_args *fapi;
#ifdef PPS_SYNC
	struct pps_kcbind_args *kapi;
#endif

	switch (cmd) {
	case PPS_IOC_CREATE:
		return (0);
	case PPS_IOC_DESTROY:
		return (0);
	case PPS_IOC_SETPARAMS:
		app = (pps_params_t *)data;
		if (app->mode & ~pps->ppscap)
			return (EINVAL);
		pps->ppsparam = *app;         
		return (0);
	case PPS_IOC_GETPARAMS:
		app = (pps_params_t *)data;
		*app = pps->ppsparam;
		app->api_version = PPS_API_VERS_1;
		return (0);
	case PPS_IOC_GETCAP:
		*(int*)data = pps->ppscap;
		return (0);
	case PPS_IOC_FETCH:
		fapi = (struct pps_fetch_args *)data;
		if (fapi->tsformat && fapi->tsformat != PPS_TSFMT_TSPEC)
			return (EINVAL);
		if (fapi->timeout.tv_sec || fapi->timeout.tv_nsec)
			return (EOPNOTSUPP);
		pps->ppsinfo.current_mode = pps->ppsparam.mode;         
		fapi->pps_info_buf = pps->ppsinfo;
		return (0);
	case PPS_IOC_KCBIND:
#ifdef PPS_SYNC
		kapi = (struct pps_kcbind_args *)data;
		/* XXX Only root should be able to do this */
		if (kapi->tsformat && kapi->tsformat != PPS_TSFMT_TSPEC)
			return (EINVAL);
		if (kapi->kernel_consumer != PPS_KC_HARDPPS)
			return (EINVAL);
		if (kapi->edge & ~pps->ppscap)
			return (EINVAL);
		pps->kcmode = kapi->edge;
		return (0);
#else
		return (EOPNOTSUPP);
#endif
	default:
		return (ENOTTY);
	}
}

void
pps_init(struct pps_state *pps)
{
	pps->ppscap |= PPS_TSFMT_TSPEC;
	if (pps->ppscap & PPS_CAPTUREASSERT)
		pps->ppscap |= PPS_OFFSETASSERT;
	if (pps->ppscap & PPS_CAPTURECLEAR)
		pps->ppscap |= PPS_OFFSETCLEAR;
}

void
pps_event(struct pps_state *pps, sysclock_t count, int event)
{
	struct globaldata *gd;
	struct timespec *tsp;
	struct timespec *osp;
	struct timespec *bt;
	struct timespec ts;
	sysclock_t *pcount;
#ifdef PPS_SYNC
	sysclock_t tcount;
#endif
	sysclock_t delta;
	pps_seq_t *pseq;
	int foff;
	int fhard;

	gd = mycpu;

	/* Things would be easier with arrays... */
	if (event == PPS_CAPTUREASSERT) {
		tsp = &pps->ppsinfo.assert_timestamp;
		osp = &pps->ppsparam.assert_offset;
		foff = pps->ppsparam.mode & PPS_OFFSETASSERT;
		fhard = pps->kcmode & PPS_CAPTUREASSERT;
		pcount = &pps->ppscount[0];
		pseq = &pps->ppsinfo.assert_sequence;
	} else {
		tsp = &pps->ppsinfo.clear_timestamp;
		osp = &pps->ppsparam.clear_offset;
		foff = pps->ppsparam.mode & PPS_OFFSETCLEAR;
		fhard = pps->kcmode & PPS_CAPTURECLEAR;
		pcount = &pps->ppscount[1];
		pseq = &pps->ppsinfo.clear_sequence;
	}

	/* Nothing really happened */
	if (*pcount == count)
		return;

	*pcount = count;

	do {
		ts.tv_sec = gd->gd_time_seconds;
		delta = count - gd->gd_cpuclock_base;
	} while (ts.tv_sec != gd->gd_time_seconds);

	if (delta >= sys_cputimer->freq) {
		ts.tv_sec += delta / sys_cputimer->freq;
		delta %= sys_cputimer->freq;
	}
	ts.tv_nsec = (sys_cputimer->freq64_nsec * delta) >> 32;
	bt = &basetime[basetime_index];
	ts.tv_sec += bt->tv_sec;
	ts.tv_nsec += bt->tv_nsec;
	while (ts.tv_nsec >= 1000000000) {
		ts.tv_nsec -= 1000000000;
		++ts.tv_sec;
	}

	(*pseq)++;
	*tsp = ts;

	if (foff) {
		timespecadd(tsp, osp);
		if (tsp->tv_nsec < 0) {
			tsp->tv_nsec += 1000000000;
			tsp->tv_sec -= 1;
		}
	}
#ifdef PPS_SYNC
	if (fhard) {
		/* magic, at its best... */
		tcount = count - pps->ppscount[2];
		pps->ppscount[2] = count;
		if (tcount >= sys_cputimer->freq) {
			delta = (1000000000 * (tcount / sys_cputimer->freq) +
				 sys_cputimer->freq64_nsec * 
				 (tcount % sys_cputimer->freq)) >> 32;
		} else {
			delta = (sys_cputimer->freq64_nsec * tcount) >> 32;
		}
		hardpps(tsp, delta);
	}
#endif
}

