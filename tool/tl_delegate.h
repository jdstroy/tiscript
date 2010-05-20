#ifndef __tl_delegate_h
#define __tl_delegate_h

// delegate-functor a la boost::bind "self-contained" function objects
// borrowed from http://www.rsdn.ru/forum/src/2949738.1.aspx

/*
usage:

  class Foo
  {
  public:
      Foo(){}
      void Method(int p1, double &p2) { }
  };

  Foo foo;

  // delegate creation:
  
  handle<functor> pdlgt = delegate(foo, &Foo::Method, 2, 22.0);

  ....
  // Invocation in galaxy far far away:
  (*pdlgt)();

*/

#include "tl_basic.h"

namespace tool
{

  namespace delegate_impl
  {
      template <class P1, class P2, class P3, class P4>
      struct delegate_data_4
      {
          P1 p1;
          P2 p2;
          P3 p3;
          P4 p4;
          delegate_data_4(const P1 &p1, const P2 &p2, const P3 &p3, const P4 &p4) : p1(p1), p2(p2), p3(p3), p4(p4){}
          typedef void enable4;

          template<class P,class F> void run(P p, F f) { (p->*f)(p1,p2,p3,p4); }

      };
      template <class P1, class P2, class P3>
      struct delegate_data_3
      {
          P1 p1;
          P2 p2;
          P3 p3;
          delegate_data_3(const P1 &p1, const P2 &p2, const P3 &p3) : p1(p1), p2(p2), p3(p3){}
          typedef void enable3;
          template<class P,class F> void run(P p, F f) { (p->*f)(p1,p2,p3); }
      };
      template <class P1, class P2>
      struct delegate_data_2
      {
          P1 p1;
          P2 p2;
          delegate_data_2(const P1 &p1, const P2 &p2) : p1(p1), p2(p2){}
          typedef void enable2;
          template<typename P,typename F> void run(P p, F f) { (p->*f)(p1,p2); }
      };
      template <class P1>
      struct delegate_data_1
      {
          P1 p1;
          delegate_data_1(const P1 &p1) : p1(p1){}
          typedef void enable1;
          template<typename P,typename F> void run(P p, F f) { (p->*f)(p1); }
      };
      struct delegate_data_0
      {
          delegate_data_0() {}
          typedef void enable0;
          template<typename P,typename F> void run(P p, F f) { (p->*f)(); }
      };

      template <class T, class F, class D>
      class delegate_impl : public tool::functor
      {
          delegate_impl();
      public:
          delegate_impl(const T* pT, F f, const D &d) : m_pT(pT), m_f(f), m_d(d) { }
          virtual void operator()() { m_d.run(m_pT.ptr(),m_f); } 
      private:
          handle<T> m_pT;
          F         m_f;
          D         m_d;
      };
  }


  // note: class T:public resource {...}

#if !defined(_MSC_VER) || (_MSC_VER >= 1300)
  
  // for compilers that support partial template instantiation (all but no vc6)
  template <class T> struct unref           { typedef T type; };
  template <class T> struct unref<T&>       { typedef T type; };
  template <class T> struct unref<const T&> { typedef T type; };

  template < class R, class T >
  functor* delegate(T* t, R (T::*f)())
  {
      typedef delegate_impl::delegate_data_0 data_t;
      return new delegate_impl::delegate_impl<T, R (T::*)(), data_t>(t, f, data_t());
  }

  template < class R, class T, class P1 >
  functor* delegate(T* t, R (T::*f)(P1), typename unref<P1>::type const &p1)
  {
      typedef delegate_impl::delegate_data_1<typename unref<P1>::type> data_t;
      return new delegate_impl::delegate_impl<T, R (T::*)(P1), data_t>(t, f, data_t(p1));
  }

  template < class R, class T, class P1, class P2 >
  functor* delegate(T* t, R (T::*f)(P1, P2), typename unref<P1>::type const &p1, typename unref<P2>::type const &p2)
  {
      typedef delegate_impl::delegate_data_2<typename unref<P1>::type, typename unref<P2>::type> data_t;
      return new delegate_impl::delegate_impl<T, R (T::*)(P1, P2), data_t>(t, f, data_t(p1, p2));
  }

  template < class R, class T, class P1, class P2, class P3 >
  functor* delegate(T* t, R (T::*f)(P1, P2, P3), typename unref<P1>::type const &p1, typename unref<P2>::type const &p2, typename unref<P3>::type const &p3)
  {
      typedef delegate_impl::delegate_data_3<typename unref<P1>::type, typename unref<P2>::type, typename unref<P3>::type> data_t;
      return new delegate_impl::delegate_impl<T, R (T::*)(P1, P2, P3), data_t>(t, f, data_t(p1, p2, p3));
  }

  template < class R, class T, class P1, class P2, class P3, class P4 >
  functor* delegate(T* t, R (T::*f)(P1, P2, P3, P4), typename unref<P1>::type const &p1, typename unref<P2>::type const &p2, typename unref<P3>::type const &p3, typename unref<P4>::type const &p4)
  {
      typedef delegate_impl::delegate_data_4<typename unref<P1>::type, typename unref<P2>::type, typename unref<P3>::type, typename unref<P4>::type> data_t;
      return new delegate_impl::delegate_impl<T, R (T::*)(P1, P2, P3, P4), data_t>(t, f, data_t(p1, p2, p3, p4));
  }

#else // VC6, no partial instantiation

  template < class R, class T >
  functor* delegate(T* t, R (T::*f)())
  {
      typedef delegate_impl::delegate_data_0 data_t;
      return new delegate_impl::delegate_impl<T, R (T::*)(), data_t>(t, f, data_t());
  }

  template < class R, class T, class P1 >
  functor* delegate(T* t, R (T::*f)(P1), P1 p1)
  {
      typedef delegate_impl::delegate_data_1<P1> data_t;
      return new delegate_impl::delegate_impl<T, R (T::*)(P1), data_t>(t, f, data_t(p1));
  }

  template < class R, class T, class P1, class P2 >
  functor* delegate(T* t, R (T::*f)(P1, P2), P1 p1, P2 p2)
  {
      typedef delegate_impl::delegate_data_2<P1, P2> data_t;
      return new delegate_impl::delegate_impl<T, R (T::*)(P1, P2), data_t>(t, f, data_t(p1, p2));
  }

  template < class R, class T, class P1, class P2, class P3 >
  functor* delegate(T* t, R (T::*f)(P1, P2, P3), P1 p1, P2 p2, P3 p3)
  {
      typedef delegate_impl::delegate_data_3<P1, P2, P3> data_t;
      return new delegate_impl::delegate_impl<T, R (T::*)(P1, P2, P3), data_t>(t, f, data_t(p1, p2, p3));
  }

  template < class R, class T, class P1, class P2, class P3, class P4 >
  functor* delegate(T* t, R (T::*f)(P1, P2, P3, P4), P1 p1, P2 p2, P3 p3, P4 p4)
  {
      typedef delegate_impl::delegate_data_4<P1, P2, P3, P4> data_t;
      return new delegate_impl::delegate_impl<T, R (T::*)(P1, P2, P3, P4), data_t>(t, f, data_t(p1, p2, p3, p4));
  }

#endif 


}
#endif