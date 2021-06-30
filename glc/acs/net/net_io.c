/* net_io.c -- Network Send and Receive Functions */

/*----------------------------------------------------------------------------
 * Copyright (c) 1995-2010,2015, Jet Propulsion Laboratory
 * Permission is granted to make and distribute copies of this software
 * without fee, provided the above copyright notice and this permission notice
 * are preserved on all copies.  All other rights reserved.  The software is
 * provided "as is" without express or implied warranty, and no representation
 * is made about its suitability for any purpose.
 *
 * Revision History:
 * 
 *   Date            By               Description
 * 
 * 13-Oct-95     Thang Trinh        Initial release (v1.0) for SunOS 4.1.x
 *				    and Solaris 2.x.
 * 20-Nov-96	 Thang Trinh	    Version 2.0 release to support little-
 *				    endian architecture and generic
 *				    application names.
 *
 * Description:
 *	This module contains functions for sending and receiving data in a
 *	connection-oriented communication.
 *
 *--------------------------------------------------------------------------*/

#ifdef VXWORKS
#include <vxWorks.h>
#include <ioLib.h>
#include <selectLib.h>
#include <sys/times.h>
#include <netinet/in.h>
#include <errno.h>
#else
#include <sys/types.h>
#include <sys/time.h>
#include <netinet/in.h>
#include <errno.h>
#endif

#include "net_appl.h"
#include "net.h"

#define NET_HDR_ID	0x3c54543e	/* ascii representation for "<TT>" */

/* TCP internal message header */

struct msg_hdr_dcl {

    int hdr_id;			/* message header id */
    int msg_len;		/* length of user's message in bytes */
};

/* external variable declarations */

extern sockfd_entry net_sockfd[];
static int net_read_excess ();

#ifdef FUNCT_HDR
/* ***************************************************************************
*
* Synopsis:
*	int net_send (sockfd, msg, length, mode)
* 
* Description:
*	net_send() sends a message to a connected endpoint in a connection-
*	oriented communication.  net_send() preserves message boundaries
*	between the sender and receiver.
*
* Return Values:
*	On success, net_send() returns the number of bytes sent.  If a
*	broken connection condition is detected, net_send() will return
*	NEOF.
*
*	On failure, it returns:
*
*	NBADFD		when sockfd is not a valid socket descriptor.
*
*	NBADADDR	when the message pointer is not a valid pointer.
*
*	NBADLENGTH	when the requested message length either exceeds
*			the maximum length allowed or is less than the
*			minimum required.
*
*	NBADMODE	when the mode is not a valid I/O mode.
*
*	NWOULDBLOCK	when the I/O mode is NON_BLOCKING and no
*			messages could be sent immediately.  It is
*			possible, however, that a partial message may have
*			been sent when this error is returned.
*
*	ERROR		on a system call error, with errno containing the
*			error indication.
*
* Environment Access:
*	None.
*
* Performance:
*	N/A
*
* Portability:
*	None.
*
* Notes:
*	None.
* 
*************************************************************************** */
#endif

