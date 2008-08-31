//|
//|
//| Copyright (c) 2001-2005
//| Andrew Fedoniouk - andrew@terrainformatica.com
//|
//| Henry Spencer's RegExp
//|
//|



/*
* regcomp and regexec -- regsub and FAIL are elsewhere
* @(#)regexp.c	1.3 of 18 April 87
*
*	Copyright (c) 1986 by University of Toronto.
*	Written by Henry Spencer.  Not derived from licensed software.
*
*	Permission is granted to anyone to use this software for any
*	purpose on any computer system, and to redistribute it freely,
*	subject to the following restrictions:
*
*	1. The author is not responsible for the consequences of use of
*		this software, no matter how awful, even if they arise
*		from defects in it.
*
*	2. The origin of this software must not be misrepresented, either
*		by explicit claim or by omission.
*
*	3. Altered versions must be plainly marked as such, and must not
*		be misrepresented as being the original software.
*
* Beware that some of this code is subtly aware of the way operator
* precedence is structured in regular expressions.  Serious changes in
* regular-expression syntax might require a total rethink.
*/
#include <stdio.h>
#include <malloc.h>
#include <string.h>

#include "tl_regexp.h"

namespace tool
{
  /*
  * The "internal use only" fields in regexp.h are present to pass info from
  * compile to execute that permits the execute phase to run lots faster on
  * simple cases.  They are:
  *
  * regstart	char that must begin a match; '\0' if none obvious
  * reganch	is the match anchored (at beginning-of-line only)?
  * regmust	string (pointer into program) that match must include, or NULL
  * regmlen	length of regmust string
  *
  * Regstart and reganch permit very fast decisions on suitable starting points
  * for a match, cutting down the work a lot.  Regmust permits fast rejection
  * of lines that cannot possibly match.  The regmust tests are costly enough
  * that regcomp() supplies a regmust only if the r.e. contains something
  * potentially expensive (at present, the only such thing detected is * or +
  * at the start of the r.e., which can involve a lot of backup).  Regmlen is
  * supplied because the test in regexec() needs it and regcomp() is computing
  * it anyway.
  */

  /*
  * Structure for regexp "program".  This is essentially a linear encoding
  * of a nondeterministic finite-state machine (aka syntax charts or
  * "railroad normal form" in parsing technology).  Each node is an opcode
  * plus a "next" pointer, possibly plus an operand.  "Next" pointers of
  * all nodes except BRANCH implement concatenation; a "next" pointer with
  * a BRANCH on both ends of it is connecting two alternatives.  (Here we
  * have one of the subtle syntax dependencies:  an individual BRANCH (as
  * opposed to a collection of them) is never concatenated with anything
  * because of operator precedence.)  The operand of some types of node is
  * a literal string; for others, it is a node leading into a sub-FSM.  In
  * particular, the operand of a BRANCH node is the first node of the branch.
  * (NB this is *not* a tree structure:  the tail of the branch connects
  * to the thing following the set of BRANCHes.)  The opcodes are:
  */

#if !defined (_WIN32)
#define MAGIC	char(0234)
#else
#define	MAGIC	char(0234)
#endif

  /* definition	number	opnd?	meaning */
#define	END	0	/* no	End of program. */
#define	BOL	1	/* no	Match "" at beginning of line. */
#define	EOL	2	/* no	Match "" at end of line. */
#define	ANY	3	/* no	Match any one character. */
#define	ANYOF	4	/* str	Match any character in this string. */
#define	ANYBUT	5	/* str	Match any character not in this string. */
#define	BRANCH	6	/* node	Match this alternative, or the next... */
#define	BACK	7	/* no	Match "", "next" ptr points backward. */
#define	EXACTLY	8	/* str	Match this string. */
#define	NOTHING	9	/* no	Match empty string. */
#define	STAR	10	/* node	Match this (simple) thing 0 or more times. */
#define	PLUS	11	/* node	Match this (simple) thing 1 or more times. */
#define	OPEN	20	/* no	Mark this point in input as start of #n. */
  /*	OPEN+1 is number 1, etc. */
#define	CLOSE	30	/* no	Analogous to OPEN. */

  /*
  * Opcode notes:
  *
  * BRANCH	The set of branches constituting a single choice are hooked
  *		together with their "next" pointers, since precedence prevents
  *		anything being concatenated to any individual branch.  The
  *		"next" pointer of the last BRANCH in a choice points to the
  *		thing following the whole choice.  This is also where the
  *		final "next" pointer of each individual branch points; each
  *		branch starts with the operand node of a BRANCH node.
  *
  * BACK		Normal "next" pointers all implicitly point forward; BACK
  *		exists to make loop structures possible.
  *
  * STAR,PLUS	'?', and complex '*' and '+', are implemented as circular
  *		BRANCH structures using BACK.  Simple cases (one character
  *		per match) are implemented with STAR and PLUS for speed
  *		and to minimize recursive plunges.
  *
  * OPEN,CLOSE	...are numbered at compile time.
  */

