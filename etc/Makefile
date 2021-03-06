#	from: @(#)Makefile	5.11 (Berkeley) 5/21/91
# $FreeBSD: src/etc/Makefile,v 1.219.2.38 2003/03/04 09:49:00 ru Exp $

SUBDIR=	sendmail

# Files that should be installed read-only (444)
#
BINUPDATE= devd.conf fbtab network.subr \
	pf.os \
	protocols \
	rc rc.firewall rc.firewall6 \
	rc.sendmail rc.shutdown \
	rc.subr rpc \
	etc.${MACHINE_ARCH}/disktab
.if defined(BINARY_UPGRADE) # location of these depends on upgrade method
BINUPDATE+=mail.rc locate.rc
.else
BINUPDATE+=${.CURDIR}/../usr.bin/mail/misc/mail.rc \
	${.CURDIR}/../usr.bin/locate/locate/locate.rc \
	${.CURDIR}/../contrib/openresolv/resolvconf.conf \
	${.CURDIR}/../contrib/dhcpcd/src/dhcpcd.conf
.endif

# Initial distribution files are installed read-write (644)
#
BIN1=	auth.conf \
	crontab csh.cshrc csh.login csh.logout \
	devtab dhclient.conf dntpd.conf \
	ftpusers gettytab group \
	hosts hosts.allow hosts.equiv hosts.lpd \
	inetd.conf login.access login.conf \
	motd modems netconfig networks newsyslog.conf \
	nscd.conf pf.conf phones printcap profile \
	regdomain.xml remote sensorsd.conf services \
	shells sysctl.conf syslog.conf \
	wpa_supplicant.conf \
	etc.${MACHINE_ARCH}/ttys

DIRS+=	lib/libssh usr.bin/openssl usr.bin/ssh usr.sbin/sshd

# Files that should be installed read-only-executable (555) root:wheel
#
BIN2=	pccard_ether rc.suspend rc.resume

DEFAULTS= compilers.conf devfs.conf make.conf periodic.conf \
	  pfi.conf rc.conf uuids

MTREE=	BSD.include.dist BSD.root.dist BSD.usr.dist BSD.var.dist
.if !defined(NOFSCHG)
MTREE_FSCHG=	-i
.endif

PPPCNF=	ppp.conf

ETCMAIL=aliases mailer.conf access.sample virtusertable.sample \
	mailertable.sample

# Use this directory as the source for new configuration files when upgrading
UPGRADE_SRCDIR?=${.CURDIR}

CHECK_DEPRECATED_DIRS+=	/usr/local

# Include file which contains obsolete files
.if exists(${DESTDIR}/etc/upgrade/Makefile_upgrade.inc)
.include "${DESTDIR}/etc/upgrade/Makefile_upgrade.inc"
.endif

remove-obsolete-files:
	@if [ -z "${TO_REMOVE}" ]; then \
		echo "Please do a 'make installworld' first."; \
		echo "See build(7) for further information."; \
		/usr/bin/false; \
	fi;
	@echo "===> Remove now obsolete files"
	@for item in ${TO_REMOVE}; do \
		if [ -e ${DESTDIR}$${item} -o -L ${DESTDIR}$${item} ]; then \
			echo "${DESTDIR}$${item}"; \
			chflags -Rf noschg "${DESTDIR}$${item}"; \
			rm -rf "${DESTDIR}$${item}"; \
		fi; \
	done;

remove-deprecated-files:
.if defined(REMOVE_DEPRECATED)
	@echo "===> Remove now deprecated files"
	@for item in ${TO_REMOVE_LATE}; do \
		if [ -e ${DESTDIR}$${item} -o -L ${DESTDIR}$${item} ]; then \
			echo "${DESTDIR}$${item}"; \
			chflags -Rf noschg "${DESTDIR}$${item}"; \
			rm -rf "${DESTDIR}$${item}"; \
		fi; \
	done;
.else
	@make check-deprecated-files
.endif

