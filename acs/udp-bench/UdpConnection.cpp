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


#include <netinet/in.h>
#include <net/if.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/socket.h>

#include <string>
#include <cstring>
#include <cerrno>
#include <cassert>
#include <unistd.h>   // usleep
#include <cstdio>
#include <iostream>

#include "UdpConnection.h"

using namespace std;



/*********************************************
* tUdpClient constructor 
*
* Initializes the sockaddr_in structures for sending and
* receiving.  Then create and binds the sockets.
*
* Also at the moment, both sockets are configured as broadcast - any IP, any
* port, so no specific IP configuration info is needed.
*
* INPUTS:
*   sServerIpAddressString - server to target
*   iPortNum               - port number on server to target
* SIDE EFFECTS:
*   Creates and binds the sockets.
*   Will set _bInitSuccessfully if construction is successful
*   Will throw a tUdpConnectionException if an error occurs.
*/

tUdpClient::tUdpClient(const string &sServerIpAddressString, int iPortNum, const char *sClientIpAddressString) 
{

  int iBroadcastEnable = (iPortNum == INADDR_ANY);
  in_addr_t ipServerAddressInBinary, ipClientAddressInBinary;
  struct sockaddr_in siClientTx;

  _bInitSuccessfully = false;

  // Use inet_pton() to go from string to binary from. Do not use inet_ntoa() or inet_aton(), they are deprecated.
  if (inet_pton(AF_INET, sServerIpAddressString.c_str(), &ipServerAddressInBinary) != 1) {
    throw tUdpConnectionException("Error converting IP address " + sServerIpAddressString + " to binary");
  }

  // Prepare the sockaddr structure for transmissions, to the specified port
  bzero((char*)&_SiHostTx, sizeof(_SiHostTx));
  _SiHostTx.sin_family      = AF_INET;
  _SiHostTx.sin_port        = htons(iPortNum);
  _SiHostTx.sin_addr.s_addr = ipServerAddressInBinary;

  // Initialize the socket to 0
  _sockTx = 0;

  // Create the socket for transmit 
  _sockTx = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
  if (_sockTx < 0) {
    throw tUdpConnectionException("Error opening UDP transmit socket");
  }
  if (setsockopt(_sockTx, SOL_SOCKET, SO_BROADCAST, 
                 &iBroadcastEnable, sizeof(iBroadcastEnable)) < 0) {
    throw tUdpConnectionException("Error configuring UDP transmit socket broadcast option");
  } 


  // If a client source IP address is provided, bind to it.
  if (sClientIpAddressString != NULL) {
    // Use inet_pton() to go from string to binary from. Do not use inet_ntoa() or inet_aton(), they are deprecated.
    if (inet_pton(AF_INET, sClientIpAddressString, &ipClientAddressInBinary) != 1) {
      throw tUdpConnectionException(string("Error converting IP address ") + sClientIpAddressString + " to binary");
    }
    bzero((char*)&siClientTx, sizeof(siClientTx));
    siClientTx.sin_family      = AF_INET;
    siClientTx.sin_port        = 0;
    siClientTx.sin_addr.s_addr = ipClientAddressInBinary;

    if (bind(_sockTx, (struct sockaddr *) &siClientTx, sizeof(siClientTx)) < 0) {
      throw tUdpConnectionException("Error binding UDP transmit socket");
    }
    // cout << "Bound client to IP address " << sClientIpAddressString << endl;

  }


  cout << "Created UDP transmit socket " << _sockTx << " to " << sServerIpAddressString << "::" << iPortNum << endl;

  _ui8MsgIndex = 0;
  _bInitSuccessfully = true;
}


/***************************************************
* tUdpClient move constructor
*
* This is used during assignment of the temporary object to the list.  If we don't have a
* move constructor, then the destructor for the temporary object will close the socket.
*
* INPUTS:
*    other - the contents of the object being moved
*/

tUdpClient::tUdpClient(tUdpClient &&other) noexcept :
  _sockTx           (other._sockTx),
  _SiHostTx         (other._SiHostTx),
  _ui8MsgIndex      (other._ui8MsgIndex),
  _bInitSuccessfully(other._bInitSuccessfully)
{
  other._sockTx = 0;  // Prevent the old object from closing the socket when it dies
}


/*********************************************
* tUdpClient::SendMessage
*
* 
* INPUTS:
*   pMessage  - the message to send
*   iNumBytes - the length of the message to send
* RETURNS:
*   Nothing.
* SIDE EFFECTS:
*   Broadcasts the message
*   Throws an exception if there is a sending error
*/

