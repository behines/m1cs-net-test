/* tPThread - Object interface to PThreads
* 
* This class somewhat duplicates the functionality of std::thread, but
* it adds capabilities not present in std::thread, like pthread_timedjoin_np,
* the ability to set stack size, and the ability to set priorities.
*
* We could shoehorn these capabilities into a class derived from std::thread,
* by calling std::thread::native_handle() and then working with that handle,
* but it starts to become unwieldy and a bit pointless.  So we just spin
* this class from scratch.
*
* This class is not intended to be instantiated directly - indeed, it is a 
* pure virtual class.  Classes that want to have a thread should derive from 
* it and overload the _Thread() virtual method with the code that you wish
* to have spawned as a thread.
*
* The class does not implement copy constructors or copy assignment, since it
* is dealing with underlying OS objects.  Move constructors and assignment
* operators are provided instead.
*
***
*
* Thirty Meter Telescope Project
* Lower Segment Electronics Box Software
*
* Copyright 2022 California Institute of Technology
* All rights reserved
*
* Author:
* Brad Hines
* Planet A Energy
* Brad.Hines@PlanetA.Energy
*
*/

#include "PThread.h"

#include <cstdio>
#include <cstdlib>
#include <fcntl.h>
#include <unistd.h>
#include <cstring>
#include <errno.h>   // For EBUSY
#include <iostream>
#include <ctime>
#include <cmath>
#include <time.h>

using namespace std;


/*******************************************************
* tPThread static member init
*
* A word on the ThreadCreationMutex here.  It is guarding a critical section, a 
* section that actually spans two threads.  When a new thread is created and the
* this pointer is passed to the PThreadHelper, it can get confusing if another
* thread is spawned while all that is in process.  So we lock a mutex in the
* StartThread() method, and we don't release it until the this pointer is
* accessed down inside the actual _Thread().
*/

bool       tPThread::_bAllHaveBeenStartedWithRequestedAttributes = true;
std::mutex tPThread::_ThreadCreationMutex;


/*******************************************************
* tPThread constructor - Constructs the trhead object with a few defaults, but does not start the thread
*                        
* Legal priorities are 1 (lowest) to 99 (highest).  To actually start the thread, you 
* must follow up with a call to StartThread().
* 
* INPUTS:
*   iPriority - priority for the thread.  Numerically larger values are higher priority
*   bForceKillOnStopRequest - if false, when StopThread() is called, _Thread() will be
*               requested to exit by setting _bExit to true.  If true, the thread will
*               be pthread_canceled.  Note that any thread that can be subject to 
*               canceling at any time must use RAII acquisition of all resources, so
*               that stack unwinding will result in freeing of all resources.
*                  In so many words, don't set this to true unles syou know what
*               you're doing.
* SIDE EFFECTS:
*   None.
*/

tPThread::tPThread(int iPriority, bool bForceKillOnStopRequest) :
  _iPriority(iPriority),
  _bForceKillOnStopRequest(bForceKillOnStopRequest)
{
  _szStackSize = THREAD_DEFAULT_STACK_SIZE;
  _ThePthread  = 0;
  _bExit       = false;
  _bWasStartedWithRequestedAttributes = false;

  //_pThis       = new (volatile tPThread *); 
  // We could throw an error if the previous line failed, but there's no need - 
  // this line here will throw if _pThis is NULL.  No point to kicking a dead horse.
  //*_pThis      = this;
}


/*******************************************************
* tPThread move constructor
*
* INPUTS:
*   The object whose contents are being moved to this one.           
* SIDE EFFECTS:
*   The old object is marked as a zombie, by setting its _ThePThread member to 0.
*   If you try to call this once StartThread() has been called, it will throw an error.
*     This prevents the "this" pointer from changing underneath a running thread.
*/

tPThread::tPThread(tPThread &&other) :
  _ThePthread      (0)
  // _pThis     (nullptr)
{
  // Prevent any moves of content once the thread has started.  Else the this pointer
  // could move out from under the thread.
  if (!IsZombieObject() && IsRunning()) {
    throw std::runtime_error("tPThread::move contructor: Attempt to move thread after spawning");
  }

  // Copy values from the other thread 
  _iPriority                           = other._iPriority;
  _szStackSize                         = other._szStackSize;
  _ThePthread                          = other._ThePthread;
  _bExit                               = other._bExit;
  _bForceKillOnStopRequest             = other._bForceKillOnStopRequest;
  _bWasStartedWithRequestedAttributes  = other._bWasStartedWithRequestedAttributes;
  // _pThis                               = other._pThis;

  // (*_pThis) = this;   // Tells PThreadHelper to use us now instead of "other".

  // Now mark the other tPThread as a zombie
  other._ThePthread = 0;
  // other._pThis      = nullptr;  

  // cout << "tPThread Move Constructor for " << (unsigned int) _ThePthread << endl;
}


