# $FreeBSD: src/sys/modules/ufs/Makefile,v 1.6.2.1.2.1 2009/10/25 01:10:29 kensmith Exp $

.PATH:	${.CURDIR}/../../ufs/ufs ${.CURDIR}/../../ufs/ffs

KMOD=	ufs
SRCS=	opt_ddb.h opt_directio.h opt_ffs.h opt_ffs_broken_fixme.h \
	opt_quota.h opt_suiddir.h opt_ufs.h \
	vnode_if.h ufs_acl.c ufs_bmap.c ufs_dirhash.c ufs_extattr.c \
	ufs_gjournal.c ufs_inode.c ufs_lookup.c ufs_quota.c ufs_vfsops.c \
	ufs_vnops.c ffs_alloc.c ffs_balloc.c ffs_inode.c ffs_snapshot.c \
	ffs_softdep.c ffs_subr.c ffs_tables.c ffs_vfsops.c ffs_vnops.c

.if !defined(KERNBUILDDIR)
CFLAGS+= -DSOFTUPDATES -DUFS_DIRHASH
.endif

.include <bsd.kmod.mk>
