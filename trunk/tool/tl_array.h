//|
//|
//| Copyright (c) 2001-2005
//| Andrew Fedoniouk - andrew@terrainformatica.com
//|
//| dynamic array
//|
//|

#ifndef __tl_array_h
#define __tl_array_h

#include <stdio.h>
#include "tl_basic.h"
#include "tl_slice.h"

namespace tool
{
// assumes element has a default constructor and operator=().

template <typename element>
class array
{
public:

  array ();
  array ( int sz );
  array ( int sz, const element &init_element );
  array ( const array<element> &the_array );
  array ( const slice<element>& d );

  ~array () { destroy(); }

  void  destroy();

  int       size () const;
  bool      is_empty () const;
  
  void      size ( int new_size );
  void      size ( int new_size, const element& init_value );
  void      clear ();
  
  array<element>& operator = ( const array<element> &the_array );
  array<element>& operator = ( const slice<element> &the_range );

  inline const element &operator[] ( int index ) const;
  inline element &operator[] ( int index );
  
  element   remove ( int index );
  void      remove ( int index, int length );
  int       insert ( int index, const element& elem );
  void      insert ( int index, const element* elems, int count );

  int       get_index(const element &e) const;

  bool      remove_by_value ( const element &el );

  bool      operator == (const array<element> &rs) const;
  bool      operator != (const array<element> &rs) const;
  bool      operator < (const array<element> &rs) const;
  bool      operator <= (const array<element> &rs) const;
  bool      operator > (const array<element> &rs) const;
  bool      operator >= (const array<element> &rs) const;

  int		    push ( const element& elem );
  void		  push ( const element* elems, int count );
  void		  push ( const slice<element>& elems );
  void      push ( const element& elem, int count );
  element   pop ();
  bool      pop (element& el);
  void      drop (int n);

  void      reverse();

  array<element> & operator+= ( const element &e )
  {
    push ( e );
    return *this;
  }
  element&        first ();
  const element&  first () const;
  element&        last  ();

  inline const element&  last  () const;
  inline int last_index () const
  {
    return size() - 1;
  }

  // will create elements buffer if it does not exist
  element* elements() 
  {  
    if(!_elements)
    {
      size(4); size(0); 
    }
    return _elements;
  }

  element* head() const { return _elements; }
  element* tail() const { return _elements + size(); }

  slice<element>  operator()() const { return slice<element>(head(),size()); }
  slice<element>  operator()(int s) const { return slice<element>(head()+s,size()-s); }
  slice<element>  operator()(int s, int e) const { return slice<element>(head()+s,e-s); }

  void set( int from, int to, const element& v )
  {
    int s = limit(from,0,size());
    int e = limit(to,0,size());
    for(;s < e; ++s ) (*this)[s] = v;
  }

  void set( int from, slice<element> by )
  {
    int s = limit(from,0,size());
    int e = limit(s + int(by.length),0,size());
    for(int i = 0; s < e; ++s ) (*this)[s] = by[i++];
  }

  void set_all_to ( const element &element_val );

  void swap( array<element> &the_array )
  {
    if(this == &the_array)
      return;
    swop(_elements,the_array._elements);
  }

  void transfer_from( array<element> &the_array )
  {
    destroy();
    _elements       = the_array._elements;
    the_array._elements = 0;
  }

  unsigned int hash() const 
  {
    unsigned int r = 0;
    element* p = head();
    element* end = tail();
    while( p < end ) r ^= tool::hash(*p++);
    return r;
  }

protected:
  /*int	 	 	_size;
    int 		_allocated_size; */

  element *	_elements;

