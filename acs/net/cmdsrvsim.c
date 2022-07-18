/**
 *****************************************************************************
 *
 * @file cmdsrvsim.c
 *      GLC Net Services Test Command Server.
 *
 * @par Project
 *      TMT Primary Mirror Control System (M1CS) \n
 *      Jet Propulsion Laboratory, Pasadena, CA
 *
 * @author	Thang Trinh
 * @date	15-Sep-2021 -- Initial delivery.
 *
 * Copyright (c) 2015-2022, California Institute of Technology
 *
 *****************************************************************************/

/* cmdsrvsim.c -- Net Services Command Server */

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <netdb.h>

#include "net_glc.h"
#include "GlcMsg.h"

#define MAXCLIENTS	20
#define MAXMSGLEN	1024

int listenfd = ERROR;
int cli_fd[MAXCLIENTS];


void event_loop ();
int  process_msg (int sockfd);


int main (int argc, char **argv)
{
    char server[128] = LSEB_CMD_SRV;
    int  i;

    for (i = 1; i < argc; i++) {
	if (!strcmp (argv[i], "-s"))
	    (void) strncpy (server, argv[++i], sizeof server);
    }

    for (i = 0; i < MAXCLIENTS; i++)
    	cli_fd[i] = ERROR;

    /* initialize server's network connection */

    if ((listenfd = net_init (server)) < 0 ) {
	(void)fprintf (stderr, "cmdsrvsim: net_init() error: %s, errno=%d\n",
				NET_ERRSTR(listenfd), errno);
	exit (listenfd);
    }
    else
        printf ("cmdsrvsim: Listening on socket %d...\n", listenfd);

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
    int  sockfd;
    int  i;

    while (1) {

        FD_ZERO (&read_fds);
        FD_SET (listenfd, &read_fds);

	for (i = 0; i < MAXCLIENTS; i++)
	    if (cli_fd[i] != ERROR) FD_SET (cli_fd[i], &read_fds);

        nfds = select (FD_SETSIZE, &read_fds, (fd_set *) 0, (fd_set *) 0,
                       (struct timeval *) 0);
        if (nfds <= 0)  {
            if (errno == EINTR)  {
                continue;
            }
            // error on select
            (void)fprintf (stderr, "cmdsrvsim: select() error: %s.", strerror (errno));
            exit (-1);
        }
        else {
            if (FD_ISSET (listenfd, &read_fds)) {

		/* accept new client connection */
		if ((sockfd = net_accept (listenfd, BLOCKING)) < 0) {
		    (void)fprintf (stderr, "cmdsrvsim: net_accept() error: %s, errno=%d\n",
					    NET_ERRSTR(sockfd), errno);
		    net_close (listenfd);
		    exit (sockfd);
		}
		(void)printf ("cmdsrvsim: Connection accepted...\n");

		int n = 0;

		while (n < MAXCLIENTS && cli_fd[n] != ERROR)
		    n++;

		if (n < MAXCLIENTS)
		    cli_fd[n] = sockfd;
		else {
		    (void)fprintf (stderr, "cmdsrvsim: Max client connections exceeded.\n");
		    net_close (sockfd);
		}

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

    int  send_rsp (int sockfd, char *cmdstr, int csn);

    (void) memset (msg, 0, sizeof msg);

    if ((len = net_recv (cli_fd[indx], msg, MAXMSGLEN, BLOCKING)) < 0) {
	(void)fprintf (stderr, "cmdsrvsim: net_recv() error: %s, errno=%d\n",
				NET_ERRSTR(len), errno);
	return len;
    }
    else if (len == NEOF) {
	(void)printf ("cmdsrvsim: Closing broken connection...\n");
	net_close (cli_fd[indx]);
	cli_fd[indx] = ERROR;
	return len;
    }

    if (((MsgHdr *) msg)->msgId == CMD_TYPE) {

	((CmdMsg *) msg)->cmd[MAX_CMD_LEN - 1] = '\0';
    	(void)printf ("%s\n", ((CmdMsg *) msg)->cmd);
	send_rsp (cli_fd[indx], ((CmdMsg *) msg)->cmd, ((CmdMsg *) msg)->hdr.seqNo);
    }
    else
    	(void)fprintf (stderr, "cmdsrvsim: Invalid message received.\n");

    return len;
}


int send_rsp (int sockfd, char *cmdstr, int csn)
{
    char	cmd[MAX_CMD_LEN] = "\0";
    int		status;
    RspMsg	rsp_msg;

    rsp_msg.hdr.msgId = RSP_TYPE;
    rsp_msg.hdr.seqNo = csn;
    (void) sscanf (cmdstr, "%80s", cmd);
    (void) sprintf (rsp_msg.rsp, "%s: Completed.", cmd);

    if ((status = net_send (sockfd, (char *) &rsp_msg, sizeof rsp_msg, BLOCKING)) <= 0)
        (void)fprintf (stderr, "cmdsrvsim: net_send() error: %s, errno=%d\n",
                                NET_ERRSTR(status), errno);
    return status;
}

