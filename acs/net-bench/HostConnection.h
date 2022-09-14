/****************************************************
* tHostConnection
*
* Forges and maintains a connection with a server
*/

#ifndef INC_HostConnection_h
#define INC_HostConnection_h

#include <string>
#include <list>
#include "PThread.h"

class tHostConnection : public tPThread {
friend class tHostConnectionList;
public:
  tHostConnection(const char *sServer, const char *sHostname, int iReceiveThreadPriority = 0);
  tHostConnection(const std::string &sServer, const std::string &sHostname, int iReceiveThreadPriority = 0);
  tHostConnection(tHostConnection &&obj) noexcept;  // Move constructor - needed so that destruction of temporary does not close file.

  ~tHostConnection();
  int ProcessIncomingMessage();

  bool IsConnected()  { return (_fdConnection >= 0); }
  operator bool()     { return  IsConnected();       }

protected:
  virtual void *_Thread();

  int  _fdConnection;
  bool _bDebug;
  int  _nReceived;
  std::string _sServer;
  std::string _sHostname;
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