/*******************************************************
* tPThread move assignment operator
*
* INPUTS:
*   The object whose contents are being assigned to this one.           
* SIDE EFFECTS:
*   The old object is marked as a zombie, by setting its _ThePThread member to 0.
*   If you try to call this once StartThread() has been called, it will throw an error.
*     This prevents the "this" pointer from changing underneath a running thread.
*/

tPThread& tPThread::operator=(tPThread&& other)
{
  // Prevent any moves of content once the thread has started.  Else the this pointer
  // could move out from under the thread.
  if (!IsZombieObject() && IsRunning()) {
    throw std::runtime_error("tPThread::move contructor: Attempt to move thread after spawning");
  }

  if (this != &other) {
    // Copy values from the other thread 
    _iPriority                           = other._iPriority;
    _szStackSize                         = other._szStackSize;
    _ThePthread                          = other._ThePthread;
    _bExit                               = other._bExit;
    _bForceKillOnStopRequest             = other._bForceKillOnStopRequest;
    _bWasStartedWithRequestedAttributes  = other._bWasStartedWithRequestedAttributes;
    // _pThis                               = other._pThis;

    // (*_pThis) = this;   // Tells PThreadHelper to use us now instead of "other".

    // Now mark the other tPThread as a zombie
    other._ThePthread = 0;
    // other._pThis      = nullptr;
  }

  // cout << "tPThread Move assignment for " << (unsigned int) _ThePthread << endl;

  return *this;
}



/*******************************************************
* tPThread::PThreadHelper - C linkage to PThread class
*
* A level of indirection is required here, in order to handle copy or move
* constructors.  The reason why is to mitigate a race condition.
*
* When the OS thread spawns this function, it is, in principle, handed a pointer 
* to a tPThread.  But if the OS thread is handed off to a different tPThread from 
* the one that has spawned the OS thread (via move construction or move assignment)
* between the time StartThread() runs and the time this helper function is actually
* called, then the pointer will become invalid when the thread is destroyed.
*
* So we need a way to tell the Linux thread "this is the correct tPThread to refer
* to henceforth."
*
* To deal with this, when we spawn the thread, rather than giving it the "this" 
* pointer for the tPThread, we give it the address of a location where the "this"
* pointer will be stored.
*
* Then when a new object comes along, it stuffs its "this" pointer into that location.
* 
*
* A fully general solution would keep a list of all the current tPThreads that refer
* to this thread, and just call one of them, and then remove dying tPThreads from that
* list.  That's too much monkeying around to address something that's never going to
* happen.  This thing only happens during copying of a temporary to a new variable,
* so we always assume that the newly copy-constructed tPThread will be taking over, 
* and we assign it as the new "this" pointer in the copy constructor.
*
* INPUTS:
*
* SIDE EFFECTS:
*
*/

void *PThreadHelper(void *pPThread) {
  // tPThread *pThread = * reinterpret_cast<tPThread **> (ppPThread);
   tPThread *pThread = reinterpret_cast<tPThread *> (pPThread);

  tPThread::_ThreadCreationMutex.unlock();

  return  pThread->_Thread();
}


/*******************************************************
* tPThread::StartThread
*
* INPUTS:
*
* SIDE EFFECTS:
*
*/

