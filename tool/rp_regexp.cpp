/*
 *  The author of this software is Andrew Fedoniouk. 
 *
 *  This is UTF-16 (Windows) port of RE package of Rob Pike 
 *  that appears to be implemented as an NFA according to Russ Cox article
 *  http://swtch.com/~rsc/regexp/regexp1.html
 *  
 *  Original functionality is extended by \d,\D, etc. "escape macros" that are predefined
 *  character classes:

  \m - is alnum
  \M - is not alnum
  \a - is alpha
  \A - is not alpha
  \b - is blank
  \B - is not blank
  \c - is cntrl
  \C - is not cntrl
  \d - is digit
  \D - is not digit
  \g - is graph
  \G - is not graph
  \l - is lower
  \L - is not lower
  \p - is print
  \P - is not print
  \n - is punct
  \N - is not punct
  \s - is space
  \S - is not space
  \u - is upper
  \U - is not upper
  \x - is xdigit
  \X - is not xdigit
  \w - is blank
  \W - is not blank

  I also made all this thread safe - it does not use static objects as in original package.
  
  Character classification routines are based on ucdata (library of M. Leisher)

  All this is using now simplifed-C++ - a.k.a. C with classes. 

 */

/*
 * The authors of this software is Rob Pike.
 *		Copyright (c) 2002 by Lucent Technologies.
 * Permission to use, copy, modify, and distribute this software for any
 * purpose without fee is hereby granted, provided that this entire notice
 * is included in all copies of any software which is or includes a copy
 * or modification of this software and in all copies of the supporting
 * documentation for such software.
 * THIS SOFTWARE IS BEING PROVIDED "AS IS", WITHOUT ANY EXPRESS OR IMPLIED
 * WARRANTY.  IN PARTICULAR, NEITHER THE AUTHORS NOR LUCENT TECHNOLOGIES MAKE ANY
 * REPRESENTATION OR WARRANTY OF ANY KIND CONCERNING THE MERCHANTABILITY
 * OF THIS SOFTWARE OR ITS FITNESS FOR ANY PARTICULAR PURPOSE.
*/

#include "rp_regexp.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "ucdata/ucdata_lt.h"

