/****************************************************
* tHostConnection
*
* Forges and maintains a connection with a server
*/

#include "HostConnection.h"
#include <errno.h>
#include <string.h>
#include <iostream>
#include <utility>

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

tHostConnection::tHostConnection(const char *sServer, const char *sHostname) :
  _sServer(sServer), _sHostname(sHostname)
{
  _bDebug = false;
  
  _fdConnection = net_connect(const_cast<char *>(sServer), const_cast<char *>(sHostname), ANY_TASK, BLOCKING);
  if (_fdConnection  < 0) {
    std::cerr << "tHostConnection: Could not connect to " << _sServer <<  " on " << _sHostname << std::endl;
    std::cerr << "                 net_connect() error: " << NET_ERRSTR(_fdConnection) << ": " << strerror(errno) << std::endl;
  }
  else {
    std::cout << "Connected to " << _sServer << " on " << _sHostname << std::endl;
  }
}


/***************************************************
* tHostConnection constructor
*
* INPUTS:
*    sServer   - string descriptor of the service we are connecting to,
*                e.g., "app_srv19"
*    sHostname - hostname or dot-separated IP address
*/

tHostConnection::tHostConnection(const std::string &sServer, const std::string &sHostname) :
  tHostConnection(sServer.c_str(), sHostname.c_str())
{
}


/***************************************************
* tHostConnection move constructor
*
* This is used during assignment of the temporary object to the list.  If we don't have a
* move constructor, then the destructor for the temporary object will close the file
* descriptor.
*
* INPUTS:
*    other - the contents of the object being moved
*/

tHostConnection::tHostConnection(tHostConnection &&other) noexcept
{
  _fdConnection = std::move(other._fdConnection);
  _bDebug       = other._bDebug;
  _sServer      = std::move(other._sServer);
  _sHostname    = std::move(other._sHostname);

  // This assignment will disarm the net_close() in the destructor for other.
  other._fdConnection = 0;
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





/***************************************************
* tHostConnectionList::AddConnection
*
* INPUTS:
*    sServer   - string descriptor of the service we are connecting to,
*                e.g., "app_srv19"
*    sHostname - hostname or dot-separated IP address
*/

int tHostConnectionList::AddConnection(const char *sServer, const char *sHostname)
{
  _ConnectionList.push_back(tHostConnection(sServer, sHostname));
  return 0;
}

int tHostConnectionList::AddConnection(const std::string &sServer, const std::string &sHostname)
{
  _ConnectionList.push_back(tHostConnection(sServer, sHostname));
  return 0;
}
