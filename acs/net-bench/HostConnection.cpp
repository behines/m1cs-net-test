/****************************************************
* tHostConnection
*
* Forges and maintains a connection with a server
*/

#include "HostConnection.h"
#include <errno.h>
#include <string.h>
#include <iostream>

extern "C" {
  #include "net_ts.h"
  #include "net_glc.h"
  #include "GlcMsg.h"
}


/***************************************************
* tHostConnection constructor
*
* INPUTS:
*    sServer   - string descriptor of the service we are connecting to,
*                e.g., "app_srv19"
*    sHostname - hostname or dot-separated IP address
*/

tHostConnection::tHostConnection(char *sServer, char *sHostname)
{
  _bDebug = false;
  
  _fdConnection = net_connect(sServer, sHostname, ANY_TASK, BLOCKING);
  if (_fdConnection  < 0) {
    std::cerr << "tHostConnection: net_connect() error: " << NET_ERRSTR(_fdConnection) << ": " << strerror(errno) << std::endl;
  }
  else {
    std::cout << "tstcli: Connected to " << sServer << " on " << sHostname << std::endl;
  }
}


/***************************************************
* tHostConnection destructor
*
*/

tHostConnection::~tHostConnection()
{
  if (_fdConnection > 0)  net_close(_fdConnection);
}



/*****************************
* ProcessTelemetry - Listens for
*
*/

int tHostConnection::ProcessTelemetry()
{
  int  len;
  char buf[1024];
  bool more = true;
  struct timeval tm, lat;
  static int pkt = 0;

  while (more) {
    (void) memset(buf, 0, sizeof buf);

    len = net_recv(_fdConnection, buf, sizeof buf, BLOCKING);

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
      if (_bDebug) {
        NET_TIMESTAMP("%3d tstcli: Received %d bytes.\n", (pkt++%50)+1, len);
      }
      else {
        timersub(&tm, &(((DataHdr *)buf)->time), &lat);
        (void) fprintf(stderr, " %02ld.%06ld %3d\n",
                               lat.tv_sec, lat.tv_usec, (pkt++%50)+1);
      }
    }
  }  /* while(more) */

  return 0;
}
