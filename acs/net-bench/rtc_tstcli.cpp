/****************************************************
* rtc_tstcli
*
* Main program for GLC-LSEB benchmark testing.
*/

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <unistd.h>
#include <errno.h>
#include <netdb.h>

extern "C" {
#include "net_ts.h"
#include "net_glc.h"
#include "GlcMsg.h"
}

#include "rtc_tstcli.h"

bool debug = false;


/*****************************
* main - Creates network connections then starts telemetry listener
*
*/

int main(int argc, char **argv)
{
  char server[16];
  // char cmd[MAX_CMD_LEN];
  int  msgfd;
  int  i;

  static char hostname[32] = "localhost";

  for (i = 1; i < argc; i++) {
    if      (!strcmp(argv[i], "-s"))  (void) strcpy(server, argv[++i]);
    else if (!strcmp(argv[i], "-h"))  (void) strcpy(hostname, argv[++i]);
    else if (!strcmp(argv[i], "-d"))   debug = true;
  }

  /* connect to server */
  msgfd = net_connect(server, hostname, ANY_TASK, BLOCKING);
  if (msgfd  < 0) {
    (void) fprintf(stderr, "tstcli: net_connect() error: %s: %s\n",
                           NET_ERRSTR(msgfd), strerror (errno));
    exit(msgfd);
  }

  (void) printf("tstcli: Connected to %s...\n", server);

  
  (void) process_tlm(msgfd);

  net_close (msgfd);
  exit (0);
}


/*****************************
* process_tlm - Listens for
*
*/

int process_tlm(int sockfd)
{
  int  len;
  char buff[1024];
  bool more = true;
  struct timeval tm, lat;
  static int pkt = 0;

  while (more) {
    (void) memset(buff, 0, sizeof buff);

    len = net_recv(sockfd, buff, sizeof buff, BLOCKING);

    gettimeofday(&tm, NULL);

    if (len < 0) {
      (void) fprintf(stderr, "tstcli: net_recv() error: %s, errno=%d\n",
                              NET_ERRSTR(len), errno);
      return len;
    }
    else if (len == NEOF) {
      (void) printf("tstcli: Ending connection...\n");
      more = false;
    }
    else {
      if (debug) {
        NET_TIMESTAMP("%3d tstcli: Received %d bytes.\n", (pkt++%50)+1, len);
      }
      else {
        timersub(&tm, &(((DataHdr *)buff)->time), &lat);
        (void) fprintf(stderr, " %02ld.%06ld %3d\n",
                               lat.tv_sec, lat.tv_usec, (pkt++%50)+1);
      }
    }
  }  /* while(more) */

  return 0;
}