  inline void set_size(int new_size)
  {
    assert( _elements );
    *(((int*)_elements) - 1) = new_size;
  }
  INLINE int get_size() const
  {
    assert( _elements );
    return *(((int*)_elements) - 1);
  }
  inline void set_allocated_size(int new_size)
  {
    assert( _elements );
    *(((int*)_elements) - 2) = new_size;
  }
  inline int allocated_size() const
  {
    if(_elements) return *(((int*)_elements) - 2);
    return 0;
  }

};

template <typename element>
inline
  array<element>::array ()
:_elements ( 0 )
{
  ;/* do nothing */
}

template <typename element>
inline
  array<element>::array ( int sz )
:_elements ( 0 )
{
  size ( sz );
}

template <typename element>
inline
  array<element>::array ( int sz, const element &init_element )
: _elements ( 0 )
{
  size ( sz, init_element );
}

template <typename element>
inline
  array<element>::array ( const slice<element>& d )
: _elements ( 0 )
{
  size ( d.length );
  copy( head(), d.start, d.length);
}

template <typename element>
inline
  array<element>::array ( const array<element> &the_array )
: _elements(0)
{
  operator= ( the_array );
}


template <typename element>
  inline void array<element>::destroy ()
{
  if(!_elements) return;
  erase(_elements,size());
  int* p = ((int*)_elements) - 2;
  free(p);
  _elements = 0;
}


template <typename element>
inline void
  array<element>::clear ()
{
  size ( 0 );
}

template <typename element>
inline
  array<element>& array<element>::operator= ( const array<element> &the_array )
{
  if ( this == &the_array )
    return *this;
  size ( the_array.size() );

  copy(_elements,the_array._elements,size());
  /*
  for ( int i = 0; i < size(); i++ )
    _elements [ i ] = the_array._elements [ i ];
  */
  return *this;
}

template <typename element>
inline
  array<element>& array<element>::operator= ( const slice<element> &the_range )
{
  size ( int(the_range.length) );
  copy(_elements,the_range.start,the_range.length);
  return *this;
}


template <typename element>
inline int
  array<element>::push ( const element& elem )
{
  assert( _elements == 0 || &elem < _elements || &elem >= (_elements + size()) );
  int tpos = size();
  size ( tpos + 1 );
  operator[] ( tpos ) = elem;
  return tpos;
}

template <typename element>
inline element
  array<element>::pop ()
{
  assert( size() > 0 );
  if( size() > 0 )
  {
    element e = _elements [ size() - 1 ] ;
    size ( size() - 1 );
    return e;
  }
  return element();
}

template <typename element>
inline bool
  array<element>::pop (element& el)
{
  assert( size() > 0 );
  if( size() > 0 )
  {
    el = _elements [ size() - 1 ];
    size ( size() - 1 );
    return true;
  }
  return false;
}


template <typename element>
inline void
  array<element>::drop (int n)
{
  assert( n >= 0 && n <= size() );
  n = min(n,size());
  size ( size() - n );
}

template <typename element>
inline element
  array<element>::remove ( int index )
{
  assert( index >= 0 && index < size() );
  if( !_elements )
    return element();
  element *dst = _elements + index;
  element r = *dst;
  //size--;
  set_size( size() - 1 );
  if(index < size())
    move(dst,dst+1, size_t(size() - index));
  erase(_elements + size(),1);
  //for ( int i = index; i < size(); i++, dst++ )
  //  *dst = * ( dst + 1 );
  return r;
}

template <typename element>
inline void
  array<element>::remove ( int index, int length )
{
  assert( index >= 0 && ( ( index + length ) <= size() ) );
  if( !_elements )
    return;
  element *dst = _elements + index;
  element *dste = dst;
  //for ( int i = 0; i < length; i++, dste++ )
  //  dste->~element();
  set_size( get_size() - length );
  if(index < size())
    move(dst,dst+length,size() - index);
  erase(_elements + size(), length);
  //for ( int i = index; i < size(); i++, dst++ )
  //  *dst = * ( dst + length );
}

template <typename element>
inline bool
  array<element>::remove_by_value ( const element& el )
{
  int idx = get_index(el);
  if( idx < 0 ) return false;
  (void)remove(idx);
  return true;
}

template <typename element>
inline int
  array<element>::insert ( int index, const element& elem )
{
  if ( index < 0 )
    index = 0;
  if ( index >= size() )
  {
    push ( elem );
    return size() - 1;
  }
  else
  {
    size ( size() + 1 );
    element *dst = _elements + size() - 1;

    move(_elements + index + 1,_elements + index,size() - index - 1);

    *( _elements + index ) = elem;
    return index;
  }
}

template <typename element>
inline void
  array<element>::insert ( int index, const element* elems, int count )
{
  assert(count > 0);
  if(count <= 0) return;
  assert( _elements == 0 || elems < _elements || elems >= (_elements + size()) );
  if ( index < 0 )
    index = 0;
  if ( index >= size() )
    push ( elems, count );
  else
  {
    int _old_size = size();
    size ( size() + count );
    element *dst = _elements + size() - 1;
    element *src = _elements + _old_size - 1;
    for ( int i = 0; i < _old_size - index; i++)
      *dst-- = *src--;
    //move<element>(_elements + index + count,_elements + index,size() - index - count);

    element *p = _elements + index;
    for ( int j = 0; j < count; j++)
      *p++ = *elems++;
    
  }
}

template <typename element>
inline element &
  array<element>::operator[] ( int index )
{
  assert( index >= 0 && index < size() );
  return _elements [ index ];
}

template <typename element>
inline const element &
  array<element>::operator[] ( int index ) const
{
  assert( index >= 0 && index < size() );
  return _elements [ index ];
}

template <typename element>
inline element &
  array<element>::last ()
{
  assert( size() > 0 );
  return _elements [ size() - 1 ];
}

template <typename element>
inline const element &
  array<element>::last () const
{
  assert( size() > 0 );
  return _elements [ size() - 1 ];
}

template <typename element>
inline element &
  array<element>::first ()
{
  assert( size() > 0 );
  return _elements [ 0 ];
}

template <typename element>
inline const element &
  array<element>::first () const
{
  assert( size() > 0 );
  return _elements [ 0 ];
}


template <typename element>
INLINE int
  array<element>::size () const
{
  return _elements? get_size(): 0;
}

template <typename element>
inline bool
  array<element>::is_empty () const
{
  return ( size() == 0 );
}


template <typename element>
inline void
  array<element>::size ( int new_size )
{
  assert(new_size >= 0);
  new_size = max(0,new_size);
  int old_size = size();
  
  if ( old_size == new_size )
    return;
  
  if ( new_size > old_size )
  {
    int allocated = allocated_size();
    if ( new_size > allocated )
    {
      int toallocate = allocated? 
          ((allocated * 3) / 2) : 
          max(4,new_size);

      if( toallocate < new_size ) toallocate = new_size;

      int *d = (int *)malloc(sizeof(int) * 2 + toallocate * sizeof(element));

      element *new_space = (element *) ( d + 2);
      
      init(new_space,new_size);

      if ( _elements )
      {
        copy(new_space, _elements, old_size < new_size? old_size: new_size );
        erase(_elements,old_size);
        free(((int *)_elements) - 2);
      }
      _elements = new_space;

      set_size(new_size);
      set_allocated_size(toallocate);

    }
    else //if ( new_size <= allocated ), simply init the tail
      init(_elements + old_size, new_size - old_size);
  } 
  else if( _elements ) // and yet new_size < old_size 
    erase(_elements + new_size, old_size - new_size);

  set_size(new_size);
}

/* Something wrong with realloc here:
template <typename element>
inline void
  array<element>::size ( int new_size )
{
  assert(new_size >= 0);
  new_size = max(0,new_size);
  int old_size = size();
  
  if ( old_size == new_size )
    return;
  
  if ( new_size > old_size )
  {
    int allocated = allocated_size();
    if ( new_size > allocated )
    {
      int toallocate = allocated? 
          ((allocated * 3) / 2) : 
          max(4,new_size);

      if( toallocate < new_size ) toallocate = new_size;

      int *d = 0;
      int* old_d = 0;
      
      if(_elements)
      {
         old_d = (((int *)_elements) - 2);
         d = (int *)realloc( old_d, sizeof(int) * 2 + toallocate * sizeof(element));
         if( d != old_d ) // reallocated inplace
           _elements = (element *) ( d + 2 );
         init(_elements + old_size, new_size - old_size);
      }
      else
      {
         d = (int *)malloc(sizeof(int) * 2 + toallocate * sizeof(element));
         _elements = (element *) ( d + 2 );
         init(_elements,new_size);
      }
      set_size(new_size);
      set_allocated_size(toallocate);

    }
    else //if ( new_size <= allocated ), simply init the tail
      init(_elements + old_size, new_size - old_size);
  } 
  else if( _elements ) // and yet new_size < old_size 
    erase(_elements + new_size, old_size - new_size);

  set_size(new_size);
}

*/

template <typename element>
inline void
  array<element>::size ( int new_size, const element& init_value )
{
  assert(new_size >= 0);
  int oldsize = size();
  size(new_size);
  for(int i = oldsize; i < new_size; i++)
     _elements[i] = init_value;    
}


template <typename element>
inline void
  array<element>::set_all_to ( const element &the_element )
{
  for ( int i = 0; i < size(); i++ )
    _elements [ i ] = the_element;
}

template <typename element>
inline int 
  array<element>::get_index(const element& e) const
  {
    for ( int i = 0; i < size(); i++ )
      if(_elements[i] == e) return i;
    return -1;
  }

template <typename element>
inline void
  array<element>::push ( const element* elems, int count )
  {
    int psz = size();
    size( size() + count );
    element* pdst = head() + psz;
    copy(pdst,elems,count);
    /*for ( int i = 0;
              i < count;
              i++ )
      push ( elems [ i ] );*/
  }

template <typename element>
inline void
  array<element>::push ( const element& elem, int count )
  {
    int psz = size();
    size( size() + count );
    element* pdst = head() + psz;
    for ( int i = 0; i < count; i++ ) pdst[ i ] = elem;
  }


template <typename element>
inline bool
  array<element>::operator ==(const array<element> &rs) const
  {
    if(size() != rs.size()) return false;
    for ( int i = size() - 1; i >= 0; --i )
      if (!(_elements[i] == rs._elements[i]) ) return false;
    return true;
  }

template <typename element>
inline bool
  array<element>::operator !=(const array<element> &rs) const
  {
    if(size() != rs.size()) return true;
    for ( int i = 0; i < size(); i++ )
      if (_elements[i] != rs._elements[i] ) return true;
    return false;
  }


template <typename element>
inline bool
  array<element>::operator < (const array<element> &rs) const
  {
    assert(size() && rs.size());

    int mi = min( size(), rs.size() ) - 1;
    for (int i = 0; i < mi; i++ )
    {
      if (_elements[i] > rs._elements[i] ) 
        return false;
      if (_elements[i] < rs._elements[i] ) return true;
    }
    
    if(size() != rs.size())
      return _elements[mi] <= rs._elements[mi];  
    else
      return _elements[mi] < rs._elements[mi];
    //bool r = _elements[mi] < rs._elements[mi];
    //if(r)
    //  r = true;
    //return r;
  }

template <typename element>
inline bool
  array<element>::operator <= (const array<element> &rs) const
  {
    assert(size() && rs.size());

    int mi = min( size(), rs.size() );
    for (int i = 0; i < mi; i++ )
    {
      if (_elements[i] > rs._elements[i] ) return false;
      if (_elements[i] < rs._elements[i] ) return true;
    }
    return true;
  }

template <typename element>
inline bool
  array<element>::operator > (const array<element> &rs) const
  {
    assert(size() && rs.size());

    int mi = min( size(), rs.size() ) - 1;
    for (int i = 0; i < mi; i++ )
    {
      if (_elements[i] < rs._elements[i] ) return false;
      if (_elements[i] > rs._elements[i] ) return true;
    }
    return _elements[mi] > rs._elements[mi];
  }

template <typename element>
inline bool
  array<element>::operator >= (const array<element> &rs) const
  {
    assert(size() && rs.size());
    int mi = min( size(), rs.size() );
    for (int i = 0; i < mi; i++ )
    {
      if (_elements[i] < rs._elements[i] ) return false;
      if (_elements[i] > rs._elements[i] ) return true;
    }
    return true;
  }

template <typename element>
inline void
  array<element>::reverse()
  {
    int i = 0; 
    int k = size() - 1;
    while( i < k )
    {
      swop( _elements[i], _elements[k] );
      ++i; --k;
    }
  }

template <typename element>
inline void
  array<element>::push ( const slice<element>& elems ) 
{ 
  push(elems.start, elems.length); 
}


#define foreach(I,A) for(int I = A.size() - 1; I >= 0; I-- )

template<typename T>
  class circular_buffer : private array<T>
  {
  private:
	  typedef typename array<T> super;

