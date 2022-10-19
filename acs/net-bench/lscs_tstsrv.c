/**
 *****************************************************************************
 *
 * @file lscs_tstsrv.c
 *      LSCS Test Server For Network Benchmarking.
 *
 * @par Project
 *      TMT Primary Mirror Control System (M1CS) \n
 *      Jet Propulsion Laboratory, Pasadena, CA
 *
 * @author	Thang Trinh
 * @date	27-May-2022 -- Initial delivery.
 *
 * Copyright (c) 2015-2023, California Institute of Technology
 *
 *****************************************************************************/

/* lscs_tstsrv.c -- LSCS Test Server */

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <unistd.h>
#include <errno.h>
#include <netdb.h>
#include <time.h>

#include "net_ts.h"
#include "net_glc.h"
#include "timer.h"
#include "GlcMsg.h"

#include "GlcLscsIf.h"

#define MAXCLIENTS	492		// Up to 492 client connections.
#define MAXMSGLEN	1024

int  listenfd = ERROR;
int  cli_fd[MAXCLIENTS];
int  tmfd = ERROR;
bool debug = false;


void event_loop ();
int  process_msg (int sockfd);
int  process_timer (int tfd);


int main (int argc, char **argv)
{
    char server[128] = LSCS_50HZ_DATA_SRV;
    int  i;

    for (i = 1; i < argc; i++) {
	if (!strcmp (argv[i], "-s"))
	    (void) strncpy (server, argv[++i], sizeof server);

	else if (!strcmp (argv[i], "-d"))
	    debug = true;

	else if (!strcmp (argv[i], "-help")) {
	    printf ("Usage: lscs_tstsrv [-d] [-s server]\n");
	    exit (1);
	}
	else {
	    printf ("Usage: lscs_tstsrv [-d] [-s server]\n");
	    exit (1);
	}
    }

    for (i = 0; i < MAXCLIENTS; i++)
    	cli_fd[i] = ERROR;

    /* initialize server's network connection */

    if ((listenfd = net_init (server)) < 0 ) {
	(void)fprintf (stderr, "lscs_tstsrv: net_init() error: %s, errno=%d\n",
				NET_ERRSTR(listenfd), errno);
	exit (listenfd);
    }
    else
        printf ("lscs_tstsrv: Listening on socket %d...\n", listenfd);

    if (timerCreate (&tmfd) == -1) {
    	(void)fprintf (stderr, "lscs_tstsrv: Error creating timer.\n");
	exit (tmfd);
    }

    /* Main event loop */

    event_loop ();

    if (listenfd != ERROR)
        net_close (listenfd);

    for (i = 0; i < MAXCLIENTS; i++)
	if (cli_fd[i] != ERROR)
	    net_close (cli_fd[i]);

    return 0;
}


void event_loop ()
{
    fd_set read_fds;       /* file descriptors to be polled */
    int  nfds;
    int  sockfd, i;
    struct timeval tm1, tm2, tm_start;
    struct timeval tm_50hz = {0, 20*1000};	// {0s, 20ms}

    while (1) {

        FD_ZERO (&read_fds);
        FD_SET (listenfd, &read_fds);

	for (i = 0; i < MAXCLIENTS; i++)
	    if (cli_fd[i] != ERROR) FD_SET (cli_fd[i], &read_fds);

	if (tmfd != ERROR) FD_SET (tmfd, &read_fds);

        nfds = select (FD_SETSIZE, &read_fds, (fd_set *) 0, (fd_set *) 0,
                       (struct timeval *) 0);

        if (nfds <= 0)  {
            if (errno == EINTR)  {
                continue;
            }
            // error on select
            (void)fprintf (stderr, "lscs_tstsrv: select() error: %s.", strerror (errno));
            exit (-1);
        }
        else {
            if (FD_ISSET (listenfd, &read_fds)) {

		/* accept new client connection */
		if ((sockfd = net_accept (listenfd, BLOCKING)) < 0) {
		    (void)fprintf (stderr, "lscs_tstsrv: net_accept() error: %s, errno=%d\n",
					    NET_ERRSTR(sockfd), errno);
		    net_close (listenfd);
		    exit (sockfd);
		}
		(void)printf ("lscs_tstsrv: Connection accepted.\n");

		int n = 0;

		while (n < MAXCLIENTS && cli_fd[n] != ERROR)
		    n++;

		if (n < MAXCLIENTS) {
		    cli_fd[n] = sockfd;
		    if (debug) fprintf (stderr, "tm_50hz.(tv_sec, tv_usec) = (%ld, %ld)\n",
							    tm_50hz.tv_sec, tm_50hz.tv_usec);
		    gettimeofday (&tm1, NULL);
		    tm2 = (struct timeval){tm1.tv_sec+1, 0};
		    timersub(&tm2, &tm1, &tm_start);

		    if (tmfd != ERROR) setTimer (tmfd, &tm_start, &tm_50hz);
		}
		else {
		    (void)fprintf (stderr, "lscs_tstsrv: Max client connections exceeded.\n");
		    net_close (sockfd);
		}

                if (--nfds <= 0)
                    continue;
            }

	    if (tmfd != ERROR && FD_ISSET (tmfd, &read_fds)) {
	    	(void) process_timer (tmfd);

                if (--nfds <= 0)
                    continue;
	    }

	    for (i = 0; i < MAXCLIENTS; i++)
		if (cli_fd[i] != ERROR && FD_ISSET (cli_fd[i], &read_fds)) {

		    /* service client's request */
		    (void) process_msg (i);
		    if (--nfds <= 0)
			break;
		}
        }
    }
}