int net_send (sockfd, msg, length, mode)
int sockfd;				/* endpoint socket descriptor */
char *msg;				/* message to be sent */
int length;				/* message length in bytes */
io_mode mode;				/* send I/O mode */
{
    int status;				/* return status */
    int nwritten;			/* number of bytes written */
    int nleft;				/* remaining bytes to write */
    int ndelay;				/* number of delays before quitting */
    char *msgptr;			/* output buffer */

    struct msg_hdr_dcl msg_hdr = {NET_HDR_ID, 0};

    /* validate socket descriptor */

    if (sockfd < 0 || sockfd >= NET_MAX_FD ||
				    net_sockfd[sockfd].type == UNDEF)
	return NBADFD;

    /* validate msg pointer */

    if (msg == (char *) NULL)
	return NBADADDR;

    /* validate msg length */

    if (length < NET_MIN_MSG_LEN || length > NET_MAX_MSG_LEN)
	return NBADLENGTH;

    /* validate and set socket I/O mode */

    if (mode != BLOCKING && mode != NON_BLOCKING)
	return NBADMODE;

    if ((status = net_setiomode (sockfd, mode)) < 0)
	return status;

    /* output internal message header */

    msg_hdr.hdr_id  = htonl (NET_HDR_ID);
    msg_hdr.msg_len = htonl (length);
    msgptr = (char *) &msg_hdr;
    nleft  = sizeof (msg_hdr);

    while (nleft > 0) {

	nwritten = write (sockfd, msgptr, nleft);

	if (nwritten == ERROR) {
	    if (errno == EINTR) {
		errno = 0;
		continue;
	    }
	    else if (errno == EWOULDBLOCK)
		return NWOULDBLOCK;

	    /* if broken pipe, return NEOF */
	    else if (errno == EPIPE)
		return NEOF;

	    else
		return ERROR;
	}
	else if (nwritten == 0)
	    return NEOF;

	/* update amount written */

	nleft  -= nwritten;
	msgptr += nwritten;
    }
    /* output actual user's message */

    ndelay = 0;
    nleft  = length;

    while (nleft > 0) {

	nwritten = write (sockfd, msg, nleft);

	if (nwritten == ERROR) {
	    if (errno == EINTR) {
		errno = 0;
		continue;
	    }
	    else if (errno == EWOULDBLOCK) {
		struct timeval delay;

		delay.tv_sec  = 0;
		delay.tv_usec = NET_MIN_USEC_DELAY;

		if (++ndelay > NET_MAX_NDELAY)
		    return NWOULDBLOCK;

		(void) select (0, (fd_set *)0, (fd_set *)0, (fd_set *)0,
								&delay);
		continue;
	    }

	    /* if broken pipe, return NEOF */
	    else if (errno == EPIPE)
		return NEOF;

	    else
		return ERROR;
	}
	else if (nwritten == 0)
	    return NEOF;

	/* update amount written */

	nleft -= nwritten;
	msg   += nwritten;
    }
    /* return number of bytes written */

    return (length - nleft);
}

#ifdef FUNCT_HDR
/* ***************************************************************************
*
* Synopsis:
*	int net_recv (sockfd, buff, maxlen, mode)
* 
* Description:
*	net_recv() receives a message from a connected endpoint in a
*	connection-oriented communication.  net_recv() preserves message
*	boundaries between the sender and receiver.  The received message
*	will be placed into the array buff for up to maxlen bytes.  If the
*	message is too long to fit in the supplied buffer, it will be
*	truncated and the excess bytes discarded.
*
* Return Values:
*	On success, net_recv() returns the number of bytes received.  If a
*	broken connection condition is detected, net_recv() will return
*	NEOF.
*
*	On failure, it returns:
*
*	NBADFD		when sockfd is not a valid socket descriptor.
*
*	NBADADDR	when the buffer pointer is not a valid pointer.
*
*	NBADLENGTH	when the requested message length is less than
*			the minimum length required.
*
*	NBADMODE	when the mode is not a valid I/O mode.
*
*	NSYNCERR	when the incoming message boundaries are out of
*			sync.
*
*	NWOULDBLOCK	when the I/O mode is NON_BLOCKING and no
*			messages were available to be received.  It is
*			possible, however, that a partial message may have
*			been received when this error is returned.
*
*	ERROR		on a system call error, with errno containing the
*			error indication.
*
* Environment Access:
*	None.
*
* Performance:
*	N/A
*
* Portability:
*	None.
*
* Notes:
*	None.
* 
*************************************************************************** */
#endif

