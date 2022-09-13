/****************************************************
* tHostConnection
*
* Forges and maintains a connection with a server
*/

#ifndef INC_HostConnection_h
#define INC_HostConnection_h

class tHostConnection {
public:
  tHostConnection(char *sServer, char *sHostname);
  ~tHostConnection();
  int ProcessTelemetry();


  bool IsConnected()  { return (_fdConnection >= 0); }

protected:
  int  _fdConnection;
  bool _bDebug;
};


#endif  // INC_HostConnection_h
