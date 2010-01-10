//|
//|
//| Copyright (c) 2001-2005
//| Andrew Fedoniouk - andrew@terrainformatica.com
//|
//| hash table backed by array
//|
//|

#ifndef __cs_HASH_TABLE_H
#define __cs_HASH_TABLE_H

#include "tl_basic.h"
#include "tl_string.h"
#include "tl_array.h"
#include "tl_hash.h"
#include <string.h>


namespace tool
{

  //template <typename c_key> inline unsigned int hash ( const c_key &the_key ) {  return the_key.hash();  }

  template <typename c_key> inline bool eq ( const c_key &key1, const c_key &key2 ) { return key1 == key2; }

  //{ return key1 == key2; }

  template <>
  inline bool eq ( char const * const &key1, char const * const &key2 )
  {
    return strcmp(key1,key2) == 0;
  }

  template <typename c_key, typename c_element>
  class hash_table
  {
  public:
    hash_table ( uint hash_size = 0x20 /*32*/ )
    {
      _hash_size = hash_size;
      _table = new array<hash_item> [ hash_size ];
    }

    hash_table ( const hash_table& c )
    {
      _hash_size = c._hash_size;
      _table = new array<hash_item> [ _hash_size ];
      copy(c);
    }

    virtual  ~hash_table()
    {
      clear ();
      delete [] _table;
    }

    bool  exists ( const c_key& the_key ) const ;

    hash_table& operator = (const hash_table& c)
    {
      copy(c);
      return *this;
    }

    //
    // Add to hash table association of object with specified name.
    //
    c_element&  operator[] ( const c_key &the_key );
    c_element   operator[] ( const c_key &the_key ) const;
    c_element&  operator() ( int the_index );
    const c_element&  operator() ( int the_index ) const;

    //
    // Search for object with specified name in hash table.
    // If object is not found false is returned.
    //
    bool  find ( const c_key& the_key, c_element& the_element ) const;
    bool  find ( const c_key& the_key, c_element* &the_element_ptr ) const;

    //
    // Remove object with specified name from hash table.
    //
    void  remove ( const c_key& the_key );

    int   get_index ( const c_key& the_key, bool create = false );

    void  clear ();

    int   size () const  { return _array.size (); }
    bool  is_empty () const { return _array.size () == 0; }

    // will return list of all keys. key allkeys[n] correspond to element in n'th position in elements()
    void  keys( array<c_key>& allkeys) const;


  protected:
    struct hash_item
    {
      unsigned int   _key_hash;
      c_key          _key;
      uint           _index;

      hash_item () : _index ( 0 )
      {
      }

      hash_item (  uint kh, const c_key& k, uint i ) : _key_hash( kh ), _key ( k ), _index ( i )
      {
      }
    };
    uint                _hash_size;
    array<hash_item> *  _table;

    array<c_element>    _array;
    c_element*          _get ( const c_key& the_key, bool create );

    void copy( const hash_table& c )
    {
      if(&c == this)
        return;
      clear();
      array<c_key> k;
      c.keys(k);
      for(int i = 0; i < k.size(); i++)
        (*this)[k[i]] = c[k[i]];
    }


  public:

    void transfer_from( hash_table& src )
    {
      swap(_hash_size,src._hash_size);
      swap(_table, src._table);
      _array.transfer_from(src._array);
      src.clear();
    }

    array<c_element>& elements ()
    {
      return _array;
    }



#ifdef _DEBUG
    void report()
    {
      dbg_printf("hash table: size=%d\n",_hash_size);
      dbg_printf("\tbuckets:\n");
      for(unsigned i = 0; i < _hash_size; i++ )
        dbg_printf("\t\t#%d size=%d\n",i,_table[i].size());
    }
#endif


  };


  template <class c_key, class c_element>
  inline c_element&
    hash_table<c_key, c_element>::operator[] ( const c_key &the_key )
  {
    return *_get ( the_key, true );
  }

  template <class c_key, class c_element>
  inline c_element
    hash_table<c_key, c_element>::operator[] ( const c_key &the_key ) const
  {
    c_element *pel =
       const_cast<hash_table<c_key, c_element>*>
                    ( this )->_get ( the_key, false );
    if(pel)
      return *pel;
    //assert(false);
    return c_element();
  }


