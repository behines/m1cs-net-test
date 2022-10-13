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
#include <linux/net_tstamp.h>

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
* Static member initialization
*
* This will read in the mapping of IP addresses to device names.
*/

tIpInterfaceInfo tUdpClient::_IpInterfaceInfo;


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
*   iServerPortNum               - port number on server to target
* SIDE EFFECTS:
*   Creates and binds the sockets.
*   Will set _bInitSuccessfully if construction is successful
*   Will throw a tUdpConnectionException if an error occurs.
*/

tUdpClient::tUdpClient(const string &sServerIpAddressString, int iServerPortNum, const char *sClientIpAddressString) 
{

  int iBroadcastEnable = (iServerPortNum == INADDR_ANY);
  in_addr_t ipServerAddressInBinary, ipClientAddressInBinary;
  struct sockaddr_in siClientTx, siClientTxBound;
  char sIpAddressString[128];
  char ifName[IFNAMSIZ];

  _bInitSuccessfully = false;

  // Use inet_pton() to go from string to binary from. Do not use inet_ntoa() or inet_aton(), they are deprecated.
  if (inet_pton(AF_INET, sServerIpAddressString.c_str(), &ipServerAddressInBinary) != 1) {
    throw tUdpConnectionException("Error converting IP address " + sServerIpAddressString + " to binary");
  }

  // Prepare the sockaddr structure for transmissions, to the specified port
  bzero((char*)&_SiHostTx, sizeof(_SiHostTx));
  _SiHostTx.sin_family      = AF_INET;
  _SiHostTx.sin_port        = htons(iServerPortNum);
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

    _sDeviceName = _IpInterfaceInfo.Ipv4BinaryAddressToDeviceName(ipClientAddressInBinary);
    socklen_t slen = _sDeviceName.length();

    if (setsockopt(_sockTx, SOL_SOCKET, SO_BINDTODEVICE, (void *) _sDeviceName.c_str(), slen) == -1) {
      throw tUdpConnectionException("Error binding socket to device");
    }
   
    socklen_t socklen = sizeof(siClientTxBound);

    if (getsockname(_sockTx, (struct sockaddr *) &siClientTxBound, &socklen) < 0) {
        throw tUdpConnectionException("Error reading back IP address of UDP transmit socket");
    }
    if (inet_ntop(AF_INET, &siClientTx.sin_addr.s_addr, sIpAddressString, 128) == NULL) {
      throw tUdpConnectionException(string("Error converting IP address to string"));
    }

    slen = IFNAMSIZ;
    if (getsockopt(_sockTx, SOL_SOCKET, SO_BINDTODEVICE, (void *) ifName, &slen) == -1) {
      cerr << "getsockopt failed: " << strerror(errno) << endl;
    }
  }

  cout << "Created UDP transmit socket " << _sockTx << " (" << sIpAddressString << "::" << siClientTxBound.sin_port << ") on "
       << ifName << " targeting " << sServerIpAddressString << "::" << iServerPortNum << endl;

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
  _bInitSuccessfully(other._bInitSuccessfully),
  _sDeviceName      (other._sDeviceName)
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