check-deprecated-files:
	@echo "===> Checking for deprecated files"
	@echo "     (harmless ELF linker warnings may appear here)"
	@tmpldd=`mktemp -t chkldd`; \
	chroot ${DESTDIR}/ find ${CHECK_DEPRECATED_DIRS:O:u} \
				-type f \( -name '*.so*' -o -perm +111 \) | \
				chroot ${DESTDIR}/ xargs ldd 2>/dev/null > $${tmpldd}; \
	count=0; for item in ${TO_REMOVE_LATE}; do \
		if [ -e ${DESTDIR}$${item} -o -L ${DESTDIR}$${item} ]; then \
			echo "${DESTDIR}$${item} is deprecated"; \
			deps=""; \
			for object in `cat $${tmpldd} | awk -v library=$${item} \
				-f ${.CURDIR}/../tools/tools/chkldd/chkldd.awk`; do \
				echo "${TO_REMOVE_LATE}" | grep -q "$${object}" \
					|| deps="$${deps} $${object}"; \
			done; \
			if [ -n "$${deps}" ]; then \
				echo "  but it's still linked from these objects:"; \
				echo "  " $${deps}; \
				echo ""; \
			fi; \
			count=`expr $$count + 1`; \
		fi; \
	done; \
	rm $${tmpldd}; \
	if [ "$$count" -gt 0 ]; then \
		echo "==================================================================" ; \
		echo "= You have $$count now deprecated files." ; \
		echo "= Once you are sure that none of your third party (ports or local)" ; \
		echo "= software are still using them, rerun with:" ; \
		echo "=     make upgrade -DREMOVE_DEPRECATED" ; \
		echo "==================================================================" ; \
	fi;

preupgrade:
	(pw -V ${DESTDIR}/etc usershow mail -q > /dev/null) || \
		pw -V ${DESTDIR}/etc useradd mail -u 6 -g 6 \
		   -c "Mail user" \
		   -d /nonexistent -s /sbin/nologin
	(pw -V ${DESTDIR}/etc usershow _pflogd -q > /dev/null) || \
		pw -V ${DESTDIR}/etc useradd _pflogd -u 64 \
		   -c "pflogd privsep user" \
		   -d /var/empty -s /sbin/nologin
	(pw -V ${DESTDIR}/etc usershow _sdpd -q > /dev/null) || \
		pw -V ${DESTDIR}/etc useradd _sdpd -u 70 \
		   -c "sdpd privsep user" \
		   -d /var/empty -s /sbin/nologin
	(pw -V ${DESTDIR}/etc usershow _dhcp -q > /dev/null) || \
		pw -V ${DESTDIR}/etc useradd _dhcp -u 77 \
		   -c "DHCP programs" \
		   -d /var/empty -s /sbin/nologin
	(pw -V ${DESTDIR}/etc groupshow input -q > /dev/null) || \
		pw -V ${DESTDIR}/etc groupadd input -g 43
	(pw -V ${DESTDIR}/etc groupshow video -q > /dev/null) || \
		pw -V ${DESTDIR}/etc groupadd video -g 44
	(pw -V ${DESTDIR}/etc groupshow authpf -q > /dev/null) || \
		pw -V ${DESTDIR}/etc groupadd authpf -g 63
	(pw -V ${DESTDIR}/etc groupshow _pflogd -q > /dev/null) || \
		pw -V ${DESTDIR}/etc groupadd _pflogd -g 64
	(pw -V ${DESTDIR}/etc groupshow _sdpd -q > /dev/null) || \
		pw -V ${DESTDIR}/etc groupadd _sdpd -g 70
	(pw -V ${DESTDIR}/etc groupshow _dhcp -q > /dev/null) || \
		pw -V ${DESTDIR}/etc groupadd _dhcp -g 77

upgrade_check:
	@if [ -n "`grep '^sendmail.*/usr/libexec/sendmail/sendmail' ${DESTDIR}/etc/mail/mailer.conf`" ]; then \
		echo "==================================================================" ; \
		echo "=  sendmail is being removed from the base system." ; \
		echo "=  /etc/mail/mailer.conf is still configured to use sendmail." ; \
		echo "=  See http://www.dragonflybsd.com/docs/docs/newhandbook/mta/" ; \
		echo "=  Quick: cp /usr/src/libexec/dma/mailer-conf/mailer.conf.dma " ; \
		echo "=            ${DESTDIR}/etc/mail/mailer.conf ; make upgrade" ; \
		echo "==================================================================" ; \
		/usr/bin/false ; \
	fi

