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

  inline const element &operator[] ( int index ) const;
  inline element &operator[] ( int index );

  element   remove ( int index );
  void      remove ( int index, int length );
  int       insert ( int index, const element& elem );
  void      insert ( int index, const element* elems, int count );

  void      set_all_to ( const element &element );

  int       get_index(const element &e) const;

  bool      operator == (const array<element> &rs) const;
  bool      operator != (const array<element> &rs) const;
  bool      operator < (const array<element> &rs) const;
  bool      operator <= (const array<element> &rs) const;
  bool      operator > (const array<element> &rs) const;
  bool      operator >= (const array<element> &rs) const;

  int		    push ( const element& elem );
  void		  push ( const element* elems, int count );
  void		  push ( const slice<element>& elems );
  element   pop ();

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
    return _size - 1;
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
  element* tail() const { return _elements + _size; }

  operator slice<element>() const { return slice<element>(head(),size()); }
  slice<element>  operator()(int s) const { return slice<element>(head()+s,size()-s); }
  slice<element>  operator()(int s, int e) const { return slice<element>(head()+s,e-s); }

  void swap( array<element> &the_array )
  {
    if(this == &the_array)
      return;
    /*tool::*/ swop(_size,the_array._size);
    /*tool::*/ swop(_allocated_size,the_array._allocated_size);
    /*tool::*/ swop(_elements,the_array._elements);
  }


  void transfer_from( array<element> &the_array )
  {
    clear();
    if ( _elements )  delete [] _elements;
    _size           = the_array._size;
    _allocated_size = the_array._allocated_size;
    _elements       = the_array._elements;
    the_array._size = 0;
    the_array._allocated_size = 0;
    the_array._elements = 0;
  }

protected:
  int	 	 		_size;
  int				_allocated_size;
  element *	_elements;

};

template <typename element>
inline
  array<element>::array ()
:_size ( 0 ), _allocated_size ( 0 ), _elements ( 0 )
{
  ;/* do nothing */
}

template <typename element>
inline
  array<element>::array ( int sz )
:_size ( 0 ), _allocated_size ( 0 ), _elements ( 0 )
{
  size ( sz );
}

template <typename element>
inline
  array<element>::array ( int sz, const element &init_element )
: _size ( 0 ), _allocated_size ( 0 ), _elements ( 0 )
{
  size ( sz, init_element );
}

template <typename element>
inline
  array<element>::array ( const slice<element>& d )
: _size ( 0 ), _allocated_size ( 0 ), _elements ( 0 )
{
  size ( d.length );
  copy( head(), d.start, d.length);
}


template <typename element>
inline
  array<element>::array ( const array<element> &the_array )
: _size ( 0 ), _allocated_size ( 0 ), _elements ( 0 )
{
  operator= ( the_array );
}


template <typename element>
  inline void array<element>::destroy ()
{
  erase(_elements,_size);
  delete [] (char*)_elements;
  _elements = 0;
  _size = 0;
  _allocated_size = 0;

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
  size ( the_array._size );

  copy(_elements,the_array._elements,_size);
  /*
  for ( int i = 0; i < _size; i++ )
    _elements [ i ] = the_array._elements [ i ];
  */
  return *this;
}

template <typename element>
inline int
  array<element>::push ( const element& elem )
{
  assert( _elements == 0 || &elem < _elements || &elem >= (_elements + _size) );
  int tpos = _size;
  size ( tpos + 1 );
  operator[] ( tpos ) = elem;
  return tpos;
}

template <typename element>
inline element
  array<element>::pop ()
{
  assert( _size > 0 );
  if( _size > 0 )
  {
    element e = _elements [ _size - 1 ];
    size ( _size - 1 );
    return e;
  }
  return element();
}

template <typename element>
inline element
  array<element>::remove ( int index )
{
  assert( index >= 0 && index < _size );
  element *dst = _elements + index;
  element r = *dst;
  _size--;
  if(index < _size)
    move(dst,dst+1,_size - index);
  erase(_elements + _size,1);
  //for ( int i = index; i < _size; i++, dst++ )
  //  *dst = * ( dst + 1 );
  return r;
}

template <typename element>
inline void
  array<element>::remove ( int index, int length )
{
  assert( index >= 0 && ( ( index + length ) <= _size ) );
  element *dst = _elements + index;
  element *dste = dst;
  //for ( int i = 0; i < length; i++, dste++ )
  //  dste->~element();
  _size -= length;
  if(index < _size)
    move(dst,dst+length,_size - index);
  erase(_elements + _size, length);
  //for ( int i = index; i < _size; i++, dst++ )
  //  *dst = * ( dst + length );
}


