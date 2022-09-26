/****************************************************
* tServer
*
* Provides a UDP server for LSEB clients
*/

#ifndef INC_Server_h
#define INC_Server_h

#include <string>
#include <list>
#include <queue>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <sys/time.h>
#include "PThread.h"
#include "UdpConnection.h"


struct tLatencySample {
  tLatencySample() {}
  tLatencySample(int nRcvdByServer, int nSentByClient, struct timeval &tmRcv, struct timeval &tmSent, struct sockaddr_in &ClientAddress) :
    _nRcvdByServer(nRcvdByServer), _nSentByClient(nSentByClient), _tmRcv(tmRcv), _tmSent(tmSent), _ClientAddress(ClientAddress) {}

  int                _nRcvdByServer;
  int                _nSentByClient;
  struct timeval     _tmRcv;
  struct timeval     _tmSent;
  struct sockaddr_in _ClientAddress;
};


class tSampleLogger {
public:
  tSampleLogger(int iPortNum);

  tSampleLogger(tSampleLogger &&obj) noexcept;  // Move constructor - needed so that destruction of temporary does not close file.
  // tSampleLogger& operator=(tSampleLogger&& other); // Move assignment operator, will add if needed

  // Copy constructor and copy assignment operator are deleted - must only use move constructor
  tSampleLogger(const tSampleLogger &) = delete;   
  tSampleLogger& operator=(const tSampleLogger &) = delete;

  ~tSampleLogger();

  void StartLoggerThread();

  void LogSample(int nRcvdByServer, int nSentByClient, struct timeval &tmRcv, struct timeval &tmSent, struct sockaddr_in &ClientAddr);
  void PrintSamples();

protected:
  std::queue<tLatencySample> _SampleQueue;

  // Mutex and condition variable to allow queue-draining method to block waiting on samples
  std::mutex              _SampleQueueMutex;
  std::condition_variable _SampleQueueCondition;

  std::thread _thread;

  int _iPortNum;
};


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
  
  void StartSampleLoggerThread() { _SampleLogger.StartLoggerThread(); }

  int ProcessIncomingMessages();

protected:
  virtual void *_Thread();
  int           _iPortNum;
  tUdpServer    _UdpServer;
  bool          _bDebug;
  tSampleLogger _SampleLogger;
  int           _nReceived;
};



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
