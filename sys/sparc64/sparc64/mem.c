/*-
 * Copyright (c) 1988 University of Utah.
 * Copyright (c) 1982, 1986, 1990 The Regents of the University of California.
 * All rights reserved.
 *
 * This code is derived from software contributed to Berkeley by
 * the Systems Programming Group of the University of Utah Computer
 * Science Department, and code derived from software contributed to
 * Berkeley by William Jolitz.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
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
 *	from: Utah $Hdr: mem.c 1.13 89/10/08$
 *	from: @(#)mem.c	7.2 (Berkeley) 5/9/91
 *	from: FreeBSD: src/sys/i386/i386/mem.c,v 1.94 2001/09/26
 */

#include <sys/cdefs.h>
__FBSDID("$FreeBSD: src/sys/sparc64/sparc64/mem.c,v 1.20.2.1.2.1 2009/10/25 01:10:29 kensmith Exp $");

/*
 * Memory special file
 *
 * NOTE: other architectures support mmap()'ing the mem and kmem devices; this
 * might cause illegal aliases to be created for the locked kernel page(s), so
 * it is not implemented.
 */

#include <sys/param.h>
#include <sys/conf.h>
#include <sys/fcntl.h>
#include <sys/kernel.h>
#include <sys/lock.h>
#include <sys/malloc.h>
#include <sys/memrange.h>
#include <sys/module.h>
#include <sys/mutex.h>
#include <sys/proc.h>
#include <sys/signalvar.h>
#include <sys/systm.h>
#include <sys/uio.h>

#include <vm/vm.h>
#include <vm/vm_param.h>
#include <vm/vm_page.h>
#include <vm/vm_kern.h>
#include <vm/pmap.h>
#include <vm/vm_extern.h>

#ifndef SUN4V
#include <machine/cache.h>
#endif
#include <machine/md_var.h>
#include <machine/pmap.h>
#include <machine/tlb.h>

#include <machine/memdev.h>

struct mem_range_softc mem_range_softc;

/* ARGSUSED */
int
memrw(struct cdev *dev, struct uio *uio, int flags)
{
	struct iovec *iov;
	vm_offset_t eva;
	vm_offset_t off;
	vm_offset_t ova;
	vm_offset_t va;
	vm_prot_t prot;
	vm_paddr_t pa;
	vm_size_t cnt;
	vm_page_t m;
	int error;
	int i;

	cnt = 0;
	error = 0;
	ova = 0;

	GIANT_REQUIRED;

	while (uio->uio_resid > 0 && error == 0) {
		iov = uio->uio_iov;
		if (iov->iov_len == 0) {
			uio->uio_iov++;
			uio->uio_iovcnt--;
			if (uio->uio_iovcnt < 0)
				panic("memrw");
			continue;
		}
		if (dev2unit(dev) == CDEV_MINOR_MEM) {
			pa = uio->uio_offset & ~PAGE_MASK;
			if (!is_physical_memory(pa)) {
				error = EFAULT;
				break;
			}

			off = uio->uio_offset & PAGE_MASK;
			cnt = PAGE_SIZE - ((vm_offset_t)iov->iov_base &
			    PAGE_MASK);
			cnt = ulmin(cnt, PAGE_SIZE - off);
			cnt = ulmin(cnt, iov->iov_len);

			m = NULL;
			for (i = 0; phys_avail[i] != 0; i += 2) {
				if (pa >= phys_avail[i] &&
				    pa < phys_avail[i + 1]) {
					m = PHYS_TO_VM_PAGE(pa);
					break;
				}
			}

			if (m != NULL) {
#ifndef SUN4V
				if (ova == 0)
					ova = kmem_alloc_wait(kernel_map,
					    PAGE_SIZE * DCACHE_COLORS);
				if (m->md.color != -1)
					va = ova + m->md.color * PAGE_SIZE;
				else
					va = ova;
#else
				if (ova == 0)
					ova = kmem_alloc_wait(kernel_map,
					    PAGE_SIZE);
				va = ova;
#endif
				pmap_qenter(va, &m, 1);
				error = uiomove((void *)(va + off), cnt,
				    uio);
				pmap_qremove(va, 1);
			} else {
				va = TLB_PHYS_TO_DIRECT(pa);
				error = uiomove((void *)(va + off), cnt,
				    uio);
			}
			break;
		}
		else if (dev2unit(dev) == CDEV_MINOR_KMEM) {
			va = trunc_page(uio->uio_offset);
			eva = round_page(uio->uio_offset + iov->iov_len);

			/*
			 * Make sure that all of the pages are currently
			 * resident so we don't create any zero fill pages.
			 */
			for (; va < eva; va += PAGE_SIZE)
				if (pmap_kextract(va) == 0)
					return (EFAULT);

			prot = (uio->uio_rw == UIO_READ) ? VM_PROT_READ :
			    VM_PROT_WRITE;
			va = uio->uio_offset;
			if (va < VM_MIN_DIRECT_ADDRESS &&
			    kernacc((void *)va, iov->iov_len, prot) == FALSE)
				return (EFAULT);

			error = uiomove((void *)va, iov->iov_len, uio);
			break;
		}
		/* else panic! */
	}
	if (ova != 0)
#ifndef SUN4V
		kmem_free_wakeup(kernel_map, ova, PAGE_SIZE * DCACHE_COLORS);
#else
		kmem_free_wakeup(kernel_map, ova, PAGE_SIZE);
#endif
	return (error);
}

void
dev_mem_md_init(void)
{
}