int process_msg (int indx)
{
    char msg[MAXMSGLEN];
    int  len;

    int  send_rsp (int sockfd, char *cmdstr);

    (void) memset (msg, 0, sizeof msg);

    if ((len = net_recv (cli_fd[indx], msg, MAXMSGLEN, BLOCKING)) < 0) {
	(void)fprintf (stderr, "lscs_tstsrv: net_recv() error: %s, errno=%d\n",
				NET_ERRSTR(len), errno);
	return len;
    }
    else if (len == NEOF) {
	(void)printf ("lscs_tstsrv: Closing broken connection...\n");
	net_close (cli_fd[indx]);
	cli_fd[indx] = ERROR;
	return len;
    }

    if (((MsgHdr *) msg)->msgId == CMD_TYPE) {

	((CmdMsg *) msg)->cmd[MAX_CMD_LEN - 1] = '\0';
    	(void)printf ("%s\n", ((CmdMsg *) msg)->cmd);
	send_rsp (cli_fd[indx], ((CmdMsg *) msg)->cmd);
    }
    else
    	(void)fprintf (stderr, "lscs_tstsrv: Invalid message received.\n");

    return len;
}


int send_rsp (int sockfd, char *cmdstr)
{
    char	cmd[MAX_CMD_LEN] = "\0";
    int		status;
    RspMsg	rsp_msg;

    rsp_msg.hdr.msgId = RSP_TYPE;
    (void) sscanf (cmdstr, "%80s", cmd);
    (void) sprintf (rsp_msg.rsp, "%s: Completed.", cmd);

    if ((status = net_send (sockfd, (char *) &rsp_msg, sizeof rsp_msg, BLOCKING)) <= 0)
        (void)fprintf (stderr, "lscs_tstsrv: net_send() error: %s, errno=%d\n",
                                NET_ERRSTR(status), errno);
    return status;
}


int process_timer (int tfd)
{
    uint64_t	   exp;
    ssize_t	   s;
    int		   i, status;
    struct timeval tm;
    SegRtDataMsg seg_msg;

    s = read (tfd, &exp, sizeof(uint64_t));
    if (s != sizeof(uint64_t))
    	perror ("read");
    else if (debug)
    	(void)fprintf (stderr, "read: timer exp = %lu\n", exp);

    for (i = 0; i < MAXCLIENTS; i++)
    	if (cli_fd[i] != ERROR) {
    	    if (debug) NET_TIMESTAMP ("lscs_tstsrv: Sending SegRtDataMsg (%lu bytes)...\n",
									sizeof(seg_msg));
	    gettimeofday (&tm, NULL);
	    seg_msg.hdr.time = tm;

	    if ((status = net_send (cli_fd[i], (char *) &seg_msg, sizeof seg_msg,
								  BLOCKING)) <= 0) {
	    	(void)fprintf (stderr, "lscs_tstsrv: net_send() error: %s, errno=%d\n",
					NET_ERRSTR(status), errno);
	    	net_close (cli_fd[i]);
		cli_fd[i] = ERROR;
	    }
    	}

    return 0;
}

