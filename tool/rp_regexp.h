#ifndef __RP_REGEXP_H__
#define __RP_REGEXP_H__

// Rob Pike RE package definitions

#include <stdlib.h>
#include <string.h>

namespace tool 
{
  namespace reimpl
  {
    typedef unsigned int unicode_cp; // UNICODE code point - 21-bit value
    typedef wchar_t rechar; // rechar is an utf-16 code unit here 
    enum
    {
      unicode_cp_self	= 0xD800,	  /* rune and UTF-16 sequences are the same (<) */
      unicode_cp_max	= 0x10FFFF,	/* maximum rune value */
    };

    struct Resub;
    struct Reclass;
    struct Reinst;
    struct Reprog;

    struct ReError
    {
      rechar msg[128];
      ReError(const rechar* s) 
      { 
        wcsncpy(msg,s,128); 
      }
    };

    /*
     *	Sub expression matches
     */
    struct Resub
    {
	    const rechar *sp;
	    const rechar *ep;
    };

    /*
     *	character class, each pair of rune's defines a range
     */
    struct Reclass{
	    unicode_cp	*end;
	    unicode_cp	spans[64];
    };

    typedef bool unicode_cp_class(unicode_cp c);

    /*
     *	Machine instructions
     */
    struct Reinst{
	    int	type;
	    union	{
		    Reclass*    cp;		      // class pointer 
        unicode_cp_class* cfp;  // unicode_cp_class function pointer
		    unicode_cp  r;		      // character
		    int	        subid;      // sub-expression id for RBRA and LBRA 
		    Reinst*     right;      // right child of OR 
	    } u1;
	    union {	// regexp relies on these two being in the same union 
		    Reinst *left;		// left child of OR
		    Reinst *next;		// next instruction for CAT & LBRA
	    } u2;
    };

    /*
     *	Reprogram definition
     */
    struct Reprog
    {
	    Reinst* startinst;	/* start pc */
	    Reclass	clazz[32];	/* .data */
	    Reinst	firstinst[5];	/* .text */
    };

    extern Reprog	*regcomp(const wchar_t*);
    extern Reprog	*regcomplit(const wchar_t*);
    extern Reprog	*regcompnl(const wchar_t*);
    extern void	regerror(const wchar_t*);
    extern int	regexec(Reprog*, const wchar_t*, Resub*, int);
    extern void	regsub(const wchar_t*, wchar_t*, int, Resub*, int);
    extern void	regfree(Reprog*);
  }
}

#endif
