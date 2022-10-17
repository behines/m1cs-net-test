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
  _nSent(0),
  _nMissingTimestamps(0)
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
  _iPortNum           = other._iPortNum;
  _bDebug             = other._bDebug;
  _nSent              = other._nSent;
  _nMissingTimestamps = other._nMissingTimestamps;
}


/***************************************************
* tClient::PrintStats
*
*/

void tClient::PrintStats()
{
  cout << _UdpClient.NetworkDeviceName() << "(" << _iPortNum << "): " << _nMissingTimestamps << " missing timestamps" << endl;
}


/***************************************************
* tClient destructor
*
*/

tClient::~tClient()
{
}


/*****************************
* tClient::RetrieveLastHardwareTxMessageTimestamp
*
* See https://stackoverflow.com/questions/3062205/setting-the-source-ip-for-a-udp-socket 
* for information on how to set the source address for a UDP message.
*/

void tClient::RetrieveLastHardwareTxMessageTimestamp()
{
  _tvLastMessageTimestamp = _UdpClient.RetrieveLastHardwareTxMessageTimestamp();

  if (_tvLastMessageTimestamp.tv_sec == 0)  ++_nMissingTimestamps;
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

  // Copy the timestamp of the previously transmitted message into the data
  // Disable GCC warning about unaligned pointer for this pointer assignment
  #pragma GCC diagnostic push
  #pragma GCC diagnostic ignored "-Waddress-of-packed-member"
    struct timeval *pLastTransmitTime = (struct timeval *) &seg_msg.data[0];
  #pragma GCC diagnostic pop

  *pLastTransmitTime = _tvLastMessageTimestamp;

  _UdpClient.SendMessage((uint8_t *) &seg_msg, sizeof(seg_msg));
  // cout << "Send" << endl;

  return 0;
}


/***************************************************
* tClientList::AddClient
*
* INPUTS:
*/

int tClientList::AddClient(const std::string &sServerIpAddressString, int iPortNum, const char *sClientIpAddressString,
                           int iSenderThreadPriority)
{
  tClientSenderThread *pSenderThread;

  _ClientList.push_back(tClient(sServerIpAddressString, iPortNum, sClientIpAddressString));

  // Add the new client to the thread associated with its network interface
  tClient &NewClient =_ClientList.back();
  const std::string &sNetworkDeviceName = NewClient.NetworkDeviceName();

  // See if there is an existing entry
  try {
    pSenderThread = _SenderThreadList.at(sNetworkDeviceName);
  } 
  catch (std::out_of_range &e) {
    // There isn't, so create one and add it to the map.
    // In principle, pSenderThread should be a std::unique_ptr for proper cleanup, 
    // but since we only destroy threads at program exit, the OS will clean up if we
    // forget.  No need to complicate things.  But nonetheless, we do clean up in the
    // destructor, but it's not RAII automatic cleanup.
    pSenderThread = new tClientSenderThread(_ClientSendMutex, _ClientSendCondition, iSenderThreadPriority);
    if (pSenderThread == nullptr) throw std::runtime_error("tClientList::AddClient: new tClientSenderThread failed.");
    _SenderThreadList[sNetworkDeviceName] = pSenderThread;
    cout << "Created ClientSenderThread for " << sNetworkDeviceName << endl;
  }

  // Now add the new client to the thread.
  pSenderThread->AddClient(NewClient);

  return 0;
}


/***************************************************
* StartSenderThreads::StartSenderThreads
*    
*/

void tClientList::StartSenderThreads()
{
  for (auto & MappedPair : _SenderThreadList) {
    // The thread is the second part of the pair
    MappedPair.second->StartThread();
  }
}


/***************************************************
* tClientList::EmitMessagesFromAll
*    
*/

int tClientList::EmitMessagesFromAll()
{
  // Notify all blocked threads to unblock and send
  std::lock_guard<std::mutex> cvLock(_ClientSendMutex);

  // Notify all threads that this is a real awakening, not spurious (works around the fact that
  // a condition variable can lead to spurious awakenings).
  for (auto & MappedPair : _SenderThreadList) {
    // The thread is the second part of the pair
    MappedPair.second->NotifyThatAwakeningIsLegitimate();
  }
  
  _ClientSendCondition.notify_all();

  //for (auto & Client : _ClientList) {
  //  Client.SendMessage();
  //}

  return 0;
}

/***************************************************
* tClientList::PrintStatsFromAll
*    
*/

void tClientList::PrintStatsFromAll()
{
  for (auto & Client : _ClientList)  Client.PrintStats();
}


/***************************************************
* tClientList::~tClientList
*    
*/

tClientList::~tClientList()
{
  // Clean up the allocated sender thread objects
  for (auto & MappedPair : _SenderThreadList) {
    // The thread is the second part of the pair
    delete MappedPair.second;
  }
}


/***************************************************
* tClientSenderThread::tClientSenderThread
*    
*/

tClientSenderThread::tClientSenderThread(std::mutex &ClientSendMutex, std::condition_variable &ClientSendCondition, 
                                         int iSenderThreadPriority) :
  tPThread(iSenderThreadPriority, true),
  _ClientSendMutex(ClientSendMutex),
  _ClientSendCondition(ClientSendCondition),
  _bIsLegitimateAwakening(false)    
{
}


/***************************************************
* tClientSenderThread move constructor
*
* This is used during assignment of the temporary object to the list.  If we don't have a
* move constructor, then the destructor for the temporary object will close the file
* descriptor.
*
* INPUTS:
*    other - the contents of the object being moved
*/

tClientSenderThread::tClientSenderThread(tClientSenderThread &&other) noexcept :
  tPThread            (move(other)),
  _ClientSendMutex    (     other._ClientSendMutex),
  _ClientSendCondition(     other._ClientSendCondition),
  _bIsLegitimateAwakening(  other._bIsLegitimateAwakening),
  _ClientPointersList (move(other._ClientPointersList))
{
}


/***************************************************
* tClientSenderThread destructor
*
*/

tClientSenderThread::~tClientSenderThread()
{
}


/***************************************************
* tClientSenderThread::AddClient
*    
*/

void tClientSenderThread::AddClient(tClient &Client)
{
  _ClientPointersList.push_back(&Client);
}


/***************************************************
* tClientSenderThread::EmitMessagesFromAll
*    
*/

int tClientSenderThread::EmitMessagesFromAll()
{
  for (auto & pClient : _ClientPointersList) {
    pClient->SendMessage();
    pthread_yield();
    pClient->RetrieveLastHardwareTxMessageTimestamp();
  }

  return 0;
}


/***************************************************
* tClientSenderThread::_Thread
*    
*/

void *tClientSenderThread::_Thread()
{
  while (!_bExit) {
    {
      std::unique_lock cvLock(_ClientSendMutex);
      // Block until notified that it is time to send.  This will unlock the mutex and then block.  
      // There's actually no need to specify a predicate here, but we do anyway.  (We don't
      // really care if we are spuriously awakened since we immediately test for our 
      // predicate in the while loop below.)
      _ClientSendCondition.wait(cvLock, [this] { return _bIsLegitimateAwakening; } );
      // The mutex is now locked, but will be unlocked when the scoped_lock is destroyed

      // Reset the predicate flag to once again prevent spurious awakenings.
      _bIsLegitimateAwakening = false;
    }

    EmitMessagesFromAll();
  }

  return 0;
}
