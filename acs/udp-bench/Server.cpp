/****************************************************
* tServer
*
* Creates a UDP server
*/

#include "Server.h"
#include <errno.h>
#include <string.h>
#include <signal.h>
#include <iostream>
#include <iomanip>
#include <utility>
#include <sys/timex.h>

extern "C" {
  #include "GlcMsg.h"
  #include "GlcLscsIf.h"
}

#define MAX_MESSAGE_SIZE (sizeof(SegRtDataMsg))  

using namespace std;


/***************************************************
* Static member initialization
*/

tSamplePrinter tSampleLogger::_SamplePrinter;




/***************************************************
* tSampleStats constructor
*
* INPUTS:
*/

tSampleStats::tSampleStats() :
  _AccTotalLatency(tag::density::cache_size=2, tag::density::num_bins=ACCUMULATOR_NBINS)
{
  // This is a kludge to force the accumulators' bins to be where we want.  We
  // stuff in a pair of fake samples at the extremes we want.  This will mess up
  // the min, max, and mean.  We will accumulator our our min and max, and we
  // can clean up the mean if we know the count.
  _AccTotalLatency(0.0);   _AccTotalLatency(ACCUMULATOR_MAX_MS); 
  _dTotalLatencyMin =  999999999999999.0;
  _dTotalLatencyMax = -10000.0;

  _nMismatches = 0;
  _nDropped    = 0;
}


/***************************************************
* tSampleStats move constructor
*
* This is used during assignment of the temporary object to the list.  If we don't have a
* move constructor, then the destructor for the temporary object will close the file
* descriptor.
*
* INPUTS:
*    other - the contents of the object being moved
*/

tSampleStats::tSampleStats(tSampleStats &&other) noexcept :
  _AccTotalLatency      (other._AccTotalLatency),
  _dTotalLatencyMin     (other._dTotalLatencyMin),
  _dTotalLatencyMax     (other._dTotalLatencyMax),
  _nMismatches          (other._nMismatches),
  _nDropped             (other._nDropped)
{
}


/***************************************************
* tSampleStats::AccumulateSample
*
*
* INPUTS:
*    
*/

void tSampleStats::AccumulateSample(double dTotalLatencyMicroseconds, bool bMismatch, bool bDropped)
{
  _AccTotalLatency(dTotalLatencyMicroseconds/100);

  if (dTotalLatencyMicroseconds < _dTotalLatencyMin) _dTotalLatencyMin = dTotalLatencyMicroseconds;
  if (dTotalLatencyMicroseconds > _dTotalLatencyMax) _dTotalLatencyMax = dTotalLatencyMicroseconds;

  _nMismatches += bMismatch;
  _nDropped    += bDropped;
}


/***************************************************
* tSampleStats::PrintStats
*
*
* INPUTS:
*    
*/

void tSampleStats::PrintStats()
{
  int i;

  tCorrectedStats cStats(_AccTotalLatency, _dTotalLatencyMin, _dTotalLatencyMax);

  cout << std::fixed << setprecision(0);
  cout << "Count: " << cStats._iCount << " Min: " << cStats._dMin << " Max: " << cStats._dMax 
          << setprecision(2) << " Mean: " << cStats._dMean  << " Median: " << cStats._dMedian
          << " Mismatches: " << _nMismatches << " Dropped: " << _nDropped
          <<  endl;

  cout << std::fixed << setprecision(2);

  for (i=0; i<= ACCUMULATOR_NBINS; i++) {
    cout << cStats._HistBins[i] << " ";
  }
  cout << endl;
  for (i=0; i<= ACCUMULATOR_NBINS; i++) {
    cout << cStats._HistCounts[i] << " ";
  }
  cout << endl;
}




/***************************************************
* tCorrectedStats::tCorrectedStats
*
*
* INPUTS:
*    
*/

