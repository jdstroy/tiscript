//|
//|
//| Copyright (c) 2001-2005
//| Andrew Fedoniouk - andrew@terra-informatica.org
//|
//| dictionary - hash map of key/value pairs backed by array
//|
//|

#ifndef __cs_dictionary_h
#define __cs_dictionary_h

#include "tl_basic.h"
#include "tl_string.h"
#include "tl_array.h"

namespace tool
{
  template <class c_key,class c_element>
  class dictionary
  {
  public:
    dictionary ( size_t hash_size = 36 )
    {
      init(hash_size); 
    }

    dictionary ( const dictionary& copy )
    {
      init(copy._hash_size);
      for ( int i = 0; i < copy.size(); i++ )
        (*this)[copy.key(i)] = copy.value(i);
    }

    void init(size_t hash_size) 
    {
      _hash_size = hash_size;
      _table = new hash_item* [ hash_size ];
      for ( unsigned i = 0; i < hash_size; i++ )
        _table [ i ] = 0;
    }

    virtual  ~dictionary();
    bool     exists ( const c_key& the_key );

    //
    // Add to hash table association of object with specified name.
    //
    c_element&  operator[] ( const c_key &the_key );
    c_element   operator() ( const c_key &the_key ) const
    {
      c_element el;
      find ( the_key, el );
      return el;
    }

    //
    // Search for object with specified name in hash table.
    // If object is not found false is returned.
    //
    bool  find ( const c_key& the_key, c_element& the_element ) const;
    bool  find ( const c_key& the_key, c_element* &the_element_ptr ) const;
    //
    // Remove object with specified name from hash table.
    // If there are several objects with the same name the one last inserted
    // is removed. If such name was not found 'false' is returned.
    //
    bool  remove ( const c_key& the_key );

    void  clear ();

    int
      size () const
    {
      return _array.size ();
    };

    bool
      is_empty () const
    {
      return _array.size () == 0;
    };

    bool operator == (const dictionary& rd) const { return _array == rd._array; }
    bool operator != (const dictionary& rd) const { return _array != rd._array; }

    struct dict_item
    {
      c_key      key;
      c_element  element;
      bool operator == (const dict_item& rd) const { return key == rd.key && element == rd.element; }
      bool operator != (const dict_item& rd) const { return key != rd.key || element != rd.element; }
    };

    // push unnamed value
    void push(const c_element& v)
    {
      dict_item it; it.element = v;
      _array.push(it);
    }

  protected:
    struct hash_item
    {
      uint        _index;
      hash_item*  _next;
      
      hash_item ()
      {
        _next = 0;
      }
      
      hash_item ( uint the_index, hash_item* the_next )
      {
        _index = the_index;
        _next  = the_next;
      }
    };

    size_t       _hash_size;
    hash_item**  _table;

    array<dict_item>  _array;

  public:
    int  get_index ( const c_key& the_key, bool create = false );

    array<dict_item>&
      elements ()
    {
      return _array;
    }

    const c_key&      key   ( int i ) const;
    c_element&        value ( int i );
    const c_element&  value ( int i ) const;
  };

  template <class c_key, class c_element>
  inline
    dictionary<c_key, c_element>::~dictionary ()
  {
    clear ();
    delete [] _table;
  }


  template <class c_key, class c_element>
  inline c_element& 
    dictionary<c_key, c_element>::operator[] ( const c_key &the_key )
  {

    int index = get_index ( the_key, true );
    return _array [ index ].element;
  }

  template <class c_key, class c_element>
  inline const c_key&
    dictionary<c_key, c_element>::key ( int i ) const
  {
    return _array [ i ].key;
  }

  template <class c_key, class c_element>
  inline c_element&
    dictionary<c_key, c_element>::value ( int i )
  {
    return _array [ i ].element;
  }

  template <class c_key, class c_element>
  inline const
    c_element& dictionary<c_key, c_element>::value ( int i ) const
  {
    return _array [ i ].element;
  }

  template <class c_key, class c_element>
  inline bool
    dictionary<c_key, c_element>::find ( const c_key& the_key, c_element& the_element ) const
  {
    int index = const_cast<dictionary<c_key, c_element>*>
                ( this )->get_index ( the_key, false );

    if ( index < 0 )
      return false;

    the_element = _array [ index ].element;
    return true;
  }

  template <class c_key, class c_element>
  inline bool
    dictionary<c_key, c_element>::find ( const c_key& the_key,
                                        c_element* &the_element_ptr ) const
  {
    int index = const_cast<dictionary<c_key, c_element>*>
                ( this )->get_index ( the_key, false );

    if ( index < 0 )
      return false;

    the_element_ptr = const_cast<c_element*> ( &_array [ index ].element );
    return true;
  }

  template <class c_key, class c_element>
  inline int
    dictionary<c_key, c_element>::get_index ( const c_key& the_key,
                                              bool create )
  {
    size_t h = hash<c_key> ( the_key ) % _hash_size;
    for ( hash_item* ip = _table [ h ];
          ip != NULL;
          ip = ip->_next )
      if ( _array [ ip->_index ].key == the_key )
        return ip->_index;

    if ( create )
    {
      uint ni = _array.size ();
      _array.size ( int ( ni + 1 ) );
      _table [ h ] = new hash_item ( ni, _table [ h ] );
      _array [ ni ].key = the_key;
      return ni;
    }
    return -1;
  }


  template <class c_key, class c_element>
  inline bool
    dictionary<c_key, c_element>::exists ( const c_key& the_key )
  {
    return ( get_index ( the_key, false ) >= 0 );
  }


  template <class c_key, class c_element>
  inline void
    dictionary<c_key,c_element>::clear ()
  {
    for ( int i = int(_hash_size); --i >= 0; )
    {
      hash_item* ip = _table [ i ];
      while ( ip != NULL )
      {
        hash_item* _next = ip->_next;
        delete ip;
        ip = _next;
      }
      _table [ i ] = 0;
    }
    _array.clear ();
  }

  template <class c_key, class c_element>
  inline bool
    dictionary<c_key, c_element>::remove ( const c_key& the_key )
  {
    size_t h = hash<c_key> ( the_key ) % _hash_size;
    hash_item* curr = _table [ h ], *prev = 0;
    while ( curr != NULL )
    {
      if ( _array [ curr->_index ].key == the_key )
      {
        if ( prev == 0 )
          _table [ h ] = curr->_next;
        else
          prev->_next = curr->_next;
        _array.remove ( curr->_index );
        delete curr;
        return true;
      }
      prev = curr;
      curr = curr->_next;
    }
    return false;
  }

};

#endif
