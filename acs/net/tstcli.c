/* tstcli.c -- Net Services Test Client */

#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>
#include <sys/time.h>
#include <netdb.h>

#include "net_appl.h"

typedef enum {
    false = 0, true = 1
} boolean;


int send_req (int sockfd);

int main (int argc, char *argv[])
{
    int  msgfd;
    char server[16];
    int  pname	= 0;
//    struct timeval delay;
    int  i;				/* loop index */
    static char hostname[32] = "localhost";

    for (i = 1; i < argc; i++) {
	if (!strcmp (argv[i], "-s"))
	    (void) strcpy (server, argv[++i]);

	else if (!strcmp (argv[i], "-h"))
	    (void) strcpy (hostname, argv[++i]);

	else if (!strcmp (argv[i], "-p"))
	    pname = atoi (argv[++i]);
    }

//    delay.tv_sec  = 0;
//    delay.tv_usec = 10000;

    /* initiate connection request to server */

    if ((msgfd = net_connect (server, hostname, pname, BLOCKING)) < 0) {
	(void)fprintf (stderr, "tstcli: net_connect() error: %s, errno=%d\n",
				NET_ERRSTR(msgfd), errno);
	exit (msgfd);
    }

    /* connection established */

    (void)printf ("tstcli: connection established...\n");

    if (send_req (msgfd) < 0) {
	net_close (msgfd);
	exit (1);
    }

    (void) net_close (msgfd);

    exit (0);
}


int send_req (int sockfd)
{
    struct {
	int  msgid;
	char msg[496];
    } req_msg;

    int stat, i;

    req_msg.msgid = 1;

    for (i = 0; i < 500; i++) {
	stat = net_send (sockfd, (char *)&req_msg, sizeof req_msg, BLOCKING);

	if (stat < 0)
	    (void)fprintf (stderr, "tstcli: net_send() error: %s\n",
				    NET_ERRSTR(stat));
#if 1
	else
	    (void)fprintf (stderr, "tstcli: %d bytes sent...\n", stat);
#endif
    }
    return (stat);
}


int recv_data (int sockfd)
{
    int stat;

    struct {
	int  msgid;
	char msg[20];
    } data_msg;

    boolean more = true;

    while (more) {

	stat = net_recv (sockfd, (char *)&data_msg, sizeof data_msg, BLOCKING);

	if (stat == NEOF) {
	    (void) printf ("Connection broken; exiting...\n");
	    more = false;
	}
	else if (stat < 0)
	    (void)fprintf (stderr, "tstcli: net_recv() error: %d\n", stat);
	else if (data_msg.msgid == 126)
	    (void) printf ("tstcli: %.20s\n", data_msg.msg);
    }
    return (stat);
}

