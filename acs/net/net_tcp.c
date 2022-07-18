/* net_tcp.c -- TCP Connection Initialization and Termination Functions */

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
 * 16-Oct-95     Thang Trinh        Initial release (v1.0) for SunOS 4.1.x
 *				    and Solaris 2.x.
 * 14-May-99     Thang Trinh        Version 3.0 release for all supported
 *				    platforms.
 *
 * Description:
 *	This module contains functions for initializing server network
 *	connections, initiating and accepting connection requests, and
 *	closing connections for client and server processes communicating
 *	using TCP/IP.
 *
 *--------------------------------------------------------------------------*/

#ifdef VXWORKS
#include <vxWorks.h>
#include <sys/types.h>
#include <sys/times.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <hostLib.h>
#include <ioLib.h>
#include <selectLib.h>
#include <sockLib.h>
#include <signal.h>
#include <string.h>
#include <errno.h>
#else   /* !VXWORKS */
#ifdef __linux__
#include <sys/ioctl.h>	/* for FIONBIO symbol */
#else
#include <sys/filio.h>	/* for FIONBIO symbol */
#endif
#include <sys/types.h>
#include <sys/time.h>
#include <sys/socket.h> 
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <netdb.h>
#include <signal.h>
#include <string.h>
#include <errno.h>
#endif

#include <stdio.h>
#include "net_appl.h"
#include "net.h"

/* global variable definitions */

sockfd_entry net_sockfd[NET_MAX_FD] = { {UNDEF, BLOCKING} };

#ifdef FUNCT_HDR
/* ***************************************************************************
*
* Synopsis:
*       int net_getservport (endpt, type)
* 
* Description:
*	net_getservport() returns the port number associated with a
*	server's endpoint name.
*
* Return Values:
*	net_getservport() returns the server's port number on success, and
*	ERROR if the endpoint name could not be found.
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

int net_getservport (endpt, type)
char *endpt;				/* server's endpoint name */
endpt_type type;			/* server's endpoint type */
{
    int i;				/* loop index */

    for (i = 0; i < NET_MAX_ENDPTS; i++) {
	if ((net_endpt[i].type == type) &&
				(strcmp (net_endpt[i].name, endpt) == 0))
	    return net_endpt[i].port;
    }
    return ERROR;
}

#ifdef FUNCT_HDR
/* ***************************************************************************
*
* Synopsis:
*       int net_init (endpt)
* 
* Description:
*	net_init() initializes the network connection for a server and
*       opens a listening endpoint to listen for incoming connection
*       requests.  net_init() is to be called by every server process as a
*       first step in establishing a reliable, full-duplex TCP connection
*       with a client process.  It returns a socket descriptor to be used
*	in a subsequent net_accept() call to accept incoming connection
*	requests.
*
* Return Values:
*	On success, net_init() returns a file descriptor for the listening
*	socket to be used in a subsequent net_accept() call to accept
*	incoming connection requests.
* 
*	On failure, it returns:
*
*	NBADENDPT       when the endpoint name is not a valid endpoint.
*
*	ERROR           on a system call error, with errno containing the
*			error indication.
*
* Environment Access:
*	None.
*
* Performance:
*	N/A
*
* Portability:
*	This function uses the Berkeley socket facility for network
*	communications.
*
* Notes:
*	None.
* 
*************************************************************************** */
#endif

int net_init (endpt)
char *endpt;				/* server's endpoint name */
{
    struct sockaddr_in server;		/* server's socket address */
    int	port;				/* server's port number */
    int listenfd;			/* server's listen socket */

    /* initialize server's address */

    (void) memset ((char *) &server, 0, sizeof (server));
    server.sin_family = AF_INET;
    server.sin_addr.s_addr = htonl (INADDR_ANY);

    /* get port number associated with endpoint name */

    if (endpt == NULL || (port = net_getservport (endpt, TCP)) == ERROR)
	return NBADENDPT;
    else
	server.sin_port = htons (port);

    /* create listening socket and bind to local address */

    if ((listenfd = socket (AF_INET, SOCK_STREAM, 0)) == ERROR)
	return ERROR;

    if (bind (listenfd, (struct sockaddr *) &server,
					    sizeof (server)) == ERROR) {
	(void) close (listenfd);
	return ERROR;
    }
    /* listen for connection requests */

    (void) listen (listenfd, 5);

    net_sockfd[listenfd].type = TCP;
    net_sockfd[listenfd].mode = BLOCKING;

    return listenfd;
}

