//|
//|
//| Copyright (c) 2001-2005
//| Andrew Fedoniouk - andrew@terrainformatica.com
//|
//| mutex, event, critical_section primitives
//|
//|


#ifndef __tl_sync_h__
#define __tl_sync_h__

#include "tl_basic.h"

#ifndef _WIN32_WCE
  //#include <process.h>
#else
  //#include <windows.h>
  //#define volatile 
#endif

#include "tl_array.h"

namespace tool 
{

#ifdef WIN32
class mutex { 
    CRITICAL_SECTION cs;
 public:
    void lock() { 
	    EnterCriticalSection(&cs);
    } 
    void unlock() {
	    LeaveCriticalSection(&cs);
    } 
    mutex() { 
	    InitializeCriticalSection(&cs);
    }   
    ~mutex() { 
	    DeleteCriticalSection(&cs);
    }
};

struct event 
{ 
    HANDLE h;

    event() { 
	    h = CreateEvent(NULL, FALSE, FALSE, NULL);
    }
    ~event() { 
	    CloseHandle(h);
    }
    void signal() { 
	    SetEvent(h);
    }
    void pulse() { 
	    PulseEvent(h);
    }

    void wait(mutex& m) { 
	    m.unlock();
	    WaitForSingleObject(h, INFINITE);
	    m.lock();
    }
    void wait() { 
	    WaitForSingleObject(h, INFINITE);
    }
    
};

#else

class mutex { 
    int             count;
    pthread_t       owner;
    pthread_mutex_t cs;

    friend class event_t;
 public:
    void lock() { 
	pthread_t self = pthread_self();
	if (owner != self) { 
	    pthread_mutex_lock(&cs); 
	    owner = self;
	}
	count += 1;
    }
    void unlock() { 
	assert(pthread_self() == owner);
	if (--count == 0) {
	    owner = 0;
	    pthread_mutex_unlock(&cs);
	} 
    }
    mutex() { 
	pthread_mutex_init(&cs, NULL);
    }   
    ~mutex() { 
	pthread_mutex_destroy(&cs);
    }
};

class event { 
    pthread_cond_t cond;

   public:
    event() { 
	pthread_cond_init(&cond, NULL);
    }
    ~event() { 
	pthread_cond_destroy(&cond);
    }
    void signal() { 
	pthread_cond_signal(&cond);
    }
    void wait(mutex_t& m) { 
	pthread_t self = pthread_self();
	assert(m.owner == self && m.count == 1);
	m.count = 0;
	m.owner = 0;
	pthread_cond_wait(&cond, &m.cs);
	m.count = 1;
	m.owner = self;
    }
};

#endif

class critical_section { 
    mutex& _m;
  public:
    critical_section(mutex& m) : _m(m) {
	    _m.lock();
    }
    ~critical_section() { 
	    _m.unlock();
    }
};

struct task 
{
  task(){}
  virtual ~task(){}
  virtual void exec() = 0;
  
};

class thread_pool
{
private:
    array<HANDLE>   thread_handles;
    array<task*>    tasks;
    mutex           guard;
    event           got_something;
    locked::counter terminate;
    locked::counter active;
    
public:
    thread_pool(int n_pool_size = 5):
        terminate(0),active(0)
    {
        unsigned long threadID;
        for(int i = 0; i < n_pool_size; i++)
        {
            //thread_handles.push((HANDLE)_beginthreadex(NULL, 0, thread, this, 0, &threadID));
            thread_handles.push(CreateThread(NULL, 0, thread, this, 0, &threadID)); 
            locked::inc(active);    
        }
    }
    
    ~thread_pool()
    {
        foreach(n, thread_handles) CloseHandle(thread_handles[n]);
    }

    void add_task(task *t)
    {
      {
        critical_section cs(guard);
        tasks.push(t);
      }
      got_something.signal();
    }

    bool terminating()
    {
        return terminate != 0;
    }

    void start()
    {
    }

    void stop()
    {
       locked::set(terminate, 1);
       int attempts = thread_handles.size() * 2;
       while(active > 0 && attempts > 0)
       {
         got_something.signal();
         ::Sleep(0);
         --attempts;
       }
       
    }

    int tasks_waiting() { 
      critical_section cs(guard);
      return tasks.size(); 
    }

protected:

    task* next_task()
    {
      critical_section cs(guard);
      // check if we already got something...
      if(tasks.size()) return tasks.remove(0);
        // no luck, wait
      got_something.wait(guard);
      // return it
      if(tasks.size()) return tasks.remove(0);
      // nothing to execute, terminating?
      return 0;
    }

    static DWORD WINAPI thread(LPVOID pParam)
    {
        thread_pool* pthis = static_cast<thread_pool*>(pParam);
        while(!pthis->terminating())
        {
            task *t = pthis->next_task();
            if(t)
            {
               t->exec();
               delete t;
            }
        }
        locked::dec(pthis->active);
        return 0;
    }
};

class timed
{
protected:
    unsigned int  elapse;			// "Sleep time" in milliseconds
    bool          is_active;
    mutex         lock;  // thread synchronization
    
public:
  timed():is_active(false),elapse(10) {}
  virtual ~timed(){}
    
    void start (unsigned int period)
    {
      critical_section cs(lock);
      // is it already active?
      if (is_active)
          return;

      // Start the thread
      DWORD threadId;    
      HANDLE threadHandle = CreateThread (NULL, 0x10000, thread_f, this, 0, &threadId);    
      //SetThreadPriority(threadHandle,THREAD_PRIORITY_TIME_CRITICAL); // this is optional
      is_active = true;
    }

    void stop()
    {
      critical_section cs(lock);
      is_active = false;
    }

    bool is_ticking() { return is_active; } 

    void set_delay(unsigned int ms) { elapse = ms; }

    static DWORD WINAPI thread_f(LPVOID param) // thread entry point
    {
      timed* to = (timed*) param;
      bool is_active = true;
      do
      {
        ::Sleep(to->elapse);
        to->on_tick();
        {
          critical_section cs(to->lock);
          is_active = to->is_active;
        }
      } while (is_active);
      return 0;
    }

    virtual void on_tick() = 0;

};





}




#endif
