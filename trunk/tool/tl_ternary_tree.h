#ifndef __tl_ternary_h__
#define __tl_ternary_h__

// ternary tree lookup_tbl table

#include "tl_array.h"
#include "memory.h"

namespace tool 
{

  inline const char* str_dup(const char* str) { return strdup(str); }
  inline const wchar* str_dup(const wchar* str) { return wcsdup(str); }

  template <typename CHARTYPE, bool case_insensitive = true>
    class lookup_tbl
  {
    typedef unsigned short int node_index;
    //const node_index nil = node_index(-1);

  public:
    enum constants 
    {
        any_one_char = '?'
    };

    lookup_tbl():total_statics(0) {}

    lookup_tbl(const CHARTYPE** tbl, size_t tbl_size)
      // initialize lookup_tbl table by static strings
      // !!!! table *must* contain only pointers into static segment
    {
      // preallocation of data structures
      strings.size(tbl_size); strings.size(0);  
      nodes.size(tbl_size * 5); nodes.size(0); 

      total_statics = tbl_size;
      for( uint i = 0; i < tbl_size; ++i, ++tbl )
      {
        assert( search(*tbl) == 0 ); // check for duplicates
        insert(*tbl);
      }
    }

    ~lookup_tbl() 
    {
      for( int i = strings.size() - 1; i >= total_statics; --i )
        free((void*)strings[i]);
    }

    void clear()
    {
      for( int i = strings.size() - 1; i >= total_statics; --i )
        free((void*)strings[i]);
      strings.destroy();
      nodes.destroy();
    }

    // 
    // returns number [1..max uint] of inserted or existing item.
    // does no insertion if item already exists. 
    //
    //uint insert(const CHARTYPE *s)
    //{
    //  return (uint)nodes[insert(s, 0, s)].eqkid;
    //}

    
    uint insert(const CHARTYPE* s)
    {   
        const CHARTYPE* str = s;
        node_index nid = 0; // root index;
        node_index* pnid = 0; 
        while (nid < total_nodes()) 
        {
          node& n = nodes[nid];
          CHARTYPE  c = to_char(*s);
          if (c < n.splitchar)
              pnid = &n.lokid;
          else if(c > n.splitchar)
              pnid = &n.hikid;
          else //(*s == p->splitchar) 
          {
              if (*s++ == 0)
                  return n.value();
              pnid = &n.eqkid;
          } 
          nid = *pnid;
        }
        //nid = total_nodes();
        for (;;) 
        {
            //assert(prevnid < total_nodes());
            if(pnid) *pnid = nodes.size();
            node n; 
            n.splitchar = to_char(*s);
            //nodes[t].eqkid = nodes.push(n);
            nid = nodes.push(n);
            if (*s++ == 0) 
            {
              const CHARTYPE *ts = strings.size() < total_statics? str:  str_dup(str);
              strings.push( ts );
              nodes[nid].value(strings.size());
              return strings.size();
            }
            pnid = &nodes[nid].eqkid;
          }
    }

    uint  operator[] ( const CHARTYPE* s ) 
    {
      return insert(s);
    }

    const char* operator()(uint sym)  
    {
      if( sym && sym <= uint(strings.size()) )
        return strings[sym - 1];
      assert(0);
      return 0;
    }

    uint size() const { return strings.size(); }

    static inline CHARTYPE to_char(CHARTYPE c)
    {
      if(case_insensitive)
        return tolower(c);
      else
        return c;
    }

    // returns n[1..max_no] or zero if not found
    uint search(const CHARTYPE *s) const
    {   
      node_index nid = 0; // root index;
      while (nid < total_nodes()) 
      {
         const node& n = nodes[nid];
         CHARTYPE  c = to_char(*s);
         if (c < n.splitchar)
             nid = n.lokid;
         else if(c > n.splitchar)
             nid = n.hikid;
         else //(*s == p->splitchar) 
         {
            if (*s++ == 0)
                return n.value();
            nid = n.eqkid;
         } 
      }
      return 0;
    }
    // partial match search
    // e.g. 
    size_t partial_match_search(const CHARTYPE *s, array<uint>& result_set ) const
    {
      partial_match_search(0,s,result_set);
      return result_set.size();
    }