  public:

    // ctor
	  circular_buffer(const int capacity = 0, const T &def = T()) :_size(0), _full(false)
	  {
			  super::size(capacity,def);
			  _begin = _end = super::head();
	  }
  		
	  ~circular_buffer() {}
  	
	  // get the buffer capacity (maximim size)
	  int capacity() const { return super::size(); }

	  // get number of items stored
	  int size() const { return _size; }

	  // is the buffer empty?
    bool is_empty () const { return !_full && _begin == _end;	}

	  // is the buffer full?
	  bool full() const {	return _full;	}

	  // push an item to the front of the buffer, \param x is item to push to the front.
    // because the buffer is circular, the last item will be overwritten if the buffer is already full.
	  void push_front(const T &x) 
	  {
		  if (super::size())
		  {
			  decrement(_begin);
			  *_begin = x;
			  if (_full)
				  decrement(_end);
			  else
			  {
				  if (++_size == super::size())
					  _full = true;
			  }
		  }
	  }
  	
	  // push an item to the back of the buffer, \param x is item to push to the back.
	  // because the buffer is circular, the first item will be overwritten if the buffer is already full.
	  void push(const T &x)
	  {
		  if (super::size())
		  {
			  *_end = x;
			  increment(_end);
			  if (_full)
				  increment(_begin);
			  else
			  {
				  if (++_size == super::size())
					  _full = true;
			  }
		  }
	  }
  	