namespace tool 
{
  namespace reimpl
  {

/*
 *  substitution list
 */
typedef unsigned short urechar;
#define nelem(x) (sizeof(x)/sizeof((x)[0]))

#define NSUBEXP 32
typedef struct Resublist	Resublist;
struct	Resublist
{
	Resub	m[NSUBEXP];
};

/* max character classes per program */
extern Reprog	RePrOg;
#define	NCLASS	(sizeof(RePrOg.clazz)/sizeof(Reclass))

/* max rune ranges per character class */
#define NCCRUNE	(sizeof(Reclass)/sizeof(unicode_cp))

/*
 * Actions and Tokens (Reinst types)
 *
 *	02xx are operators, value == precedence
 *	03xx are tokens, i.e. operands for operators
 */
enum REINST_TYPE 
{
  RUNE =		0177,
  OPERATOR=	0200,	/* Bitmask of all operators */
  START=		0200,	/* Start, used for marker on stack */
  RBRA=		  0201,	/* Right bracket, ) */
  LBRA=		  0202,	/* Left bracket, ( */
  OR=		    0203,	/* Alternation, | */
  CAT=		  0204,	/* Concatentation, implicit operator */
  STAR=		  0205,	/* Closure, * */
  PLUS=		  0206,	/* a+ == aa* */
  QUEST=		0207,	/* a? == a|nothing, i.e. 0 or 1 a's */
  ANY=		  0300,	/* Any character except newline, . */
  ANYNL=		0301,	/* Any character including newline, . */
  NOP=		  0302,	/* No operation, internal use only */
  BOL=		  0303,	/* Beginning of line, ^ */
  EOL=		  0304,	/* End of line, $ */
  CCLASS=		0305,	/* Character class, [] */
  NCCLASS=	0306,	/* Negated character class, [] */
  PDCLASS=	0307,	/* AF: Character class, \d,\w, etc. */
  NPDCLASS=	0311,	/* AF: Negated character class, \d,\w, etc. */
  END=		  0377	/* Terminate: match found */
};

/*
 *  regexec execution lists
 */
#define LISTSIZE	10
#define BIGLISTSIZE	(10*LISTSIZE)

struct Relist
{
	Reinst*		inst;	/* Reinstruction of the thread */
	Resublist	se;		/* matched subexpressions in this thread */
};

struct	Reljunk  /* RE execution environment */
{
	Relist*	relist[2];
	Relist*	reliste[2];
	int	    starttype;
	unicode_cp	  startchar;
	const rechar*	starts;
	const rechar*	eol;
};

static Relist*	_renewthread(Relist*, Reinst*, int, Resublist*);
static void	    _renewmatch(Resub*, int, Resublist*);
static Relist*	_renewemptythread(Relist*, Reinst*, int, const rechar*);

// converts one or two UTF-16 code units to unicode code point
int rechar2ucp(unicode_cp* r, const rechar* s)
{
  if( *s < 0xD800 || *s > 0xDBFF )
  {
    // not a surrogate pair
    *r = *s; 
    return 1; 
  }
  // it is a surrogate pair
  *r = ( *s - 0xD800 ) * 0x400 + (*(s+1) - 0xDC00 ) + 0x10000;
  return 2;
}

// Lookup of r (unicode code point) in sequence of UTF-16 code units.
// It is a functional equivalent of strchr()
static const rechar* ucp_pos_in_rechar_str(const rechar* s, unicode_cp r)
{
   //assert( r < 0x10FFFF ); // wrong value of UNICODE CP.
   rechar w2[2] = {0,0};
   unsigned nc = 1;
   if( r < 0x10000 ) w2[0] = rechar(r);
   else { w2[0] = 0xD800 + (r >> 10);  w2[1] = 0xDC00 | (r & 0x3FF); nc = 2; }
   if( nc == 2 )
     while(s && *s) { if( w2[0] == *s && w2[1] == *(s+1)) return s; s++; }
   else
     while(s && *s) { if( w2[0] == *s ) return s; s++; }
   return 0;
}

/*
 *  save a new match in mp
 */
static void _renewmatch(Resub *mp, int ms, Resublist *sp)
{
	int i;

	if(mp==0 || ms<=0)
		return;
	if(mp[0].sp==0 || sp->m[0].sp<mp[0].sp ||
	   (sp->m[0].sp==mp[0].sp && sp->m[0].ep>mp[0].ep)){
		for(i=0; i<ms && i<NSUBEXP; i++)
			mp[i] = sp->m[i];
		for(; i<ms; i++)
			mp[i].sp = mp[i].ep = 0;
	}
}

/*
 * Note optimization in _renewthread:
 * 	*lp must be pending when _renewthread called; if *l has been looked
 *		at already, the optimization is a bug.
 */
static Relist* _renewthread(
    Relist *lp,	/* _relist to add to */
	  Reinst *ip,		/* instruction to add */
	  int ms,
	  Resublist *sep)		/* pointers to subexpressions */
{
	Relist *p;

	for(p=lp; p->inst; p++){
		if(p->inst == ip){
			if(sep->m[0].sp < p->se.m[0].sp){
				if(ms > 1)
					p->se = *sep;
				else
					p->se.m[0] = sep->m[0];
			}
			return 0;
		}
	}
	p->inst = ip;
	if(ms > 1)
		p->se = *sep;
	else
		p->se.m[0] = sep->m[0];
	(++p)->inst = 0;
	return p;
}

/*
 * same as renewthread, but called with
 * initial empty start pointer.
 */
Relist* _renewemptythread(
  Relist *lp,	  /* _relist to add to */
	Reinst *ip,		/* instruction to add */
	int ms,
	const rechar *sp)		  /* pointers to subexpressions */
{
	Relist *p;

	for(p=lp; p->inst; p++){
		if(p->inst == ip){
			if(sp < p->se.m[0].sp) {
				if(ms > 1)
					memset(&p->se, 0, sizeof(p->se));
				p->se.m[0].sp = sp;
			}
			return 0;
		}
	}
	p->inst = ip;
	if(ms > 1)
		memset(&p->se, 0, sizeof(p->se));
	p->se.m[0].sp = sp;
	(++p)->inst = 0;
	return p;
}


void regerror(const rechar *s)
{
	//rechar buf[132];
	//strcpy(buf, "regerror: ");
	//strcat(buf, s);
	//strcat(buf, "\n");
	//write(2, buf, strlen(buf));
	//exits("regerr");
  //printf("regerror:%s\n",s);
  throw ReError(s);
}

/*
 *  return	0 if no match
 *		>0 if a match
 *		<0 if we ran out of _relist space
 */
static int
regexec1(Reprog *progp,	/* program to run */
	const rechar *bol,	/* string to run machine on */
	Resub *mp,	/* subexpression elements */
	int ms,		/* number of elements at mp */
	Reljunk *j
)
{
	int flag=0;
	Reinst *inst;
	Relist *tlp;
	const rechar *s;
	int i, checkstart;
	unicode_cp r, *rp, *ep;
	int n;
	Relist* tl;		/* This list, next list */
	Relist* nl;
	Relist* tle;		/* ends of this and next list */
	Relist* nle;
	int match;
	const rechar *p;

	match = 0;
	checkstart = j->starttype;
	if(mp)
		for(i=0; i<ms; i++) {
			mp[i].sp = 0;
			mp[i].ep = 0;
		}
	j->relist[0][0].inst = 0;
	j->relist[1][0].inst = 0;
        j->eol = bol + wcslen(bol);

	/* Execute machine once for each character, including terminal NUL */
	s = j->starts;
	do{
		/* fast check for first rechar */
		if(checkstart) {
			switch(j->starttype) {
			case RUNE:
				p = ucp_pos_in_rechar_str(s, j->startchar);
				if(p == 0 || s >= j->eol)
					return match;
				s = p;
				break;
			case BOL:
				if(s == bol)
					break;
				p = ucp_pos_in_rechar_str(s, '\n');
				if(p == 0 || s >= j->eol)
					return match;
				s = p+1;
				break;
			}
		}
		r = *(urechar*)s;
		if(r < unicode_cp_self)
			n = 1;
		else
			n = rechar2ucp(&r, s);

		/* switch run lists */
		tl = j->relist[flag];
		tle = j->reliste[flag];
		nl = j->relist[flag^=1];
		nle = j->reliste[flag];
		nl->inst = 0;

		/* Add first instruction to current list */
		if(match == 0)
			_renewemptythread(tl, progp->startinst, ms, s);

		/* Execute machine until current list is empty */
		for(tlp=tl; tlp->inst; tlp++){	/* assignment = */
			for(inst = tlp->inst; ; inst = inst->u2.next){
				switch(inst->type){
				case RUNE:	/* regular character */
					if(inst->u1.r == r){
						if(_renewthread(nl, inst->u2.next, ms, &tlp->se)==nle)
							return -1;
					}
					break;
				case LBRA:
					tlp->se.m[inst->u1.subid].sp = s;
					continue;
				case RBRA:
					tlp->se.m[inst->u1.subid].ep = s;
					continue;
				case ANY:
					if(r != '\n')
						if(_renewthread(nl, inst->u2.next, ms, &tlp->se)==nle)
							return -1;
					break;
				case ANYNL:
					if(_renewthread(nl, inst->u2.next, ms, &tlp->se)==nle)
							return -1;
					break;
				case BOL:
					if(s == bol || *(s-1) == '\n')
						continue;
					break;
				case EOL:
					if(s == j->eol || r == 0 || r == '\n')
						continue;
					break;
				case CCLASS:
					ep = inst->u1.cp->end;
					for(rp = inst->u1.cp->spans; rp < ep; rp += 2)
						if(r >= rp[0] && r <= rp[1]){
							if(_renewthread(nl, inst->u2.next, ms, &tlp->se)==nle)
								return -1;
							break;
						}
					break;
				case NCCLASS:
					ep = inst->u1.cp->end;
					for(rp = inst->u1.cp->spans; rp < ep; rp += 2)
						if(r >= rp[0] && r <= rp[1])
							break;
					if(rp == ep)
						if(_renewthread(nl, inst->u2.next, ms, &tlp->se)==nle)
							return -1;
					break;

				case PDCLASS:
					if( inst->u1.cfp(r))
          {
						if(_renewthread(nl, inst->u2.next, ms, &tlp->se)==nle)
							return -1;
					}
					break;
				case NPDCLASS:
					if( !inst->u1.cfp(r))
          {
						if(_renewthread(nl, inst->u2.next, ms, &tlp->se)==nle)
							return -1;
					}
					break;

				case OR:
					/* evaluate right choice later */
					if(_renewthread(tlp, inst->u1.right, ms, &tlp->se) == tle)
						return -1;
					/* efficiency: advance and re-evaluate */
					continue;
				case END:	/* Match! */
					match = 1;
					tlp->se.m[0].ep = s;
					if(mp != 0)
						_renewmatch(mp, ms, &tlp->se);
					break;
				}
				break;
			}
		}
		if(s == j->eol)
			break;
		checkstart = j->starttype && nl->inst==0;
		s += n;
	}while(r);
	return match;
}

static int
regexec2(Reprog *progp,	/* program to run */
	const rechar *bol,	  /* string to run machine on */
	Resub *mp,	          /* subexpression elements */
	int ms,		            /* number of elements at mp */
	Reljunk *j
)
{
	int rv;
	Relist *relist0, *relist1;

	/* mark space */
	relist0 = (Relist*)malloc(BIGLISTSIZE*sizeof(Relist));
	if(relist0 == 0)
		return -1;
	relist1 = (Relist*)malloc(BIGLISTSIZE*sizeof(Relist));
	if(relist1 == 0){
		free(relist1);
		return -1;
	}
	j->relist[0] = relist0;
	j->relist[1] = relist1;
	j->reliste[0] = relist0 + BIGLISTSIZE - 2;
	j->reliste[1] = relist1 + BIGLISTSIZE - 2;

	rv = regexec1(progp, bol, mp, ms, j);
	free(relist0);
	free(relist1);
	return rv;
}

extern int
regexec(Reprog *progp,	/* program to run */
	const wchar_t *bol,	/* string to run machine on */
	Resub *mp,	/* subexpression elements */
	int ms)		/* number of elements at mp */
{
	Reljunk j;
	Relist relist0[LISTSIZE], relist1[LISTSIZE];
	int rv;

	/*
 	 *  use user-specified starting/ending location if specified
	 */
	j.starts = bol;
	j.eol = 0;
	if(mp && ms>0){
		if(mp->sp)
			j.starts = mp->sp;
		if(mp->ep)
			j.eol = mp->ep;
	}
	j.starttype = 0;
	j.startchar = 0;
	if(progp->startinst->type == RUNE && progp->startinst->u1.r < unicode_cp_self) {
		j.starttype = RUNE;
		j.startchar = progp->startinst->u1.r;
	}
	if(progp->startinst->type == BOL)
		j.starttype = BOL;

	/* mark space */
	j.relist[0] = relist0;
	j.relist[1] = relist1;
	j.reliste[0] = relist0 + nelem(relist0) - 2;
	j.reliste[1] = relist1 + nelem(relist1) - 2;

	rv = regexec1(progp, bol, mp, ms, &j);
	if(rv >= 0)
		return rv;
	rv = regexec2(progp, bol, mp, ms, &j);
	if(rv >= 0)
		return rv;
	return -1;
}


/* substitute into one string using the matches from the last regexec() */
void regsub(
  const wchar_t* sp,	// source string 
	wchar_t* dp,	// destination string 
	int dlen,
	Resub *mp,// subexpression elements
	int ms)		// number of elements pointed to by mp
{
	const rechar *ssp, *ep;
	int i;

	ep = dp+dlen-1;
	while(*sp != '\0'){
		if(*sp == '\\'){
			switch(*++sp){
			case '0':
			case '1':
			case '2':
			case '3':
			case '4':
			case '5':
			case '6':
			case '7':
			case '8':
			case '9':
				i = *sp-'0';
				if(mp[i].sp != 0 && mp!=0 && ms>i)
					for(ssp = mp[i].sp;
					     ssp < mp[i].ep;
					     ssp++)
						if(dp < ep)
							*dp++ = *ssp;
				break;
			case '\\':
				if(dp < ep)
					*dp++ = '\\';
				break;
			case '\0':
				sp--;
				break;
			default:
				if(dp < ep)
					*dp++ = *sp;
				break;
			}
		}else if(*sp == '&'){				
			if(mp[0].sp != 0 && mp!=0 && ms>0)
			if(mp[0].sp != 0)
				for(ssp = mp[0].sp;
				     ssp < mp[0].ep; ssp++)
					if(dp < ep)
						*dp++ = *ssp;
		}else{
			if(dp < ep)
				*dp++ = *sp;
		}
		sp++;
	}
	*dp = '\0';
}

/*
 * Parser 
 */
struct Node
{
	Reinst*	first;
	Reinst*	last;
  Node(): first(0),last(0) {}
};

#define	NSTACK	20

struct ReCEnv // compiler environment
{
  Node	andstack[NSTACK];
  Node	*andp;
  int	atorstack[NSTACK];
  int*	atorp;
  int	cursubid;		/* id of current subexpression */
  int	subidstack[NSTACK];	/* parallel to atorstack */
  int*	subidp;
  bool	lastwasand;	/* Last token was operand */
  int	nbra;
  const rechar*	exprp;		/* pointer to next character in source expression */
  bool	lexdone;
  int	nclass;
  Reclass* classp;
  Reinst*	 freep;
  int	errors;
  unicode_cp	yyrune;		/* last lex'd rune */
  Reclass* yyclassp;	/* last lex'd class */
  unicode_cp_class* cfp;    /* last lex'd char class function */ 

