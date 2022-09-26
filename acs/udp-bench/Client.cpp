/****************************************************
* tClient
*
* Sends messages to a UDP server
*/

#include "Client.h"
#include <errno.h>
#include <string.h>
#include <signal.h>
#include <iostream>
#include <utility>

extern "C" {
  #include "GlcMsg.h"
  #include "GlcLscsIf.h"
}

#define MAX_MESSAGE_SIZE (sizeof(SegRtDataMsg))

using namespace std;




/***************************************************
* tClient constructor
*
* INPUTS:
*/

tClient::tClient(const std::string &sServerIpAddressString, int iPortNum, const char *sClientIpAddressString) :
  _iPortNum(iPortNum),
  _UdpClient(sServerIpAddressString, iPortNum, sClientIpAddressString),
  _bDebug(false),
  _nSent(0)
{
}

/***************************************************
* tClient move constructor
*
* This is used during assignment of the temporary object to the list.  If we don't have a
* move constructor, then the destructor for the temporary object will close the file
* descriptor.
*
* INPUTS:
*    other - the contents of the object being moved
*/

tClient::tClient(tClient &&other) noexcept :
  _UdpClient(move(other._UdpClient))
{
  _iPortNum = other._iPortNum;
  _bDebug   = other._bDebug;
  _nSent    = other._nSent;
}


/***************************************************
* tClient destructor
*
*/

tClient::~tClient()
{
}


/*****************************
* tClient::SendMessage
*
* See https://stackoverflow.com/questions/3062205/setting-the-source-ip-for-a-udp-socket 
* for information on how to set the source address for a UDP message.
*/

int tClient::SendMessage()
{
    struct timeval tm;
    SegRtDataMsg seg_msg;

    gettimeofday (&tm, NULL);
    seg_msg.hdr.time      = tm;
    seg_msg.hdr.hdr.msgId = ++_nSent;
    _UdpClient.SendMessage((uint8_t *) &seg_msg, sizeof(seg_msg));
    // cout << "Send" << endl;

    return 0;
}


/***************************************************
* tClientList::AddConnection
*
* INPUTS:
*/

int tClientList::AddClient(const std::string &sServerIpAddressString, int iPortNum, const char *sClientIpAddressString)
{
  _ClientList.push_back(tClient(sServerIpAddressString, iPortNum, sClientIpAddressString));

  return 0;
}





/***************************************************
* tClientList::EmitMessagesFromAll
*    
*/

int tClientList::EmitMessagesFromAll()
{
  for (auto & Client : _ClientList) {
    Client.SendMessage();
  }

  return 0;
}