	  // pop item from the front of the buffer, returns the item
	  T pop_front()
	  {
		  assert(super::size());
		  assert(_size > 0);
		  if (_size > 0)
		  {
        T t;
        tool::swap(t,*_begin);
			  increment(_begin);
			  _size--;
			  _full = false;
        return t;
		  }
      return T();
	  }
  	
	  // pop item from the back of the buffer, returns the item
	  T pop()
	  {
		  assert(super::size());
		  assert(_size > 0);
		  if (_size > 0)
		  {
        T t;
        tool::swap(t,*_end);
			  decrement(_end);
			  _size--;
			  _full = false;
        return t;
		  }
      return T();
	  }

	  // access the front item
	  T &first()       
	  {
		  assert(super::size());
		  assert(_size>0); 
		  return *_begin; 
	  }

	  // access the front item
	  const T &first() const 
	  { 
		  assert(super::size());
		  assert(_size>0); 
		  return *_begin; 
	  }

	  // access the back item
	  T &last()
	  { 
		  assert(super::size());
		  assert(_size>0); 
		  T* pe = _end; 
		  decrement(pe); 
		  return *pe; 
	  }

	  // access the back item
	  const T &last() const
	  { 
		  assert(super::size());
		  assert(_size>0); 
		  T* pe = _end; 
		  decrement(pe); 
		  return *pe; 
	  }