    //
    // neighbour search 
    //
    // 'd' is Hamming distance of a query word: 
    //      A measure of the difference between two strings, 
    //      expressed by the number of characters that need to be changed to 
    //      obtain one from the other. 
    //      E.g., 0101 and 0110 has a Hamming distance of two 
    //      whereas "Butter" and "ladder" are four characters apart.
    //
    
    size_t neighbour_search(const CHARTYPE *s, array<uint>& result_set, int d) const
    {
      neighbour_search(0, s, result_set, d);
      return result_set.size();
    }

 //protected:
    struct node 
    {
       CHARTYPE       splitchar;
       node_index lokid, eqkid, hikid;
       node():splitchar(0), lokid(-1), eqkid(-1), hikid(-1){}
       uint value() const 
       {
         assert(splitchar == 0);
         return (uint) eqkid;
       }
       void value(uint v)  
       {
         assert(splitchar == 0);
         eqkid = (node_index)v;
       }
    };
    array<node>   nodes;
    array<const CHARTYPE*>  strings;
    int           total_statics;

    size_t      total_nodes() const { return (uint) nodes.size(); }


    node_index  insert(const CHARTYPE *str, node_index nid, const CHARTYPE *s)
    { 
       CHARTYPE c = to_char(*s);
       if (nid >= total_nodes()) 
       {
            node n; n.splitchar = c;
            nodes.push(n);
            nid = nodes.last_index(); 
       }
       const node& n = nodes[nid];
       if (c < n.splitchar)
          nodes[nid].lokid = insert(str,n.lokid, s);
       else if(c > n.splitchar)
          nodes[nid].hikid = insert(str,n.hikid, s);
       else //*s == p->splitchar 
       {
          if (c == 0)
          //"...we'll exploit the fact that a node with a 
          // null splitchar cannot have an eqkid, 
          // and store the data in that field."
          {
              assert(nodes[nid].value() == ushort(-1));
              const node& t = nodes[nid]; 
              const CHARTYPE *ts = strings.size() < total_statics? str:  strdup(str);
              strings.push( ts );
              nodes[nid].value(strings.size());
          }
          else
              nodes[nid].eqkid = insert(str,n.eqkid, ++s);
       }
       return nid;
    }

    //partial match search
    void partial_match_search(node_index nid, const CHARTYPE *s, array<uint>& result_set ) const 
    {    
         if (nid >= total_nodes()) 
           return;
         const node& n = nodes[nid];
         CHARTYPE c = to_char(*s);
         if (c == any_one_char || c < n.splitchar)
            partial_match_search(n.lokid, s, result_set);
         if (c == any_one_char || c == n.splitchar)
            if (n.splitchar && c)
                partial_match_search(n.eqkid, s+1, result_set);
         if (c == 0 && n.splitchar == 0)
                result_set.push( (uint) n.value());
         if (c == any_one_char || c > n.splitchar)
                partial_match_search(n.hikid, s, result_set);
    }

    static int  str_length(const CHARTYPE* s)
    {
      const CHARTYPE* ps = s; while(*s) ++s; return s - ps;
    }

    void neighbour_search(node_index nid, const CHARTYPE *s, array<uint>& result_set, int d) const
    {   
        if (nid >= total_nodes() || d < 0) return;

        const node& n = nodes[nid];      
        CHARTYPE c = to_char(*s);

        if (d > 0 || c < n.splitchar)
            neighbour_search(n.lokid, s, result_set, d);
        if (n.splitchar == 0) 
        {
           if ( str_length(s) <= d)
              result_set.push(n.value());
        } 
        else
           neighbour_search(n.eqkid, *s ? s+1:s, result_set, (c == n.splitchar) ? d:d-1 );
        if (d > 0 || c > n.splitchar)
           neighbour_search(n.hikid, s, result_set, d);
    }


  };



}

#endif