  /*
  * A node is one char of opcode followed by two chars of "next" pointer.
  * "Next" pointers are stored as two 8-bit pieces, high order first.  The
  * value is a positive offset from the opcode of the node containing it.
  * An operand, if any, simply follows the node.  (Note that much of the
  * code generation knows about this implicit relationship.)
  *
  * Using two bytes for the "next" pointer is vast overkill for most things,
  * but allows patterns to get big without disasters.
  */
#define	OP(p)	(*(p))
#define	NEXT(p)	(((*((p)+1)&0377)<<8) + (*((p)+2)&0377))
#define	OPERAND(p)	((p) + 3)

  /*
  * See regmagic.h for one further detail of program structure.
  */

  /*
  * Utility definitions.
  */
#ifndef CHARBITS
#define	UCHARAT(p)	((int)*(unsigned char *)(p))
#else
#define	UCHARAT(p)	((int)*(p)&CHARBITS)
#endif

#ifndef PLATFORM_WINCE
#define	FAIL(m)	throw regexp::error(m);
#else
#define	FAIL(m)	void(0)
#endif


#define	ISMULT(c)	((c) == '*' || (c) == '+' || (c) == '?')
#define	META	"^$.[()|?+*\\"

  /*
  * Flags to be passed up and down.
  */
#define	HASWIDTH	01	/* Known never to match null string. */
#define	SIMPLE		02	/* Simple enough to be STAR/PLUS operand. */
#define	SPSTART		04	/* Starts with * or +. */
#define	WORST		0	/* Worst case. */

  /*
  - compile - compile a regular expression into internal code
  *
  * We can't allocate space until we know how big the compiled form will be,
  * but we can't compile it (and thus know how big it is) until we've got a
  * place to put the code.  So we cheat:  we compile it twice, once with code
  * generation turned off and size counting turned on, and once "for real".
  * This also means that we don't allocate space until we are sure that the
  * thing really will compile successfully, and we never have to move the
  * code and thus invalidate pointers into it.  (Note that it has to be in
  * one piece because free() must be able to free it all.)
  *
  * Beware that the optimization-preparation code in here knows about some
  * of the structure of the compiled regexp.
  */
  bool
    regexp::compile ( const char* exp )
  {
    clean();

    char *scan;
    char *longest;
    int len;
    int flags;

    if ( exp == NULL )
      FAIL ( "NULL argument" );

    compiler_vars cvars;

    // First pass: determine size, legality.
    cvars.regparse = const_cast<char *> ( exp );
    cvars.regnpar = 1;
    cvars.regsize = 0L;
    cvars.regcode = &regdummy;
    regc ( cvars, MAGIC );
    if ( reg ( cvars, 0, &flags ) == NULL )
      return false;

    // Small enough for pointer-storage convention?
    if ( cvars.regsize >= 32767L )		/* Probably could be 65535L. */
      FAIL ( "regexp too big" );

    // Allocate space.
    program = new char [ cvars.regsize ];
    if ( program == NULL )
      FAIL ( "out of space" );

    // Second pass: emit code.
    cvars.regparse = const_cast<char *> ( exp );
    cvars.regnpar = 1;
    cvars.regcode = program;
    regc ( cvars, MAGIC );
    if ( reg ( cvars, 0, &flags ) == NULL )
      return false;

    // Dig out information for optimizations.
    regstart = '\0';	    // Worst-case defaults.
    reganch = 0;
    regmust = NULL;
    regmlen = 0;
    scan = program + 1;			// First BRANCH.
    if ( OP ( regnext ( scan ) ) == END )
    {
      // Only one top-level choice.
      scan = OPERAND ( scan );

      //* Starting-point info. */
      if ( OP ( scan ) == EXACTLY )
        regstart = *OPERAND ( scan );
      else if ( OP ( scan ) == BOL )
        reganch++;

      /*
      * If there's something expensive in the r.e., find the
      * longest literal string that must appear and make it the
      * regmust.  Resolve ties in favor of later strings, since
      * the regstart check works with the beginning of the r.e.
      * and avoiding duplication strengthens checking.  Not a
      * strong reason, but sufficient in the absence of others.
      */

      if ( flags & SPSTART )
      {
        longest = NULL;
        len = 0;
        for ( ; scan != NULL; scan = regnext ( scan ) )
          if ( OP ( scan ) == EXACTLY
               && int ( strlen ( OPERAND ( scan ) ) ) >= len )
          {
            longest = OPERAND ( scan );
            len = int(strlen ( OPERAND ( scan ) ));
          }
        regmust = const_cast<char *> ( longest );
        regmlen = len;
      }
    }

    return true;
  }