	  // access the i'th item in the buffer
	  T &operator[](const int i)
	  {
		  if (_begin < _end)
			  return *(_begin + i);
		  else
		  {
        const int s1 = super::head() + super::size() - _begin;
			  if ( i < s1)
				  return *(_begin+i);
			  else
				  return *(super::head() + i - s1);
		  }
	  }

	  // access the i'th item in the buffer
	  const T &operator[](const int i) const
	  {
		  if (_begin < _end)
			  return *(_begin + i);
		  else
		  {
        const int s1 = super::head() + super::size() - _begin;
			  if ( i < s1)
				  return *(_begin+i);
			  else
				  return *(super::head() + i - s1);
		  }
	  }

	  // shift the contents left
	  void left_shift()
	  {
		  decrement(_begin);
		  decrement(_end);
	  }

	  // shift the contents right
	  void right_shift()
	  {
		  decrement(_begin);
		  decrement(_end);
	  }

	  // clear the buffer
	  void clear()
	  {
                  erase(super::head(),super::size());
		  _begin = _end = super::head();
		  _size = 0;
		  _full = false;
	  }

	  // swap the contents of this buffer with another
	  void swap(circular_buffer<T> &buffer)
	  {
		  super::swap(buffer);
		  tool::swap(_begin,buffer._begin);
		  tool::swap(_end  ,buffer._end  );
		  tool::swap(_size ,buffer._size );
		  tool::swap(_full ,buffer._full );
	  }

	  // resize the (capacity) of the buffer, \param capacity new capacity.
    // if the new buffer capacity is too small for the existing contents, only the front-most items will remain.
	  void size(const int cap)
	  {
		  if (cap==capacity())
			  return;

		  circular_buffer<T> tmp(cap);
		  swap(tmp);

		  T*  i = tmp._begin;
		  T*  j = _begin;
		  int k = 0;
  	
		  for (; k < cap && k < capacity() && k < tmp.size(); j++,k++)
		  {
			  *j = *i;
			  tmp.increment(i);
		  }
		  _end = j;
		  _size = k;
		  _full = (_size == cap);
	  }

  private:
	  T*    _begin;
	  T*    _end;
	  int   _size;
	  bool  _full;
  	
	  // increment an item ptr, checking for wrap-around
	  void increment(const T* &i) const
	  {
		  i++;
		  if (i == super::tail())
			  i = super::head();
	  }

	  // decrement an item ptr, checking for wrap-around
	  void decrement(const T* &i) const
	  {
		  if ( i == super::head() )
			  i += super::size() - 1;
		  else
			  i--;
	  }

	  /// Increment an iterator, checking for wrap-around
	  void increment(T* &i) 
	  {
		  i++;
		  if (i == super::tail())
			  i = super::head();
	  }

	  /// Decrement an iterator, checking for wrap-around
	  void decrement(T* &i) 
	  {
		  if ( i == super::head() )
			  i += super::size() - 1;
		  else
			  i--;
	  }
};

}

#endif