tCorrectedStats::tCorrectedStats(tSampleAccumulator &Acc, double dMin, double dMax) :
  _dMin(dMin),
  _dMax(dMax)
{
  std::size_t i;

  // Adjust the mean to remove the two samples we stuffed in at the beginning in order to set the bin size
  double dOriginalMean  =           mean(Acc);
  double iOriginalCount = extract::count(Acc);
  double dSum           = dOriginalMean * iOriginalCount - ACCUMULATOR_MAX_MS;  
  _iCount               = iOriginalCount - 2;

  if (_iCount > 0) {
    _dMean   = dSum / _iCount;
    _dMedian = median(Acc);

    tBoostHistogram Histogram = density(Acc);

    for (i=1; i<Histogram.size(); i++) {
      _HistBins[i-1]   = Histogram[i].first;
      _HistCounts[i-1] = iOriginalCount * Histogram[i].second;
    }
  }
}







/***************************************************
* tSampleLogger constructor
*
* INPUTS:
*    
*/

tSampleLogger::tSampleLogger(int iPortNum) :
  _SampleQueue(SAMPLE_QUEUE_BUFFER_DEPTH),
  _iPortNum(iPortNum)
{
  _SamplePrinter.AddLogger(this);
}




/***************************************************
* tSampleLogger move constructor
*
* This is used during assignment of the temporary object to the list.  If we don't have a
* move constructor, then the destructor for the temporary object will close the file
* descriptor.
*
* INPUTS:
*    other - the contents of the object being moved
*/

tSampleLogger::tSampleLogger(tSampleLogger &&other) noexcept :
  _SampleQueue(std::move(other._SampleQueue)),
  _iPortNum             (other._iPortNum)
{
  _SamplePrinter.RemoveLogger(&other);
  _SamplePrinter.AddLogger(this);
}



tSampleLogger::~tSampleLogger() 
{
  // _thread.join(); 
}


/***************************************************
* tSampleLogger::LogSample
*
* INPUTS:
*    
*/

void tSampleLogger::LogSample(int nRcvdByServer, int nSentByClient, struct timeval &tmRcv, struct timeval &tmSent, 
                              struct timeval &tmHwRcv_TAI, struct timeval &tmHwSentPrevious_TAI, struct sockaddr_in &ClientAddress)
{
  std::lock_guard<std::mutex> cvLock(_SampleQueueMutex);
  _SampleQueue.push_back(tLatencySample(nRcvdByServer, nSentByClient, tmRcv, tmSent, tmHwRcv_TAI, tmHwSentPrevious_TAI, ClientAddress));
  // When using boost, we can query how full the buffer is and wait until it is half full before printing anything, to reduce thread thrashing

  #if 0
    #if defined(USE_BOOST_CIRCULAR_BUFFER) && defined(DEFER_PRINTING_WHEN_USING_BOOST)
      if (_SampleQueue.size() > SAMPLE_QUEUE_BUFFER_DEPTH/2)  _SampleQueueCondition.notify_one();
    #else
      _SampleQueueCondition.notify_one();
    #endif
  #endif
}


/***************************************************
* tSampleLogger::DrainSampleQueue
*
* This method should be called from a non-realtime thread
*
* INPUTS:
*    
*/

void tSampleLogger::DrainSampleQueue()
{
  tLatencySample Sample;

  while (!_SampleQueue.empty()) {
    {
      std::scoped_lock cvLock(_SampleQueueMutex);
      Sample = _SampleQueue.front();
      _SampleQueue.pop_front();
    }

    ProcessSample(Sample);
  }
}


/***************************************************
* tSampleLogger::ProcessSample
*
* The slightly tricky thing here is that we don't get the timestamp
* when the packet was sent until the *next* packet arrives.  So what
* we do is implement a 1-packet delay in the output.
*
* To do that this routine has a one-sample memory.  It basically does
* all of its calculations using the data from the previous sample,
* *except* that for the sending hardware timestamp, it uses the
* value from the newly arrived sample.
*
* INPUTS:
*    
*/

