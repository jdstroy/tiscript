//|
//|
//| Copyright (c) 2001-2005
//| Andrew Fedoniouk - andrew@terra-informatica.org
//|
//| pool of unique instances, e.g. strings
//|
//|

#ifndef __tl_handle_pool_h
#define __tl_handle_pool_h

#include "tl_basic.h"

namespace tool
{

  //#define POOL_TRAITS pool_traits<c_element>  

  template <typename c_element>
  class handle_pool
  {

    handle_pool& operator = (const handle_pool& c)
    {
      copy(c);
      return *this;
    }

    handle_pool ( const handle_pool& c )
    {
      _hash_size = c._hash_size;
      _table = new array<hash_item> [ _hash_size ];
      copy(c);
    }


  public:
    handle_pool ( uint hash_size = 32 )
    {
      _hash_size = hash_size;
      _table = new array<hash_item> [ hash_size ];
    }

    virtual  ~handle_pool()
    {
      clear ();
      delete [] _table;
    }

    bool  exists ( const c_element& the_key );

    //
    // Add to hash table association of object with specified name.
    //
    uint  operator[] ( const c_element &the_key );
    uint  operator[] ( const c_element &the_key ) const;
    c_element*  operator() ( uint the_index );
    const c_element*  operator() ( uint the_index ) const;

    inline c_element* intern( const c_element &elem )
    {
      return _array[ int(get_index( elem,true)) ];
    }

    inline c_element* intern( const c_element* elem )
    {
      return _array[ int(get_index( elem, true)) ];
    }

    //
    // Remove object with specified name from hash table.
    //
    void  remove ( const c_element& the_key );

    uint get_index ( const c_element& the_key, bool create = false );
    uint get_index ( const c_element* the_key, bool create = false );

    void  clear ();

    int   size () const  { return _array.size (); }
    bool  is_empty () const { return _array.size () == 0; }

  protected:
    struct hash_item
    {
      uint    _key_hash;
      uint    _index;

      hash_item () : _index ( 0 )
      {
      }

      hash_item (  uint kh, uint i ) : _key_hash( kh ), _index ( i )
      {
      }
    };
    uint                _hash_size;
    array<hash_item> *  _table;

    array< handle<c_element> >  _array;

  public:
    array< handle<c_element> >& elements ()
    {
      return _array;
    }

#ifdef _DEBUG
    void report() 
    {
      FILE *f = fopen("c:\\handle_pool.txt","wt+");
	  if( !f ) return;
      fprintf(f,"pool size: %d\n",_array.size());
      fprintf(f,"hash table: size=%d\n",_hash_size);
      fprintf(f,"\tbuckets:\n");
      for(unsigned i = 0; i < _hash_size; i++ )
      {
        fprintf(f,"\t\t#%d size=%d",i,_table[i].size());
        if(_table[i].size() == 1)
          fprintf(f," *");
        else if(_table[i].size() > 1)
        {
          for(int k = 0; k < _table[i].size();++k)
          {
            fprintf(f," %X(%d)",_table[i][k]._key_hash,_table[i][k]._index);
            c_element *elm = _array[_table[i][k]._index];
            elm = elm;
          }
          //fprintf(f," !!!!");
        }
        fprintf(f,"\n");
      }
      fclose(f);
      
    }
#endif


  };


  template <typename c_element>
  inline uint
    handle_pool<c_element>::operator[] ( const c_element &the_key )
  {
    return get_index ( the_key, true );
  }

  template <typename c_element>
  inline uint
    handle_pool<c_element>::operator[] ( const c_element &the_key ) const 
  {
    return const_cast<handle_pool<c_element>*>
                    ( this )->get_index ( the_key, false );
  }

  template <typename c_element>
  inline uint
    handle_pool<c_element>::get_index ( const c_element& the_key,
                                              bool create )
  {
    uint hk = the_key.hash();
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
      if ( (hk == it._key_hash) && (the_key == *(_array[it._index].ptr())) )
        return it._index;
    }

    if ( create )
    {
      uint ni = _array.size ();
      _array.push ( new c_element(the_key) );
      bucket.push ( hash_item ( hk, ni ) );
      return ni;
    }
    return (uint)-1;
  }

  template <typename c_element>
  inline uint
    handle_pool<c_element>::get_index ( const c_element* the_key,
                                              bool create )
  {
    uint hk = the_key->hash();
    uint h = hk;
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
      if ( (hk == it._key_hash) && (*the_key == *(_array[it._index].ptr())) )
        return it._index;
    }

    if ( create )
    {
      uint ni = _array.size ();
      _array.push ( the_key );
      bucket.push ( hash_item ( hk, ni ) );
      return ni;
    }
    return (uint)-1;
  }


  template <typename c_element>
  inline void
    handle_pool<c_element>::remove ( const c_element& the_key )
  {
    uint h = hash<c_element> ( the_key ) % _hash_size;
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


  template <typename c_element>
  inline c_element*
    handle_pool<c_element>::operator() ( uint the_index )
  {
    return _array [ the_index ];
  }

  template <typename c_element>
  inline const c_element*
    handle_pool<c_element>::operator() ( uint the_index ) const
  {
    return _array [ the_index ];
  }

  template <typename c_element>
  inline bool
    handle_pool<c_element>::exists ( const c_element& the_key )
  {
    return ( get_index ( the_key, false ) != (size_t)-1 );
  }

  template <typename c_element>
  inline void
    handle_pool<c_element>::clear ()
  {
    for ( size_t i = 0; i < _hash_size; ++i )
      _table [ i ].clear ();
    _array.clear ();
  }
  


};

#endif
