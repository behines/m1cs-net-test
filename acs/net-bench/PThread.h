/* tPThread - Manage a Linux thread
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


#ifndef TPTHREAD_H_
#define TPTHREAD_H_

// This define allows us to pull in pthread_timedjoin_np from pthread.h
// See https://linux.die.net/man/3/pthread_timedjoin_np
//#define _GNU_SOURCE  // But we get a warning that it's already 
//                        defined.  Must be defined by gcc
#include <pthread.h>
#include <atomic>
#include <mutex>

// Default stack size for created threads.  Linux default is 8MB, which is a lot to
// hand out by default when there's only 512 MB of RAM.  We declare this here 
// rather than in LsebConfig.h, so that this class can live as a standalone library.
#define THREAD_DEFAULT_STACK_SIZE          (2000000)



// A C helper function to allow the OS to spawn tPThread::_Thread() as a thread.
void *PThreadHelper(void *pPThread);
//void *PThreadHelper(void *ppPThread);


/*******************************
* tPThread
*
* Methods for creating, monitoring, and killing threads.
*/

class tPThread {
friend void *PThreadHelper(void *ppPThread);
public:
  tPThread(int iPriority, bool bForceKillOnStopRequest = false);

  // No copy constructor or copy assigment - move only.  (std::thread does the same thing)
  tPThread(const tPThread &) = delete;
  tPThread& operator=(const tPThread &) = delete;

  tPThread(tPThread &&other);            // Move constructor
  tPThread& operator=(tPThread&& other); // Move assignment operator
  
  virtual ~tPThread();

  // If you want a non-default stack size, this must be called prior to StartThread
  void SetStackSize(size_t szStackSizeToUse) { _szStackSize = szStackSizeToUse; }

  pthread_t StartThread();
  virtual void StopThread(bool bWaitForExit = false);
  void *WaitForExit();
  bool WaitForExitWithTimeout(float fTimeoutInSeconds, void **ppResult);


  operator pthread_t () { return _ThePthread; }

  bool IsRunning() const;

  bool WasStartedWithRequestedAttributes() { return _bWasStartedWithRequestedAttributes; } 

  static bool HaveAllBeenStartedWithRequestedAttributes() {
    return _bAllHaveBeenStartedWithRequestedAttributes;
  }

  // A zombie object is one whose contents have been moved to another
  // tPThread, but the tPThread is not yet destroyed.
  bool IsZombieObject() const { return (_ThePthread == 0); }  //_pThis == nullptr); }

protected:
  virtual void *_Thread() = 0;
  
  int                 _iPriority;
  size_t              _szStackSize;
  pthread_t           _ThePthread;
  bool                _bExit;
  bool                _bForceKillOnStopRequest;
  bool                _bWasStartedWithRequestedAttributes;
  static bool         _bAllHaveBeenStartedWithRequestedAttributes;


  // TODO - I think this is unnecessary now that there are no copy constructors,
  // as long as a mutex guards the startup of the thread so that no move
  // constructor or move assignment can take place until the thread has started.
  // volatile tPThread **_pThis;          // A pointer to the this pointer

  // See comments at top of .cpp file for static members
  static std::mutex   _ThreadCreationMutex;
};

#endif /* TPTHREAD_H_ */
