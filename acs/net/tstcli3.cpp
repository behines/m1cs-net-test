/* tstcli.c -- Test Client */

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <netdb.h>

#include "net_glc.h"
#include "GlcMsg.h"


int main (int argc, char **argv)
{
    char server[16];
    char cmd[MAX_CMD_LEN];
    int  msgfd;
    int  i;

    static char hostname[32] = "localhost";

    int  send_cmd (int sockfd, char *cmd);
    int  process_rsp (int sockfd);

    for (i = 1; i < argc; i++) {
	if (!strcmp (argv[i], "-s"))
	    (void) strcpy (server, argv[++i]);

	else if (!strcmp (argv[i], "-h"))
	    (void) strcpy (hostname, argv[++i]);
    }

    /* connect to server */

    if ((msgfd = net_connect (server, hostname, ANY_TASK, BLOCKING)) < 0) {
	(void)fprintf (stderr, "tstcli: net_connect() error: %s: %s\n",
				NET_ERRSTR(msgfd), strerror (errno));
	exit (msgfd);
    }

    (void)printf ("tstcli: connected to %s...\n", server);

    while (fgets (cmd, MAX_CMD_LEN, stdin)) {
	(void) send_cmd (msgfd, cmd);
	if (process_rsp (msgfd) <= 0)
	    break;
    }

    net_close (msgfd);
    exit (0);
}


int send_cmd (int sockfd, char *cmd)
{
    int	     status;
    CmdMsg  cmd_msg;

    cmd_msg.hdr.msgId = CMD_TYPE;
    cmd_msg.hdr.srcId = ANY_TASK;
    cmd_msg.hdr.seqNo = 1;
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
			"tstcli: connection closed by foreign host...\n");
    }
    else
	(void)fprintf (stderr, "%s\n", ((RspMsg *) buf)->rsp);

    return status;
}

