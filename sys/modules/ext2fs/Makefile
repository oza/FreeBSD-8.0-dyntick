# $FreeBSD: src/sys/modules/ext2fs/Makefile,v 1.6.22.1.2.1 2009/10/25 01:10:29 kensmith Exp $

.PATH:	${.CURDIR}/../../gnu/fs/ext2fs
KMOD=	ext2fs
SRCS=	opt_ddb.h opt_quota.h opt_suiddir.h vnode_if.h \
	ext2_alloc.c ext2_balloc.c ext2_bmap.c ext2_inode.c \
	ext2_inode_cnv.c ext2_linux_balloc.c ext2_linux_ialloc.c \
	ext2_lookup.c ext2_subr.c ext2_vfsops.c ext2_vnops.c

.include <bsd.kmod.mk>
