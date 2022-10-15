/****************************************************
* tServer
*
* Provides a UDP server for LSEB clients
*/

#ifndef INC_Server_h
#define INC_Server_h

#include <string>
#include <list>
#include <deque>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <sys/time.h>
#include <boost/circular_buffer.hpp>
#include <boost/accumulators/accumulators.hpp>
#include <boost/accumulators/statistics/stats.hpp>
#include <boost/accumulators/statistics/density.hpp>
#include <boost/accumulators/statistics/count.hpp>
#include <boost/accumulators/statistics/median.hpp>
#include <boost/accumulators/statistics/mean.hpp>
#include <boost/accumulators/statistics/min.hpp>
#include <boost/accumulators/statistics/max.hpp>


#include "PThread.h"
#include "UdpConnection.h"

#define SAMPLES_PER_SECOND         (50)
#define SAMPLE_QUEUE_DEPTH_SECONDS ( 2)
#define STATS_PRINT_PERIOD_SECONDS ( 5)

// We allow the circular buffers to get about half full
#define SAMPLE_QUEUE_DRAIN_PERIOD_MS (SAMPLE_QUEUE_DEPTH_SECONDS*1000/2)



#define USE_BOOST_CIRCULAR_BUFFER
//#define DEFER_PRINTING_WHEN_USING_BOOST

#ifdef  USE_BOOST_CIRCULAR_BUFFER
#define SAMPLE_QUEUE_BUFFER_DEPTH (SAMPLES_PER_SECOND*SAMPLE_QUEUE_DEPTH_SECONDS)
#else
#define SAMPLE_QUEUE_BUFFER_DEPTH 
#endif

#define IP_ADDRESS_MAX_LEN_IN_CHARS (45)

#define ACCUMULATOR_NBINS      (50  )
#define ACCUMULATOR_MS_PER_BIN ( 0.2)
#define ACCUMULATOR_MAX_MS     (ACCUMULATOR_NBINS*ACCUMULATOR_MS_PER_BIN)

using namespace boost::accumulators;

class tSampleLogger;

typedef accumulator_set<double, stats<tag::count, tag::mean, tag::median(with_p_square_quantile), tag::density>> tSampleAccumulator;
typedef accumulator_set<double, stats<                       tag::median(with_p_square_quantile)              >> tMedianAccumulator;
typedef boost::iterator_range<std::vector<std::pair<double, double>>::iterator> tBoostHistogram;


typedef enum LATENCY_MEASUREMENT_TYPE { LM_TOTAL, LM_SERVER, LM_CLIENT, LM_NETWORK, LM_NUM_MEASUREMENTS } LATENCY_MEASUREMENT_TYPE;

/***************************
* tLatencySample
*/

struct tLatencySample {
  tLatencySample() {        _nRcvdByServer  = -1; }  // An uninitialized sample
  bool   IsValid() { return _nRcvdByServer != -1; }

  tLatencySample(int nRcvdByServer, int nSentByClient, struct timeval &tmRcv, struct timeval &tmSent, 
                 struct timeval &tmHwRcv_TAI, struct timeval &tmHwSentPrevious_TAI, struct sockaddr_in &ClientAddress) :
    _nRcvdByServer(nRcvdByServer), _nSentByClient(nSentByClient), _tmRcv(tmRcv), _tmSent(tmSent), 
    _tmHwRcv_TAI(tmHwRcv_TAI), _tmHwSentPrevious_TAI(tmHwSentPrevious_TAI), _ClientAddress(ClientAddress) {}


  int                _nRcvdByServer;
  int                _nSentByClient;
  struct timeval     _tmRcv;
  struct timeval     _tmSent;
  struct timeval     _tmHwRcv_TAI;
  struct timeval     _tmHwSentPrevious_TAI;
  struct sockaddr_in _ClientAddress;
};

/***************************
* tCorrectedStats
*
* Stats de-normalized and corrected for the fudge we had to do
*/

struct tCorrectedStats {
  tCorrectedStats() {}
  tCorrectedStats(const tSampleAccumulator &Acc, double dMin, double dMax);

  int    _iCount;
  double _dMin;
  double _dMax;
  double _dMean;
  double _dMedian;
  double _HistBins  [ACCUMULATOR_NBINS+1];
  int    _HistCounts[ACCUMULATOR_NBINS+1];
};


/***************************
* tCorrectedStatsSummer
*
* Adds up the stats from the  492 individual loggers
*/

struct tCorrectedStatsSummer {
  tCorrectedStatsSummer() {}
  tCorrectedStatsSummer(const double HistBins[ACCUMULATOR_NBINS+1]);

  void Accumulate(const tCorrectedStats &Stats);
  void Print();

  int                _iCount;
  double             _dSum;
  double             _dMin;
  double             _dMax;
  tMedianAccumulator _MedianAccumulator;

  double             _HistBins  [ACCUMULATOR_NBINS+1];
  int                _HistCounts[ACCUMULATOR_NBINS+1];
};


/***************************
* tSampleStats
*
* Accumulates states for a single SampleLogger
*/

class tSampleStats {
public:
  tSampleStats();

  tSampleStats(tSampleStats &&obj) noexcept;  // Move constructor - needed so that destruction of temporary does not close file.
  // tSampleLogger& operator=(tSampleLogger&& other); // Move assignment operator, will add if needed

