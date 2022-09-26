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

protected:
  //virtual void *_Thread();
  int           _iPortNum;
  tUdpClient    _UdpClient;
  bool          _bDebug;
  int           _nSent;
};



class tClientList {
public:
  tClientList() : _bExit(false) {}
  int AddClient(const std::string &sServerIpAddressString, int iPortNum, const char *sClientIpAddressString = NULL);

  bool IsEmpty() { return _ClientList.empty(); }

  int EmitMessagesFromAll();

protected:
  std::list<tClient> _ClientList;
  bool _bExit;
};


#endif  // INC_Client_h