void tSampleLogger::ProcessSample(const tLatencySample &Sample)
{
  struct timeval tmDiff, tmRcvKernelLatency, tmSendKernelLatency, tmHwRcvUTC, tmHwSentUTC;
  double dTotalLatencyMicroseconds, dRcvKernelLatencyMicroseconds, dSendKernelLatencyMicroseconds;
  bool   bMismatch, bDropped;

  // The first time through, we won't have a valid previous sample, so bail out until next time.
  if (_PreviousSample.IsValid()) {

    timersub(&_PreviousSample._tmRcv, &_PreviousSample._tmSent, &tmDiff);

    // The hardware timestamps are in TAI instead of UTC.  Convert.
    tmHwRcvUTC  = _SamplePrinter.TAI_to_UTC(_PreviousSample._tmHwRcv_TAI);
    tmHwSentUTC = _SamplePrinter.TAI_to_UTC(Sample._tmHwSentPrevious_TAI);

    timersub(&_PreviousSample._tmRcv, &tmHwRcvUTC,              &tmRcvKernelLatency);
    timersub(&tmHwSentUTC,            &_PreviousSample._tmSent, &tmSendKernelLatency);

    //PrintSample(_PreviousSample._tmSent, _PreviousSample._tmRcv, tmDiff, 
    //            tmHwRcvUTC, tmRcvKernelLatency, tmHwSentUTC, tmSendKernelLatency,
    //            _PreviousSample._nRcvdByServer, _PreviousSample._nSentByClient);

    dTotalLatencyMicroseconds      = tmDiff.tv_sec*1.0e6 + tmDiff.tv_usec;
    //dSRcvKernelLatencyMicroseconds = ;
    //dSendKernelLatencyMicroseconds = ;

    bMismatch = (_PreviousSample._nRcvdByServer != _PreviousSample._nSentByClient);
    bDropped  = (Sample._nSentByClient != _PreviousSample._nSentByClient + 1);

   _Stats.AccumulateSample(dTotalLatencyMicroseconds, bMismatch, bDropped);

  }
  else {
    // First time through, let's get the host IP as a string
    // Sample._ClientAddress is a sockaddr_in.  inet_ntop wants a struct in_addr, which is 
    // the sin_addr member of the sockaddr_in
    inet_ntop(AF_INET, &Sample._ClientAddress.sin_addr, _sSourceIpString, sizeof(_sSourceIpString));
  }

  _PreviousSample = Sample;
}


/***************************************************
* tSampleLogger::PrintSample
*
* INPUTS:
*    
*/

void tSampleLogger::PrintSample(const struct timeval &tmSent, 
                                const struct timeval &tmReceived, const struct timeval &tmDiff, 
                                const struct timeval &tmHwReceived, const struct timeval &tmRcvKernelLatency,
                                const struct timeval &tmHwSent, const struct timeval &tmSendKernelLatency,
                                int nReceivedByServer, int nSentByClient)
{
  #if 0
  (void) printf("%s::(%d): Sent: %02ld.%06ld  Rcvd: %02ld.%06ld  Lat: %02ld.%06ld  HwRcvd: %02ld.%06ld  RcvKLat: %02ld.%06ld  "
                          "HwSent: %02ld.%06ld  SndKLat: %02ld.%06ld  Nrcvd:%3d   NSent:%3d\n", 
                _sSourceIpString, _iPortNum,
                tmSent.tv_sec,              tmSent.tv_usec, 
                tmReceived .tv_sec,         tmReceived .tv_usec,
                tmDiff.tv_sec,              tmDiff.tv_usec, 
                tmHwReceived.tv_sec,        tmHwReceived.tv_usec, 
                tmRcvKernelLatency.tv_sec,  tmRcvKernelLatency.tv_usec,
                tmHwSent.tv_sec,            tmHwSent.tv_usec,
                tmSendKernelLatency.tv_sec, tmSendKernelLatency.tv_usec,
                ((nReceivedByServer-1)%50)+1,
                ((nSentByClient    -1)%50)+1);
  #endif
}