  /*
  - reg - regular expression, i.e. main body or parenthesized thing
  *
  * Caller must absorb opening parenthesis.
  *
  * Combining parenthesis handling with the base level of regular expression
  * is a trifle forced, but the need to tie the tails of the branches to what
  * follows makes it hard to avoid.
  */
  char *
    regexp::reg ( compiler_vars& cvars,
                  int paren, int* flagp ) /* paren - Parenthesized? */
  {
    char *ret;
    char *br;
    char *ender;
    int parno = 0;
    int flags;

    *flagp = HASWIDTH;	// Tentatively.

    // Make an OPEN node, if parenthesized.
    if ( paren )
    {
      if ( cvars.regnpar >= NSUBEXP )
        FAIL ( "too many ()" );
      parno = cvars.regnpar;
      cvars.regnpar++;
      ret = regnode ( cvars, OPEN + parno );
    }
    else
      ret = NULL;

    // Pick up the branches, linking them together.
    br = regbranch ( cvars, &flags );

    if ( br == NULL )
      return ( NULL );

    if ( ret != NULL )
      regtail ( cvars, ret, br ); // OPEN -> first.
    else
      ret = br;

    if ( !( flags & HASWIDTH ) )
      *flagp &= ~HASWIDTH;
    *flagp |= flags & SPSTART;
    while ( *cvars.regparse == '|' )
    {
      cvars.regparse++;
      br = regbranch ( cvars, &flags );
      if ( br == NULL )
        return 0;
      regtail ( cvars, ret, br );   // BRANCH -> BRANCH.
      if ( !( flags & HASWIDTH ) )
        *flagp &= ~HASWIDTH;
      *flagp |= flags & SPSTART;
    }

    // Make a closing node, and hook it on the end.
    ender = regnode ( cvars, ( paren ) ? CLOSE + parno : END );
    regtail ( cvars, ret, ender );

    // Hook the tails of the branches to the closing node.
    for ( br = ret; br != NULL; br = regnext ( br ) )
      regoptail ( cvars, br, ender );

    // Check for proper termination.
    if ( paren && *cvars.regparse++ != ')' )
    {
      FAIL ( "unmatched ()" );
    }
    else if ( !paren && *cvars.regparse != '\0' )
    {
      if ( *cvars.regparse == ')' )
      {
        FAIL ( "unmatched ()" );
      }
      else
        FAIL ( "junk on end" );   // "Can't happen"
      // NOTREACHED
    }
    return ( ret );
  }

  /*
  - regbranch - one alternative of an | operator
  *
  * Implements the concatenation operator.
  */
  char *
    regexp::regbranch ( compiler_vars& cvars, int* flagp )
  {
    register char *ret;
    register char *chain;
    register char *latest;
    int flags;

    *flagp = WORST;		// Tentatively.

    ret = regnode ( cvars, BRANCH );
    chain = NULL;
    while ( *cvars.regparse != '\0' && *cvars.regparse != '|' && *cvars.regparse != ')' )
    {
      latest = regpiece ( cvars, &flags );
      if ( latest == NULL )
        return ( NULL );
      *flagp |= flags & HASWIDTH;
      if ( chain == NULL )	// First piece.
        *flagp |= flags & SPSTART;
      else
        regtail ( cvars, chain, latest );
      chain = latest;
    }
    if ( chain == NULL )  // Loop ran zero times.
      (void) regnode ( cvars, NOTHING );

    return ( ret );
  }