upgrade_base:	upgrade_check preupgrade remove-obsolete-files
	# files that should be dirs (not possible with Makefile_upgrade.inc)
.for f in usr/include/dev/misc/ppi usr/share/initrd/sbin var/db/entropy
	[ -d ${DESTDIR}/${f} ] || rm -f ${DESTDIR}/${f}
.endfor
.if !defined(BINARY_UPGRADE) # binary upgrade just copies these files
	cd ${UPGRADE_SRCDIR}/../share/mk; ${MAKE} install
.endif
	cd ${UPGRADE_SRCDIR}; \
	    ${INSTALL} -o ${BINOWN} -g ${BINGRP} -m 444 \
		${BINUPDATE} ${DESTDIR}/etc; \
	    cap_mkdb ${DESTDIR}/etc/login.conf; \
	    for f in ${BIN1}; do \
		[ -e "${DESTDIR}/etc/$${f##*/}" ] || \
		    ${INSTALL} -o ${BINOWN} -g ${BINGRP} -m 644 \
			$$f ${DESTDIR}/etc; \
	    done; \
	    ${INSTALL} -o ${BINOWN} -g ${BINGRP} -m 555 \
		${BIN2} ${DESTDIR}/etc;
	cd ${UPGRADE_SRCDIR}/defaults; ${INSTALL} -o ${BINOWN} -g ${BINGRP} -m 444 \
	    ${DEFAULTS} ${DESTDIR}/etc/defaults
	cd ${UPGRADE_SRCDIR}/periodic; ${MAKE} install
	mkdir -p ${DESTDIR}/etc/rc.d
	cd ${UPGRADE_SRCDIR}/rc.d; ${MAKE} install
	cd ${UPGRADE_SRCDIR}/devd; ${MAKE} install
	# "../share/termcap/make etc-termcap" expanded inline here:
	${LN} -fs /usr/share/misc/termcap ${DESTDIR}/etc/termcap
	# "../usr.sbin/rmt/make etc-rmt" expanded inline here:
	${LN} -fs /usr/sbin/rmt ${DESTDIR}/etc/rmt
	cd ${UPGRADE_SRCDIR}/mtree; ${INSTALL} -o ${BINOWN} -g ${BINGRP} -m 444 \
	    ${MTREE} ${DESTDIR}/etc/mtree
	cd ${UPGRADE_SRCDIR}/bluetooth; ${MAKE} install
	cd ${UPGRADE_SRCDIR}/calendar; ${MAKE} install
	cd ${UPGRADE_SRCDIR}; ${INSTALL} -o ${BINOWN} -g ${BINGRP} -m 444 Makefile.usr ${DESTDIR}/usr/Makefile
.if !exists(${DESTDIR}/etc/pam.d)
	mkdir -p ${DESTDIR}/etc/pam.d
	cd ${UPGRADE_SRCDIR}/pam.d; ${MAKE} install
	sh ${DESTDIR}/etc/pam.d/convert.sh ${DESTDIR}/etc/pam.d ${DESTDIR}/etc/pam.conf
.else
.for pamconf in README convert.sh atrun cron passwd su system
.if !exists(${DESTDIR}/etc/pam.d/${pamconf})
	${INSTALL} -o ${BINOWN} -g ${BINGRP} -m 644 ${UPGRADE_SRCDIR}/pam.d/${pamconf} ${DESTDIR}/etc/pam.d
.endif
.endfor
.endif
.if !defined(BINARY_UPGRADE) # binary upgrade just copies these files
	cd ${UPGRADE_SRCDIR}/..; ${INSTALL} -o ${BINOWN} -g ${BINGRP} -m 444 \
	    COPYRIGHT ${DESTDIR}/
.endif
	rm -f ${DESTDIR}/usr/include/machine/ioctl_meteor.h
	rm -f ${DESTDIR}/usr/include/machine/ioctl_bt848.h
	${LN} -s "../dev/video/bktr/ioctl_bt848.h" ${DESTDIR}/usr/include/machine/ioctl_bt848.h
	${LN} -s "../dev/video/meteor/ioctl_meteor.h" ${DESTDIR}/usr/include/machine/ioctl_meteor.h