/***************************************************
* tSamplePrinter::tSamplePrinter
*
* INPUTS:
*/

tSamplePrinter::tSamplePrinter()
{
  struct ntptimeval ntv;
  ntp_gettime(&ntv);

  cout << "TAI offset = " << ntv.tai << " seconds" << endl;
  _tvTaiOffset = { ntv.tai, 0 };

  _PrintingThread = std::thread(&tSamplePrinter::SamplePrintingEndlessLoop, this);
}


/***************************************************
* tSamplePrinter::TAI_to_UTC - convert TAI time to UTC time
*
* INPUTS:
*/

struct timeval tSamplePrinter::TAI_to_UTC(const struct timeval &tv_TAI)
{
  struct timeval tv_UTC;

  timersub(&tv_TAI, &_tvTaiOffset, &tv_UTC);

  return tv_UTC;
}


/***************************************************
* tSamplePrinter::AddLogger - adds the logger to the list of loggers that we should print out
*
* INPUTS:
*/

void tSamplePrinter::AddLogger(tSampleLogger *pLogger)
{
  _SampleLoggerList.push_back(pLogger);
}


/***************************************************
* tSamplePrinter::RemoveLogger - deletes the specified logger from the list
*
* INPUTS:
*/

void tSamplePrinter::RemoveLogger(tSampleLogger *pLoggerToRemove)
{
  _SampleLoggerList.remove(pLoggerToRemove);
}


/***************************************************
* tSamplePrinter::AccumulateStats
*
* INPUTS:
*/

void tSamplePrinter::AccumulateStats()
{
  // Loop over all the loggers in our list
   for (auto & pLogger : _SampleLoggerList) {
    pLogger->DrainSampleQueue();
  }
}


/***************************************************
* tSamplePrinter::AccumulateStats
*
* INPUTS:
*/

void tSamplePrinter::PrintAccumulatedStats()
{
  AccumulateStats();

  tSampleLogger *pSampleLogger = _SampleLoggerList.front();

  pSampleLogger->_Stats.PrintStats();
}


/***************************************************
* tSamplePrinter::SamplePrintingEndlessLoop
*
* INPUTS:
*/

void tSamplePrinter::SamplePrintingEndlessLoop()
{
  int iLoopsPerPrintPeriod = (STATS_PRINT_PERIOD_SECONDS*1000 / SAMPLE_QUEUE_DRAIN_PERIOD_MS);
  int i;

  auto  DrainTime = chrono::system_clock::now();
  chrono::duration<int, std::milli> DrainIntervalInMs(SAMPLE_QUEUE_DRAIN_PERIOD_MS);  

  while (1) {


    for (i=0; i<iLoopsPerPrintPeriod; i++) {
      // We allow the queues to get about half full
      DrainTime += DrainIntervalInMs;
      std::this_thread::sleep_until(DrainTime);

      // Loop over all the loggers in our list
      for (auto & pLogger : _SampleLoggerList) {
        pLogger->DrainSampleQueue();
      }
    }

    PrintAccumulatedStats();
  }
}



/***************************************************
* tServer constructor
*
* INPUTS:
*/

tServer::tServer(int iPortNum, int iReceiveThreadPriority) :
  tPThread(iReceiveThreadPriority, true),
  _iPortNum(iPortNum),
  _UdpServer(iPortNum),
  _SampleLogger(iPortNum),
  _nReceived(0)
{
}


/***************************************************
* tServer move constructor
*
* This is used during assignment of the temporary object to the list.  If we don't have a
* move constructor, then the destructor for the temporary object will close the file
* descriptor.
*
* INPUTS:
*    other - the contents of the object being moved
*/

