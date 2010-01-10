/* error.c - error strings */
/*
        Copyright (c) 2001-2004 Terra Informatica Software, Inc.
        and Andrew Fedoniouk andrew@terrainformatica.com
        All rights reserved
*/

#include <stdarg.h>
#include "cs.h"
#include "cs_com.h"

namespace tis 
{


/* error output buffer size */
#define ERR_BUF_SIZE    100

#define NumErrorFields       4 // number of fields in 

/* error table entry */
typedef struct {
    int code;
    char *text;
} ErrString;

static ErrString errStrings[] = {
{       CsErrExit,                 "Exit"                 },
{       CsErrInsufficientMemory,   "Insufficient memory"  },
{       CsErrStackOverflow,        "Stack overflow"       },
{       CsErrTooManyArguments,     "Too many arguments - %V"        },
//{       CsErrTooFewArguments,      "Too few arguments - %V"       },
{       CsErrTypeError,            "Wrong type - (%V)"            },
{       CsErrUnexpectedTypeError,  "Wrong type - (%V), expected instance of %s"   },
{       CsErrUnboundVariable,      "Variable not found - %V"            },
{       CsErrIndexOutOfBounds,     "Index out of bounds - %V"       },
{       CsErrNoMethod,             "%s (%V) has no method - %V"          },
{       CsErrBadOpcode,            "Bad opcode - %b"            },
{       CsErrRestart,              "Restart"                },
{       CsErrWrite,                "Writing file"             },
{       CsErrBadParseCode,         "Bad parse code"           },
{       CsErrImpossible,           "Internal error"           },
{       CsErrNoHashValue,          "Can't use as an index"          },
{       CsErrTooManyLiterals,      "Too many literals"            },
{       CsErrTooMuchCode,          "Too much code"              },
{       CsErrStoreIntoConstant,    "Attempt to store into a constant"   },
{       CsErrIsNotLValue,          "Expresion is not an l-value"    },
//{       CsErrSyntaxError,          "%M(%L) : syntax error : %E"     },
{       CsErrSyntaxError,          "bad syntax : %s\nline:%S\nhere:%s\nfile:(%M(%L))"   },
//cs_scn.cpp(752) : error C2143: syntax error : missing ';' before '.'
{       CsErrReadOnlyProperty,    "Attempt to set a read-only property %V"  },
{       CsErrWriteOnlyProperty,   "Attempt to get a write-only property %V" },
{       CsErrFileNotFound,        "File not found - %S"         },
{       CsErrNewInstance,          "Can't create an instance"              },
{       CsErrNoProperty,           "Object %V has no property - %V"        },
{       CsErrStackEmpty,           "Stack empty - %V"                      },
{       CsErrNotAnObjectFile,      "Wrong bytecode format in %S"               },
{       CsErrWrongObjectVersion,   "Wrong obj file version number - %i" },
{       CsErrValueError,           "Bad value - %V"                        },
{       CsErrRegexpError,          "RegExp error - %s"                     },
{       CsErrIOError,              "IO error - %s"                      },
{       CsErrIOTimeout,            "IO timeout"                         },
{       CsErrNoSuchFeature,        "%V does not support %s"             },
{       CsErrNotAllowed,           "operation %s is not available"      },
{       CsErrAlreadyDefined,       "const or method %s already defined" },
{       CsErrGenericError,         "%s"                                 },
{       CsErrGenericErrorW,        "%S"                                 },
{       CsErrTooFewArguments,      "Wrong number of arguments - %V"     },
{       CsErrAssertion,            "Assertion failed on %V"             },
{       CsErrAssertion2,           "Assertion failed on %V:%V"          },
{       0,                          0                                       }
};


void CsStreamError(VM *c,stream *, int code,va_list ap);

/* CsThrowKnownError - call the error handler */
void CsThrowKnownError(VM *c,int code,...)
{
#ifdef _DEBUG
    if(code == CsErrUnexpectedTypeError)
      code = code;
#endif
    if(code != CsErrStackOverflow)
    {
      value message, err;
    
      va_list ap;
      va_start(ap,code);
      //(*c->errorHandler)(c,code,ap);

      string_stream s(256);
    
      CsStreamError(c,&s, code,ap);
      va_end(ap);
    
      message = s.string_o(c);
    
      s.close();
    
      err = CsError(c, code, message);
      c->val[0] = err;
    }
    else
    {
      c->standardError->printf(L"Stack overflow!\n");
      c->val[0] = NULL_VALUE;
    }
    
    THROW_ERROR(code);
        
}

void CsThrowErrorV(VM *c,const char* msg,va_list ap)
{
    value message, err;
    
    string_stream s(256);
    
    CsStreamError(c,&s, 256, ap);
    
    message = s.string_o(c);
    
    s.close();
    
    err = CsError(c, 256, message);

    c->val[0] = err;
    
    THROW_ERROR(256);
        
}

void CsThrowError(VM *c,const char* msg,...)
{
    va_list ap;
    va_start(ap,msg);
  CsThrowErrorV(c,msg,ap);
  va_end(ap);
}


// %E - compiler error

/* CsStreamError - output error in stream*/
void CsStreamError(VM *c,stream *s, int code,va_list ap)
{
    char buf[ERR_BUF_SIZE + 1],*fmt,*dst;
    int cnt,ch;

    /* get the error text format string */
    fmt = CsGetErrorText(code);

    /* initialize the output buffer */
    dst = buf;
    cnt = 0;

    /* parse the format string */
    //CsStreamPutS("Error: ",s);
    while ((ch = *fmt++) != '\0')
        switch (ch) {
        case '%':
            if (*fmt != '\0') {
                if (cnt > 0) {
                    *dst = '\0';
                    s->put_str(buf);
                    dst = buf;
                    cnt = 0;
                }
                switch (*fmt++) {

                case 'M':
                    s->printf(L"%s",c->compiler->input->stream_name());
                    break;
                case 'L':
                    s->printf(L"%d",c->compiler->lineNumber);
                    break;
                case 'E':
                    s->put_str(c->errorMessage);
                    //s->printf(L"\n  in line %d\n",c->compiler->lineNumber);
                    break;
                case 'V':
                    {
                      value v = va_arg(ap,value);
                      //s->put_str(CsTypeName(v));
                      //s->put_str("(");
                      CsPrint(c,v,s);
                      //s->put_str(")");
                    }
                    break;
                case 's':
                    s->put_str(va_arg(ap,char *));
                    break;
                case 'S':
                  {
                    const wchar* pm = va_arg(ap,wchar *);
                    while(*pm)
                    {
                      if( *pm <= ' ' )
                        s->put(' ');
                      else
                        s->put(*pm);
                      ++pm;
                    }
                  }
                  break;
                case 'i':
                    s->printf(L"%d",(int)va_arg(ap,int_t));
                    break;
                case 'b':
                    s->printf(L"%02x",(int)va_arg(ap,int_t));
                    break;
                }
                break;
            }
            /* fall through */
        default:
            *dst++ = ch;
            if (++cnt >= ERR_BUF_SIZE) {
                *dst = '\0';
                s->put_str(buf);
                dst = buf;
                cnt = 0;
            }
        }
    if (cnt > 0) {
        *dst = '\0';
        s->put_str(buf);
        dst = buf;
        cnt = 0;
    }
  //CsStreamPutC('\n',s);
}

/* CsGetErrorText - get the text for an error code */
char *CsGetErrorText(int code)
{
#ifdef _DEBUG
    if(CsErrAlreadyDefined == code)
      code = code;
#endif

    ErrString *p;
    for (p = errStrings; p->text != 0; ++p)
        if (code == p->code)
            return p->text;
    return "Unknown error";
}


//extern dispatch CsErrorDispatch;

static value CSF_ctor(VM *c);
static value CSF_toString(VM *c);

/* Error methods */

static c_method methods[] = {
C_METHOD_ENTRY( "this",      CSF_ctor            ),
C_METHOD_ENTRY( "toLocaleString",   CSF_std_toLocaleString  ),
C_METHOD_ENTRY( "toString",         CSF_toString        ),

C_METHOD_ENTRY( 0,          0         )
};

static value CSF_name(VM *c,value obj);
static value CSF_message(VM *c,value obj);
static value CSF_stackTrace(VM *c,value obj);

/* Error properties */

static vp_method properties[] = {
VP_METHOD_ENTRY( "name",            CSF_name,         0         ),
VP_METHOD_ENTRY( "message",         CSF_message,        0         ),
VP_METHOD_ENTRY( "stackTrace",      CSF_stackTrace,   0         ),

VP_METHOD_ENTRY( 0,                0,         0         )
};

bool ErrorPrint(VM *c,value obj,stream *s, bool toLocale = false)
{
  CsDisplay(c,CsErrorName(obj),s);
  s->put_str(": ");
  CsDisplay(c,CsErrorMessage(obj),s);
  //s->put_str("\nStack trace:\n");
  s->put_str("\n");
  CsDisplay(c,CsErrorStackTrace(obj),s);
  s->put_str("\n");
    return true;

}

/* GetIntegerProperty - Integer get property handler */
static bool GetErrorProperty(VM *c,value& obj,value tag,value *pValue)
{
    return CsGetVirtualProperty(c,obj,c->errorObject,tag,pValue);
}

/* SetIntegerProperty - Integer set property handler */
static bool SetErrorProperty(VM *c,value obj,value tag,value value)
{
    return  CsSetVirtualProperty(c,obj,c->errorObject,tag,value);
}

/* FloatSize - Float size handler */
static long ErrorSize(value obj)
{
    return sizeof(CsFixedVector) + sizeof(value) * NumErrorFields;
}

static void ErrorScan(VM *c,value obj)
{
    long i;
    dispatch *d = CsQuickGetDispatch(obj);
    for (i = 0; i < NumErrorFields; ++i)
        CsSetFixedVectorElement(obj,i,CsCopyValue(c,CsFixedVectorElement(obj,i)));
}


/* 'Error' pdispatch */
dispatch CsErrorDispatch = {
    "Error",
    &CsErrorDispatch,
    GetErrorProperty,
    SetErrorProperty,
    CsDefaultNewInstance,
    ErrorPrint,
    ErrorSize,
    CsDefaultCopy,
    ErrorScan,
    CsDefaultHash,
    CsDefaultGetItem,
    CsDefaultSetItem
};

/* CsInitErrorType - initialize the 'File' obj */
void CsInitErrorType(VM *c)
{
    c->errorObject = CsEnterType(CsGlobalScope(c),"Error",&CsErrorDispatch);
    CsEnterMethods(c,c->errorObject,methods);
    CsEnterVPMethods(c,c->errorObject,properties);
}

value CsError(VM *c, int n, value message)
{
    value v[NumErrorFields];
    string_stream s(256);

    v[0] = CsInternCString(c,"Error");
    v[1] = message;
    //CsInitStringOutputStream(c,&s,256);
    CsStreamStackTrace(c,&s);
    v[2] = s.string_o(c);
    v[3] = CsMakeInteger(n);
    s.close();

    return CsMakeFixedVector(c,&CsErrorDispatch,NumErrorFields,v);
}

void CsWarning( VM* c, const char* msg )
{
  c->standardError->printf(L"WARNING:%S\n",msg);
  CsStreamStackTrace(c,c->standardError);
  c->standardError->printf(L"\n");
}

static value CSF_ctor(VM *c)
{
    value message;
    CsParseArguments(c,"**V",&message);
    return (CsCtorRes(c) = CsError(c,0,message));

}
static value CSF_toString(VM *c)
{
    value obj;

    CsParseArguments(c,"V=*",&obj,&CsErrorDispatch);

    string_stream s(64);

    ErrorPrint(c,obj,&s);
    
    value r;
    r = s.string_o(c);
      
    s.close();
    //return CsFormatString(c, "%s:%s\n%s", NumErrorFields, CsFixedVectorAddress(obj));
    //CsErrorName(obj), CsErrorMessage(obj) CsStackTrace(obj)

    return r;
}

static value CSF_name(VM *c,value obj)
{
  return  CsErrorName(obj);
}
static value CSF_message(VM *c,value obj)
{
  return  CsErrorMessage(obj);
}
static value CSF_stackTrace(VM *c,value obj)
{
  return CsErrorStackTrace(obj);
}

}