void tUdpClient::SendMessage(uint8_t *pMessage, int iNumBytes)
{
  int retval;
  //char sMsg[100];

  assert(pMessage != nullptr);
  assert(iNumBytes > 0);

  retval = sendto(_sockTx, pMessage, iNumBytes, 0, (struct sockaddr *) &_SiHostTx, sizeof(_SiHostTx));
  if (retval == -1) {
    throw tUdpConnectionException(std::string("SendMessage: ") + strerror(errno));
  }
}



/*********************************************
* tUdpClient destructor 
*
* SIDE EFFECTS:
*   Shuts down the Tx socket
*/

tUdpClient::~tUdpClient()
{
  if (_sockTx > 0)  close(_sockTx);

  _sockTx = 0;
}




/*********************************************
* tUdpServer constructor 
*
* Initializes the sockaddr_in structures for sending and
* receiving.  Then create and binds the sockets.
*
* Also at the moment, both sockets are configured as broadcast - any IP, any
* port, so no specific IP configuration info is needed.
*
* INPUTS:
*   Logger - the application-wide logger object
* SIDE EFFECTS:
*   Creates and binds the sockets.
*   Will set _bInitSuccessfully if construction is successful
*   Will throw a tUdpConnectionException if an error occurs.
*/

tUdpServer::tUdpServer(int iPortNum)
{
  int iBroadcastEnable = 1;

  _bInitSuccessfully = false;

  // Initialize the server
  // Prepare the sockaddr structure for receiving on the specified port
  bzero((char*)&_SiMe, sizeof(_SiMe));
  _SiMe.sin_family      = AF_INET;
  _SiMe.sin_port        = htons(iPortNum);
  _SiMe.sin_addr.s_addr = INADDR_ANY;

  // Initialize the sockets to 0
  _sockRx = 0;


  // Create the socket for receive 
  _sockRx = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
  if (_sockRx < 0) {
    throw tUdpConnectionException("Error opening UDP receive socket");
  }
  if (setsockopt(_sockRx, SOL_SOCKET, SO_BROADCAST, 
                 &iBroadcastEnable, sizeof(iBroadcastEnable)) < 0) {
    throw tUdpConnectionException("Error configuring UDP receive socket for broadcast");
  } 

  // The receive socket has to be bound.  This associates the port with
  // the socket, so that the protocol layer knows which programs should get
  // which datagrams.
  if (bind(_sockRx, (struct sockaddr *) &_SiMe, sizeof(_SiMe)) < 0) {
		throw tUdpConnectionException("Error binding UDP receive socket");
	}

  _ui8MsgIndex = 0;
  _bInitSuccessfully = true;
}


/***************************************************
* tUdpServer move constructor
*
* This is used during assignment of the temporary object to the list.  If we don't have a
* move constructor, then the destructor for the temporary object will close the socket.
*
* INPUTS:
*    other - the contents of the object being moved
*/

tUdpServer::tUdpServer(tUdpServer &&other) noexcept :
  _sockRx           (other._sockRx),
  _SiMe             (other._SiMe),
  _ui8MsgIndex      (other._ui8MsgIndex),
  _bInitSuccessfully(other._bInitSuccessfully)
{
  other._sockRx = 0;  // Prevent the old object from closing the socket when it dies
}


/*********************************************
* tUdpServer destructor 
*
* SIDE EFFECTS:
*   Shuts down the Rx socket
*/

tUdpServer::~tUdpServer()
{
  // It turns out that on Linux, close() isn't enough to cause the recvfrom() in the
  // thread to unblock.  You have to call shutdown() on the socket.  We then close
  // it in any associated receive thread as it is exiting.
  // close(_sockRx);
  if (_sockRx > 0)  shutdown(_sockRx, SHUT_RDWR);

  _sockRx = 0;
}


/*********************************************
* tUdpServer::ReceiveMessage 
*
* SIDE EFFECTS:
*
* RETURNS:
*   ssize_t
*   If not NULL, *pClientAddress is populated with info on the source of the packet
*/

ssize_t tUdpServer::ReceiveMessage(void *buf, size_t szBufSize, struct sockaddr_in *pClientAddress)
{
  ssize_t n = 0;
  socklen_t sz = sizeof(*pClientAddress);

  n = recvfrom(_sockRx, buf, szBufSize, 0, (struct sockaddr *) pClientAddress, &sz);

  if (n < 0) {
	  throw tUdpConnectionException(std::string("ReceiveMessage: ") + strerror(errno));
  }

  return n;
}