  ReCEnv(): andp(0), atorp(0), cursubid(0), subidp(0),
            lastwasand(false), nbra(0), exprp(0), lexdone(false), nclass(0), classp(0),
            freep(0), errors(0), yyrune(0), yyclassp(0),cfp(0) {}
};

/* predeclared crap */
static	void	_operator(ReCEnv& cenv,int);
static	void	pushand(ReCEnv& cenv,Reinst*, Reinst*);
static	void	pushator(ReCEnv& cenv,int);
static	void	evaluntil(ReCEnv& cenv,int);
static	int	  bldcclass(ReCEnv& cenv);

//static jmp_buf regkaboom;

static	void rcerror(ReCEnv& cenv, const rechar *s)
{
	cenv.errors++;
	regerror(s);

	//longjmp(regkaboom, 1);
}

static	Reinst* newinst(ReCEnv& cenv, int t)
{
	cenv.freep->type = t;
	cenv.freep->u2.left = 0;
	cenv.freep->u1.right = 0;
	return cenv.freep++;
}

static	void _operand(ReCEnv& cenv, int t)
{
	Reinst *i;

	if(cenv.lastwasand)
		_operator(cenv,CAT);	/* catenate is implicit */
	i = newinst(cenv,t);

	if(t == CCLASS || t == NCCLASS)
		i->u1.cp = cenv.yyclassp;
	else if(t == PDCLASS || t == NPDCLASS)
		i->u1.cfp = cenv.cfp;
	else if(t == RUNE)
		i->u1.r = cenv.yyrune;

	pushand(cenv,i, i);
	cenv.lastwasand = true;
}

static	void _operator(ReCEnv& cenv, int t)
{
	if(t==RBRA && --cenv.nbra<0)
		rcerror(cenv,L"unmatched right paren");
	if(t==LBRA){
		if(++cenv.cursubid >= NSUBEXP)
			rcerror (cenv,L"too many subexpressions");
		cenv.nbra++;
		if(cenv.lastwasand)
			_operator(cenv,CAT);
	} else
		evaluntil(cenv,t);
	if(t != RBRA)
		pushator(cenv,t);
	cenv.lastwasand = false;
	if(t==STAR || t==QUEST || t==PLUS || t==RBRA)
		cenv.lastwasand = true;	/* these look like operands */
}

static	void regerr2(ReCEnv& cenv, rechar *s, int c)
{
	rechar buf[100];
	rechar *cp = buf;
	while(*s)
		*cp++ = *s++;
	*cp++ = c;
	*cp = '\0'; 
	rcerror(cenv,buf);
}

static	void cant(ReCEnv& cenv, rechar *s)
{
	rechar buf[100];
	wcscpy(buf, L"can't happen: ");
	wcscat(buf, s);
	rcerror(cenv,buf);
}

static	void pushand(ReCEnv& cenv, Reinst *f, Reinst *l)
{
	if(cenv.andp >= &cenv.andstack[NSTACK])
		cant(cenv,L"operand stack overflow");
	cenv.andp->first = f;
	cenv.andp->last = l;
	cenv.andp++;
}

static	void pushator(ReCEnv& cenv, int t)
{
	if(cenv.atorp >= &cenv.atorstack[NSTACK])
		cant(cenv,L"operator stack overflow");
	*cenv.atorp++ = t;
	*cenv.subidp++ = cenv.cursubid;
}

static	Node* popand(ReCEnv& cenv, int op)
{
	Reinst *inst;
	if(cenv.andp <= &cenv.andstack[0])
  {
		regerr2(cenv, L"missing operand for ", op);
		inst = newinst(cenv,NOP);
		pushand(cenv,inst,inst);
	}
	return --cenv.andp;
}

static int popator(ReCEnv& cenv)
{
	if(cenv.atorp <= &cenv.atorstack[0])
		cant(cenv, L"operator stack underflow");
	--cenv.subidp;
	return *--cenv.atorp;
}

static void evaluntil(ReCEnv& cenv, int pri)
{
	Node *op1, *op2;
	Reinst *inst1, *inst2;

	while( pri == RBRA || cenv.atorp[-1] >= pri )
  {
		switch(popator(cenv))
    {
		default:
			rcerror(cenv,L"unknown _operator in evaluntil");
			break;
		case LBRA:		/* must have been RBRA */
			op1 = popand(cenv,'(');
			inst2 = newinst(cenv,RBRA);
			inst2->u1.subid = *cenv.subidp;
			op1->last->u2.next = inst2;
			inst1 = newinst(cenv,LBRA);
			inst1->u1.subid = *cenv.subidp;
			inst1->u2.next = op1->first;
			pushand(cenv,inst1, inst2);
			return;
		case OR:
			op2 = popand(cenv,'|');
			op1 = popand(cenv,'|');
			inst2 = newinst(cenv,NOP);
			op2->last->u2.next = inst2;
			op1->last->u2.next = inst2;
			inst1 = newinst(cenv,OR);
			inst1->u1.right = op1->first;
			inst1->u2.left = op2->first;
			pushand(cenv,inst1, inst2);
			break;
		case CAT:
			op2 = popand(cenv,0);
			op1 = popand(cenv,0);
			op1->last->u2.next = op2->first;
			pushand(cenv,op1->first, op2->last);
			break;
		case STAR:
			op2 = popand(cenv,'*');
			inst1 = newinst(cenv,OR);
			op2->last->u2.next = inst1;
			inst1->u1.right = op2->first;
			pushand(cenv,inst1, inst1);
			break;
		case PLUS:
			op2 = popand(cenv,'+');
			inst1 = newinst(cenv,OR);
			op2->last->u2.next = inst1;
			inst1->u1.right = op2->first;
			pushand(cenv,op2->first, inst1);
			break;
		case QUEST:
			op2 = popand(cenv,'?');
			inst1 = newinst(cenv,OR);
			inst2 = newinst(cenv,NOP);
			inst1->u2.left = inst2;
			inst1->u1.right = op2->first;
			op2->last->u2.next = inst2;
			pushand(cenv,inst1, inst2);
			break;
		}
	}
}

static	Reprog* optimize(ReCEnv& cenv, Reprog *pp)
{
	Reinst *inst, *target;
	int size;
	Reprog *npp;
	Reclass *cl;
	int diff;

	/*
	 *  get rid of NOOP chains
	 */
	for(inst=pp->firstinst; inst->type!=END; inst++){
		target = inst->u2.next;
		while(target->type == NOP)
			target = target->u2.next;
		inst->u2.next = target;
	}

	/*
	 *  The original allocation is for an area larger than
	 *  necessary.  Reallocate to the actual space used
	 *  and then relocate the code.
	 */
	size = sizeof(Reprog) + (cenv.freep - pp->firstinst) * sizeof(Reinst);
	npp = (Reprog*)realloc(pp, size);
	if(npp==0 || npp==pp)
		return pp;
	diff = (rechar *)npp - (rechar *)pp;
	cenv.freep = (Reinst *)((rechar *)cenv.freep + diff);
	for(inst=npp->firstinst; inst < cenv.freep; inst++){
		switch(inst->type)
    {
		  case OR:
		  case STAR:
		  case PLUS:
		  case QUEST:
			  inst->u1.right = (Reinst*)((rechar*)inst->u1.right + diff);
			  break;
		  case CCLASS:
		  case NCCLASS:
			  inst->u1.right = (Reinst*)((rechar*)inst->u1.right + diff);
			  cl = inst->u1.cp;
			  cl->end = (unicode_cp*)((rechar*)cl->end + diff);
			  break;
		}
		inst->u2.left = (Reinst*)((rechar*)inst->u2.left + diff);
	}
	npp->startinst = (Reinst*)((rechar*)npp->startinst + diff);
	return npp;
}

#ifdef	DEBUG
static	void
dumpstack(void){
	Node *stk;
	int *ip;

	print("operators\n");
	for(ip=atorstack; ip<atorp; ip++)
		print("0%o\n", *ip);
	print("operands\n");
	for(stk=andstack; stk<andp; stk++)
		print("0%o\t0%o\n", stk->first->type, stk->last->type);
}

static	void
dump(Reprog *pp)
{
	Reinst *l;
	unicode_cp *p;

	l = pp->firstinst;
	do{
		print("%d:\t0%o\t%d\t%d", l-pp->firstinst, l->type,
			l->u2.left-pp->firstinst, l->u1.right-pp->firstinst);
		if(l->type == RUNE)
			print("\t%C\n", l->u1.r);
		else if(l->type == CCLASS || l->type == NCCLASS){
			print("\t[");
			if(l->type == NCCLASS)
				print("^");
			for(p = l->u1.cp->spans; p < l->u1.cp->end; p += 2)
				if(p[0] == p[1])
					print("%C", p[0]);
				else
					print("%C-%C", p[0], p[1]);
			print("]\n");
		} else
			print("\n");
	}while(l++->type);
}
#endif

static	Reclass* newclass(ReCEnv& cenv)
{
	if(cenv.nclass >= NCLASS)
		regerr2(cenv,L"too many character classes; limit", NCLASS+'0');
	return &(cenv.classp[cenv.nclass++]);
}

/* filter functions */
static bool isalnum_f(unicode_cp c)      { return 0 != ucisalnum(c); }
static bool isalpha_f(unicode_cp c)      { return 0 != ucisalpha(c); }
static bool isblank_f(unicode_cp c)      { return 0 != ucisblank(c); }
static bool iscntrl_f(unicode_cp c)      { return 0 != uciscntrl(c); }
static bool isdigit_f(unicode_cp c)      { return 0 != ucisdigit(c); }
static bool isgraph_f(unicode_cp c)      { return 0 != ucisgraph(c); }
static bool islower_f(unicode_cp c)      { return 0 != ucislower(c); }
static bool isprint_f(unicode_cp c)      { return 0 != ucisprint(c); }
static bool ispunct_f(unicode_cp c)      { return 0 != ucispunct(c); }
static bool isspace_f(unicode_cp c)      { return 0 != ucisspace(c); }
static bool isupper_f(unicode_cp c)      { return 0 != ucisupper(c); }
static bool isxdigit_f(unicode_cp c)     { return 0 != ucisxdigit(c); }
static bool isword_f(unicode_cp c)       { return 0 != ucisalnum(c) || c == '_'; }

//static const rechar* predefined_class_table = L"mabcdglpnsuxwMABCDGLPNSUXW";

static	bool nextc(ReCEnv& cenv, unicode_cp *rp)
{
	if(cenv.lexdone){
		*rp = 0;
		return true;
	}
	cenv.exprp += rechar2ucp(rp, cenv.exprp);
	if(*rp == '\\')
  {
		cenv.exprp += rechar2ucp(rp, cenv.exprp);
    //if( wcschr(predefined_class_table,*rp))
    //  return false;
		return true;
	}
	if(*rp == 0)
		cenv.lexdone = true;
	return false;
}


static	int lex(ReCEnv& cenv, int literal, int dot_type)
{
	int quoted;
	quoted = nextc(cenv,&cenv.yyrune);
	if(literal || quoted){
		if(cenv.yyrune == 0)
			return END;
    if( quoted )
      switch(cenv.yyrune)
      {
        case 'm': cenv.cfp = isalnum_f; return PDCLASS;
        case 'M': cenv.cfp = isalnum_f; return NPDCLASS;
        case 'a': cenv.cfp = isalpha_f; return PDCLASS;
        case 'A': cenv.cfp = isalpha_f; return NPDCLASS;
        case 'b': cenv.cfp = isblank_f; return PDCLASS;
        case 'B': cenv.cfp = isblank_f; return NPDCLASS;
        case 'c': cenv.cfp = iscntrl_f; return PDCLASS;
        case 'C': cenv.cfp = iscntrl_f; return NPDCLASS;
        case 'd': cenv.cfp = isdigit_f; return PDCLASS;
        case 'D': cenv.cfp = isdigit_f; return NPDCLASS;
        case 'g': cenv.cfp = isgraph_f; return PDCLASS;
        case 'G': cenv.cfp = isgraph_f; return NPDCLASS;
        case 'l': cenv.cfp = islower_f; return PDCLASS;
        case 'L': cenv.cfp = islower_f; return NPDCLASS;
        case 'p': cenv.cfp = isprint_f; return PDCLASS;
        case 'P': cenv.cfp = isprint_f; return NPDCLASS;
        case 'n': cenv.cfp = ispunct_f; return PDCLASS;
        case 'N': cenv.cfp = ispunct_f; return NPDCLASS;
        case 's': cenv.cfp = isspace_f; return PDCLASS;
        case 'S': cenv.cfp = isspace_f; return NPDCLASS;
        case 'u': cenv.cfp = isupper_f; return PDCLASS;
        case 'U': cenv.cfp = isupper_f; return NPDCLASS;
        case 'x': cenv.cfp = isxdigit_f; return PDCLASS;
        case 'X': cenv.cfp = isxdigit_f; return NPDCLASS;
        case 'w': cenv.cfp = isblank_f; return PDCLASS;
        case 'W': cenv.cfp = isblank_f; return NPDCLASS;
    }
		return RUNE;
	}

	switch(cenv.yyrune)
  {
	case 0:
		return END;
	case '*':
		return STAR;
	case '?':
		return QUEST;
	case '+':
		return PLUS;
	case '|':
		return OR;
	case '.':
		return dot_type;
	case '(':
		return LBRA;
	case ')':
		return RBRA;
	case '^':
		return BOL;
	case '$':
		return EOL;
	case '[':
		return bldcclass(cenv);
	}
	return RUNE;
}

static int bldcclass(ReCEnv& cenv)
{
	int type;
	unicode_cp r[NCCRUNE];
	unicode_cp *p, *ep, *np;
	unicode_cp rune;
	int quoted;

	/* we have already seen the '[' */
	type = CCLASS;
	cenv.yyclassp = newclass(cenv);

	/* look ahead for negation */
	/* SPECIAL CASE!!! negated classes don't match \n */
	ep = r;
	quoted = nextc(cenv,&rune);
	if(!quoted && rune == '^'){
		type = NCCLASS;
		quoted = nextc(cenv,&rune);
		*ep++ = '\n';
		*ep++ = '\n';
	}
  /* '-' at start */
	if(!quoted && rune == '-'){
		quoted = nextc(cenv,&rune);
		*ep++ = '-';
		*ep++ = '-';
	}

	/* parse class into a set of spans */
	for(; ep<&r[NCCRUNE];){
		if(rune == 0){
			rcerror(cenv,L"malformed '[]'");
			return 0;
		}
		if(!quoted && rune == ']')
			break;
		if(!quoted && rune == '-'){
			if(ep == r){
				rcerror(cenv,L"malformed '[]'");
				return 0;
			}
			quoted = nextc(cenv,&rune);
			if((!quoted && rune == ']') || rune == 0){
				rcerror(cenv,L"malformed '[]'");
				return 0;
			}
			*(ep-1) = rune;
		} else {
			*ep++ = rune;
			*ep++ = rune;
		}
		quoted = nextc(cenv,&rune);
	}

	/* sort on span start */
	for(p = r; p < ep; p += 2){
		for(np = p; np < ep; np += 2)
			if(*np < *p){
				rune = np[0];
				np[0] = p[0];
				p[0] = rune;
				rune = np[1];
				np[1] = p[1];
				p[1] = rune;
			}
	}

	/* merge spans */
	np = cenv.yyclassp->spans;
	p = r;
	if(r == ep)
		cenv.yyclassp->end = np;
	else {
		np[0] = *p++;
		np[1] = *p++;
		for(; p < ep; p += 2)
			if(p[0] <= np[1]){
				if(p[1] > np[1])
					np[1] = p[1];
			} else {
				np += 2;
				np[0] = p[0];
				np[1] = p[1];
			}
		cenv.yyclassp->end = np+2;
	}

	return type;
}

static	Reprog* regcomp1(ReCEnv& cenv, const rechar *s, int literal, int dot_type)
{
	int token;
	Reprog *pp;

	/* get memory for the program */
	pp = (Reprog*)malloc(sizeof(Reprog) + 6*sizeof(Reinst)*wcslen(s)*sizeof(rechar));
	if(pp == 0){
		regerror(L"out of memory");
		return 0;
	}
	cenv.freep = pp->firstinst;
	cenv.classp = pp->clazz;
	cenv.errors = 0;

	//if(setjmp(regkaboom))
	//	goto out;

	/* go compile the sucker */
	cenv.lexdone = false;
	cenv.exprp = s;
	cenv.nclass = 0;
	cenv.nbra = 0;
	cenv.atorp = cenv.atorstack;
	cenv.andp = cenv.andstack;
	cenv.subidp = cenv.subidstack;
	cenv.lastwasand = false;
	cenv.cursubid = 0;

	/* Start with a low priority _operator to prime parser */
	pushator(cenv,START-1);
	while((token = lex(cenv,literal, dot_type)) != END){
		if((token & 0300) == OPERATOR)
			_operator(cenv,token);
		else
			_operand(cenv,token);
	}

	/* Close with a low priority _operator */
	evaluntil(cenv,START);

	/* Force END */
	_operand(cenv,END);
	evaluntil(cenv,START);
#ifdef DEBUG
	dumpstack();
#endif
	if(cenv.nbra)
		rcerror(cenv,L"unmatched left paren");
	--cenv.andp;	/* points to first and only operand */
	pp->startinst = cenv.andp->first;
#ifdef DEBUG
	dump(pp);
#endif
	pp = optimize(cenv,pp);
#ifdef DEBUG
	print("start: %d\n", andp->first-pp->firstinst);
	dump(pp);
#endif
//out:
	if(cenv.errors)
  {
		free(pp);
		pp = 0;
	}
	return pp;
}

Reprog* regcomp(const wchar_t *s)
{
  ReCEnv cenv;
	return regcomp1(cenv,s, 0, ANY);
}

Reprog* regcomplit(const wchar_t *s)
{
  ReCEnv cenv;
	return regcomp1(cenv,s, 1, ANY);
}

Reprog* regcompnl(const wchar_t *s)
{
  ReCEnv cenv;
	return regcomp1(cenv,s, 0, ANYNL);
}

void	regfree(Reprog* rep)
{
  if(rep) free((void*)rep);
}

}}