#ifdef FUNCT_HDR
/* ***************************************************************************
*
* Synopsis:
*	int net_accept (listenfd, mode)
* 
* Description:
*	net_accept() accepts a connection request from and establishes a
*	full-duplex TCP connection with a client process that issued a
*	net_connect() call to connect to a server.  The server must have
*	previously called net_init() prior to calling net_accept().
*
* Return Values:
*	On success, net_accept() returns a file descriptor for the
*	connected socket to be used in a subsequent net_send(),
*	net_recv(), or net_close() call.
*
*	On failure, it returns:
*
*	NBADFD          when listenfd is not a valid socket descriptor.
*
*	NBADMODE        when the mode is not a valid I/O mode.
*
*	NWOULDBLOCK     when the I/O mode is NON_BLOCKING and no
*			connection requests are present to be accepted.
*
*	ERROR           on a system call error, with errno containing the
*			error indication.
*
* Environment Access:
*	None.
*
* Performance:
*	N/A
*
* Portability:
*	This function uses the Berkeley socket facility for network
*	communications.
*
* Notes:
*	None.
* 
*************************************************************************** */
#endif

int net_accept (listenfd, mode)
int listenfd;				/* listen socket descriptor */
io_mode mode;				/* listen socket I/O mode */
{
    int sockfd;				/* connected socket descriptor */
    struct sockaddr_in	client;		/* client's socket address */
    socklen_t client_len;		/* length of client's address */
    int status;				/* return status */
    int on = 1;				/* option flag for setsockopt() */
    struct linger off = {1, 0};		/* linger flag for setsockopt() */

    /* validate socket descriptor and I/O mode */

    if (listenfd < 0 || listenfd >= NET_MAX_FD ||
					net_sockfd[listenfd].type == UNDEF)
	return NBADFD;

    if (mode != BLOCKING && mode != NON_BLOCKING)
	return NBADMODE;

    /* set socket I/O mode */

    if ((status = net_setiomode (listenfd, mode)) < 0)
	return status;

    /* accept connection requests */

    client_len = sizeof (client);
    sockfd = accept (listenfd, (struct sockaddr *) &client, &client_len);

    if (sockfd == ERROR) {

	if (errno == EWOULDBLOCK)
	    return NWOULDBLOCK;
	else
	    return ERROR;
    }

    /* set option to not linger */

    if (setsockopt (sockfd, SOL_SOCKET, SO_LINGER, (char *) &off,
						   sizeof off) == ERROR) {
	(void) close (sockfd);
	return ERROR;
    }

    /* set option to not delay-send */

    if (setsockopt (sockfd, IPPROTO_TCP, TCP_NODELAY, (char *) &on,
						      sizeof on) == ERROR) {
	(void) close (sockfd);
	return ERROR;
    }

    net_sockfd[sockfd].type = TCP;
    net_sockfd[sockfd].mode = BLOCKING;

    /* ignore broken pipe signals */

    (void) signal (SIGPIPE, SIG_IGN);

    return sockfd;
}

#ifdef FUNCT_HDR
/* ***************************************************************************
*
* Synopsis:
*       int net_connect (endpt, hostname, pname, mode)
* 
* Description:
*	net_connect() initiates a connection request to a listening
*	process and is called by a client process to establish a
*	connection with a server.  Once the connection request is accepted
*	by a server calling net_accept(), messages can be exchanged over
*	the established connection.
*
* Return Values:
*	On success, net_connect() returns a file descriptor for the
*	connected socket to be used in a subsequent net_send(),
*	net_recv(), or net_close() call.
*
*	On failure, it returns:
*
*	NBADENDPT	when the endpoint name is not a valid endpoint.
*
*	NBADHOST	when the hostname is not a valid hostname.
*
*	NBADPROCESS	when the process name is not a valid process.
*
*	NBADMODE	when the mode is not a valid I/O mode.
*
*	NWOULDBLOCK	when the I/O mode is NON_BLOCKING and the
*			connection cannot be completed immediately.
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
*	This function uses the Berkeley socket facility for network
*	communications.
*
* Notes:
*	None.
* 
*************************************************************************** */
#endif