pthread_t tPThread::StartThread()
{
  pthread_attr_t attr;
  struct sched_param param;
  int retval = 0;

  // Protection against starting more than one thread at a time
  std::scoped_lock lock(_ThreadCreationMutex);

  /***
  * Prepare the thread attributes object
  *   1. SCHED_FIFO is a realtime scheduling policy.  RT threads are always higher 
  *      priority than normal threads (per 'man 7 sched')
  *   2. Within that, priorities are 1..99, with 99 being highest.
  *   3. Then we have to setinheritsched to PTHREAD_EXPLICIT_SCHED.  This tells
  *      pthread_create that it should actually use (rather than ignore) attr.
  * 
  * If the user specifies priority 0, that is the (sole) non-realtime priority.
  * In that case, we skip all the steps above and just launch a vanilla thread.
  * 
  * the code below also resorts to a standard priority-zero thread if it can't
  * get sufficient privilege.
  */    

  if (_iPriority > 0) {
    /*** Realtime thread ***/
    pthread_attr_init(&attr);

    if (pthread_attr_setstacksize(&attr, _szStackSize) != 0) {
      fprintf(stderr, "   Thread stack size set to %lu failed: %s.\n", _szStackSize, strerror(retval));
      throw std::runtime_error("Could not set stack size");
    }

    retval = pthread_attr_setschedpolicy(&attr, SCHED_FIFO);
    if (retval != 0) {
      fprintf(stderr, "tPThread pthread_attr_setschedpolicy failed: %s\n", strerror(retval));
      return _ThePthread;
    }

    param.sched_priority = _iPriority;

    retval = pthread_attr_setschedparam(&attr, &param);
    if (retval != 0) {
      fprintf(stderr, "tPThread pthread setschedparam failed: %s\n", strerror(retval));
      return _ThePthread;
    }

    retval = pthread_attr_setinheritsched(&attr, PTHREAD_EXPLICIT_SCHED);
    if (retval != 0) {
      fprintf(stderr, "tPThread: Could not set Explicit sched policy: %s\n", strerror(retval));
      return _ThePthread;
    }

    /***
    * And now create the thread.  Instead of passing in the this pointer as the arg,
    * we pass in a pointer to the this pointer.  See the comments for PThreadHelper 
    * for the explanation as to why.
    */
    retval = pthread_create(&_ThePthread, &attr, PThreadHelper, this); // _pThis);
    pthread_attr_destroy(&attr);
    
    if (retval == 0)  {
      /*** Realtime thread successfully created, go ahead and exit ***/
      _bWasStartedWithRequestedAttributes = true;
      return _ThePthread;
    }

    // If we got here, we failed
    _bWasStartedWithRequestedAttributes = false;
    _bAllHaveBeenStartedWithRequestedAttributes = false;
  }

  /*** Create a standard (non-realtime) thread ***/
  pthread_attr_init(&attr);
  if (pthread_attr_setstacksize(&attr, _szStackSize) != 0) {
    fprintf(stderr, "   Thread stack size set to %lu failed: %s.\n", _szStackSize, strerror(retval));
    throw std::runtime_error("Could not set stack size");
  }
  retval = pthread_create(&_ThePthread, &attr, PThreadHelper, this); // _pThis);
  // Use NULL to get the standard stack size
  // retval = pthread_create(&_ThePthread, NULL, PThreadHelper, _pThis);
  if (retval == 0)  { 
    // Success
    if (_iPriority <= 0) {
      // We wanted a standard thread, and that's what was created
      _bWasStartedWithRequestedAttributes = true;
    }
  } // fprintf(stderr, "   ...Success!\n");
  else {
    fprintf(stderr, "   Thread creation failed: %s.\n", strerror(retval));
    _bAllHaveBeenStartedWithRequestedAttributes = false;
  }
  pthread_attr_destroy(&attr);

  // Grab the mutex again.  This will cause us to get stuck here, until
  // PThreadHelper signals that it is underway and has decoded the this
  // pointer
  _ThreadCreationMutex.lock();

  //cout << "Started thread " << (unsigned int) _ThePthread 
  //     << ", this=" << (void *) this << endl;

  return _ThePthread;
}


/*******************************************************
* tPThread::StopThread
*
* Terminates the thread.  The base class provides a brute-force, shoot-it-in-the-head
* kill.  Derived classes can overload this method to provide a more natural death.
*
* INPUTS:
*   bWaitForExit - If true, this method will not return until the thread has terminated.
* SIDE EFFECTS:
*   If _bForceKillOnStopRequest is true, the thread will be canceled.  Any thread created
*     this way must ensure that all its resources are allocated using RAII semantics.  This
*     will ensure that all resources are freed when its stack is unwound upon terminmation.
*   If it is false, the thread will be requested to exit by setting _bExit to true.  
*/

void tPThread::StopThread(bool bWaitForExit)
{
  _bExit = true;  // Can do this regardless

  if (!IsZombieObject()) {
    if (_bForceKillOnStopRequest && _ThePthread != 0)  {
      pthread_cancel(_ThePthread);
    }

    if (bWaitForExit)  WaitForExit();
  }
}


/*******************************************************
* tPThread::WaitForExit
*
* INPUTS:
*   None
* RETURNS:
*   The return value from the thread.
* SIDE EFFECTS:
*   Will block until the thread has exited.
*   Will zero _ThePthread once the thread has exited
*/

void *tPThread::WaitForExit()
{
  void *pRetVal = NULL;

  if (IsZombieObject())  throw std::runtime_error("Zombie object in tPThread::WaitForExit");

  if (_ThePthread != 0) {
    pthread_join(_ThePthread, &pRetVal);
  }
  _ThePthread = 0;

  return pRetVal;
}


