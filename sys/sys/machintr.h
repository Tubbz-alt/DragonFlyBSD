/*
 * Copyright (c) 2005 The DragonFly Project.  All rights reserved.
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
 * $DragonFly: src/sys/sys/machintr.h,v 1.2 2005/11/02 20:23:23 dillon Exp $
 */

#ifndef _SYS_QUEUE_H_
#include <sys/queue.h>
#endif

enum machintr_type { MACHINTR_ICU, MACHINTR_APIC };

#define MACHINTR_VAR_SIZEMASK	0xFFFF

#define MACHINTR_VAR_PICMODE	(0x00010000|sizeof(int))

#define MACHINTR_VECTOR_SETUP		1
#define MACHINTR_VECTOR_TEARDOWN	2

/*
 * Machine interrupt ABIs - registered at boot-time
 */
struct machintr_abi {
    enum machintr_type type;
    void	(*intrdis)(int);		/* hardware disable irq */
    void	(*intren)(int);			/* hardware enable irq */
    int		(*vectorctl)(int, int, int);	/* hardware intr vector ctl */
    int		(*setvar)(int, const void *);	/* set miscellanious info */
    int		(*getvar)(int, void *);		/* get miscellanious info */
    void	(*finalize)(void);		/* final before ints enabled */
};

#define machintr_intren(intr)	MachIntrABI.intren(intr)
#define machintr_intrdis(intr)	MachIntrABI.intrdis(intr)
#define machintr_vector_setup(intr, flags)	\
	    MachIntrABI.vectorctl(MACHINTR_VECTOR_SETUP, intr, flags)
#define machintr_vector_teardown(intr)		\
	    MachIntrABI.vectorctl(MACHINTR_VECTOR_TEARDOWN, intr, 0)

#ifdef _KERNEL

extern struct machintr_abi MachIntrABI;
extern int machintr_setvar_simple(int, int);

#endif
