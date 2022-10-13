/****************************************************
* tServer
*
* Creates a UDP server
*/

#include "Server.h"
#include <errno.h>
#include <string.h>
#include <signal.h>
#include <iostream>
#include <iomanip>
#include <utility>

extern "C" {
  #include "GlcMsg.h"
  #include "GlcLscsIf.h"
}

#define MAX_MESSAGE_SIZE (sizeof(SegRtDataMsg))  

using namespace std;


/***************************************************
* Static member initialization
*/

tSamplePrinter tSampleLogger::_SamplePrinter;


/***************************************************
* tSampleLogger constructor
*
* INPUTS:
*    sServer   - string descriptor of the service we are connecting to,
*                e.g., "app_srv19"
*    sHostname - hostname or dot-separated IP address
*/

tSampleLogger::tSampleLogger(int iPortNum) :
  _SampleQueue(SAMPLE_QUEUE_BUFFER_DEPTH),
  //_thread(),  // Thread creation is deferred.  See tSampleLogger::StartLoggerThread()
  _iPortNum(iPortNum)
{
  _SamplePrinter.AddLogger(this);
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
  _SampleQueue(std::move(other._SampleQueue)),
  // Mutex and condition variable cannot be moved.  
  //_thread     (move(other._thread)),
  _iPortNum   (other._iPortNum)
{
  _SamplePrinter.RemoveLogger(&other);
  _SamplePrinter.AddLogger(this);
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

//void tSampleLogger::StartLoggerThread()
//{
//   _thread = std::thread(&tSampleLogger::PrintSamples, this);
//}


/***************************************************
* tSampleLogger::LogSample
*
* INPUTS:
*    
*/

void tSampleLogger::LogSample(int nRcvdByServer, int nSentByClient, struct timeval &tmRcv, struct timeval &tmSent, struct sockaddr_in &ClientAddress)
{
  std::lock_guard<std::mutex> cvLock(_SampleQueueMutex);
  _SampleQueue.push_back(tLatencySample(nRcvdByServer, nSentByClient, tmRcv, tmSent, ClientAddress));
  // When using boost, we can query how full the buffer is and wait until it is half full before printing anything, to reduce thread thrashing

  #if 0
    #if defined(USE_BOOST_CIRCULAR_BUFFER) && defined(DEFER_PRINTING_WHEN_USING_BOOST)
      if (_SampleQueue.size() > SAMPLE_QUEUE_BUFFER_DEPTH/2)  _SampleQueueCondition.notify_one();
    #else
      _SampleQueueCondition.notify_one();
    #endif
  #endif
}


/***************************************************
* tSampleLogger::DrainSampleQueue
*
* This method should be called from a non-realtime thread
*
* INPUTS:
*    
*/

void tSampleLogger::DrainSampleQueue()
{
  tLatencySample Sample;

  while (!_SampleQueue.empty()) {
    {
      std::scoped_lock cvLock(_SampleQueueMutex);
      Sample = _SampleQueue.front();
      _SampleQueue.pop_front();
    }

    ProcessSample(Sample);
  }
}


/***************************************************
* tSampleLogger::PopAndProcessSample
*
* INPUTS:
*    
*/

void tSampleLogger::ProcessSample(const tLatencySample &Sample)
{
  struct timeval tmDiff;
  char sHostIpString[40];
  double dLatencyMicroseconds;

  // Sample._ClientAddress is a sockaddr_in.  inet_ntop wants a struct in_addr, which is 
  // the sin_addr member of the sockaddr_in
  inet_ntop(AF_INET, &Sample._ClientAddress.sin_addr, sHostIpString, 40);

  timersub(&Sample._tmRcv, &Sample._tmSent, &tmDiff);

  PrintSample(sHostIpString, Sample._tmRcv, Sample._tmSent, tmDiff,
              Sample._nRcvdByServer, Sample._nSentByClient);

  dLatencyMicroseconds = tmDiff.tv_sec*1.0e6 + tmDiff.tv_usec;
}


/***************************************************
* tSampleLogger::PrintSample
*
* INPUTS:
*    
*/

void tSampleLogger:: PrintSample(const char *sHostIpString, const struct timeval &tmSent, 
                                 const struct timeval &tmReceived, const struct timeval &tmDiff, 
                                 int nReceivedByServer, int nSentByClient)
{
  (void) printf("%s::(%d): Sent: %02ld.%06ld  Rcvd: %02ld.%06ld  Lat: %02ld.%06ld  Nrcvd:%3d   NSent:%3d\n", 
                sHostIpString, _iPortNum,
                tmSent.tv_sec,      tmSent.tv_usec, 
                tmReceived .tv_sec, tmReceived .tv_usec,
                tmDiff.tv_sec,      tmDiff.tv_usec, 
                ((nReceivedByServer-1)%50)+1,
                ((nSentByClient    -1)%50)+1);
}



/***************************************************
* tSamplePrinter::tSamplePrinter
*
* INPUTS:
*/

tSamplePrinter::tSamplePrinter()
{
  _PrintingThread = std::thread(&tSamplePrinter::SamplePrintingEndlessLoop, this);
}


/***************************************************
* tSamplePrinter::AddLogger - adds the logger to the list of loggers that we should print out
*
* INPUTS:
*/

void tSamplePrinter::AddLogger(tSampleLogger *pLogger)
{
  _SampleLoggerList.push_back(pLogger);
}


/***************************************************
* tSamplePrinter::RemoveLogger - deletes the specified logger from the list
*
* INPUTS:
*/

void tSamplePrinter::RemoveLogger(tSampleLogger *pLoggerToRemove)
{
  _SampleLoggerList.remove(pLoggerToRemove);
}


/***************************************************
* tSampleLogge::SamplePrintingEndlessLoop
*
* INPUTS:
*/

void tSamplePrinter::SamplePrintingEndlessLoop()
{
  while (1) {
    // Loop over all the loggers in our list
    for (auto & pLogger : _SampleLoggerList) {
      pLogger->DrainSampleQueue();
    }

    std::this_thread::sleep_for(std::chrono::milliseconds(20));
  }
}



/***************************************************
* tServer constructor
*
* INPUTS:
*/

tServer::tServer(int iPortNum, int iReceiveThreadPriority) :
  tPThread(iReceiveThreadPriority, true),
  _iPortNum(iPortNum),
  _UdpServer(iPortNum),
  _SampleLogger(iPortNum),
  _nReceived(0)
{
}


/***************************************************
* tServer move constructor
*
* This is used during assignment of the temporary object to the list.  If we don't have a
* move constructor, then the destructor for the temporary object will close the file
* descriptor.
*
* INPUTS:
*    other - the contents of the object being moved
*/

tServer::tServer(tServer &&other) noexcept :
  tPThread     (move(other)),
  _UdpServer   (move(other._UdpServer)),
  _SampleLogger(move(other._SampleLogger))
{
  _bDebug       = other._bDebug;
  _nReceived    = other._nReceived;
  _iPortNum     = other._iPortNum;
}

/***************************************************
* tServer destructor
*
*/

tServer::~tServer()
{
}

/***************************************************
* tServer::_Thread
*
*/

void *tServer::_Thread()
{
  string s = "Starting thread on port " + to_string(_iPortNum) + "\n";
  cout << s;

  ProcessIncomingMessages();

  return 0;
}

/*****************************
* tServer::ProcessIncomingMessage
*
*/

int tServer::ProcessIncomingMessages()
{
  ssize_t  len;;
  char buf[MAX_MESSAGE_SIZE];
  int            nSent;
  struct timeval tmRcv, tmSent;
  struct sockaddr_in ClientAddress;

  while (!_bExit) {  // Flag from base tPThread class
    len = _UdpServer.ReceiveMessage(buf, sizeof(buf), &ClientAddress);

    if (len != MAX_MESSAGE_SIZE) {
      cerr << "Error: len = " << len << endl;
      throw(std::runtime_error("ERROR: Bad Received Message Size"));
    }

    gettimeofday(&tmRcv, NULL);
    tmSent = ((DataHdr *) buf)->time;
    nSent  = ((DataHdr *) buf)->hdr.msgId;

    _SampleLogger.LogSample(++_nReceived, nSent, tmRcv, tmSent, ClientAddress);
  }

  return 0;
}


/***************************************************
* tServerList constructor
*
* INPUTS:
*    
*/

tServerList::tServerList(int iFirstPortNum, int iLastPortNum, int iReceiveThreadPriority)
{
  int iPortNum;

  _bExit = false;

  for (iPortNum=iFirstPortNum; iPortNum<=iLastPortNum; iPortNum++) {
    AddServer(iPortNum, iReceiveThreadPriority);
  }
}

/***************************************************
* tServerList::AddConnection
*
* INPUTS:
*    sServer   - string descriptor of the service we are connecting to,
*                e.g., "app_srv19"
*    sHostname - hostname or dot-separated IP address
*/

int tServerList::AddServer(int iPortNum, int iReceiveThreadPriority)
{
  _ServerList.push_back(tServer(iPortNum, iReceiveThreadPriority));
  //_ServerList.back().StartSampleLoggerThread();

  return 0;
}

/***************************************************
* tServerList::ProcessTelemetryUsingThreads
*
* 
*    
*/

int tServerList::ProcessTelemetry()
{
  sigset_t  sigset;
  int       sig;

  for (auto & Server : _ServerList) {
    Server.StartThread();
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

  for (auto & Server : _ServerList) {
    if (Server.IsRunning())  Server.StopThread(true);
  }

  return 0;
}
