/*****************************************************************************
 *
 * Filename: tag_list.h
 *
 * Definition of basic routines that create linux-boot tag list.
 *
 * Revision information:
 *
 * 22AUG2004	kb_admin	initial creation
 *
 * BEGIN_KBDD_BLOCK
 * No warranty, expressed or implied, is included with this software.  It is
 * provided "AS IS" and no warranty of any kind including statutory or aspects
 * relating to merchantability or fitness for any purpose is provided.  All
 * intellectual property rights of others is maintained with the respective
 * owners.  This software is not copyrighted and is intended for reference
 * only.
 * END_BLOCK
 *
 * $FreeBSD: src/sys/boot/arm/at91/libat91/tag_list.h,v 1.2.12.1.2.1 2009/10/25 01:10:29 kensmith Exp $
 ****************************************************************************/

#ifndef _TAG_LIST_H_
#define _TAG_LIST_H_

extern void InitTagList(char *parms, void*);

#endif /* _TAG_LIST_H_ */
