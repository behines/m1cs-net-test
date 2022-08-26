/* tstcli.c -- Test Client */

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <unistd.h>
#include <errno.h>
#include <netdb.h>

#include "net_ts.h"
#include "net_glc.h"
#include "GlcMsg.h"

bool debug = false;


int main (int argc, char **argv)
{
    char server[16];
    // char cmd[MAX_CMD_LEN];
    int  msgfd;
    int  i;

    static char hostname[32] = "localhost";

    int  send_cmd (int sockfd, char *cmd);
    int  process_rsp (int sockfd);
    int  process_tlm (int sockfd);

    for (i = 1; i < argc; i++) {
	if (!strcmp (argv[i], "-s"))
	    (void) strcpy (server, argv[++i]);

	else if (!strcmp (argv[i], "-h"))
	    (void) strcpy (hostname, argv[++i]);

    	else if (!strcmp (argv[i], "-d"))
	    debug = true;
    }

    /* connect to server */

    if ((msgfd = net_connect (server, hostname, ANY_TASK, BLOCKING)) < 0) {
	(void)fprintf (stderr, "tstcli: net_connect() error: %s: %s\n",
				NET_ERRSTR(msgfd), strerror (errno));
	exit (msgfd);
    }

    (void)printf ("tstcli: Connected to %s...\n", server);
#if 0
    while (fgets (cmd, MAX_CMD_LEN, stdin)) {
	(void) send_cmd (msgfd, cmd);
	if (process_rsp (msgfd) <= 0)
	    break;
    }
#endif
    (void) process_tlm (msgfd);

    net_close (msgfd);
    exit (0);
}


int send_cmd (int sockfd, char *cmd)
{
    int	     status;
    CmdMsg  cmd_msg;

    cmd_msg.hdr.msgId = CMD_TYPE;
    cmd_msg.hdr.srcId = ANY_TASK;
    (void)strcpy (cmd_msg.cmd, cmd);

    if ((status = net_send (sockfd, (char *) &cmd_msg, sizeof cmd_msg,
							BLOCKING)) <= 0)
	(void)fprintf (stderr, "tstcli: net_send() error: %s\n",
				NET_ERRSTR(status));
    else
	(void) printf ("Command sent...\n");

    return status;
}


int process_rsp (int sockfd)
{
    int	     status;
    char     buf[BUFSIZ];

    (void) memset (buf, 0, sizeof buf);

    status = net_recv (sockfd, buf, sizeof buf, BLOCKING);

    if (status < 0) {
	(void)fprintf (stderr, "tstcli: net_recv() error: %s, errno=%d\n",
				NET_ERRSTR(status), errno);
    }
    else if (status == NEOF) {
	(void)fprintf (stderr,
			"tstcli: Connection closed by foreign host...\n");
    }
    else
	(void)fprintf (stderr, "%s\n", ((RspMsg *) buf)->rsp);

    return status;
}


int process_tlm (int sockfd)
{
    int  len;
    char buff[1024];
    bool more = true;
    struct timeval tm, lat;
    static int pkt = 0;

    while (more) {
	(void) memset (buff, 0, sizeof buff);

	len = net_recv (sockfd, buff, sizeof buff, BLOCKING);

	gettimeofday (&tm, NULL);

	if (len < 0) {
	    (void)fprintf (stderr, "tstcli: net_recv() error: %s, errno=%d\n",
				    NET_ERRSTR(len), errno);
	    return (len);
	}
	else if (len == NEOF) {
	    (void)printf ("tstcli: Ending connection...\n");
	    more = false;
	}
	else {
	    if (debug) {
	    	NET_TIMESTAMP ("%3d tstcli: Received %d bytes.\n", (pkt++%50)+1, len);
	    }
	    else {
		timersub (&tm, &(((DataHdr *)buff)->time), &lat);
		(void)fprintf (stderr, " %02ld.%06ld %3d\n",
					lat.tv_sec, lat.tv_usec, (pkt++%50)+1);
	    }
    	}
    }

    return 0;
}

