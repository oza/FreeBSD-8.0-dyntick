/******************************************************************************
 * Client-facing interface for the Xenbus driver.  In other words, the
 * interface between the Xenbus and the device-specific code, be it the
 * frontend or the backend of that driver.
 *
 * Copyright (C) 2005 XenSource Ltd
 * 
 * This file may be distributed separately from the Linux kernel, or
 * incorporated into other software packages, subject to the following license:
 * 
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this source file (the "Software"), to deal in the Software without
 * restriction, including without limitation the rights to use, copy, modify,
 * merge, publish, distribute, sublicense, and/or sell copies of the Software,
 * and to permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 * 
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
 * IN THE SOFTWARE.
 */


#if 0
#define DPRINTK(fmt, args...) \
    printk("xenbus_client (%s:%d) " fmt ".\n", __FUNCTION__, __LINE__, ##args)
#else
#define DPRINTK(fmt, args...) ((void)0)
#endif

#include <sys/cdefs.h>
__FBSDID("$FreeBSD: src/sys/xen/xenbus/xenbus_client.c,v 1.5.2.1.2.1 2009/10/25 01:10:29 kensmith Exp $");

#include <sys/cdefs.h>
#include <sys/types.h>
#include <sys/malloc.h>
#include <sys/libkern.h>

#include <machine/xen/xen-os.h>
#include <xen/hypervisor.h>
#include <xen/evtchn.h>
#include <xen/gnttab.h>
#include <xen/xenbus/xenbusvar.h>
#include <machine/stdarg.h>

const char *
xenbus_strstate(XenbusState state)
{
	static const char *const name[] = {
		[ XenbusStateUnknown      ] = "Unknown",
		[ XenbusStateInitialising ] = "Initialising",
		[ XenbusStateInitWait     ] = "InitWait",
		[ XenbusStateInitialised  ] = "Initialised",
		[ XenbusStateConnected    ] = "Connected",
		[ XenbusStateClosing      ] = "Closing",
		[ XenbusStateClosed	  ] = "Closed",
	};

	return ((state < (XenbusStateClosed + 1)) ? name[state] : "INVALID");
}

int 
xenbus_watch_path(device_t dev, char *path, struct xenbus_watch *watch, 
    void (*callback)(struct xenbus_watch *, const char **, unsigned int))
{
	int error;

	watch->node = path;
	watch->callback = callback;

	error = register_xenbus_watch(watch);

	if (error) {
		watch->node = NULL;
		watch->callback = NULL;
		xenbus_dev_fatal(dev, error, "adding watch on %s", path);
	}

	return (error);
}

int
xenbus_watch_path2(device_t dev, const char *path,
    const char *path2, struct xenbus_watch *watch, 
    void (*callback)(struct xenbus_watch *, const char **, unsigned int))
{
	int error;
	char *state = malloc(strlen(path) + 1 + strlen(path2) + 1,
	    M_DEVBUF, M_WAITOK);

	strcpy(state, path);
	strcat(state, "/");
	strcat(state, path2);

	error = xenbus_watch_path(dev, state, watch, callback);
	if (error) {
		free(state, M_DEVBUF);
	}

	return (error);
}

/**
 * Return the path to the error node for the given device, or NULL on failure.
 * If the value returned is non-NULL, then it is the caller's to kfree.
 */
static char *
error_path(device_t dev)
{
	char *path_buffer = malloc(strlen("error/")
	    + strlen(xenbus_get_node(dev)) + 1, M_DEVBUF, M_WAITOK);

	strcpy(path_buffer, "error/");
	strcpy(path_buffer + strlen("error/"), xenbus_get_node(dev));

	return (path_buffer);
}


static void
_dev_error(device_t dev, int err, const char *fmt, va_list ap)
{
	int ret;
	unsigned int len;
	char *printf_buffer = NULL, *path_buffer = NULL;

#define PRINTF_BUFFER_SIZE 4096
	printf_buffer = malloc(PRINTF_BUFFER_SIZE, M_DEVBUF, M_WAITOK);

	len = sprintf(printf_buffer, "%i ", err);
	ret = vsnprintf(printf_buffer+len, PRINTF_BUFFER_SIZE-len, fmt, ap);

	KASSERT(len + ret <= PRINTF_BUFFER_SIZE-1, ("xenbus error message too big"));
#if 0	
	dev_err(&dev->dev, "%s\n", printf_buffer);
#endif		
	path_buffer = error_path(dev);

	if (path_buffer == NULL) {
		printf("xenbus: failed to write error node for %s (%s)\n",
		       xenbus_get_node(dev), printf_buffer);
		goto fail;
	}

	if (xenbus_write(XBT_NIL, path_buffer, "error", printf_buffer) != 0) {
		printf("xenbus: failed to write error node for %s (%s)\n",
		       xenbus_get_node(dev), printf_buffer);
		goto fail;
	}

 fail:
	if (printf_buffer)
		free(printf_buffer, M_DEVBUF);
	if (path_buffer)
		free(path_buffer, M_DEVBUF);
}

void
xenbus_dev_error(device_t dev, int err, const char *fmt, ...)
{
	va_list ap;

	va_start(ap, fmt);
	_dev_error(dev, err, fmt, ap);
	va_end(ap);
}

void
xenbus_dev_fatal(device_t dev, int err, const char *fmt, ...)
{
	va_list ap;

	va_start(ap, fmt);
	_dev_error(dev, err, fmt, ap);
	va_end(ap);
	
	xenbus_set_state(dev, XenbusStateClosing);
}

int
xenbus_grant_ring(device_t dev, unsigned long ring_mfn, int *refp)
{
	int error;
	grant_ref_t ref;

	error = gnttab_grant_foreign_access(
		xenbus_get_otherend_id(dev), ring_mfn, 0, &ref);
	if (error) {
		xenbus_dev_fatal(dev, error, "granting access to ring page");
		return (error);
	}

	*refp = ref;
	return (0);
}

int
xenbus_alloc_evtchn(device_t dev, int *port)
{
	struct evtchn_alloc_unbound alloc_unbound;
	int err;

	alloc_unbound.dom        = DOMID_SELF;
	alloc_unbound.remote_dom = xenbus_get_otherend_id(dev);

	err = HYPERVISOR_event_channel_op(EVTCHNOP_alloc_unbound,
					  &alloc_unbound);

	if (err) {
		xenbus_dev_fatal(dev, -err, "allocating event channel");
		return (-err);
	}
	*port = alloc_unbound.port;
	return (0);
}

int
xenbus_free_evtchn(device_t dev, int port)
{
	struct evtchn_close close;
	int err;

	close.port = port;

	err = HYPERVISOR_event_channel_op(EVTCHNOP_close, &close);
	if (err) {
		xenbus_dev_error(dev, -err, "freeing event channel %d", port);
		return (-err);
	}
	return (0);
}

XenbusState
xenbus_read_driver_state(const char *path)
{
	XenbusState result;
	int error;

	error = xenbus_gather(XBT_NIL, path, "state", "%d", &result, NULL);
	if (error)
		result = XenbusStateClosed;

	return (result);
}