  template <class c_key, class c_element>
  inline bool
    hash_table<c_key, c_element>::find ( const c_key& the_key,
                                          c_element& the_element ) const
  {
    c_element *pe = const_cast<hash_table<c_key, c_element>*>
                    ( this )->_get ( the_key, false );
    if ( pe )
    {
      the_element = *pe;
      return true;
    }
    return false;
  }


  template <class c_key, class c_element>
  inline bool
    hash_table<c_key, c_element>::find ( const c_key& the_key,
                                          c_element* &the_element_ptr ) const
  {
    c_element *pe = const_cast<hash_table<c_key, c_element>*>
                    ( this )->_get ( the_key, false );
    if ( pe )
    {
      the_element_ptr = pe;
      return true;
    }
    return false;
  }


  template <class c_key, class c_element>
  inline c_element*
    hash_table<c_key, c_element>::_get ( const c_key& the_key, bool create )
  {
    int idx = get_index ( the_key, create );
    if ( idx < 0 )
      return 0;
    return &_array [ idx ];
  }

  template <class c_key, class c_element>
  inline int
    hash_table<c_key, c_element>::get_index ( const c_key& the_key,
                                              bool create )
  {
    unsigned int hk = hash<c_key> ( the_key );
    uint h;
#ifdef UNDER_CE    
    switch(_hash_size)
    {
      case 0x10: h = hk & 0x0F; break;    
      case 0x20: h = hk & 0x1F; break;
      case 0x40: h = hk & 0x3F; break;
      case 0x80: h = hk & 0x7F; break;
      default:   h = hk % _hash_size; break;
    }
#else    
    h = hk % _hash_size;
#endif    

    int i;
    array<hash_item> &bucket = _table [ h ];

    for ( i = 0; i < bucket.size (); i++ )
    {
      const hash_item &it = bucket [ i ];
      if ((hk == it._key_hash) && eq(it._key,the_key))
          return int(it._index);
    }

    if ( create )
    {
      uint ni = _array.size ();
      _array.size ( int ( ni + 1 ) );
      bucket.push ( hash_item ( hk, the_key, ni ) );
      return int(ni);
    }
    return -1;
  }

  template <class c_key, class c_element>
  inline void
    hash_table<c_key, c_element>::keys ( array<c_key>& allkeys ) const
  {
    allkeys.size(size());
    for ( uint i = 0; i < _hash_size; ++i )
    {
      array<hash_item> &bucket = _table [ i ];
      for(int k = 0; k < bucket.size(); k++)
      {
        hash_item& hi = bucket[k];
        allkeys[hi._index] = hi._key;
      }
    }
  }

  template <class c_key, class c_element>
  inline void
    hash_table<c_key, c_element>::remove ( const c_key& the_key )
  {
    uint h = hash<c_key> ( the_key ) % _hash_size;
    int i;
    array<hash_item> &bucket = _table [ h ];

    for ( i = 0; i < bucket.size(); i++ )
    {
      const hash_item &it = bucket [ i ];
      if ( it._key == the_key )
      {
        unsigned int index = it._index;
        _array.remove ( index );
        bucket.remove ( i );
        // adjust other references
        for ( h = 0; h < _hash_size; h++ )
        {
          array<hash_item> &bucket = _table [ h ];
          for ( i = 0; i < bucket.size(); i++ )
          {
            hash_item &it = bucket [ i ];
            if ( it._index > index )
              it._index--;
          }
        }
        return;
      }
    }
  }


  template <class c_key, class c_element>
  inline c_element&
    hash_table<c_key, c_element>::operator() ( int the_index )
  {
    return _array [ the_index ];
  }

  template <class c_key, class c_element>
  inline const c_element&
    hash_table<c_key, c_element>::operator() ( int the_index ) const
  {
    return _array [ the_index ];
  }

  template <class c_key, class c_element>
  inline bool
    hash_table<c_key, c_element>::exists ( const c_key& the_key ) const
  {
    return
      const_cast<hash_table<c_key, c_element>*>
                    ( this )->_get ( the_key, false ) != 0;
  }

  template <class c_key, class c_element>
  inline void
    hash_table<c_key, c_element>::clear ()
  {
    for ( uint i = 0; i < _hash_size; ++i )
      _table [ i ].clear ();
    _array.clear ();
  }



};

#endif