  /*
  - regpiece - something followed by possible [*+?]
  *
  * Note that the branching code sequences used for ? and the general cases
  * of * and + are somewhat optimized:  they use the same NOTHING node as
  * both the endmarker for their branch list and the body of the last branch.
  * It might seem that this node could be dispensed with entirely, but the
  * endmarker role is not redundant.
  */
  char *
    regexp::regpiece ( compiler_vars& cvars, int* flagp )
  {
    register char *ret;
    register char op;
    register char *next;
    int flags;

    ret = regatom ( cvars, &flags );
    if ( ret == NULL )
      return ( NULL );

    op = *cvars.regparse;
    if ( !ISMULT ( op ) )
    {
      *flagp = flags;
      return ( ret );
    }

    if ( !( flags & HASWIDTH ) && op != '?' )
      FAIL ( "*+ operand could be empty" );

    *flagp = ( op != '+' ) ? ( WORST | SPSTART ) : ( WORST | HASWIDTH );

    if ( op == '*' && ( flags & SIMPLE ) )
      reginsert ( cvars, STAR, ret );
    else if ( op == '*' )
    {
      /* Emit x* as (x&|), where & means "self". */
      reginsert ( cvars, BRANCH, ret );                     /* Either x */
      regoptail ( cvars, ret, regnode ( cvars, BACK ) );    /* and loop */
      regoptail ( cvars, ret, ret );                        /* back     */
      regtail   ( cvars, ret, regnode ( cvars, BRANCH ) );  /* or       */
      regtail   ( cvars, ret, regnode ( cvars, NOTHING) );  /* null.    */
    }
    else if ( op == '+' && ( flags & SIMPLE ) )
      reginsert ( cvars, PLUS, ret );
    else if ( op == '+' )
    {
      /* Emit x+ as x(&|), where & means "self". */
      next = regnode ( cvars, BRANCH );                   /* Either */
      regtail ( cvars, ret, next );
      regtail ( cvars, regnode ( cvars,BACK ), ret );     /* loop back */
      regtail ( cvars, next, regnode ( cvars,BRANCH ) );  /* or */
      regtail ( cvars, ret, regnode ( cvars,NOTHING ) );  /* null. */
    }
    else if ( op == '?' )
    {
      /* Emit x? as (x|) */
      reginsert ( cvars, BRANCH, ret );                     /* Either x */
      regtail   ( cvars, ret, regnode ( cvars, BRANCH ) );  /* or */
      next = regnode ( cvars, NOTHING );                    /* null. */
      regtail   ( cvars, ret, next );
      regoptail ( cvars, ret, next );
    }
    cvars.regparse++;
    if ( ISMULT ( *cvars.regparse ) )
      FAIL ( "nested *?+" );

    return ( ret );
  }


  /*
  - regatom - the lowest level
  *
  * Optimization:  gobbles an entire sequence of ordinary characters so that
  * it can turn them into a single node, which is smaller to store and
  * faster to run.  Backslashed characters are exceptions, each becoming a
  * separate node; the code is simpler that way and it's not worth fixing.
  */
  char *
    regexp::regatom ( compiler_vars& cvars, int* flagp )
  {
    register char *ret;
    int flags;

    *flagp = WORST;		/* Tentatively. */

    switch ( *cvars.regparse++ )
    {
    case '^':
      ret = regnode ( cvars, BOL );
      break;
    case '$':
      ret = regnode ( cvars, EOL );
      break;
    case '.':
      ret = regnode ( cvars, ANY );
      *flagp |= HASWIDTH | SIMPLE;
      break;
    case '[':
      {
        int rclass;
        int classend;

        if ( *cvars.regparse == '^' )
        {
          /* Complement of range. */
          ret = regnode ( cvars, ANYBUT );
          cvars.regparse++;
        }
        else
          ret = regnode ( cvars, ANYOF );
        if ( *cvars.regparse == ']' || *cvars.regparse == '-' )
          regc ( cvars, *cvars.regparse++ );
        while ( *cvars.regparse != '\0' && *cvars.regparse != ']' )
        {
          if ( *cvars.regparse == '-' )
          {
            cvars.regparse++;
            if ( *cvars.regparse == ']' || *cvars.regparse == '\0' )
              regc ( cvars, '-' );
            else
            {
              rclass   = UCHARAT ( cvars.regparse - 2 ) + 1;
              classend = UCHARAT ( cvars.regparse );
              if ( rclass > classend + 1 )
                FAIL ( "invalid [] range" );
              for ( ; rclass <= classend; rclass++ )
                regc ( cvars, rclass );
              cvars.regparse++;
            }
          }
          else
            regc ( cvars, *cvars.regparse++ );
        }
        regc ( cvars, '\0' );
        if ( *cvars.regparse != ']' )
          FAIL ( "unmatched []" );
        cvars.regparse++;
        *flagp |= HASWIDTH | SIMPLE;
      }
      break;
    case '(':
      ret = reg ( cvars, 1, &flags );
      if ( ret == NULL )
        return ( NULL );
      *flagp |= flags & ( HASWIDTH | SPSTART );
      break;
    case '\0':
    case '|':
    case ')':
      FAIL ( "internal urp" );  /* Supposed to be caught earlier. */
      break;
    case '?':
    case '+':
    case '*':
      FAIL ( "?+* follows nothing" );
      break;
    case '\\':
      if ( *cvars.regparse == '\0' )
        FAIL ( "trailing \\" );
      ret = regnode ( cvars, EXACTLY );
      regc ( cvars, *cvars.regparse++ );
      regc ( cvars, '\0' );
      *flagp |= HASWIDTH | SIMPLE;
      break;
    default:
      {
        register int len;
        register char ender;

        cvars.regparse--;
        len = int(strcspn ( cvars.regparse, META ));
        if ( len <= 0 )
          FAIL ( "internal disaster" );
        ender = *( cvars.regparse + len );
        if ( len > 1 && ISMULT ( ender ) )
          len--;		/* Back off clear of ?+* operand. */
        *flagp |= HASWIDTH;
        if ( len == 1 )
          *flagp |= SIMPLE;
        ret = regnode ( cvars, EXACTLY );
        while ( len > 0 )
        {
          regc ( cvars, *cvars.regparse++ );
          len--;
        }
        regc ( cvars, '\0' );
      }
      break;
    }

    return ( ret );
  }