/*******************************************************
* tPThread::WaitForExitWithTimeout
*
* INPUTS:
*   fTimeoutInSeconds - How long to wait for ht thread to exit before returning
*   ppResult          - the address of a void * into which to place the return value
*                       of the thread.
* RETURNS:
*   true if successful, false if a timeout occurred
*   *ppResult will be updated with the return value from the thread
* SIDE EFFECTS:
*   Will zero _ThePthread if the thread has exited
*/

bool tPThread::WaitForExitWithTimeout(float fTimeoutInSeconds, void **ppResult)
{
  int iResult;
  struct timespec TimeoutTime;
  float fTimeoutRoundedDownToSeconds, fTimeoutSubSecond;

  if (IsZombieObject())  throw std::runtime_error("Zombie object in tPThread::WaitForExitWithTimeout");

  if (_ThePthread != 0) {
    clock_gettime(CLOCK_REALTIME, &TimeoutTime);
  
    fTimeoutRoundedDownToSeconds = floorf(fTimeoutInSeconds);
    fTimeoutSubSecond            = fTimeoutInSeconds - fTimeoutRoundedDownToSeconds;

    cout << "Timeout: " << fTimeoutRoundedDownToSeconds 
                 << ":" << fTimeoutSubSecond << endl;
    cout << "Waiting: Now = " << TimeoutTime.tv_sec <<":"<<TimeoutTime.tv_nsec << endl;
    // Compute the future time corresponding to a timeout.
    // Offset from current time.
    TimeoutTime.tv_sec  += (int) (fTimeoutRoundedDownToSeconds);
    TimeoutTime.tv_nsec += (int) (fTimeoutSubSecond * 1e9);

    cout << "   After add = " << TimeoutTime.tv_sec <<":"<<TimeoutTime.tv_nsec << endl;

    while (TimeoutTime.tv_nsec > 1e9) {
      TimeoutTime.tv_nsec -= 1e9;
      TimeoutTime.tv_sec  += 1;
    }
    
    cout << "    Deadline = " << TimeoutTime.tv_sec <<":"<<TimeoutTime.tv_nsec << endl;
    // Now try to wait on the thread
    iResult = pthread_timedjoin_np(_ThePthread, ppResult, &TimeoutTime);
    cout << "    iResult = " << iResult << endl;

  }

  // If we timed out, return false
  if (iResult != 0) return false;

  // We successfully joined, zero _ThePthread and return true
  _ThePthread = 0;

  return true;
}


/*******************************************************
* tPThread::IsRunning
*
* The slightly tricky thing here is that we can't just try _ThePthread==0.  It only gets
* set to zero at the end of WaitForExit.  If the thread exits naturally, with nobody 
* watching, only the OS will know if it has exited.  So we first check _ThePthread, and 
* then if it's non-zero, we also call pthread_tryjoin_np() to find out if the thread 
* is running.
*
* INPUTS:
*   None.
* RETURNS:
*   true if the thread is still running, false otherwise.
* SIDE EFFECTS:
*   Will throw an exception if the underlying OS thread has been handed over to a
*      different tPThread via a move constructor or move assignment.
*/

bool tPThread::IsRunning() const
{
  if (IsZombieObject())  throw std::runtime_error("Zombie object in tPThread::IsRunning");

  if (_ThePthread == 0) return false;

  // pthread_tryjoin_np will return 0 if the thread is terminated, or EBUSY if
  // still running.
  return (pthread_tryjoin_np(_ThePthread, NULL) == EBUSY);
}


/*******************************************************
* tPThread destructor
*
* Requests that the thread stop and waits for its exit.  Whether this is a 
* cooperative or forced termination depends on the value of 
* _bForceKillOnStopRequest
*
* SIDE EFFECTS:
*   Terminates the thread.
*/

tPThread::~tPThread()
{
  // Do nothing if we're a zombie object
  if (!IsZombieObject()) {
    // Now we need to make a couple of method calls.  But remember that our this pointer
    // may no longer be valid.  So call the methods via our own version of the this pointer.
    // Cannot call virtual method from destructor, must only call our own class's 
    // method.  Have to do it explicitly, else can seg fault.  
    //   That said, if called at non-destruction time, StopThread() is virtual.  And we'd
    // like it to be virtual here but it can't be.  Instead, the destructors up the chain,
    // which are virtual, can each call their own StopThread(), which will typically call
    // this base class StopThread() as well.  It is harmless for it to be called more than
    // once.
    // pthread_t StoppingThread = _ThePthread;
    //cout << "~tPThread: Stopping thread " << StoppingThread << endl;
    tPThread::StopThread();  

    /******* If Ctors and Dtors call a virtual method, the virtual-ness is ignored *******/
    WaitForExit();
    //cout << "~tPThread: Thread " << StoppingThread << " exited" << endl;

    // delete _pThis;
  }
}

