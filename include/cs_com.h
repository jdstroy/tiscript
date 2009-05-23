/* cs_com.h - compiler definitions */
/*
        Copyright (c) 2001-2004 Terra Informatica Software, Inc.
        and Andrew Fedoniouk andrew@terrainformatica.com
        All rights reserved
*/

#ifndef __CSCOM_H__
#define __CSCOM_H__

#include "cs.h"

namespace tis 
{

/* CsToken definitions */
#define T_NOTOKEN       -1
#define T_EOF           0

/* non-character tokens */
#define _TMIN           256
#define T_STRING        256
#define T_IDENTIFIER    257
#define T_INTEGER       258
#define T_FLOAT         259
#define T_SYMBOL        260
#define T_FUNCTION      261
#define T_VAR           262
#define T_IF            263
#define T_ELSE          264
#define T_WHILE         265
#define T_RETURN        266
#define T_FOR           267
#define T_BREAK         268
#define T_CONTINUE      269
#define T_DO            270
#define T_SWITCH        271
#define T_CASE          272
#define T_DEFAULT       273
#define T_NULL          274
#define T_LE            275     /* '<=' */
#define T_EQ            276     /* '==' */
#define T_NE            277     /* '!=' */
#define T_GE            278     /* '>=' */
#define T_SHL           279     /* '<<' */
#define T_SHR           280     /* '>>' */
#define T_AND           281     /* '&&' */
#define T_OR            282     /* '||' */
#define T_INC           283     /* '++' */
#define T_DEC           284     /* '--' */
#define T_ADDEQ         285     /* '+=' */
#define T_SUBEQ         286     /* '-=' */
#define T_MULEQ         287     /* '*=' */
#define T_DIVEQ         288     /* '/=' */
#define T_REMEQ         289     /* '%=' */
#define T_ANDEQ         290     /* '&=' */
#define T_OREQ          291     /* '|=' */
#define T_XOREQ         292     /* '^=' */
#define T_SHLEQ         293     /* '<<=' */
#define T_SHREQ         294     /* '>>=' */
#define T_TYPE          295
#define T_SUPER         296
#define T_NEW           297
#define T_DOTDOT        298
#define T_TRY           299
#define T_CATCH         300
#define T_FINALLY       301
#define T_THROW         302
#define T_TYPEOF        303
#define T_INSTANCEOF    304
#define T_IN            305
#define T_OUTPUT_STRING 306     /* string between %> and <% */
#define T_EQ_STRONG     307     /* '===' identity*/
#define T_NE_STRONG     308     /* '!==' non identity */
#define T_PROPERTY      309
#define T_CONST         310
#define T_GET           311
#define T_SET           312
#define T_INCLUDE       313
#define T_LIKE          314
#define T_YIELD         315
#define T_USHL          316     /* '<<<' */
#define T_USHR          317     /* '>>>' */
#define T_USHLEQ        318     /* '<<<=' */
#define T_USHREQ        319     /* '>>>=' */
#define T_CAR           320     /* '~/' */
#define T_CDR           321     /* '~%' */
#define T_RCAR          322     /* '/~' */
#define T_RCDR          323     /* '%~' */
#define T_CLASS         324     /* class */
#define T_NAMESPACE     325     /* namespace */
#define T_DEBUG         326
#define _TMAX           T_DEBUG

/* argument structure */
typedef struct argument ARGUMENT;
struct argument {
    char *arg_name;             /* argument name */
    bool  immutable;
    struct argument *arg_next;  /* next argument */
};

/* argument table structure */
typedef struct atable ATABLE;
struct atable {
    ARGUMENT *at_arguments;     /* first argument */
    ARGUMENT **at_pNextArgument;/* pointer to where to store the next argument */
    struct atable *at_next;     /* next argument table */
};


/* break/continue stack entry structure */
typedef struct sentry SENTRY;
struct sentry {
    int               level;                  // block level 
    int               label;                  // label 
    const char*       name;                   // name of the loop  
};



/* case entry structure */
typedef struct centry CENTRY;
struct centry {
    int value;
    int label;
    CENTRY *next;
};

/* switch entry structure */
typedef struct swentry SWENTRY;
struct swentry {
    int nCases;
    CENTRY *cases;
    int defaultLabel;
    int label;
};

/* try/catch/finally entry structure */
typedef struct TCENTRY TCENTRY;
struct TCENTRY {
    int handler;
    int start;
    int end;
    TCENTRY *next;
};

/* limits */
#define TKNSIZE         255     /* maximum CsToken size */
#define LSIZE           255     /* maximum line size */
#define SSIZE           20      /* break/continue/switch stack size */

/* line number entry */
typedef struct LineNumberEntry LineNumberEntry;
struct LineNumberEntry {
    int line;
    int pc;
};

/* line number block */
#define kLineNumberBlockSize    128
typedef struct LineNumberBlock LineNumberBlock;
struct LineNumberBlock {
    LineNumberBlock *next;
    int count;
    LineNumberEntry entries[kLineNumberBlockSize];
};

/* compiler context structure */
struct CsCompiler {
    VM *ic;                 /* compiler - interpreter context */
    stream *input;                   /* compiler - input stream */
    int blockLevel;                     /* compiler - nesting level */
    ATABLE *arguments;                  /* compiler - argument frames */
    TCENTRY *exceptions;                /* compiler - exceptions */
    SENTRY bstack[SSIZE],*bsp,*bsbase;  /* compiler - break stack */
    SENTRY cstack[SSIZE],*csp,*csbase;  /* compiler - continue stack */
    SWENTRY sstack[SSIZE],*ssp,*ssbase; /* compiler - switch stack */
    byte *codebuf;                      /* compiler - code buffer */
    byte *cbase,*cptr,*ctop;            /* compiler - code buffer positions */
    pvalue literalbuf;                 /* compiler - literal buffer */
    long lbase,lptr,ltop;               /* compiler - literal buffer positions */
    bool emitLineNumbersP;              /* compiler - true to emit line number opcodes */
    LineNumberBlock *lineNumbers;       /* compiler - line number table entries */
    LineNumberBlock *currentBlock;      /* compiler - where to store new line numbers */
    int_t t_value;             /* scanner - integer value */
    float_t t_fvalue;              /* scanner - float value */
    char  t_token[TKNSIZE+1];           /* scanner - token string */
    tool::array<wchar> t_wtoken;        /* scanner - wide char token string */
    bool lineNumberChangedP;            /* scanner - line number has changed */
    int lineNumber;                     /* scanner - line number */
    int savedToken;                     /* scanner - look ahead CsToken */
    int savedChar;                      /* scanner - look ahead character */
    tool::array<wchar> line;            /* scanner - last input line */
    wchar *linePtr;                     /* scanner - line pointer */
    bool atEOF;                         /* scanner - input end of file flag */
    int  functionLevel;                 /* scanner - function level */
    const char* qualifiedName;          /* scanner - full name of object being parsed: class and function */
    bool  JSONonly;                     /* parse only data declarations - JSON data literals */
    bool is_generator;                  /* 'yield' is seen in function body - compile_code() */

    struct TryCatchDef
    {
      int  finallyAddr;                 /* current finally block addr */
      bool inTry;                       /* is in try block parsing */  
      TryCatchDef* prev;
      TryCatchDef(): finallyAddr(0), inTry(true), prev(0) {}
    };

    TryCatchDef *tcStack;               /* current try/catch stack */    

    const wchar* get_wtoken_string()
    {
      if( t_wtoken.size() == 0 )
        t_wtoken.push(0);
      else if (t_wtoken.last() != 0)
        t_wtoken.push(0);
      return t_wtoken.head();
    }
};

/* prototypes for scanner.c */
int CsToken(CsCompiler *c);
void CsSaveToken(CsCompiler *c,int tkn);
char *CsTokenName(int tkn);
void CsParseError(CsCompiler *c,char *msg);
void CsParseWarning(CsCompiler *c,char *msg);

}

#endif