template <typename element>
inline int
  array<element>::insert ( int index, const element& elem )
{
  if ( index < 0 )
    index = 0;
  if ( index >= _size )
  {
    push ( elem );
    return size() - 1;
  }
  else
  {
    size ( _size + 1 );
    element *dst = _elements + _size - 1;

    move(_elements + index + 1,_elements + index,_size - index - 1);

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
  assert( _elements == 0 || elems < _elements || elems >= (_elements + _size) );
  if ( index < 0 )
    index = 0;
  if ( index >= _size )
    push ( elems, count );
  else
  {
    int _old_size = _size;
    size ( _size + count );
    element *dst = _elements + _size - 1;
    element *src = _elements + _old_size - 1;
    for ( int i = 0; i < _old_size - index; i++)
      *dst-- = *src--;
    //move<element>(_elements + index + count,_elements + index,_size - index - count);

    element *p = _elements + index;
    for ( int j = 0; j < count; j++)
      *p++ = *elems++;

  }
}

template <typename element>
inline element &
  array<element>::operator[] ( int index )
{
  assert( index >= 0 && index < _size );
  return _elements [ index ];
}

template <typename element>
inline const element &
  array<element>::operator[] ( int index ) const
{
  assert( index >= 0 && index < _size );
  return _elements [ index ];
}

template <typename element>
inline element &
  array<element>::last ()
{
  assert( _size > 0 );
  return _elements [ _size - 1 ];
}

template <typename element>
inline const element &
  array<element>::last () const
{
  assert( _size > 0 );
  return _elements [ _size - 1 ];
}

template <typename element>
inline element &
  array<element>::first ()
{
  assert( _size > 0 );
  return _elements [ 0 ];
}

template <typename element>
inline const element &
  array<element>::first () const
{
  assert( _size > 0 );
  return _elements [ 0 ];
}


template <typename element>
inline int
  array<element>::size () const
{
  return _size;
}

template <typename element>
inline bool
  array<element>::is_empty () const
{
  return ( _size == 0 );
}

template <typename element>
inline void
  array<element>::size ( int new_size )
{
  assert(new_size >= 0);
  if ( _size == new_size )
    return;
  if ( new_size > _size )
  {
    if ( new_size >  _allocated_size )
    {
      //int toallocate = _allocated_size? ((_allocated_size * 5) / 4):8;
      int toallocate = _allocated_size? ((_allocated_size * 3) / 2):4;
      //int toallocate = _allocated_size + (new_size -  _allocated_size) * 2;


      _allocated_size = ( new_size < toallocate ) ? toallocate : new_size;
      
      element *new_space = (element *) new char[ _allocated_size * sizeof(element) ];
      init(new_space,new_size);
      copy(new_space,_elements, _size < new_size? _size: new_size );

      //for (int i = 0; i < min ( _size, new_size ); i++ )
      //  new_space [ i ] = _elements [ i ];
      if ( _elements )
      {
        erase(_elements,_size);
        delete [] (char*)_elements;
      }
      _elements = new_space;

    }
    else
      init(_elements+_size,new_size - _size);
  }
  else
  {
    erase(_elements + new_size,_size - new_size);
  }

  _size = new_size;
}


template <typename element>
inline void
  array<element>::size ( int new_size, const element& init_value )
{
  assert(new_size >= 0);
  int oldsize = _size;
  size(new_size);
  for(int i = oldsize; i < _size; i++)
     _elements[i] = init_value;
}


template <typename element>
inline void
  array<element>::set_all_to ( const element &the_element )
{
  for ( int i = 0; i < _size; i++ )
    _elements [ i ] = the_element;
}

template <typename element>
inline int
  array<element>::get_index(const element& e) const
  {
    for ( int i = 0; i < _size; i++ )
      if(_elements[i] == e) return i;
    return -1;
  }

template <typename element>
inline void
  array<element>::push ( const element* elems, int count )
  {
    for ( int i = 0;
              i < count;
              i++ )
      push ( elems [ i ] );
  }



template <typename element>
inline bool
  array<element>::operator ==(const array<element> &rs) const
  {
    if(_size != rs._size) return false;
    for ( int i = _size - 1; i >= 0; --i )
      if (!(_elements[i] == rs._elements[i]) ) return false;
    return true;
  }

template <typename element>
inline bool
  array<element>::operator !=(const array<element> &rs) const
  {
    if(_size != rs._size) return true;
    for ( int i = 0; i < _size; i++ )
      if (_elements[i] != rs._elements[i] ) return true;
    return false;
  }


template <typename element>
inline bool
  array<element>::operator < (const array<element> &rs) const
  {
    assert(_size && rs._size);

    int mi = min( _size, rs._size ) - 1;
    for (int i = 0; i < mi; i++ )
    {
      if (_elements[i] > rs._elements[i] )
        return false;
      if (_elements[i] < rs._elements[i] ) return true;
    }

    if(_size != rs._size)
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
    assert(_size && rs._size);

    int mi = min( _size, rs._size );
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
    assert(_size && rs._size);

    int mi = min( _size, rs._size ) - 1;
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
    assert(_size && rs._size);
    int mi = min( _size, rs._size );
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
      swap( _elements[i], _elements[k] );
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


};

#endif