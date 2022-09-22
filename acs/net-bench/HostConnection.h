/****************************************************
* tHostConnection
*
* Forges and maintains a connection with a server
*/

#ifndef INC_HostConnection_h
#define INC_HostConnection_h

#include <string>
#include <list>
#include <queue>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <sys/time.h>
#include "PThread.h"


struct tLatencySample {
  tLatencySample() {}
  tLatencySample(int nSamp, struct timeval &tmRcv, struct timeval &tmSent) :
    _nSamp(nSamp), _tmRcv(tmRcv), _tmSent(tmSent) {}

  int            _nSamp;
  struct timeval _tmRcv;
  struct timeval _tmSent;
};


class tSampleLogger {
public:
  tSampleLogger(const std::string &sServer, const std::string &sHostname);

  tSampleLogger(tSampleLogger &&obj) noexcept;  // Move constructor - needed so that destruction of temporary does not close file.
  // tSampleLogger& operator=(tSampleLogger&& other); // Move assignment operator, will add if needed

  // Copy constructor and copy assignment operator are deleted - must only use move constructor
  tSampleLogger(const tSampleLogger &) = delete;   
  tSampleLogger& operator=(const tSampleLogger &) = delete;

  ~tSampleLogger();

  void StartLoggerThread();


  void LogSample(int nSamp, struct timeval &tmRcv, struct timeval &tmSent);
  void PrintSamples();

protected:
  std::string _sServer;
  std::string _sHostname;
  std::queue<tLatencySample> _SampleQueue;

  // Mutex and condition variable to allow queue-draining method to block waiting on samples
  std::mutex              _SampleQueueMutex;
  std::condition_variable _SampleQueueCondition;

  std::thread _thread;
};


class tHostConnection : public tPThread {
friend class tHostConnectionList;
public:
  tHostConnection(const char *sServer, const char *sHostname, int iReceiveThreadPriority = 0);
  tHostConnection(const std::string &sServer, const std::string &sHostname, int iReceiveThreadPriority = 0);

  tHostConnection(tHostConnection &&obj) noexcept;  // Move constructor - needed so that destruction of temporary does not close file.
  // tHostConnection& operator=(tHostConnection&& other); // Move assignment operator, will add if needed

  // Copy constructor and copy assignment operator are deleted - must only use move constructor
  tHostConnection(const tHostConnection &) = delete;   
  tHostConnection& operator=(const tHostConnection &) = delete;

  ~tHostConnection();
  
  void StartSampleLoggerThread() { _SampleLogger.StartLoggerThread(); }

  int ProcessIncomingMessage();

  bool IsConnected()  { return (_fdConnection >= 0); }
  operator bool()     { return  IsConnected();       }

protected:
  virtual void *_Thread();

  int           _fdConnection;
  bool          _bDebug;
  tSampleLogger _SampleLogger;
  int           _nReceived;
  std::string   _sServer;
  std::string   _sHostname;
};



class tHostConnectionList {
public:
  tHostConnectionList() : _bExit(false) {}
  int AddConnection(const char *sServer, const char *sHostname, int iReceiveThreadPriority);
  int AddConnection(const std::string &sServer, const std::string &sHostname, int iReceiveThreadPriority);

  bool IsEmpty() { return _ConnectionList.empty(); }

  int ProcessTelemetryUsingSelect();
  int ProcessTelemetryUsingThreads();

protected:
  std::list<tHostConnection> _ConnectionList;
  bool _bExit;
};


#endif  // INC_HostConnection_h
