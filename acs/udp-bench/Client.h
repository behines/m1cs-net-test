/****************************************************
* tClient
*
* LSEB UDP clients
*/

#ifndef INC_Client_h
#define INC_Client_h

#include <string>
#include <list>
#include <queue>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <sys/time.h>
#include "PThread.h"
#include "UdpConnection.h"



class tClient {
friend class tClientList;
public:
  tClient(const std::string &sServerIpAddressString, int iPortNum, const char *sClientIpAddressString = NULL);

  tClient(tClient &&obj) noexcept;  // Move constructor - needed so that destruction of temporary does not close file.
  // tHostConnection& operator=(tHostConnection&& other); // Move assignment operator, will add if needed

  // Copy constructor and copy assignment operator are deleted - must only use move constructor
  tClient(const tClient &) = delete;   
  tClient& operator=(const tClient &) = delete;

  ~tClient();
  
  int SendMessage();
  void RetrieveLastHardwareTxMessageTimestamp();

  void PrintStats();

  const std::string &NetworkDeviceName() { return _UdpClient.NetworkDeviceName(); }

protected:
  //virtual void *_Thread();
  int            _iPortNum;
  tUdpClient     _UdpClient;
  bool           _bDebug;
  int            _nSent;
  int            _nMissingTimestamps;

  struct timeval _tvLastMessageTimestamp;
};


class tClientSenderThread : public tPThread {
public:
  tClientSenderThread(std::mutex &ClientSendMutex, std::condition_variable &ClientSendCondition,
                      int iSenderThreadPriority = 0);

  tClientSenderThread(tClientSenderThread &&obj) noexcept;  // Move constructor - needed so that destruction of temporary does not close file.
  // tHostConnection& operator=(tHostConnection&& other); // Move assignment operator, will add if needed

  // Copy constructor and copy assignment operator are deleted - must only use move constructor
  tClientSenderThread(const tClientSenderThread &) = delete;   
  tClientSenderThread& operator=(const tClientSenderThread &) = delete;

  ~tClientSenderThread();
  
  int EmitMessagesFromAll();

  void AddClient(tClient &Client);

  void NotifyThatAwakeningIsLegitimate() { _bIsLegitimateAwakening = true; }

protected:
  virtual void *_Thread();

  std::mutex              &_ClientSendMutex;
  std::condition_variable &_ClientSendCondition;
  bool                     _bIsLegitimateAwakening;

  std::list<tClient *>     _ClientPointersList;
};




class tClientList {
public:
  tClientList() : _bExit(false) {}
  int AddClient(const std::string &sServerIpAddressString, int iPortNum, const char *sClientIpAddressString,
                int iSenderThreadPriority);

  void StartSenderThreads();
  
  int EmitMessagesFromAll();
  void PrintStatsFromAll();

  ~tClientList();

protected:
  std::list<tClient> _ClientList;
  std::map<std::string,tClientSenderThread *> _SenderThreadList;

  // Mutex and condition variable for notifying individual threads to run
  std::mutex              _ClientSendMutex;
  std::condition_variable _ClientSendCondition;

  bool _bExit;
};


#endif  // INC_Client_h
