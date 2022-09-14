/****************************************************
* tHostConnection
*
* Forges and maintains a connection with a server
*/

#ifndef INC_HostConnection_h
#define INC_HostConnection_h

#include <string>
#include <list>

class tHostConnection {
public:
  tHostConnection(const char *sServer, const char *sHostname);
  tHostConnection(const std::string &sServer, const std::string &sHostname);
  tHostConnection(tHostConnection &&obj) noexcept;  // Move constructor - needed so that destruction of temporary does not close file.

  ~tHostConnection();
  int ProcessTelemetry();

  bool IsConnected()  { return (_fdConnection >= 0); }
  operator bool()     { return  IsConnected();       }

protected:
  int  _fdConnection;
  bool _bDebug;
  std::string _sHostname;
  std::string _sServer;
};



class tHostConnectionList {
public:
  tHostConnectionList() {}
  int AddConnection(const char *sServer, const char *sHostname);
  int AddConnection(const std::string &sServer, const std::string &sHostname);

  bool IsEmpty() { return _ConnectionList.empty(); }

  int ProcessTelemetry() { if (!IsEmpty())  { return _ConnectionList.front().ProcessTelemetry(); } return 0; }

protected:
  std::list<tHostConnection> _ConnectionList;
};


#endif  // INC_HostConnection_h