  /*
  - regnode - emit a node
  */
  char *	/* Location. */
    regexp::regnode ( compiler_vars& cvars, char op )
  {
    register char *ret;
    register char *ptr;

    ret = cvars.regcode;
    if ( ret == &regdummy )
    {
      cvars.regsize += 3;
      return ( ret );
    }

    ptr = ret;
    *ptr++ = op;
    *ptr++ = '\0';		/* Null "next" pointer. */
    *ptr++ = '\0';
    cvars.regcode = ptr;

    return ( ret );
  }


  /*
  - regc - emit (if appropriate) a byte of code
  */
  void
    regexp::regc ( compiler_vars& cvars, char b )
  {
    if ( cvars.regcode != &regdummy )
      *cvars.regcode++ = b;
    else
      cvars.regsize++;
  }


  /*
  - reginsert - insert an operator in front of already-emitted operand
  *
  * Means relocating the operand.
  */
  void
    regexp::reginsert ( compiler_vars& cvars, char op, char* opnd )
  {
    register char *src;
    register char *dst;
    register char *place;

    if ( cvars.regcode == &regdummy )
    {
      cvars.regsize += 3;
      return;
    }

    src = cvars.regcode;
    cvars.regcode += 3;
    dst = cvars.regcode;
    while ( src > opnd )
      *--dst = *--src;

    place    = opnd;		/* Op node, where operand used to be. */
    *place++ = op;
    *place++ = '\0';
    *place++ = '\0';
  }


  /*
  - regtail - set the next-pointer at the end of a node chain
  */
  void
    regexp::regtail ( compiler_vars& cvars, char* p, char* val )
  {
    register char *scan;
    register char *temp;
    register int offset;

    if ( p == &regdummy )
      return;

    /* Find last node. */
    scan = p;
    for (;;)
    {
      temp = regnext ( scan );
      if ( temp == NULL )
        break;
      scan = temp;
    }

    if ( OP ( scan ) == BACK )
      offset = int(scan - val);
    else
      offset = int(val - scan);
    *( scan + 1 ) = ( offset >> 8 ) & 0377;
    *( scan + 2 ) = offset & 0377;
  }


  /*
  - regoptail - regtail on operand of first argument; nop if operandless
  */
  void
    regexp::regoptail ( compiler_vars& cvars, char* p,  char* val )
  {
    /* "Operandless" and "op != BRANCH" are synonymous in practice. */
    if ( p == NULL || p == &regdummy || OP ( p ) != BRANCH )
      return;
    regtail ( cvars, OPERAND ( p ), val );
  }


  /*
  * regexec and friends
  */

#ifdef DEBUG
  int   regnarrate = 0;
  void  regdump ( regexp* r );
  char* regprop ( char* op  );
#endif


  /*
  - regexec - match a regexp against a string
  */
  bool
    regexp::exec ( const char* string )
  {
    char *s;
    source = const_cast<char*>(string);
    // Be paranoid...
    if ( string == NULL )
    {
      FAIL ( "NULL parameter" );
      return false;
    }

    exec_vars evars;

    // Check validity of program.
    if ( UCHARAT ( program ) != (unsigned char) MAGIC )
    {
      FAIL ( "corrupted program" );
      return false;
    }

    // If there is a "must appear" string, look for it.
    if ( regmust != NULL )
    {
      s = const_cast<char *> ( string );
      while ( ( s = strchr ( s, regmust [ 0 ] ) ) != NULL )
      {
        if ( strncmp ( s, regmust, regmlen ) == 0 )
          break;	// Found it.
        s++;
      }
      if ( s == NULL )  // Not present.
        return false;
    }

    // Mark beginning of line for ^ .
    evars.regbol = const_cast<char *> ( string );

    // Simplest case:  anchored match need be tried only once.
    if ( reganch )
      return ( regtry ( evars, const_cast<char *> ( string ) ) != 0 );

    // Messy cases:  unanchored match.
    s = const_cast<char *> ( string );
    if ( regstart != '\0' )
      // We know what char it must start with.
      while ( ( s = strchr ( s, regstart ) ) != NULL )
      {
        if ( regtry ( evars, s ) )
          return true;
        s++;
      }
      else
       // We don't -- general case.
        do
        {
          if ( regtry ( evars, s ) )
            return true;
        }
        while ( *s++ != '\0' );

    /* Failure. */
    return false;
  }

