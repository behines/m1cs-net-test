/****************************************************
* tHostConnection
*
* Forges and maintains a connection with a server
*/

#ifndef INC_HostConnection_h
#define INC_HostConnection_h

class tHostConnection {
public:
  tHostConnection(const char *sServer, const char *sHostname);
  tHostConnection(tHostConnection &&obj) noexcept;  // Move constructor - needed so that destruction of temporary does not close file.

  ~tHostConnection();
  int ProcessTelemetry();


  bool IsConnected()  { return (_fdConnection >= 0); }
  operator bool()     { return  IsConnected();       }

protected:
  int  _fdConnection;
  bool _bDebug;
};


#endif  // INC_HostConnection_h
