/* tstsrv.c -- Net Services Test Server */

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <netdb.h>

#include "net_appl.h"
#include "net_glc.h"

int process_msg(int sockfd);

typedef enum {
    false = 0, true = 1
} boolean;


int main (int argc, char *argv[])
{
    int listenfd;
    int msgfd;
    char server[16]= LSCS_50HZ_DATA_SRV;
    int  i;				/* loop index */

    for (i = 1; i < argc; i++) {
	if (!strcmp (argv[i], "-s"))
	    (void) strcpy (server, argv[++i]);
    }

    /* initialize server's network connection */

    if ((listenfd = net_init (server)) < 0 ) {
	(void)fprintf (stderr, "tstsrv: net_init() error: %s, errno=%d\n",
				NET_ERRSTR(listenfd), errno);
	exit (listenfd);
    }
    else printf ("tstsrv: listening on socket %d\n", listenfd);

    /* accept connection */

    if ((msgfd = net_accept (listenfd, BLOCKING)) < 0) {
	(void)fprintf (stderr, "tstsrv: net_accept() error: %s, errno=%d\n",
				NET_ERRSTR(msgfd), errno);
	net_close (listenfd);
	exit (msgfd);
    }
    (void)printf ("tstsrv: connection accepted...\n");

    process_msg (msgfd);		/* service client's requests */

    net_close (listenfd);
    net_close (msgfd);

    return 0;
}


int process_msg (int sockfd)
{
    int  len;
    char buff[1024];
    boolean more = true;

    while (more) {
	(void) memset (buff, 0, sizeof buff);

	len = net_recv (sockfd, buff, 1024, BLOCKING);

	if (len < 0) {
	    (void)fprintf (stderr, "tstsrv: net_recv() error: %s, errno=%d\n",
				    NET_ERRSTR(len), errno);
	    return (len);
	}
	else if (len == NEOF) {
	    (void)printf ("tstsrv: ending connection...\n");
	    more = false;
	}
	else
	    (void)printf ("tstsrv: received %d bytes\n", len);
    }

    return 0;
}