  /*
  - regtry - try match at specific point
  */
  int			/* 0 failure, 1 success */
    regexp::regtry ( exec_vars& evars, char* string )
  {
    int i;
    char **sp;
    char **ep;

    evars.reginput  = const_cast<char *> ( string );
    evars.regstartp = startp;
    evars.regendp   = endp;

    sp = startp;
    ep = endp;
    for ( i = NSUBEXP; i > 0; i-- )
    {
      *sp++ = NULL;
      *ep++ = NULL;
    }
    if ( regmatch ( evars, program + 1 ) )
    {
      startp [ 0 ] = const_cast<char *> ( string );
      endp   [ 0 ] = evars.reginput;
      return ( 1 );
    }
    else
      return ( 0 );
  }


  /*
  - regmatch - main matching routine
  *
  * Conceptually the strategy is simple:  check to see whether the current
  * node matches, call self recursively to see whether the rest matches,
  * and then act accordingly.  In practice we make some effort to avoid
  * recursion, in particular by going through "ordinary" nodes (that don't
  * need to know whether the rest of the match failed) by a loop instead of
  * by recursion.
  */
  int			/* 0 failure, 1 success */
    regexp::regmatch ( exec_vars& evars, char* prog )
  {
    char *scan;	/* Current node. */
    char *next; /* Next node. */

    scan = prog;
    while ( scan != NULL )
    {
      next = regnext ( scan );

      switch ( OP ( scan ) )
      {
      case BOL:
        if ( evars.reginput != evars.regbol )
          return ( 0 );
        break;
      case EOL:
        if ( *evars.reginput != '\0' )
          return ( 0 );
        break;
      case ANY:
        if ( *evars.reginput == '\0' )
          return ( 0 );
        evars.reginput++;
        break;
      case EXACTLY:
        {
          int len;
          const char *opnd;

          opnd = OPERAND ( scan );
          /* Inline the first character, for speed. */
          if ( *opnd != *evars.reginput )
            return ( 0 );
          len = int(strlen ( opnd ) );
          if ( len > 1 && strncmp ( opnd, evars.reginput, len ) != 0 )
            return ( 0 );
          evars.reginput += len;
        }
        break;
      case ANYOF:
        if ( *evars.reginput == '\0' ||
             strchr ( OPERAND ( scan ), *evars.reginput ) == NULL )
          return ( 0 );
        evars.reginput++;
        break;
      case ANYBUT:
        if ( *evars.reginput == '\0' ||
             strchr ( OPERAND ( scan ), *evars.reginput ) != NULL )
          return ( 0 );
        evars.reginput++;
        break;
      case NOTHING:
        break;
      case BACK:
        break;
      case OPEN + 1:
      case OPEN + 2:
      case OPEN + 3:
      case OPEN + 4:
      case OPEN + 5:
      case OPEN + 6:
      case OPEN + 7:
      case OPEN + 8:
      case OPEN + 9:
        {
          register int no;
          register char *save;

          no = OP ( scan ) - OPEN;
          save = evars.reginput;

          if ( regmatch ( evars, next ) )
          {
            /*
            * Don't set startp if some later
            * invocation of the same parentheses
            * already has.
            */
            if ( evars.regstartp [ no ] == NULL )
              evars.regstartp [ no ] = save;
            return ( 1 );
          }
          else
            return ( 0 );
        }
        break;
      case CLOSE + 1:
      case CLOSE + 2:
      case CLOSE + 3:
      case CLOSE + 4:
      case CLOSE + 5:
      case CLOSE + 6:
      case CLOSE + 7:
      case CLOSE + 8:
      case CLOSE + 9:
        {
          register int no;
          register char *save;

          no = OP ( scan ) - CLOSE;
          save = evars.reginput;

          if ( regmatch ( evars, next ) )
          {
            /*
            * Don't set endp if some later
            * invocation of the same parentheses
            * already has.
            */
            if ( evars.regendp [ no ] == NULL )
              evars.regendp [ no ] = save;
            return ( 1 );
          }
          else
            return ( 0 );
        }
        break;
      case BRANCH:
        {
          register char *save;

          if ( OP ( next ) != BRANCH )  /* No choice. */
            next = OPERAND ( scan );    /* Avoid recursion. */
          else
          {
            do
            {
              save = evars.reginput;
              if ( regmatch ( evars, OPERAND ( scan ) ) )
                return ( 1 );
              evars.reginput = save;
              scan = regnext ( scan );
            }
            while ( scan != NULL && OP ( scan ) == BRANCH );
            return ( 0 );
            /* NOTREACHED */
          }
        }
        break;
      case STAR:
      case PLUS:
        {
          register char nextch;
          register int no;
          register char *save;
          register int min;

          /*
          * Lookahead to avoid useless match attempts
          * when we know what character comes next.
          */
          nextch = '\0';
          if ( OP ( next ) == EXACTLY )
            nextch = *OPERAND ( next );
          min = ( OP ( scan ) == STAR ) ? 0 : 1;
          save = evars.reginput;
          no = regrepeat ( evars, OPERAND ( scan ) );
          while ( no >= min )
          {
            /* If it could work, try it. */
            if ( nextch == '\0' || *evars.reginput == nextch )
              if ( regmatch ( evars, next ) )
            return ( 1 );
            /* Couldn't or didn't -- back up. */
            no--;
            evars.reginput = save + no;
          }
          return ( 0 );
        }
        break;
      case END:
        return ( 1 );	/* Success! */
        break;
      default:
        FAIL ( "memory corruption" );
        return ( 0 );
        break;
      }
      scan = next;
    }


    /*
    * We get here only if there's trouble -- normally "case END" is
    * the terminating point.
    */
    FAIL ( "corrupted pointers" );
    return ( 0 );
  }


