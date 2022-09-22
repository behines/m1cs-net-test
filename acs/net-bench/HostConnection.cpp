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
* tSampleLogger constructor
*
* INPUTS:
*    sServer   - string descriptor of the service we are connecting to,
*                e.g., "app_srv19"
*    sHostname - hostname or dot-separated IP address
*/

tSampleLogger::tSampleLogger(const string &sServer, const string &sHostname) :
  _sServer(sServer), _sHostname(sHostname),
  _SampleQueue(),
  _thread()  // Thread creation is deferred.  See tSampleLogger::StartLoggerThread()
{
}




/***************************************************
* tSampleLogger move constructor
*
* This is used during assignment of the temporary object to the list.  If we don't have a
* move constructor, then the destructor for the temporary object will close the file
* descriptor.
*
* INPUTS:
*    other - the contents of the object being moved
*/

tSampleLogger::tSampleLogger(tSampleLogger &&other) noexcept :
  _sServer    (move(other._sServer)),
  _sHostname  (move(other._sHostname)),
  _SampleQueue(move(other._SampleQueue)),
  // Mutex and condition variable cannot be moved.  
  _thread     (move(other._thread))
{
}



tSampleLogger::~tSampleLogger() 
{
  // _thread.join(); 
}


/***************************************************
* tSampleLogger::StartLoggerThread
*
* Actually starts the logger thread running.  This has to wait until
* after construction, because mutexes and condition variables cannot
* be moved or copied.  Since temporaries are created and moved during
* AddConnection, we have to postpone starting the thread until that
* is complete.
*/

void tSampleLogger::StartLoggerThread()
{
   _thread = std::thread(&tSampleLogger::PrintSamples, this);
}


/***************************************************
* tSampleLogger LogSample
*
* INPUTS:
*    
*/

void tSampleLogger::LogSample(int nSamp, struct timeval &tmRcv, struct timeval &tmSent)
{
  std::lock_guard<std::mutex> cvLock(_SampleQueueMutex);
  _SampleQueue.push(tLatencySample(nSamp, tmRcv, tmSent));
  _SampleQueueCondition.notify_one();
}


/***************************************************
* tSampleLogger PrintSamples
*
* INPUTS:
*/

void tSampleLogger::PrintSamples()
{
  tLatencySample Sample;
  struct timeval tmDiff;

  while (1) {
    {
      std::unique_lock cvLock(_SampleQueueMutex);
      // Block until notified that there is something in the queue.  This will unlock the 
      // mutex and then block.  
      // There's actually no need to specify a predicate here, but we do anyway.  (We don't
      // really care if we are spuriously awakened since we immediately test for our 
      // predicate in the while loop below.)
      _SampleQueueCondition.wait(cvLock, [this] { return !_SampleQueue.empty(); } );
      // The mutex is now locked, but will be unlocked when the scoped_lock is destroyed
    }

    // Drain the queue
    while (!_SampleQueue.empty()) {
      {
        std::scoped_lock cvLock(_SampleQueueMutex);
        Sample = _SampleQueue.front();
        _SampleQueue.pop();
      }

      timersub(&Sample._tmRcv, &Sample._tmSent, &tmDiff);
      (void) printf("%s::%s: Sent: %02ld.%06ld  Rcvd: %02ld.%06ld  Lat: %02ld.%06ld  N: %3d\n", 
                     _sHostname.c_str(), _sServer.c_str(),
                     Sample._tmSent.tv_sec, Sample._tmSent.tv_usec, 
                     Sample._tmRcv .tv_sec, Sample._tmRcv .tv_usec,
                     tmDiff.tv_sec, tmDiff.tv_usec, 
                     ((Sample._nSamp-1)%50)+1);
    }
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

tHostConnection::tHostConnection(const char *sServer, const char *sHostname, int iReceiveThreadPriority) :
  tHostConnection(string(sServer), string(sHostname), iReceiveThreadPriority)
{
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
  tPThread(iReceiveThreadPriority, true),
  _SampleLogger(sServer, sHostname),
  _nReceived(0), _sServer(sServer), _sHostname(sHostname)
{
  _bDebug = false;
  
  _fdConnection = net_connect(const_cast<char *>(sServer.c_str()), const_cast<char *>(sHostname.c_str()), ANY_TASK, BLOCKING);
  if (_fdConnection  < 0) {
    cerr << "tHostConnection: Could not connect to " << _sServer <<  " on " << _sHostname << endl;
    cerr << "                 net_connect() error: " << NET_ERRSTR(_fdConnection) << ": " << strerror(errno) << endl;
  }
  else {
    cout << "Connected to " << _sServer << " on " << _sHostname << endl;
  }

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
  tPThread(move(other)),
  _SampleLogger(move(other._SampleLogger)),
  _sServer     (move(other._sServer)),
  _sHostname   (move(other._sHostname))
{
  _fdConnection = other._fdConnection;
  _bDebug       = other._bDebug;
  _nReceived    = other._nReceived;

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
  cout << "Starting thread for " << _sHostname << "::" << _sServer << endl;
  while (!_bExit) {  // Flag from base tPThread class
    ProcessIncomingMessage();
  }

  return 0;
}



/*****************************
* tHostConnection::ProcessIncomingMessage
*
*/

int tHostConnection::ProcessIncomingMessage()
{
  int  len;
  char buf[MAX_MESSAGE_SIZE];
  struct timeval tmRcv, tmSent;

  //(void) memset(buf, 0, sizeof(buf));

  len = net_recv(_fdConnection, buf, sizeof(buf), BLOCKING);

  gettimeofday(&tmRcv, NULL);

  if (len < 0) {
    cerr << _sHostname << "::" << _sServer << " : net_recv() error " << errno << ": " << NET_ERRSTR(_fdConnection) << endl;
    return len;
  }
  else if (len == NEOF) {
    cout << _sHostname << "::" << _sServer << ": Ending connection..." << endl;
  }
  else {
    if (_bDebug) {
      cerr << _sHostname << "::" << _sServer << ": ";
      NET_TIMESTAMP("%3d: Received %d bytes.\n", (_nReceived++%50)+1, len);
    }
    else {
      tmSent = ((DataHdr *) buf)->time;
      _SampleLogger.LogSample(++_nReceived, tmRcv, tmSent);
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
  _ConnectionList.back().StartSampleLoggerThread();
  return 0;
}

int tHostConnectionList::AddConnection(const string &sServer, const string &sHostname, int iReceiveThreadPriority)
{
  _ConnectionList.push_back(tHostConnection(sServer, sHostname, iReceiveThreadPriority));
  _ConnectionList.back().StartSampleLoggerThread();
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