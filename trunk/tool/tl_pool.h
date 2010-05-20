//|
//|
//| Copyright (c) 2001-2005
//| Andrew Fedoniouk - andrew@terrainformatica.com
//|
//| pool, table of unique instances of some class, e.g. strings
//|
//|

#ifndef __tl_pool_h
#define __tl_pool_h

#include "tl_sync.h"

namespace tool
{

  template <typename c_element>
    struct pool_traits
  {
    static unsigned int hash(const c_element& e) { return tool::hash<c_element>(e); }
    static bool equal(const c_element& l, const c_element& r) { return l == r; }
    static c_element create(const c_element& key) { return key; }
  };

  //#define POOL_TRAITS pool_traits<c_element>

  template <typename c_element, typename c_traits = pool_traits<c_element> >
  class pool
  {
  public:
    typedef c_element element_type;

    pool ( uint hash_size = 36 )
    {
      _hash_size = hash_size;
      _table = new array<hash_item> [ hash_size ];
    }

    pool ( const pool& c )
    {
      _hash_size = c._hash_size;
      _table = new array<hash_item> [ _hash_size ];
      copy(c);
    }

    virtual  ~pool()
    {
      clear ();
      delete [] _table;
    }

    bool  exists ( const c_element& the_key );

    pool& operator = (const pool& c)
    {
      copy(c);
      return *this;
    }
    bool operator==(const pool& rs) const
    {
      return _array == rs._array;
    }

    //
    // Add to hash table association of object with specified name.
    //
    uint  operator[] ( const c_element &the_key );
    //uint  operator[] ( const c_element &the_key ) const;
    c_element&  operator() ( uint the_index );
    const c_element&  operator() ( uint the_index ) const;

    inline const c_element& intern( const c_element &elem )
    {
      critical_section cs(_guard);
      return _array[ int(get_index( elem,true)) ];
    }

    //
    // Remove object with specified name from hash table.
    //
    void  remove ( const c_element& the_key );

    uint get_index ( const c_element& the_key, bool create = false );

    void  clear ();

    size_t size () const  { return (size_t)_array.size (); }
    bool   is_empty () const { return _array.size () == 0; }

  protected:
    struct hash_item
    {
      uint			 _key_hash;
      uint           _index;
      hash_item () : _index ( 0 ) {}
      hash_item (  uint kh, uint i ) : _key_hash( kh ), _index ( i ) {}
    };
    uint              _hash_size;
    array<hash_item> *  _table;

    array<c_element>    _array;
    mutex               _guard;  
    //size_t              _get ( const c_element& the_key, bool create );

    /*
    void copy( const pool& c )
    {
      if(&c == this)
        return;
      clear();
      array<c_key> k;
      c.keys(k);
      for(int i = 0; i < k.size(); i++)
        (*this)[k[i]] = c[k[i]];
    }
    */

  public:
    array<c_element>& elements ()
    {
      return _array;
    }

    unsigned int hash() const
    {
      return _array.hash();
    }

#ifdef _DEBUG
    void report()
    {
      printf("hash table: size=%d\n",_hash_size);
      printf("\tbuckets:\n");
      for(unsigned i = 0; i < _hash_size; i++ )
        printf("\t\t#%d size=%d\n",i,_table[i].size());
    }
#endif


  };


  template <typename c_element, typename c_traits>
  inline uint
    pool<c_element,c_traits>::operator[] ( const c_element &the_key )
  {
    return get_index ( the_key, true );
  }

  /*template <typename c_element, typename c_traits>
  inline uint
    pool<c_element,c_traits>::operator[] ( const c_element &the_key ) const
  {
    return const_cast<pool<c_element,c_traits>*>
                    ( this )->get_index ( the_key, false );
  }*/

  template <typename c_element, typename c_traits>
  inline uint
    pool<c_element,c_traits>::get_index ( const c_element& the_key,
                                              bool create )
  {
    uint hk = c_traits::hash ( the_key );
    uint h = hk % _hash_size;
    int i;
    array<hash_item> &bucket = _table [ h ];

    for ( i = 0; i < bucket.size (); i++ )
    {
      const hash_item &it = bucket [ i ];
      if ( (hk == it._key_hash) && c_traits::equal(the_key, _array[it._index]) )
        return it._index;
    }

    if ( create )
    {
      uint ni = _array.size ();
      _array.push ( c_traits::create(the_key) );
      bucket.push ( hash_item ( hk, ni ) );
      return ni;
    }
    return (uint)-1;
  }

  template <typename c_element, typename c_traits>
  inline void
    pool<c_element,c_traits>::remove ( const c_element& the_key )
  {
    size_t h = hash ( the_key ) % _hash_size;
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


  template <typename c_element, typename c_traits>
  inline c_element&
    pool<c_element,c_traits>::operator() ( uint the_index )
  {
    return _array [ the_index ];
  }

  template <typename c_element, typename c_traits>
  inline const c_element&
    pool<c_element,c_traits>::operator() ( uint the_index ) const
  {
    return _array [ the_index ];
  }

  template <typename c_element, typename c_traits>
  inline bool
    pool<c_element,c_traits>::exists ( const c_element& the_key )
  {
    return ( get_index ( the_key, false ) != uint(-1) );
  }

  template <typename c_element, typename c_traits>
  inline void
    pool<c_element,c_traits>::clear ()
  {
    for ( size_t i = 0; i < _hash_size; ++i )
      _table [ i ].clear ();
    _array.clear ();
  }



};

#endif