  /*
  - regrepeat - repeatedly match something simple, report how many
  */
  int
    regexp::regrepeat ( exec_vars& evars, char* p )
  {
    int count = 0;
    const char *scan;
    const char *opnd;

    scan = evars.reginput;
    opnd = OPERAND ( p );
    switch ( OP ( p ) )
    {
    case ANY:
      count = int(strlen( scan ));
      scan += count;
      break;
    case EXACTLY:
      while ( *opnd == *scan )
      {
        count++;
        scan++;
      }
      break;
    case ANYOF:
      while ( *scan != '\0' && strchr ( opnd, *scan ) != NULL )
      {
        count++;
        scan++;
      }
      break;
    case ANYBUT:
      while ( *scan != '\0' && strchr ( opnd, *scan ) == NULL )
      {
        count++;
        scan++;
      }
      break;
    default:      /* Oh dear.  Called inappropriately. */
      FAIL ( "internal foulup" );
      count = 0;  /* Best compromise. */
      break;
    }
    evars.reginput = const_cast<char *> ( scan );

    return ( count );
  }


  /*
  - regnext - dig the "next" pointer out of a node
  */
  char *
    regexp::regnext ( char* p )
  {
    int offset;

    if ( p == &regdummy )
      return ( NULL );

    offset = NEXT ( p );
    if ( offset == 0 )
      return ( NULL );

    if ( OP ( p ) == BACK )
      return ( p - offset );
    else
      return ( p + offset );
  }


  int
    regexp::count()
  {
    for ( int i = 0; i < NSUBEXP; i++ )
      if (startp [ i ] == 0 || endp [ i ] == 0 )
        return i;
    return 0;
  }

  string
    regexp::operator[] ( int index )
  {
    if ( index < 0 || index >= NSUBEXP )
      return string();
    if ( startp [ index ] != 0 && endp [ index ] != 0)
      return string ( startp [ index ], int(endp [ index ] - startp [ index ]));
    return string();
  }

  int
    regexp::start( int index )
  {
    if ( index < 0 || index >= NSUBEXP )
      return 0;
    return int(startp [ index ] - source);
  }
  int
    regexp::length( int index )
  {
    if ( index < 0 || index >= NSUBEXP )
      return 0;
    return int(endp [ index ] - startp [ index ]);
  }

  void
    regexp::clean()
  {
    source = 0;
    for ( int i = 0; i < NSUBEXP; i++ )
      startp [ i ] = endp [ i ] = 0;
    delete[] program;
  }


  regexp::regexp(): program ( 0 )
  {
    clean();
  }

  regexp::~regexp()
  {
    clean();
  }


  char regexp::regdummy = 0;


  //|
  //|  find all possible sub-strings that matches with the regular expression
  //|
  bool
    regexp::split ( const char *str, array<string>& all_matches )
  {
    // clear up anything in the array
    all_matches.clear();
    const char *p = str;
    bool rc = false;  // assume no matches
    // match any sub-string
    while ( exec ( p ) )
    {
      rc = true;
      string s ( startp [ 0 ], int(endp [ 0 ] - startp [ 0 ]));
      all_matches.push ( s );
      p = endp [ 0 ];
      // check if we have reached the end of the string.
      if ( ( *p == 0 ) || ( *( p + 1 ) == 0 ) )
      {
        break;
      }
    }
    return rc;
  }


