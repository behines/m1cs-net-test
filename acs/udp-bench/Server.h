/****************************************************
* tServer
*
* Provides a UDP server for LSEB clients
*/

#ifndef INC_Server_h
#define INC_Server_h

#include <string>
#include <list>
#include <deque>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <sys/time.h>
#include <boost/circular_buffer.hpp>
#include "PThread.h"
#include "UdpConnection.h"

#define USE_BOOST_CIRCULAR_BUFFER
//#define DEFER_PRINTING_WHEN_USING_BOOST

#ifdef  USE_BOOST_CIRCULAR_BUFFER
#define SAMPLE_QUEUE_BUFFER_DEPTH (100)
#else
#define SAMPLE_QUEUE_BUFFER_DEPTH 
#endif

class tSampleLogger;

/***************************
* tLatencySample
*/

struct tLatencySample {
  tLatencySample() {}
  tLatencySample(int nRcvdByServer, int nSentByClient, struct timeval &tmRcv, struct timeval &tmSent, 
                 struct timeval &tmHwRcv, struct sockaddr_in &ClientAddress) :
    _nRcvdByServer(nRcvdByServer), _nSentByClient(nSentByClient), _tmRcv(tmRcv), _tmSent(tmSent), 
    _tmHwRcv(tmHwRcv), _ClientAddress(ClientAddress) {}

  int                _nRcvdByServer;
  int                _nSentByClient;
  struct timeval     _tmRcv;
  struct timeval     _tmSent;
  struct timeval     _tmHwRcv;
  struct sockaddr_in _ClientAddress;
};


/***************************
* tSamplePrinter - A thread that continuously drains the _SampleQueue of each tSampleLogger
*
* This thread runs at user priority, while the server threads run at realtime priority.
*/

class tSamplePrinter {
public:
  tSamplePrinter();

  void AddLogger   (tSampleLogger *pLogger);
  void RemoveLogger(tSampleLogger *pLoggerToRemove);

protected:
  void SamplePrintingEndlessLoop();

  std::thread                _PrintingThread;
  std::list<tSampleLogger *> _SampleLoggerList;
};


/***************************
* tSampleLogger - A buffer for logging arriving samples and relaying them for printing
*/

class tSampleLogger {
public:
  friend class tSamplePrinter;

  tSampleLogger(int iPortNum);

  tSampleLogger(tSampleLogger &&obj) noexcept;  // Move constructor - needed so that destruction of temporary does not close file.
  // tSampleLogger& operator=(tSampleLogger&& other); // Move assignment operator, will add if needed

  // Copy constructor and copy assignment operator are deleted - must only use move constructor
  tSampleLogger(const tSampleLogger &) = delete;   
  tSampleLogger& operator=(const tSampleLogger &) = delete;
  
  void DrainSampleQueue();
  void ProcessSample(const tLatencySample &Sample);
  void PrintSample(const char *sHostIpString, const struct timeval &tmSent,
                   const struct timeval &tmReceived, const struct timeval &tmDiff,
                   const struct timeval &tmHwReceived, const struct timeval &tmRcvKernelLatency,
                   int nReceivedByServer, int nSentByClient);

  ~tSampleLogger();

  void StartLoggerThread();

  void LogSample(int nRcvdByServer, int nSentByClient, struct timeval &tmRcv, struct timeval &tmSent,
                struct timeval &tmHwRcv, struct sockaddr_in &ClientAddress);

protected:
  #ifdef  USE_BOOST_CIRCULAR_BUFFER
    boost::circular_buffer<tLatencySample> _SampleQueue;
  #else
    std::deque<tLatencySample> _SampleQueue;
  #endif

  // Mutex and condition variable to allow queue-draining method to block waiting on samples
  std::mutex              _SampleQueueMutex;
  // std::condition_variable _SampleQueueCondition;
  // std::thread _thread;

  static tSamplePrinter   _SamplePrinter;

  int _iPortNum;
};



/***************************
* tServer - A thread that hosts a tUdpServer to listen for messages
*/

class tServer : public tPThread {
friend class tServerList;
public:
  tServer(int iPortNum, int iReceiveThreadPriority = 0);

  tServer(tServer &&obj) noexcept;  // Move constructor - needed so that destruction of temporary does not close file.
  // tHostConnection& operator=(tHostConnection&& other); // Move assignment operator, will add if needed

  // Copy constructor and copy assignment operator are deleted - must only use move constructor
  tServer(const tServer &) = delete;   
  tServer& operator=(const tServer &) = delete;

  ~tServer();
  
  //void StartSampleLoggerThread() { _SampleLogger.StartLoggerThread(); }

  int ProcessIncomingMessages();

protected:
  virtual void *_Thread();
  int           _iPortNum;
  tUdpServer    _UdpServer;
  bool          _bDebug;
  tSampleLogger _SampleLogger;
  int           _nReceived;
};


/***************************
* tServerList - Container for all the Server thread objects
*/

class tServerList {
public:
  tServerList(int iFirstPortNum, int iLastPortNum, int iReceiveThreadPriority);
  int AddServer(int iPortNum, int iReceiveThreadPriority);

  bool IsEmpty() { return _ServerList.empty(); }

  int ProcessTelemetry();

protected:
  std::list<tServer> _ServerList;
  bool _bExit;
};


#endif  // INC_Server_h
