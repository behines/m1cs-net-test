/* UdpConnection - Sens and receive UDP messages on a pair of sockets
*
* Obviously this class does not implement a "connection", but it acts like
* one.
*
* At the moment, only message sending is implemented, but the receive socket
* does get created and bound.  It will be a trivial matter to implement 
* receiving as well.  
*
* Also at the moment, both sockets are configured as broadcast - any IP, any
* port, so no specific IP configuration info is needed.
*
**
***
*
* Thirty Meter Telescope Project
* Lower Segment Electronics Box Software
*
* Copyright 2022 Jet Propulsion Laboratory
* All rights reserved
*
* Author:
* Brad Hines
* Planet A Energy
* Brad.Hines@PlanetA.Energy
*
*/


#ifndef UDPCONNECTION_H_
#define UDPCONNECTION_H_

#include <arpa/inet.h>
#include <sys/socket.h>
#include <string>

class tLogger;


/*********************
* tUdpConnectionException - Exception thrown by the class
*/

class tUdpConnectionException : public std::runtime_error {
public:
  tUdpConnectionException() : std::runtime_error("ERROR: Unknown UDP Exception") { }
  tUdpConnectionException(const std::string &s) : std::runtime_error(std::string("tUdpConnection: ") + s) { }
};



/*********************
* tUdpClient
*
* Creates sockets and provides a simplified interface to send messages
*/

class tUdpClient {
public:
  tUdpClient(const std::string &sServerIpAddressString, int iPortNum, const char *sClientIpAddressString = NULL);

  tUdpClient(tUdpClient &&obj) noexcept;  // Move constructor - needed so that destruction of temporary does not close file.
  // tHostConnection& operator=(tHostConnection&& other); // Move assignment operator, will add if needed

  // Copy constructor and copy assignment operator are deleted - must only use move constructor
  tUdpClient(const tUdpClient &) = delete;   
  tUdpClient& operator=(const tUdpClient &) = delete;

  virtual ~tUdpClient();


  void SendMessage(uint8_t *pMessage, int iNumBytes);

  bool IsInitialized()  { return _bInitSuccessfully; }

protected:
  int                _sockTx;
  struct sockaddr_in _SiHostTx;

  uint8_t            _ui8MsgIndex;
  bool               _bInitSuccessfully;
};


/*********************
* tUdpServer
*
* Creates sockets and provides a simplified interface to send messages
*/

class tUdpServer {
public:
  tUdpServer(int iPortNum);
  tUdpServer(tUdpServer &&obj) noexcept;  // Move constructor - needed so that destruction of temporary does not close file.
  // tHostConnection& operator=(tHostConnection&& other); // Move assignment operator, will add if needed

  // Copy constructor and copy assignment operator are deleted - must only use move constructor
  tUdpServer(const tUdpServer &) = delete;   
  tUdpServer& operator=(const tUdpServer &) = delete;


  virtual ~tUdpServer();

  ssize_t ReceiveMessage(void *buf, size_t iBufSize, struct sockaddr_in *pClientAddress);

  bool IsInitialized()  { return _bInitSuccessfully; }

protected:
  int                _sockRx;
  struct sockaddr_in _SiMe;

  uint8_t            _ui8MsgIndex;
  bool               _bInitSuccessfully;
};



#endif /* UDPCONNECTION_H_ */