tServer::tServer(tServer &&other) noexcept :
  tPThread     (move(other)),
  _UdpServer   (move(other._UdpServer)),
  _SampleLogger(move(other._SampleLogger))
{
  _bDebug       = other._bDebug;
  _nReceived    = other._nReceived;
  _iPortNum     = other._iPortNum;
}

/***************************************************
* tServer destructor
*
*/

tServer::~tServer()
{
}

/***************************************************
* tServer::_Thread
*
*/

void *tServer::_Thread()
{
  string s = "Starting thread on port " + to_string(_iPortNum) + "\n";
  cout << s;

  ProcessIncomingMessages();

  return 0;
}

/*****************************
* tServer::ProcessIncomingMessage
*
*/

int tServer::ProcessIncomingMessages()
{
  ssize_t  len;;
  char buf[MAX_MESSAGE_SIZE];
  int            nSent;
  struct timeval tmRcv, tmSent, tmHwRcv_TAI, tmHwSentPrevious_TAI;
  struct sockaddr_in ClientAddress;

  while (!_bExit) {  // Flag from base tPThread class
    len = _UdpServer.ReceiveMessage(buf, sizeof(buf), &ClientAddress);

    if (len != MAX_MESSAGE_SIZE) {
      cerr << "Error: len = " << len << endl;
      throw(std::runtime_error("ERROR: Bad Received Message Size"));
    }

    gettimeofday(&tmRcv, NULL);

    // Unpack the payload
    const SegRtDataMsg *pMsg = (const SegRtDataMsg *) buf;

    tmSent                 =                        pMsg->hdr.time;
    nSent                  =                        pMsg->hdr.hdr.msgId;
    // Disable GCC warning about unaligned pointer for this pointer assignment
    #pragma GCC diagnostic push
    #pragma GCC diagnostic ignored "-Waddress-of-packed-member"
      tmHwSentPrevious_TAI = * (struct timeval *) &(pMsg->data[0]);
    #pragma GCC diagnostic pop

    // Retrieve the hardware timestamp
    tmHwRcv_TAI = _UdpServer.GetHardwareTimestampOfLastMessage();

    _SampleLogger.LogSample(++_nReceived, nSent, tmRcv, tmSent, tmHwRcv_TAI, tmHwSentPrevious_TAI, ClientAddress);
  }

  return 0;
}


/***************************************************
* tServerList constructor
*
* INPUTS:
*    
*/

tServerList::tServerList(int iFirstPortNum, int iLastPortNum, int iReceiveThreadPriority)
{
  int iPortNum;

  _bExit = false;

  for (iPortNum=iFirstPortNum; iPortNum<=iLastPortNum; iPortNum++) {
    AddServer(iPortNum, iReceiveThreadPriority);
  }
}

/***************************************************
* tServerList::AddConnection
*
* INPUTS:
*    
*/

int tServerList::AddServer(int iPortNum, int iReceiveThreadPriority)
{
  _ServerList.push_back(tServer(iPortNum, iReceiveThreadPriority));
  //_ServerList.back().StartSampleLoggerThread();

  return 0;
}

/***************************************************
* tServerList::ProcessTelemetryUsingThreads
*
* 
*    
*/

int tServerList::ProcessTelemetry()
{
  sigset_t  sigset;
  int       sig;

  for (auto & Server : _ServerList) {
    Server.StartThread();
  }

  if (!tPThread::HaveAllBeenStartedWithRequestedAttributes()) {
    cerr << "** Warning: Some threads not created with desired attributes **" << endl;
    cerr << "   You probably need to run as root." << endl;
  }

  // Wait for Ctrl-C
  /* Set up the mask of signals to temporarily block. */
  sigemptyset(&sigset);
  sigaddset(&sigset, SIGINT);

  /* Wait for a signal to arrive. */
  sigwait(&sigset, &sig);
  cout << "Ctrl-C, exiting..." << endl;

  for (auto & Server : _ServerList) {
    if (Server.IsRunning())  Server.StopThread(true);
  }

  return 0;
}