  /*
  - regsub - perform substitutions after a regexp match

  string regexp::subst ( const char* source )
  {
    char  *src;
    string dst;
    char   c;
    int    no;
    int    len;

    if ( source == NULL || dest == NULL )
    {
      FAIL ( "NULL parm to regsub" );
      return;
    }

    if ( UCHARAT ( program ) != MAGIC )
    {
      FAIL ( "damaged regexp fed to regsub" );
      return;
    }

    src = source;
    while ( ( c = *src++ ) != '\0' )
    {
      if ( c == '&' )
        no = 0;
      else if ( c == '\\' && '0' <= *src && *src <= '9' )
        no = *src++ - '0';
      else
        no = -1;

      if ( no < 0 )
      {
        // Ordinary character.
        if ( c == '\\' && ( *src == '\\' || *src == '&' ) )
          c = *src++;
        dst += c;
      }
      else if ( prog->startp [ no ] != NULL && prog->endp [ no ] != NULL )
      {
        len = prog->endp [ no ] - prog->startp [ no ];
        dst.add ( startp [ no ], len );
        //if ( len != 0 && *( dst - 1 ) == '\0' ) // strncpy hit NUL.
        //{
        //	FAIL ( "damaged match string" );
        //	return;
        //}
      }
    }
    return dst;
  }
  */


#ifdef DEBUG_REGEXP
  static char *regprop();


  void
    regdump ( regexp* r )
  {
    register char *s;
    register char op = EXACTLY;	/* Arbitrary non-END op. */
    register char *next;

    s = r->program + 1;
    while ( op != END )
    {
      /* While that wasn't END last time... */
      op = OP ( s );
      printf ( "%2d%s", s - r->program, regprop ( s ));	/* Where, what. */
      next = regnext ( s );
      if ( next == NULL )   /* Next ptr. */
        printf ( "(0)" );
      else
        printf ( "(%d)", ( s - r->program ) + ( next - s ) );
      s += 3;
      if ( op == ANYOF || op == ANYBUT || op == EXACTLY )
      {
        /* Literal string, where present. */
        while ( *s != '\0' )
        {
          putchar ( *s );
          s++;
        }
        s++;
      }
      putchar ( '\n' );
    }

    /* Header fields of interest. */
    if ( r->regstart != '\0' )
      printf ( "start `%c' ", r->regstart );
    if ( r->reganch )
      printf ( "anchored " );
    if ( r->regmust != NULL )
      printf ( "must have \"%s\"", r->regmust );
    printf ( "\n" );
  }


  /*
  - regprop - printable representation of opcode
  */
  static char *
    regprop ( char* op )
  {
    register char *p;
    static char buf [ 50 ];

    (void) strcpy ( buf, ":" );

    switch ( OP ( op ) )
    {
    case BOL:
      p = "BOL";
      break;
    case EOL:
      p = "EOL";
      break;
    case ANY:
      p = "ANY";
      break;
    case ANYOF:
      p = "ANYOF";
      break;
    case ANYBUT:
      p = "ANYBUT";
      break;
    case BRANCH:
      p = "BRANCH";
      break;
    case EXACTLY:
      p = "EXACTLY";
      break;
    case NOTHING:
      p = "NOTHING";
      break;
    case BACK:
      p = "BACK";
      break;
    case END:
      p = "END";
      break;
    case OPEN + 1:
    case OPEN + 2:
    case OPEN + 3:
    case OPEN + 4:
    case OPEN + 5:
    case OPEN + 6:
    case OPEN + 7:
    case OPEN + 8:
    case OPEN + 9:
      sprintf ( buf + strlen ( buf ), "OPEN%d", OP ( op ) - OPEN );
      p = NULL;
      break;
    case CLOSE + 1:
    case CLOSE + 2:
    case CLOSE + 3:
    case CLOSE + 4:
    case CLOSE + 5:
    case CLOSE + 6:
    case CLOSE + 7:
    case CLOSE + 8:
    case CLOSE + 9:
      sprintf ( buf + strlen ( buf ), "CLOSE%d", OP ( op ) - CLOSE );
      p = NULL;
      break;
    case STAR:
      p = "STAR";
      break;
    case PLUS:
      p = "PLUS";
      break;
    default:
      FAIL ( "corrupted opcode" );
      break;
    }
    if ( p != NULL )
      (void) strcat ( buf, p );
    return ( buf );
  }
#endif
};