.if exists(${DESTDIR}/usr/lib/gcc2)
	ldconfig -m ${DESTDIR}/usr/lib/gcc2
.endif
.for lib in ${COMPAT_LIBS:M*.so.*}
.if exists(${DESTDIR}/usr/lib/${lib})
	chflags noschg ${DESTDIR}/usr/lib/${lib}
	rm -f ${DESTDIR}/usr/lib/${lib}
.endif
.endfor
	cd ${.CURDIR}/../lib/libssh; ${MAKE} -f Makefile.etc install
.if exists(${DESTDIR}/boot)
.if exists(${DESTDIR}/kernel)
	chflags noschg ${DESTDIR}/kernel
	mv ${DESTDIR}/kernel ${DESTDIR}/boot/kernel
	chflags schg ${DESTDIR}/boot/kernel
.endif
.if exists(${DESTDIR}/modules)
	mv ${DESTDIR}/modules ${DESTDIR}/boot/modules
	rm -rf ${DESTDIR}/boot/modules/modules
.endif
.if exists(${DESTDIR}/kernel.old)
	mv ${DESTDIR}/kernel.old ${DESTDIR}/boot/kernel.old
.endif
.if exists(${DESTDIR}/modules.old)
	mv ${DESTDIR}/modules.old ${DESTDIR}/boot/modules.old
	rm -rf ${DESTDIR}/boot/modules.old/modules.old
.endif
.endif
.if exists(${DESTDIR}/etc/settings.conf)
	mv ${DESTDIR}/etc/settings.conf ${DESTDIR}/etc/pkg_radd.conf
.endif
	for ext in a so.0; do \
		f=${DESTDIR}/usr/lib/libpthread.$$ext; \
		dest=$$(readlink "$$f"); \
		case "$$dest" in \
		libthread_xu.*) \
			ln -fs thread/$$dest $$f ;; \
		esac; \
	done
	ldconfig -R
	mkdir -p ${DESTDIR}/dev
	mkdir -p ${DESTDIR}/var/run/sem
	chmod 1777 ${DESTDIR}/var/run/sem

