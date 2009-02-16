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

#ifdef WINDOWS
  #define WIN32_LEAN_AND_MEAN
  #include <windows.h>
#else
  #include <pthread.h>
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

inline void yield()
{
  ::Sleep(1); // 1 is a MUST here, seems like ::Sleep(0); is not enough on Vista
}

#else

class mutex {
    friend class event;
    int             count;
    pthread_t       owner;
    pthread_mutex_t cs;

    friend class event_t;
 public:
    inline void lock()
    {
        pthread_t _self = pthread_self();
        if (owner != _self) {
      pthread_mutex_lock(&cs);
            owner = _self;
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
    void wait(mutex& m) {
  pthread_t self = pthread_self();
  assert(m.owner == self && m.count == 1);
  m.count = 0;
  m.owner = 0;
  pthread_cond_wait(&cond, &m.cs);
  m.count = 1;
  m.owner = self;
    }
};

inline void yield()
{
  sched_yield();
}


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

class critical_section_cond { 
    mutex* _pm;
  public:
    critical_section_cond() : _pm(0) {}
    void lock(mutex& m) 
    { 
      _pm = &m; 
      if(_pm) _pm->lock(); 
    }
    void unlock() 
    { 
      if(_pm) { _pm->unlock(); _pm = 0; }
    }

    ~critical_section_cond() 
    { 
      if(_pm)
	      _pm->unlock();
    }
};


#ifdef WINDOWS

struct task
{
  struct task* next;

  task():next(0) {}
  virtual ~task(){}
  virtual void exec() = 0;
  virtual void stop() = 0;

};

class thread_pool
{
private:

#ifdef WINDOWS
    array<HANDLE>   thread_handles;
#else
    array<pthread_t>  thread_handles;
#endif
    task* first;
    task* first_running;

    mutex           guard;
    mutex           running_guard;
    event           got_something;
    locked::counter terminate;
    locked::counter active;

public:
    thread_pool(int n_pool_size = 5):
        terminate(0),active(0), first(0), first_running(0)
    {
        //unsigned long threadID;
        for(int i = 0; i < n_pool_size; i++)
        {
#ifdef WINDOWS
            thread_handles.push( CreateThread(0, 0, thread, (LPVOID)this, 0, 0));
#else
          thread_handles.push(_beginthread(thread, 0,this));
#endif
            //thread_handles.push(CreateThread(NULL, 0, thread, this, 0, &threadID));
          locked::inc(active);
        }
    }

    ~thread_pool()
    {
        stop();
#ifdef WINDOWS
        foreach(n, thread_handles)
          CloseHandle(thread_handles[n]);
#endif
    }

    void add_task(task *t)
    {
      {
        critical_section cs(guard);
        t->next = first;
        first = t;
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

       {
         critical_section cs(guard);
         while(first)
         {
           task* t = first;
           first = t->next;
           delete t;
         }
       }
       {
         critical_section cs(running_guard);
         for(task* tr = first_running; tr; tr = tr->next)
           tr->stop();
       }

       int attempts = thread_handles.size() * 2;
       while(active > 0/* && attempts > 0*/)
       {
         got_something.signal();
         yield();
         --attempts;
       }
    }

    int tasks_waiting() {
      critical_section cs(guard);
      return first != 0;
    }


protected:

    task* next_task()
    {
      task* t = 0;
      if(terminate)
        return 0;

      got_something.wait();

      bool has_next = false;

      {
        critical_section cs(guard);
        if(terminate)
      return 0;

        if(first)
        {
          t = first;
          first = first->next;
          if( first )
            has_next = true;
        }
    }

      if(has_next)
        got_something.signal();

      return t;
    }
#ifdef WINDOWS
    static DWORD WINAPI thread(LPVOID pParam)
#else
    static void thread(LPVOID pParam)
#endif

    {
        thread_pool* pthis = static_cast<thread_pool*>(pParam);
        while(!pthis->terminating())
        {
            task *t = pthis->next_task();
            if(!t)
              break;

            {
              critical_section cs(pthis->running_guard);
              t->next = pthis->first_running;
              pthis->first_running = t;
            }
               t->exec();
            {
              critical_section cs(pthis->running_guard);
              if(pthis->first_running == t)
                pthis->first_running = t->next;
              else
                for(task *r = pthis->first_running; r; r = r->next)
                  if( r->next == t )
                  {
                    r->next = t->next;
                    break;
                  }
            }
               delete t;
            }
        locked::dec(pthis->active);
#ifdef WINDOWS
        ExitThread(0);
        return 0;
#else
        _endthread();
#endif
    }
};

#endif


}


#endif