int net_connect (endpt, hostname, pname, mode)
char *endpt;				/* server's endpoint name */
char *hostname;				/* server's hostname */
int pname;				/* client's program name */
io_mode mode;				/* I/O mode of connection attempt */
{
    struct sockaddr_in client;		/* client's socket address */
    struct sockaddr_in server;		/* server's socket address */
    int	port;				/* server's port number */
    struct hostent *hostp;		/* server's host entry pointer */
    int sockfd;				/* connecting socket descriptor */
    int ndelay = 0;			/* number of delays before quitting */
    int on = 1;				/* option flag for setsockopt() */

    /* initialize server's address */

    (void) memset ((char *) &server, 0, sizeof (server));
    server.sin_family = AF_INET;

    /* get port number associated with endpoint name */

    if (endpt == NULL || (port = net_getservport (endpt, TCP)) == ERROR)
	return NBADENDPT;
    else
	server.sin_port = htons (port);

    /* get server's host address */

#ifdef VXWORKS
    if ((server.sin_addr.s_addr = hostGetByName (hostname)) == ERROR)
	return NBADHOST;
#else
    if ((hostp = gethostbyname (hostname)) == NULL)
	return NBADHOST;

    (void) memcpy ((char *) &server.sin_addr, hostp->h_addr, hostp->h_length);
#endif

    /* create socket and bind to local address */

    (void) memset ((char *) &client, 0, sizeof (client));
    client.sin_family      = AF_INET;
    client.sin_addr.s_addr = htonl (INADDR_ANY);

    if (pname >= 0 && pname < MAXTASKS)
	client.sin_port = htons (net_port[pname]);
    else
	return NBADPROCESS;

    if ((sockfd = socket (AF_INET, SOCK_STREAM, 0)) == ERROR)
	return ERROR;

    /* set option to reuse address */

    if (setsockopt (sockfd, SOL_SOCKET, SO_REUSEADDR, (char *) &on,
						      sizeof on) == ERROR) {
	(void) close (sockfd);
	return ERROR;
    }

    if (bind (sockfd, (struct sockaddr *) &client,
					  sizeof (client)) == ERROR) {
	(void) close (sockfd);
	return ERROR;
    }

    /* set socket I/O mode */

    if (mode == NON_BLOCKING) {

	if (ioctl (sockfd, FIONBIO, (char *) &on) == ERROR) {
	    (void) close (sockfd);
	    return ERROR;
	}
    }
    else if (mode != BLOCKING) {
	(void) close (sockfd);
	return NBADMODE;
    }

    /* connect to the server */
again:
    if (connect (sockfd, (struct sockaddr *) &server,
					     sizeof (server)) == ERROR) {

	if (errno == EINPROGRESS || errno == EALREADY) {
	    /* try to complete connection for non-blocking socket only */

	    struct timeval delay;

	    delay.tv_sec  = 0;
	    delay.tv_usec = NET_MIN_USEC_DELAY;

	    if (++ndelay > NET_MAX_NDELAY) {
		(void) close (sockfd);
		return NWOULDBLOCK;
	    }
	    (void) select (0, (fd_set *)0, (fd_set *)0, (fd_set *)0, &delay);
	    goto again;
	}
	else if (errno != EISCONN) {
	    (void) close (sockfd);
	    return ERROR;
	}
    }

    net_sockfd[sockfd].type = TCP;
    net_sockfd[sockfd].mode = mode;

    /* ignore broken pipe signals */

    (void) signal (SIGPIPE, SIG_IGN);

    return sockfd;
}

#ifdef FUNCT_HDR
/* ***************************************************************************
*
* Synopsis:
*	int net_close (sockfd)
* 
* Description:
*	net_close() closes the communication endpoint identified by the
*	supplied socket descriptor and deletes the descriptor for future
*	re-use.
*
* Return Values:
*	net_close() returns SUCCESS on success.
*
*	On failure, it returns:
*
*	NBADFD		when sockfd is not a valid socket descriptor.
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

int net_close (sockfd)
int sockfd;				/* socket descriptor to be closed */
{
    /* validate socket descriptor */

    if (sockfd < 0 || sockfd >= NET_MAX_FD ||
				net_sockfd[sockfd].type == UNDEF)
	return NBADFD;

    if (close (sockfd) == ERROR)
	return ERROR;

    net_sockfd[sockfd].type = UNDEF;
    net_sockfd[sockfd].mode = BLOCKING;

    return (0);
}