tUdpServer::tUdpServer(int iServerPortNum)
{
  int iBroadcastEnable = 1;

  _bInitSuccessfully = false;

  // Initialize the server
  // Prepare the sockaddr structure for receiving on the specified port
  bzero((char*)&_SiMe, sizeof(_SiMe));
  _SiMe.sin_family      = AF_INET;
  _SiMe.sin_port        = htons(iServerPortNum);
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
  
  // Configure the socket to support hardware timestamping
  int flags = SOF_TIMESTAMPING_RX_HARDWARE |
              SOF_TIMESTAMPING_RAW_HARDWARE;
  // SO_TIMESTAMPING_NEW guarantees that we get 64-bit timestamps
  if (setsockopt(_sockRx, SOL_SOCKET, SO_TIMESTAMPING_NEW, &flags, sizeof(flags)) < 0) {
    throw tUdpConnectionException("Error configuring UDP receive socket hardware timestamp support");
  }

  // The receive socket has to be bound.  This associates the port with
  // the socket, so that the protocol layer knows which programs should get
  // which datagrams.
  if (bind(_sockRx, (struct sockaddr *) &_SiMe, sizeof(_SiMe)) < 0) {
		throw tUdpConnectionException("Error binding UDP receive socket");
	}

  _ui8MsgIndex = 0;
  _bInitSuccessfully = true;

#if 0
  // Initialize message header structure for use with recvmsg()
  memset(&_MsgHdr,  0, sizeof(_MsgHdr));
  memset(&_iov,     0, sizeof(_iov));
  _MsgHdr.msg_iov        = &_iov;  // _iov will itself be populated in the call to ReceiveMessage()
  _MsgHdr.msg_iovlen     = 1;
  //_MsgHdr.msg_name       = &_SiMe;
  //_MsgHdr.msg_namelen    = sizeof(_SiMe);
  _MsgHdr.msg_control    = _MsgControlBuf;
  _MsgHdr.msg_controllen = UDPCONNECTION_RECEIVE_MESSAGE_CONTROL_BUF_LEN;
  #endif
  // Initialize message header structure for use with recvmsg()
  memset(&_MsgHdr,  0, sizeof(_MsgHdr));
  memset(&_iov,     0, sizeof(_iov));
  _MsgHdr.msg_iov        = &_iov;  // _iov will itself be populated in the call to ReceiveMessage()
  _MsgHdr.msg_iovlen     = 1;
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
  _bInitSuccessfully(other._bInitSuccessfully),
  _iov              (other._iov),
  _MsgHdr           (other._MsgHdr)
{
  _MsgHdr.msg_iov = &_iov;
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
  //struct cmsghdr     _MsgControlBuf;
  //socklen_t sz = sizeof(*pClientAddress);

  // n = recvfrom(_sockRx, buf, szBufSize, 0, (struct sockaddr *) pClientAddress, &sz);

  //memset(&_MsgHdr,  0, sizeof(_MsgHdr));
  //memset(&_iov,     0, sizeof(_iov));
  //_MsgHdr = _MsgHdr;

  // For sample recvmsg code that retrieves timestamps, see https://github.com/Xilinx-CNS/onload/blob/master/src/tests/onload/hwtimestamping/rx_timestamping.c
  _iov.iov_base       = buf;
  _iov.iov_len        = szBufSize;
  _MsgHdr.msg_name    = pClientAddress;
  _MsgHdr.msg_namelen = sizeof(*pClientAddress);
  _MsgHdr.msg_control    = _MsgControlBuf;
  _MsgHdr.msg_controllen = sizeof(_MsgControlBuf); //UDPCONNECTION_RECEIVE_MESSAGE_CONTROL_BUF_LEN;

  n = recvmsg(_sockRx, &_MsgHdr, 0);

  if (n < 0) {
	  throw tUdpConnectionException(std::string("ReceiveMessage: ") + strerror(errno));
  }

  return n;
}


/*********************************************
* tUdpServer::GetHardwareTimestampOfLastMessage
*
* SIDE EFFECTS:
*
* RETURNS:
*   ssize_t
*   If not NULL, *pClientAddress is populated with info on the source of the packet
*/

struct timespec tUdpServer::GetHardwareTimestampOfLastMessage()
{
  struct cmsghdr *pCmsgHdr;
  struct timespec *ts = NULL;

  // See "man CMSG_FIRSTHDR" for details on these macros
  for (pCmsgHdr = CMSG_FIRSTHDR(&_MsgHdr); pCmsgHdr != NULL; pCmsgHdr = CMSG_NXTHDR(&_MsgHdr, pCmsgHdr))  {
    if (pCmsgHdr->cmsg_level == SOL_SOCKET && pCmsgHdr->cmsg_type == SCM_TIMESTAMPING) {  // Note that SCM_TIMESTAMPING and SO_TIMESTAMPING are equivalent
        ts = (struct timespec *) CMSG_DATA(pCmsgHdr);
        // Hardware timestamps are passed in ts[2]
        return ts[2];
    }
  }
  return {0,0};
}