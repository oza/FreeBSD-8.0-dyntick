# $FreeBSD: src/sys/modules/gem/Makefile,v 1.2.2.1.2.1 2009/10/25 01:10:29 kensmith Exp $

.PATH: ${.CURDIR}/../../dev/gem

KMOD=	if_gem
SRCS=	bus_if.h device_if.h if_gem.c if_gem_pci.c miibus_if.h pci_if.h

.if ${MACHINE} == "sparc64"
SRCS+=	if_gem_sbus.c ofw_bus_if.h
.endif

.include <bsd.kmod.mk>
