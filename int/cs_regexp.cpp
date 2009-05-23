/* RegExp.c - 'RegExp' handler */
/*
        Copyright (c) 2001-2004 Terra Informatica Software, Inc.
        and Andrew Fedoniouk andrew@terrainformatica.com
        All rights reserved
*/

#include "cs.h"
#include "tl_wregexp.h"

namespace tis 
{

/* 'RegExp' */

/* method handlers */
static value CSF_ctor(VM *c);
static value CSF_test(VM *c);
static value CSF_exec(VM *c);

/* file methods */
static c_method methods[] = {
C_METHOD_ENTRY( "this",     CSF_ctor          ),
//C_METHOD_ENTRY( "toLocaleString",  CSF_std_toLocaleString  ),
C_METHOD_ENTRY( "test",            CSF_test          ),
C_METHOD_ENTRY( "exec",            CSF_exec          ),
C_METHOD_ENTRY( 0,                  0                  )
};

/* file properties */
static value CSF_length(VM *c,value obj);
static value CSF_input(VM *c,value obj);
static value CSF_source(VM *c,value obj);
static value CSF_index(VM *c,value obj);
static value CSF_lastIndex(VM *c,value obj);

static vp_method properties[] = {
VP_METHOD_ENTRY( "length",         CSF_length,    0     ),
VP_METHOD_ENTRY( "input",          CSF_input,     0     ),
VP_METHOD_ENTRY( "source",         CSF_source,    0     ),
VP_METHOD_ENTRY( "index",          CSF_index,     0     ),
VP_METHOD_ENTRY( "lastIndex",      CSF_lastIndex, 0     ),
VP_METHOD_ENTRY( 0,                0,             0     )
};

inline tool::wregexp* RegExpValue(VM *c, value obj) 
{  
  return CsRegExpP(c,obj)? (tool::wregexp*)CsCObjectValue(obj):0; 
}

/* prototypes */
static void DestroyRegExp(VM *c,value obj);

value RegExpGetItem(VM *c,value obj,value tag)
{
  if(!CsRegExpP(c,obj))
    return c->undefinedValue;
  if(!CsIntegerP(tag))
     CsTypeError(c,tag);
  
  tool::wregexp* pre = RegExpValue(c,obj);
  if(!pre)
      return c->undefinedValue;

  int_t idx = CsIntegerValue(tag);
  if(idx < pre->get_number_of_matches())
      return string_to_value(c, pre->get_match(idx));

  return c->undefinedValue;

}
void RegExpSetItem(VM *c,value obj,value tag,value value)
{
   CsThrowKnownError(c,CsErrReadOnlyProperty,tag);
}


/* CsInitRegExp - initialize the 'RegExp' obj */
void CsInitRegExp(VM *c)
{
    /* create the 'RegExp' type */
    if (!(c->regexpDispatch = CsEnterCPtrObjectType(CsGlobalScope(c),NULL,"RegExp",methods,properties)))
        CsInsufficientMemory(c);

    /* setup alternate handlers */
    c->regexpDispatch->destroy = DestroyRegExp;
    c->regexpDispatch->getItem = RegExpGetItem;
    c->regexpDispatch->setItem = RegExpSetItem;
    //c->regexpDispatch->dataSize = sizeof(CsRegExp) - sizeof(CsCObject);
}

bool CsRegExpP(VM *c, value obj)
{
  return CsIsType(obj,c->regexpDispatch);
}

/* CsMakeRegExp - make a 'RegExp' obj */
value CsMakeRegExp(VM *c,tool::wregexp *re)
{
  value v = CsMakeCPtrObject(c,c->regexpDispatch,re);
  return v;
}

inline void SetRegExpValue(value obj, tool::wregexp* pw)
{
  CsSetCObjectValue(obj,pw);
}

/* CSF_ctor - built-in method 'initialize' */
static value CSF_ctor(VM *c)
{
    value src,flags;
    value val;
    CsParseArguments(c,"V=*VV?",&val,c->regexpDispatch,&src,&flags);


    //tool::wregexp* pre = RegExpValue(c,val);
    //pre->source = src;
    //pre->flags = flags;
    
    tool::ustring f = value_to_string(flags);
    tool::ustring s = value_to_string(src);
    
    tool::wregexp* pre = new tool::wregexp();
    if( !pre->compile(s,f.index_of('i') >= 0, f.index_of('g') >= 0) )
    {
      delete pre;
      return val;
    }
    SetRegExpValue(val,pre);
    
    CsCtorRes(c) = val;
    return val;
}

/* DestroyRegExp - destroy a file obj */
static void DestroyRegExp(VM *c,value obj)
{
  tool::wregexp* pre = RegExpValue(c,obj);
  delete pre;
  SetRegExpValue(obj, 0);
}

static value CSF_length(VM *c,value obj)
{
  if(CsRegExpP(c,obj))
  {
    tool::wregexp* pre = RegExpValue(c,obj);
    if(!pre)
      return CsMakeInteger(0);
    return CsMakeInteger(pre->get_number_of_matches());
  }
  return c->undefinedValue;
}

static value CSF_index(VM *c,value obj)
{
  if(CsRegExpP(c,obj))
  {
    tool::wregexp* pre = RegExpValue(c,obj);
    if(!pre)
      return CsMakeInteger(0);
    return CsMakeInteger(pre->get_match_start());
  }
  return c->undefinedValue;
}
static value CSF_lastIndex(VM *c,value obj)
{
  if(CsRegExpP(c,obj))
  {
    tool::wregexp* pre = RegExpValue(c,obj);
    if(!pre)
      return CsMakeInteger(0);
    return CsMakeInteger(pre->get_match_end());
  }
  return c->undefinedValue;
}


static value CSF_input(VM *c,value obj)
{
  if(CsRegExpP(c,obj))
  {
    tool::wregexp* pre = RegExpValue(c,obj);
    if(!pre)
      return c->undefinedValue;
    return string_to_value(c,pre->m_test);
  }
  return c->undefinedValue;
}

static value CSF_source(VM *c,value obj)
{
  if(CsRegExpP(c,obj))
  {
    tool::wregexp* pre = RegExpValue(c,obj);
    if(!pre)
      return c->undefinedValue;
    return string_to_value(c,pre->m_pattern);
  }
  return c->undefinedValue;
}


/* CSF_test - built-in method 'test' */
static value CSF_test(VM *c)
{
    //return c->falseValue;
    value obj;
    wchar *str;
    CsParseArguments(c,"V=*S",&obj,c->regexpDispatch,&str);
    tool::wregexp* pre = RegExpValue(c,obj);
    if(!pre)
      return c->undefinedValue;
    return pre->exec(str)? c->trueValue: c->falseValue;
}

static value CSF_exec(VM *c)
{
    //return c->falseValue;
    value obj;
    wchar *str;
    CsParseArguments(c,"V=*S",&obj,c->regexpDispatch,&str);
    tool::wregexp* pre = RegExpValue(c,obj);
    if(!pre)
      return c->undefinedValue;

    int idx = 0;
    if(pre->get_number_of_matches())
      idx = pre->get_match_end();

#pragma TODO("wrong return?!?")

    return pre->exec(str)? obj: c->nullValue;
}

value CSF_string_match(VM *c)
{
    value obj;
    value pat;
    CsParseArguments(c,"V*V",&obj,&pat);

    obj = CsToString(c,obj);
    tool::ustring test = value_to_string(obj);

    if(CsRegExpP(c,pat))
    {
      tool::wregexp* pre = RegExpValue(c,pat);
      if(!pre)
        CsThrowKnownError(c,CsErrRegexpError,"wrong RE object");
      if(pre->exec(test))
      {
        if(pre->get_number_of_matches() == 1)
          return string_to_value( c, pre->get_match() );
        else
        {
          value vec = CsMakeVector(c,pre->get_number_of_matches());
          CsPush(c,vec);
          for( int i = 0; i < pre->get_number_of_matches(); ++i )
          {
            CsSetVectorElement(c, CsTop(c), i, string_to_value( c, pre->get_match(i) ) );
          }
          return CsPop(c);
        }
        //return pre->exec(test)?pat: c->nullValue;
      }
      return c->nullValue;
    }
    else if(CsStringP(pat))
    {
      tool::auto_ptr<tool::wregexp> pre(new tool::wregexp);
      if(!pre->compile(value_to_string(pat),false,false))
      {
        CsThrowKnownError(c,CsErrRegexpError,"bad expression");
      }
      if(pre->exec(test))
        return CsMakeRegExp(c,pre.release());
      return c->nullValue;
    }
    else
      CsTypeError(c,pat);
    return c->undefinedValue;
}

bool CsIsLike( VM *c, value what, value pat )
{
    if( !CsStringP(what) )
      CsThrowKnownError(c, CsErrUnexpectedTypeError, what, "left side of 'like' must be string");

    tool::wchars str( CsStringAddress(what), CsStringSize(what) );

    if( CsStringP(pat) )
    {
      return tool::is_like(str, CsStringAddress(pat));
    }
    else if(CsRegExpP(c,pat))
    {
      tool::wregexp* pre = RegExpValue(c,pat);
      if(!pre) CsThrowKnownError(c,CsErrRegexpError,"wrong RE object");
      return pre->exec(str.start)? true: false;
    }
    else
      CsThrowKnownError(c, CsErrUnexpectedTypeError, pat, "right side of 'like' must be string or regexp");
    return false;
}

value CSF_string_search(VM *c)
{
    value obj;
    value pat;
    CsParseArguments(c,"V*V",&obj,&pat);

    obj = CsToString(c,obj);
    tool::ustring test = value_to_string(obj);

    if(CsRegExpP(c,pat))
    {
      tool::wregexp* pre = RegExpValue(c,pat);
      if(!pre)
        CsThrowKnownError(c,CsErrRegexpError,"wrong RE object");
      pre->m_nextIndex = 0;
      if(pre->exec(test))
        return CsMakeInteger(pre->get_match_start());
      else
        return CsMakeInteger(-1);
    }
    else if(CsStringP(pat))
    {
      tool::auto_ptr<tool::wregexp> pre(new tool::wregexp);
      if(!pre->compile(value_to_string(pat),false,false))
      {
        CsThrowKnownError(c,CsErrRegexpError,"bad expression");
      }
      if(pre->exec(test))
        return CsMakeInteger(pre->get_match_start());
      else
        return CsMakeInteger(-1);
    }
    else
      CsTypeError(c,pat);
    return c->undefinedValue;
}


value CSF_string_replace(VM *c)
{
    value obj;
    value pat;
    value rep;
    CsParseArguments(c,"V*VV",&obj,&pat,&rep);

    obj = CsToString(c,obj);
    tool::ustring test = value_to_string(obj);

    if(!CsStringP(rep))
      CsTypeError(c,rep);
    
    if( CsStringP(pat) )
    {
      if(test.replace( CsStringAddress(pat), CsStringAddress(rep) ))
        return string_to_value(c, test);
      else
        return obj;
    }

    if(!CsRegExpP(c,pat))
        CsThrowKnownError(c,CsErrRegexpError,"first parameter is not a RE object");

    tool::wregexp* pre = RegExpValue(c,pat);
    if(!pre)
      CsThrowKnownError(c,CsErrRegexpError,"wrong RE object");

    bool g = pre->m_global;
    pre->m_nextIndex = 0;

    if(!pre->exec(test))
      return obj;

    pre->m_global = g;

    tool::ustring reps = value_to_string(rep);

    int start = 0;
    int end = pre->get_match_start(0);

    string_stream s(test.length());
    
    for( int i = 1; i <= pre->get_number_of_matches(); ++i )
    {
      s.put_str( (const wchar*)test + start, (const wchar*)test + end );
      s.put_str( reps );
      start = pre->get_match_end(i-1);
      end = i == pre->get_number_of_matches()? test.length(): pre->get_match_start(i);
    }

    s.put_str( (const wchar*)test + start, (const wchar*)test + end );

    return s.string_o(c);
}

value CSF_string_split(VM *c)
{
    value obj;
    value pat;
    int maxn = 0x80000;
    CsParseArguments(c,"V*V|i",&obj,&pat,&maxn);

    obj = CsToString(c,obj);
    tool::ustring t;
    tool::wchars test = value_to_wchars(obj,t);

    const wchar * start = test.start;
    const wchar * end = start;
    tool::array< tool::wchars > slices;

    if(CsRegExpP(c,pat))
    {
      tool::wregexp *pre = RegExpValue(c,pat);
      if(!pre)
        CsThrowKnownError(c,CsErrRegexpError,"wrong RE object");

      bool g = pre->m_global;
      pre->m_nextIndex = 0;

      //start = pre->text().start;

      pre->m_global = true;
      if( pre->exec(test.start) )
      {
        maxn = min(maxn, pre->get_number_of_matches());
        for(int i = 0; i < maxn; ++i )
        {
          end = test.start + pre->get_n_match(i).begin;
          slices.push( tool::wchars( start, end ) );
          start = test.start + pre->get_n_match(i).end;
        }
        end = test.end();
        slices.push( tool::wchars( start, end ) );
      }
      else
        slices.push( test );


      pre->m_global = g;
    }
    else if(CsStringP(pat))
    {
      tool::ustring t1;
      tool::wchars spat = value_to_wchars(pat,t1);
      
      for(int i = 0; *start && i < maxn; ++i )
      {
        end = wcsstr(start,spat.start);
        if(end == 0) break;
        slices.push( tool::wchars( start, end ) );
        start = end + spat.length;
      }
      end = test.end();
      slices.push( tool::wchars( start, end ) );
    }
    else
      CsTypeError(c,pat);

    value vec = CsMakeVector(c, slices.size() );
    CsPush(c,vec);
    for( int i = 0; i < slices.size(); ++i )
    {
      CsSetVectorElement(c, CsTop(c), i, 
        CsMakeCharString(c,slices[i].start,slices[i].length) ); 
    }
    return CsPop(c);
}



}