  // Copy constructor and copy assignment operator are deleted - must only use move constructor
  tSampleStats(const tSampleStats &) = delete;   
  tSampleStats& operator=(const tSampleStats &) = delete;

  // This method won't work until you've called ComputeCorrectedStats the first time
  const double *HistBins() { return _CorrectedStats[LM_TOTAL]._HistBins; }

  void AccumulateSample(const double dLatencyMicroseconds[LM_NUM_MEASUREMENTS], bool bMismatch, bool bDropped);

  void ComputeCorrectedStats();
  const tCorrectedStats &CorrectedStats(LATENCY_MEASUREMENT_TYPE lmType) { return _CorrectedStats[lmType]; }

  void PrintStats();

protected:
  std::mutex         _AccumulatorMutex;
  tSampleAccumulator _AccLatency[LM_NUM_MEASUREMENTS];
  double             _dLatencyMin[LM_NUM_MEASUREMENTS];
  double             _dLatencyMax[LM_NUM_MEASUREMENTS];
  tCorrectedStats    _CorrectedStats[LM_NUM_MEASUREMENTS];
  int                _nMismatches;
  int                _nDropped;

};


/***************************
* tSamplePrinter - A thread that continuously drains the _SampleQueue of each tSampleLogger
*
* This thread runs at user priority, while the server threads run at realtime priority.
*/

class tSamplePrinter {
public:
  tSamplePrinter();

  void AddLogger   (tSampleLogger *pLogger);
  void RemoveLogger(tSampleLogger *pLoggerToRemove);

  long int TAIOffset();
  struct timeval TAI_to_UTC(const struct timeval &tv_TAI);

  void AccumulateStats();
  void PrintAccumulatedStats();

protected:
  void SamplePrintingEndlessLoop();

  std::thread                _PrintingThread;
  std::list<tSampleLogger *> _SampleLoggerList;

  struct timeval             _tvTaiOffset;
};


/***************************
* tSampleLogger - A buffer for logging arriving samples and relaying them for printing
*/

class tSampleLogger {
public:
  friend class tSamplePrinter;

  tSampleLogger(int iPortNum);

  tSampleLogger(tSampleLogger &&obj) noexcept;  // Move constructor - needed so that destruction of temporary does not close file.
  // tSampleLogger& operator=(tSampleLogger&& other); // Move assignment operator, will add if needed

  // Copy constructor and copy assignment operator are deleted - must only use move constructor
  tSampleLogger(const tSampleLogger &) = delete;   
  tSampleLogger& operator=(const tSampleLogger &) = delete;
  
  ~tSampleLogger();

  void DrainSampleQueue();
  void ComputeCorrectedStats() { _Stats.ComputeCorrectedStats(); }

  void ProcessSample(const tLatencySample &Sample);
  void PrintSample(const struct timeval &tmSent,
                   const struct timeval &tmReceived, const struct timeval &tmDiff,
                   const struct timeval &tmHwReceived, const struct timeval &tmRcvKernelLatency,
                   const struct timeval &tmHwSent, const struct timeval &tmSendKernelLatency,
                   int nReceivedByServer, int nSentByClient);

void LogSample(int nRcvdByServer, int nSentByClient, struct timeval &tmRcv, struct timeval &tmSent,
                 struct timeval &tmHwRcv_TAI, struct timeval &tmHwSentPrevious_TAI, struct sockaddr_in &ClientAddress);

protected:
  #ifdef  USE_BOOST_CIRCULAR_BUFFER
    boost::circular_buffer<tLatencySample> _SampleQueue;
  #else
    std::deque<tLatencySample> _SampleQueue;
  #endif

  // Mutex to allow queue-draining method to coordinate with realtime thread
  std::mutex              _SampleQueueMutex;

  tLatencySample          _PreviousSample;
  char                    _sSourceIpString[IP_ADDRESS_MAX_LEN_IN_CHARS];

  tSampleStats            _Stats;
  static tSamplePrinter   _SamplePrinter;

  int _iPortNum;
};


/***************************
* tServer - A thread that hosts a tUdpServer to listen for messages
*/

class tServer : public tPThread {
friend class tServerList;
public:
  tServer(int iPortNum, int iReceiveThreadPriority = 0);

  tServer(tServer &&obj) noexcept;  // Move constructor - needed so that destruction of temporary does not close file.
  // tHostConnection& operator=(tHostConnection&& other); // Move assignment operator, will add if needed

  // Copy constructor and copy assignment operator are deleted - must only use move constructor
  tServer(const tServer &) = delete;   
  tServer& operator=(const tServer &) = delete;

  ~tServer();

  int ProcessIncomingMessages();

protected:
  virtual void *_Thread();
  int           _iPortNum;
  tUdpServer    _UdpServer;
  bool          _bDebug;
  tSampleLogger _SampleLogger;
  int           _nReceived;
};


/***************************
* tServerList - Container for all the Server thread objects
*/

class tServerList {
public:
  tServerList(int iFirstPortNum, int iLastPortNum, int iReceiveThreadPriority);
  int AddServer(int iPortNum, int iReceiveThreadPriority);

  bool IsEmpty() { return _ServerList.empty(); }

  int ProcessTelemetry();

protected:
  std::list<tServer> _ServerList;
  bool _bExit;
};


#endif  // INC_Server_h
