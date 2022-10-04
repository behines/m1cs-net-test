/****************************************************
* tHostConnection
*
* Forges and maintains a connection with a server
*/

#include "HostConnection.h"
#include <errno.h>
#include <string.h>
#include <signal.h>
#include <iostream>
#include <utility>

extern "C" {
  #include "net_ts.h"
  #include "net_glc.h"
  #include "GlcMsg.h"
  #include "GlcLscsIf.h"
}

#define MAX_MESSAGE_SIZE (sizeof(SegRtDataMsg))

using namespace std;


/***************************************************
* tHostConnection constructor
*
* INPUTS:
*    sServer   - string descriptor of the service we are connecting to,
*                e.g., "app_srv19"
*    sHostname - hostname or dot-separated IP address
*/

tHostConnection::tHostConnection(const char *sServer, const char *sHostname, int iReceiveThreadPriority) :
  tPThread(iReceiveThreadPriority, true),
  _nReceived(0), _sServer(sServer), _sHostname(sHostname)
{
  _bDebug = false;
  
  _fdConnection = net_connect(const_cast<char *>(sServer), const_cast<char *>(sHostname), ANY_TASK, BLOCKING);
  if (_fdConnection  < 0) {
    cerr << "tHostConnection: Could not connect to " << _sServer <<  " on " << _sHostname << endl;
    cerr << "                 net_connect() error: " << NET_ERRSTR(_fdConnection) << ": " << strerror(errno) << endl;
  }
  else {
    // GLB: single call to << operator is thread safe however multiple << is not
    // cout << "Connected to " << _sServer << " on " << _sHostname << endl;
    string s = "Connected to " + _sServer + " on " + _sHostname + "\n";
    cout << s;
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

tHostConnection::tHostConnection(const string &sServer, const string &sHostname, int iReceiveThreadPriority) :
  tHostConnection(sServer.c_str(), sHostname.c_str(), iReceiveThreadPriority)
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

tHostConnection::tHostConnection(tHostConnection &&other) noexcept :
  tPThread(std::move(other))
{
  _fdConnection = move(other._fdConnection);
  _bDebug       = other._bDebug;
  _sServer      = move(other._sServer);
  _sHostname    = move(other._sHostname);

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


/***************************************************
* tHostConnection::_Thread
*
*/

void *tHostConnection::_Thread()
{
  // GLB: single call to << operator is thread safe however multiple << is not
  //cout << "Starting thread for " << _sHostname << "::" << _sServer << endl;
  string s = "Starting thread for " + _sHostname + "::" + _sServer + "\n";
  cout << s;
  while (!_bExit) {  // Flag from base tPThread class
    ProcessIncomingMessage();
  }

  return 0;
}



/*****************************
* ProcessIncomingMessage
*
*/

int tHostConnection::ProcessIncomingMessage()
{
  int  len;
  char buf[MAX_MESSAGE_SIZE];
  struct timeval tmRcv, tmSent, tmDiff;

  //(void) memset(buf, 0, sizeof(buf));

  len = net_recv(_fdConnection, buf, sizeof(buf), BLOCKING);

  gettimeofday(&tmRcv, NULL);

  if (len < 0) {
    cerr << _sHostname << "::" << _sServer << " : net_recv() error " << errno << ": " << NET_ERRSTR(_fdConnection) << endl;
    return len;
  }
  else if (len == NEOF) {
    //cout << _sHostname << "::" << _sServer << ": Ending connection..." << endl;
    string s = _sHostname + "::" + _sServer + ": Ending connection...\n";
    cout << s;
  }
  else {
    if (_bDebug) {
      cerr << _sHostname << "::" << _sServer << ": ";
      NET_TIMESTAMP("%3d: Received %d bytes.\n", (_nReceived++%50)+1, len);
    }
    else {
      tmSent = ((DataHdr *) buf)->time;
      timersub(&tmRcv, &tmSent, &tmDiff);
      (void) printf("%s::%s: Sent: %02ld.%06ld  Rcvd: %02ld.%06ld  Lat: %02ld.%06ld  N: %3d\n", 
                     _sHostname.c_str(), _sServer.c_str(),
                     tmSent.tv_sec, tmSent.tv_usec, 
                     tmRcv .tv_sec, tmRcv .tv_usec,
                     tmDiff.tv_sec, tmDiff.tv_usec, 
                     (_nReceived++%50)+1);
    }
  }
 
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

int tHostConnectionList::AddConnection(const char *sServer, const char *sHostname, int iReceiveThreadPriority)
{
  _ConnectionList.push_back(tHostConnection(sServer, sHostname, iReceiveThreadPriority));
  return 0;
}

int tHostConnectionList::AddConnection(const string &sServer, const string &sHostname, int iReceiveThreadPriority)
{
  _ConnectionList.push_back(tHostConnection(sServer, sHostname, iReceiveThreadPriority));
  return 0;
}




/***************************************************
* tHostConnectionList::ProcessTelemetryUsingSelect
*
* 
*    
*/

int tHostConnectionList::ProcessTelemetryUsingSelect()
{
  fd_set fdSetPersistent, fdSet;
  int numFdsWithData;

  // Initialize the FD block for select.  Must be re-inited at every iteration
  FD_ZERO(&fdSetPersistent);
  for (auto & Connection : _ConnectionList) {
    if (Connection.IsConnected())  FD_SET(Connection._fdConnection, &fdSetPersistent);
  }


  while (!_bExit) {

    fdSet = fdSetPersistent;

    // Wait for something to show up
    numFdsWithData = select(FD_SETSIZE, &fdSet, (fd_set *)0, (fd_set *)0, (struct timeval *)0);
    if (numFdsWithData <= 0) {
      if (errno == EINTR) {
        // A signal was caught, exit the whlie loop
        continue;
      }

      // error on select
      // This is okay if it occurs at program exit time.
      cerr << "tHostConnectionList: select() error: " << strerror(errno) << endl;
      return -errno;
    }


    // Now process messages that we received.  Note that a 
    // disconnection will result in the bit being set as well.  Why?  Because
    // the disconnecting socket generates the POLLHUP event.  And we see from
    // the select man page that EPOLLHUP is one of the flags that is detected
    // in the POLLIN_SET.
    for (auto & Connection : _ConnectionList) {
      if (Connection.IsConnected()  &&  FD_ISSET(Connection._fdConnection, &fdSet)) {
        Connection.ProcessIncomingMessage();
      }
    }
  }

  return 0;
}


/***************************************************
* tHostConnectionList::ProcessTelemetryUsingThreads
*
* 
*    
*/

int tHostConnectionList::ProcessTelemetryUsingThreads()
{
  sigset_t  sigset;
  int       sig;

  for (auto & Connection : _ConnectionList) {
    if (Connection.IsConnected())  Connection.StartThread();
  }

  if (!tPThread::HaveAllBeenStartedWithRequestedAttributes()) {
    cerr << "** Warning: Some threads not created with desired attributes **" << endl;
    cerr << "   You probably need to run as root." << endl;
  }

  // Wait for Ctrl-C
  /* Set up the mask of signals to temporarily block. */
  sigemptyset(&sigset);
  sigaddset(&sigset, SIGINT);

  /* Wait for a signal to arrive. */
  sigwait(&sigset, &sig);
  cout << "Ctrl-C, exiting..." << endl;

  for (auto & Connection : _ConnectionList) {
    if (Connection.IsRunning())  Connection.StopThread(true);
  }

  return 0;
}