#ifdef FUNCT_HDR
/* ***************************************************************************
*
* Synopsis:
*	int net_getpeername (sockfd, pname, hostname, namelen)
* 
* Description:
*	net_getpeername() returns the host name and process name of the
*	peer connected to the endpoint identified by the supplied socket.
*
* Return Values:
*	net_getpeername() returns SUCCESS on success.
*
*	On failure, it returns:
*
*	NBADFD		when sockfd is not a valid socket descriptor.
*
*	NBADADDR	when pname and/or hostname are not valid pointers.
*
*	NBADHOST	when the peer's host entry cannot be found.  This
*			is an internal error condition.
*
*	NBADPROCESS	when the peer's process name entry cannot be
*			found.  This is an internal error condition.
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
*	This function uses the Berkeley socket facility for network
*	communications.
*
* Notes:
*	None.
* 
*************************************************************************** */
#endif

int net_getpeername (sockfd, pname, hostname, namelen)
int sockfd;				/* socket descriptor for which peer
					   name is returned */
int *pname;				/* returned peer process name */
char *hostname;				/* returned peer's hostname */
int namelen;				/* hostname length in bytes */
{
    struct sockaddr_in peer;		/* peer's socket address */
    socklen_t peer_len = sizeof (peer);	/* length of peer's address */
    struct hostent *hostp;		/* peer's host entry pointer */
    int i;				/* loop index */

    /* validate socket descriptor */

    if (sockfd < 0 || sockfd >= NET_MAX_FD ||
				net_sockfd[sockfd].type != TCP)
	return NBADFD;

    /* validate name pointers */

    if (pname == NULL || hostname == NULL)
	return NBADADDR;

    if (getpeername (sockfd, (struct sockaddr *) &peer, &peer_len) < 0)
	return ERROR;

    /* set peer's hostname */

#ifdef VXWORKS
    {
    char hname[MAXHOSTNAMELEN+1];

    if (hostGetByAddr (peer.sin_addr.s_addr, hname) == ERROR)
	return NBADHOST;

    (void) strncpy (hostname, hname, namelen);
    }
#else
    if ((hostp = gethostbyaddr ((char *) &peer.sin_addr,
				sizeof (peer.sin_addr), AF_INET)) == NULL)
	return NBADHOST;

    (void) strncpy (hostname, hostp->h_name, namelen);
#endif

    /* set peer's process name */

    for (i = 0; i < MAXTASKS; i++) {
	if (net_port[i] == ntohs (peer.sin_port)) {
	    *pname = i;
	    break;
	}
    }
    if (i >= MAXTASKS) {
	for (i = 0; i < NET_MAX_ENDPTS; i++) {
	    if (net_endpt[i].port == ntohs (peer.sin_port)) {
		*pname = net_endpt[i].pname;
		break;
	    }
	}
	if (i >= NET_MAX_ENDPTS)
	    return NBADPROCESS;
    }

    return (0);
}

#ifdef FUNCT_HDR
/* ***************************************************************************
*
* Synopsis:
*       int net_setiomode (sockfd, mode)
* 
* Description:
*	net_setiomode() sets the I/O mode of the supplied socket descriptor.
*	Mode can be either BLOCKING or NON_BLOCKING.
*
* Return Values:
*	net_setiomode() returns SUCCESS on success.
*
*	On failure, it returns:
*
*	NBADFD		when sockfd is not a valid socket descriptor.
*
*	ERROR		on an ioctl() call error, with errno containing the
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

int net_setiomode (sockfd, mode)
int sockfd;				/* socket descriptor */
io_mode mode;				/* socket I/O mode */
{
    int on  = 1;			/* on/off flags for ioctl() */
    int off = 0;

    if (net_sockfd[sockfd].type == UNDEF)
	return NBADFD;

    /* change socket I/O mode if different */

    if (net_sockfd[sockfd].mode != mode ) {

	if (mode == NON_BLOCKING) {

	    /* set socket to non-blocking */

	    if (ioctl (sockfd, FIONBIO, (char *) &on) == ERROR)
		return ERROR;
	}
	else {
	    /* set socket to blocking */

	    if (ioctl (sockfd, FIONBIO, (char *) &off) == ERROR)
		return ERROR;
	}
	net_sockfd[sockfd].mode = mode;
    }
    return (0);
}