int net_recv (sockfd, buff, maxlen, mode)
int sockfd;				/* endpoint socket descriptor */
char *buff;				/* buffer area to receive msg into */
int maxlen;				/* length in bytes of buffer area */
io_mode mode;				/* receive I/O mode */
{
    int status;				/* return status */
    int nread;				/* number of bytes read */
    int nleft;				/* remaining bytes to read */
    int nbytes;				/* number of bytes placed in buff */
    int nexcess;			/* number of excess bytes */
    int ndelay;				/* number of delays before quitting */
    char *bufptr;			/* input buffer pointer */
    struct msg_hdr_dcl msg_hdr;		/* internal message header */


    /* validate socket descriptor */

    if (sockfd < 0 || sockfd >= NET_MAX_FD ||
				    net_sockfd[sockfd].type == UNDEF)
	return NBADFD;

    /* validate buff pointer */

    if (buff == (char *) NULL)
	return NBADADDR;

    /* validate buffer length */

    if (maxlen < NET_MIN_MSG_LEN)
	return NBADLENGTH;

    /* validate and set socket I/O mode */

    if (mode != BLOCKING && mode != NON_BLOCKING)
	return NBADMODE;

    if ((status = net_setiomode (sockfd, mode)) < 0)
	return status;

    /* read internal message header */

    bufptr = (char *) &msg_hdr;
    nleft  = sizeof (msg_hdr);

    while (nleft > 0) {

	nread = read (sockfd, bufptr, nleft);

	if (nread == ERROR) {
	    if (errno == EINTR) {
		errno = 0;
		continue;
	    }
	    else if (errno == EWOULDBLOCK)
		return NWOULDBLOCK;

	    else
		return ERROR;
	}
	else if (nread == 0)
	    return NEOF;

	/* update amount read */

	nleft  -= nread;
	bufptr += nread;
    }
    /* check message header id */

    if (ntohl (msg_hdr.hdr_id) != NET_HDR_ID)
	return NSYNCERR;

    /* read message into user's buffer */

    nbytes = 0;
    ndelay = 0;
    if (ntohl (msg_hdr.msg_len) < maxlen)
	nleft = ntohl (msg_hdr.msg_len);
    else
	nleft = maxlen;

    while (nleft > 0) {

	nread = read (sockfd, buff, nleft);

	if (nread == ERROR) {
	    if (errno == EINTR) {
		errno = 0;
		continue;
	    }
	    else if (errno == EWOULDBLOCK) {
		struct timeval delay;

		delay.tv_sec  = 0;
		delay.tv_usec = NET_MIN_USEC_DELAY;

		if (++ndelay > NET_MAX_NDELAY)
		    return NWOULDBLOCK;

		(void) select (0, (fd_set *)0, (fd_set *)0, (fd_set *)0,
								&delay);
		continue;
	    }
	    else
		return ERROR;
	}
	else if (nread == 0)
	    return NEOF;

	/* update amount read */

	nleft  -= nread;
	nbytes += nread;
	buff   += nread;
    }
    /* read and discard excess bytes */

    nexcess = ntohl (msg_hdr.msg_len) - maxlen;
    if (nexcess > 0) {
	status = net_read_excess (sockfd, nexcess);
	if (status < 0)
	    return (status);
    }

    /* return number of bytes placed in user's buffer */

    return (nbytes);
}

#ifdef FUNCT_HDR
/* ***************************************************************************
*
* Synopsis:
*       static int net_read_excess (sockfd, nexcess)
* 
* Description:
*	net_read_excess() reads and discards excess bytes from a connected
*	endpoint.  It is called by net_recv() to truncate a received
*	message that is too long to fit in the user-supplied buffer.
*
* Return Values:
*	On success, net_read_excess() returns the number of bytes read and
*	discarded.  If a broken connection condition is detected,
*	net_read_excess() will return NEOF.
*
*	On failure, it returns:
*
*	NWOULDBLOCK	when the I/O mode is NON_BLOCKING and no
*			messages were available to be received.
*
*	ERROR		on a system call error, with errno containing the
*			error indication.
*
* Environment Access:
*	None.
*
* Performance:
*	N/A
*
* Portability:
*	None.
*
* Notes:
*	None.
* 
*************************************************************************** */
#endif

#define NET_BUFSIZE	4096		/* size of read buffer */

static int net_read_excess (sockfd, nexcess)
int sockfd;				/* endpoint socket descriptor */
int nexcess;				/* number of excess bytes to discard */
{
    int nread;				/* number of bytes read */
    int nleft;				/* remaining bytes to read */
    int ndelay;				/* number of delays before quitting */
    char buff[NET_BUFSIZE];		/* excess read buffer */

    ndelay = 0;
    nleft  = nexcess;

    while (nleft > 0) {

	/* read excess bytes in NET_BUFSIZE increments */

	nread = read (sockfd, buff,
			      (nleft > NET_BUFSIZE)? NET_BUFSIZE : nleft);

	if (nread == ERROR) {
	    if (errno == EINTR) {
		errno = 0;
		continue;
	    }
	    else if (errno == EWOULDBLOCK) {
		struct timeval delay;

		delay.tv_sec  = 0;
		delay.tv_usec = NET_MIN_USEC_DELAY;

		if (++ndelay > NET_MAX_NDELAY)
		    return NWOULDBLOCK;

		(void) select (0, (fd_set *)0, (fd_set *)0, (fd_set *)0,
								&delay);
		continue;
	    }
	    else
		return ERROR;
	}
	else if (nread == 0)
	    return NEOF;

	/* update amount read */

	nleft -= nread;
    }
    return (nexcess - nleft);
}

