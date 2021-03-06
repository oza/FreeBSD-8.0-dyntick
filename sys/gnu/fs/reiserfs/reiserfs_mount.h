/*
 * Copyright 2000 Hans Reiser
 * See README for licensing and copyright details
 * 
 * Ported to FreeBSD by Jean-S�bastien P�dron <jspedron@club-internet.fr>
 * 
 * $FreeBSD: src/sys/gnu/fs/reiserfs/reiserfs_mount.h,v 1.2.22.1.2.1 2009/10/25 01:10:29 kensmith Exp $
 */

#ifndef _GNU_REISERFS_REISERFS_MOUNT_H
#define _GNU_REISERFS_REISERFS_MOUNT_H

#if defined(_KERNEL)

#ifdef MALLOC_DECLARE
MALLOC_DECLARE(M_REISERFSMNT);
MALLOC_DECLARE(M_REISERFSPATH);
MALLOC_DECLARE(M_REISERFSNODE);
MALLOC_DECLARE(M_REISERFSCOOKIES);
#endif

/* This structure describes the ReiserFS specific mount structure data. */
struct reiserfs_mount {
	struct mount	*rm_mountp;
	struct cdev	*rm_dev;
	struct vnode	*rm_devvp;

	struct reiserfs_sb_info *rm_reiserfs;

	struct g_consumer *rm_cp;
	struct bufobj	*rm_bo;
};

/* Convert mount ptr to reiserfs_mount ptr. */
#define VFSTOREISERFS(mp)	((struct reiserfs_mount *)((mp)->mnt_data))

#endif /* defined(_KERNEL) */

/* Arguments to mount ReiserFS filesystems. */
struct reiserfs_args {
	char	*fspec;		/* blocks special holding the fs to mount */
	struct export_args export;	/* network export information */
};

#endif /* !defined _GNU_REISERFS_REISERFS_MOUNT_H */
