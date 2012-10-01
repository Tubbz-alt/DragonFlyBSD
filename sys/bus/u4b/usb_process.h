/* $FreeBSD$ */
/*-
 * Copyright (c) 2008 Hans Petter Selasky. All rights reserved.
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
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#ifndef _USB_PROCESS_H_
#define	_USB_PROCESS_H_

#include <sys/interrupt.h>
#include <sys/signalvar.h>
#include <sys/thread.h>

/* defines */
#define	USB_PRI_HIGH	TDPRI_INT_HIGH
#define	USB_PRI_MED	TDPRI_INT_MED

#define	USB_PROC_WAIT_TIMEOUT 2
#define	USB_PROC_WAIT_DRAIN   1
#define	USB_PROC_WAIT_NORMAL  0

/* structure prototypes */

struct usb_proc_msg;

/*
 * The following structure defines the USB process.
 */
struct usb_process {
	TAILQ_HEAD(, usb_proc_msg) up_qhead;
	struct cv up_cv;
	struct cv up_drain;

	struct thread *up_ptr;
	struct thread *up_curtd;
	struct lock *up_lock;

	usb_size_t up_msg_num;

	uint8_t	up_prio;
	uint8_t	up_gone;
	uint8_t	up_msleep;
	uint8_t	up_csleep;
	uint8_t	up_dsleep;
};

/* prototypes */

uint8_t	usb_proc_is_gone(struct usb_process *up);
int	usb_proc_create(struct usb_process *up, struct lock *p_lock,
	    const char *pmesg, uint8_t prio);
void	usb_proc_drain(struct usb_process *up);
void	usb_proc_mwait(struct usb_process *up, void *pm0, void *pm1);
void	usb_proc_free(struct usb_process *up);
void   *usb_proc_msignal(struct usb_process *up, void *pm0, void *pm1);
void	usb_proc_rewakeup(struct usb_process *up);

#endif					/* _USB_PROCESS_H_ */