# The existence of cleartext_pass_ok means pam config files are out of date.
	@set - `fgrep -e cleartext_pass_ok -e pam_opie ${DESTDIR}/etc/pam.d/*`; \
	if [ $$# -gt 0 ] ; \
	then \
		echo "It appears your PAM configuration files need to be updated"; \
		echo "This can be done by manually editing the files or:"; \
		echo "	cd ${UPGRADE_SRCDIR}/pam.d"; \
		echo "	make install"; \
	fi

# Move the dhcpcd duid, secret and lease files back outside the chroot.
# Then ensure the chroot is empty.
	DHCPCD_DB="${DESTDIR}/var/db/dhcpcd"; \
	DHCPCD_CHROOT=`pw -V ${DESTDIR}/etc usershow _dhcp | cut -d: -f9`; \
	DHCPCD_CHROOT_DB="${DESTDIR}$$DHCPCD_CHROOT/var/db/dhcpcd"; \
	if [ -f "$$DHCPCD_CHROOT_DB/duid" ] && \
	    [ ! -f "$$DHCPCD_DB/duid" ]; then \
		mkdir -p "$$DHCPCD_DB" && \
		mv "$$DHCPCD_CHROOT_DB"/* "$$DHCPCD_DB"; \
		chown -R root:wheel "$$DHCPCD_DB"; \
		rm -rf "$$DHCPCD_CHROOT"/*; \
	fi; \
	if [ "$$DHCPCD_CHROOT" != /var/empty ]; then \
		pw -V ${DESTDIR}/etc usermod _dhcp -d /var/empty; \
		rm -rf "$$DHCPCD_CHROOT"; \
	fi

upgrade_etc:	upgrade_base remove-deprecated-files
	@echo "Upgrade was successful"

distribution:
	cd ${.CURDIR}; \
	    ${INSTALL} -o ${BINOWN} -g ${BINGRP} -m 444 \
		${BINUPDATE} ${DESTDIR}/etc; \
	    ${INSTALL} -o ${BINOWN} -g ${BINGRP} -m 644 \
		${BIN1} ${DESTDIR}/etc; \
	    cap_mkdb ${DESTDIR}/etc/login.conf; \
	    ${INSTALL} -o ${BINOWN} -g ${BINGRP} -m 555 \
		${BIN2} ${DESTDIR}/etc; \
	    ${INSTALL} -o ${BINOWN} -g ${BINGRP} -m 600 \
		master.passwd nsmb.conf ${DESTDIR}/etc; \
	    pwd_mkdb -p -d ${DESTDIR}/etc ${DESTDIR}/etc/master.passwd
	cd ${.CURDIR}/defaults; ${INSTALL} -o ${BINOWN} -g ${BINGRP} -m 644 \
	    ${DEFAULTS} ${DESTDIR}/etc/defaults
	cd ${.CURDIR}/pam.d; ${MAKE} install
	cd ${.CURDIR}/bluetooth; ${MAKE} install
	cd ${.CURDIR}/calendar; ${MAKE} install
	cd ${.CURDIR}/periodic; ${MAKE} install
	cd ${.CURDIR}/rc.d; ${MAKE} install
	cd ${.CURDIR}/devd; ${MAKE} install
	cd ${.CURDIR}/../share/termcap; ${MAKE} etc-termcap
	cd ${.CURDIR}/../usr.sbin/rmt; ${MAKE} etc-rmt
	cd ${.CURDIR}; \
	    ${INSTALL} -o ${BINOWN} -g ${BINGRP} -m 444 \
		Makefile.usr ${DESTDIR}/usr/Makefile
.for dir in ${DIRS}
.if exists(${.CURDIR}/../${dir}/Makefile.etc)
	cd ${.CURDIR}/../${dir}; ${MAKE} -f Makefile.etc obj
	cd ${.CURDIR}/../${dir}; ${MAKE} -f Makefile.etc install
.endif
.endfor
	mkdir -p ${DESTDIR}/dev
	cd ${.CURDIR}/../share/skel; \
	    ${INSTALL} -o ${BINOWN} -g ${BINGRP} -m 644 \
		dot.cshrc ${DESTDIR}/root/.cshrc; \
	    ${INSTALL} -o ${BINOWN} -g ${BINGRP} -m 644 \
		dot.login ${DESTDIR}/root/.login; \
	    ${INSTALL} -o ${BINOWN} -g ${BINGRP} -m 644 \
		dot.profile ${DESTDIR}/root/.profile; \
	    rm -f ${DESTDIR}/.cshrc ${DESTDIR}/.profile; \
	cd ${.CURDIR}/mtree; ${INSTALL} -o ${BINOWN} -g ${BINGRP} -m 444 \
	    ${MTREE} ${DESTDIR}/etc/mtree
	cd ${.CURDIR}/ppp; ${INSTALL} -o root -g ${BINGRP} -m 600 \
	    ${PPPCNF} ${DESTDIR}/etc/ppp
	cd ${.CURDIR}/mail; ${INSTALL} -o ${BINOWN} -g ${BINGRP} -m 644 \
	    ${ETCMAIL} ${DESTDIR}/etc/mail
	@if [ -d ${DESTDIR}/etc/mail -a -f ${DESTDIR}/etc/mail/aliases -a \
	      ! -f ${DESTDIR}/etc/aliases ]; then \
		set -x; \
		${LN} -s mail/aliases ${DESTDIR}/etc/aliases; \
	fi
	${INSTALL} -o ${BINOWN} -g operator -m 664 /dev/null \
	    ${DESTDIR}/etc/dumpdates
	${INSTALL} -o nobody -g ${BINGRP} -m 644 /dev/null \
	    ${DESTDIR}/var/db/locate.database
	${INSTALL} -o ${BINOWN} -g ${BINGRP} -m 600 /dev/null \
	    ${DESTDIR}/var/log/auth.log
	${INSTALL} -o ${BINOWN} -g ${BINGRP} -m 600 /dev/null \
	    ${DESTDIR}/var/log/cron
	${INSTALL} -o ${BINOWN} -g ${BINGRP} -m 640 /dev/null \
	    ${DESTDIR}/var/log/daemon
	${INSTALL} -o ${BINOWN} -g ${BINGRP} -m 644 /dev/null \
	    ${DESTDIR}/var/log/lpd-errs
	${INSTALL} -o ${BINOWN} -g ${BINGRP} -m 640 /dev/null \
	    ${DESTDIR}/var/log/maillog
	${INSTALL} -o ${BINOWN} -g ${BINGRP} -m 644 /dev/null \
	    ${DESTDIR}/var/log/lastlogx
	${INSTALL} -o ${BINOWN} -g ${BINGRP} -m 644 /dev/null \
	    ${DESTDIR}/var/log/messages
	${INSTALL} -o ${BINOWN} -g ${BINGRP} -m 600 /dev/null \
	    ${DESTDIR}/var/log/security
	${INSTALL} -o ${BINOWN} -g network -m 640 /dev/null \
	    ${DESTDIR}/var/log/slip.log
	${INSTALL} -o ${BINOWN} -g network -m 640 /dev/null \
	    ${DESTDIR}/var/log/ppp.log
	${INSTALL} -o ${BINOWN} -g ${BINGRP} -m 644 /dev/null \
	    ${DESTDIR}/var/log/wtmpx
	${INSTALL} -o ${BINOWN} -g ${BINGRP} -m 644 /dev/null \
	    ${DESTDIR}/var/run/utmpx
	${INSTALL} -o ${BINOWN} -g ${BINGRP} -m 644 ${.CURDIR}/minfree \
	    ${DESTDIR}/var/crash
	cd ${.CURDIR}/..; ${INSTALL} -o ${BINOWN} -g ${BINGRP} -m 444 \
	    COPYRIGHT ${DESTDIR}/
	rm -f ${DESTDIR}/usr/include/machine/ioctl_bt848.h
	rm -f ${DESTDIR}/usr/include/machine/ioctl_meteor.h
	${LN} -s "../dev/video/bktr/ioctl_bt848.h" ${DESTDIR}/usr/include/machine/ioctl_bt848.h
	${LN} -s "../dev/video/meteor/ioctl_meteor.h" ${DESTDIR}/usr/include/machine/ioctl_meteor.h
.if !defined(NOMAN)
	cd ${.CURDIR}/../share/man; ${MAKE} makedb
.endif

#
#
distrib-dirs:
	mtree ${MTREE_FSCHG} -deU -f ${.CURDIR}/mtree/BSD.root.dist \
		-p ${DESTDIR}/
	mtree ${MTREE_FSCHG} -deU -f ${.CURDIR}/mtree/BSD.var.dist \
		-p ${DESTDIR}/var
	mtree ${MTREE_FSCHG} -deU -f ${.CURDIR}/mtree/BSD.usr.dist \
		-p ${DESTDIR}/usr
	mtree ${MTREE_FSCHG} -deU -f ${.CURDIR}/mtree/BSD.include.dist \
		-p ${DESTDIR}/usr/include
	-chflags xlink ${DESTDIR}/* ${DESTDIR}/usr/* ${DESTDIR}/var/* \
			${DESTDIR}/home/*
	cd ${DESTDIR}/; test -e sys || (rm -f sys;  ${LN} -s usr/src/sys sys)
.if !defined(NO_SHARE)
	cd ${DESTDIR}/usr/share/nls; \
	rm -rf "POSIX"; \
	rm -rf "en_US.US-ASCII"; \
	${LN} -s "C" "POSIX"; \
	${LN} -s "C" "en_US.US-ASCII"
.endif

etc-examples:
	cd ${.CURDIR}; ${INSTALL} -o ${BINOWN} -g ${BINGRP} -m 444 \
	    ${BINUPDATE} ${BIN1} ${BIN2} nsmb.conf \
	    ${DESTDIR}/usr/share/examples/etc
	cd ${.CURDIR}/defaults; ${INSTALL} -o ${BINOWN} -g ${BINGRP} -m 444 \
	    ${DEFAULTS} ${DESTDIR}/usr/share/examples/etc/defaults

.include <bsd.prog.mk>
