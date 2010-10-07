/*
        Copyright (c) 2001-2004 Terra Informatica Software, Inc.
        and Andrew Fedoniouk andrew@terrainformatica.com
        All rights reserved
*/

#include "cs.h"
#include <time.h>

// for tool::datetime_t...
//#define WIN32_LEAN_AND_MEAN
//#include <windows.h>

namespace tis 
{

/* method handlers */
static value CSF_ctor(VM *c);
static value CSF_now(VM *c);

/*static value CSF_getYear(VM *c);
static value CSF_getFullYear(VM *c);
static value CSF_getMonth(VM *c);
static value CSF_getDate(VM *c);
static value CSF_getDay(VM *c);
static value CSF_getHours(VM *c);
static value CSF_getMinutes(VM *c);
static value CSF_getSeconds(VM *c);
static value CSF_getMilliseconds(VM *c);

static value CSF_getUTCYear(VM *c);
static value CSF_getUTCFullYear(VM *c);
static value CSF_getUTCMonth(VM *c);
static value CSF_getUTCDate(VM *c);
static value CSF_getUTCDay(VM *c);
static value CSF_getUTCHours(VM *c);
static value CSF_getUTCMinutes(VM *c);
static value CSF_getUTCSeconds(VM *c);
static value CSF_getUTCMilliseconds(VM *c);

static value CSF_setYear(VM *c);
static value CSF_setFullYear(VM *c);
static value CSF_setMonth(VM *c);
static value CSF_setDate(VM *c);
static value CSF_setHours(VM *c);
static value CSF_setMinutes(VM *c);
static value CSF_setSeconds(VM *c);
static value CSF_setMilliseconds(VM *c);

static value CSF_setUTCYear(VM *c);
static value CSF_setUTCFullYear(VM *c);
static value CSF_setUTCMonth(VM *c);
static value CSF_setUTCDate(VM *c);
static value CSF_setUTCHours(VM *c);
static value CSF_setUTCMinutes(VM *c);
static value CSF_setUTCSeconds(VM *c);
static value CSF_setUTCMilliseconds(VM *c); */

static value CSF_toGMTString(VM *c);
static value CSF_toString(VM* c);
static value CSF_toUTCString(VM *c);
static value CSF_toISOString(VM *c);
static value CSF_toLocaleString(VM *c);
static value CSF_monthName(VM *c);
static value CSF_dayOfWeekName(VM *c);

static value CSF_valueOf(VM *c);
static value CSF_parse(VM *c);
static value CSF_UTC(VM *c);
static value CSF_setTime(VM *c);
static value CSF_ticks(VM *c);

static value CSF_day(VM *c,value obj); static void CSF_set_day(VM *c,value obj,value value);
static value CSF_dayOfWeek(VM *c,value obj); 
static value CSF_firstDayOfWeek(VM *c,value obj); 
static value CSF_month(VM *c,value obj); static void CSF_set_month(VM *c,value obj,value value);
static value CSF_year(VM *c,value obj); static void CSF_set_year(VM *c,value obj,value value);
static value CSF_hour(VM *c,value obj); static void CSF_set_hour(VM *c,value obj,value value);
static value CSF_minute(VM *c,value obj); static void CSF_set_minute(VM *c,value obj,value value);
static value CSF_second(VM *c,value obj); static void CSF_set_second(VM *c,value obj,value value);
static value CSF_millisecond(VM *c,value obj); static void CSF_set_millisecond(VM *c,value obj,value value);


static value CSF_utc_day(VM *c,value obj); static void CSF_set_utc_day(VM *c,value obj,value value);
static value CSF_utc_dayOfWeek(VM *c,value obj); 
static value CSF_utc_month(VM *c,value obj); static void CSF_set_utc_month(VM *c,value obj,value value);
static value CSF_utc_year(VM *c,value obj); static void CSF_set_utc_year(VM *c,value obj,value value);
static value CSF_utc_hour(VM *c,value obj); static void CSF_set_utc_hour(VM *c,value obj,value value);
static value CSF_utc_minute(VM *c,value obj); static void CSF_set_utc_minute(VM *c,value obj,value value);
static value CSF_utc_second(VM *c,value obj); static void CSF_set_utc_second(VM *c,value obj,value value);
static value CSF_utc_millisecond(VM *c,value obj); static void CSF_set_utc_millisecond(VM *c,value obj,value value);

static value CSF_localOffset(VM *c); 
static value CSF_localTimeZone(VM *c); 
static value CSF_isDaylight(VM *c);


/* file methods */
static c_method methods[] = {
C_METHOD_ENTRY( "this",      CSF_ctor            ),

/*
C_METHOD_ENTRY( "getDate",          CSF_getDate         ),
C_METHOD_ENTRY( "getMonth",         CSF_getMonth        ),
C_METHOD_ENTRY( "getYear",          CSF_getYear         ),
C_METHOD_ENTRY( "getFullYear",      CSF_getFullYear     ),
C_METHOD_ENTRY( "getHours",         CSF_getHours        ),
C_METHOD_ENTRY( "getMinutes",       CSF_getMinutes      ),
C_METHOD_ENTRY( "getSeconds",       CSF_getSeconds      ),
C_METHOD_ENTRY( "getMilliseconds",  CSF_getMilliseconds ),
C_METHOD_ENTRY( "getDay",           CSF_getDay ),

C_METHOD_ENTRY( "getUTCDate",          CSF_getUTCDate         ),
C_METHOD_ENTRY( "getUTCMonth",         CSF_getUTCMonth        ),
C_METHOD_ENTRY( "getUTCYear",          CSF_getUTCYear         ),
C_METHOD_ENTRY( "getUTCFullYear",      CSF_getUTCFullYear     ),
C_METHOD_ENTRY( "getUTCHours",         CSF_getUTCHours        ),
C_METHOD_ENTRY( "getUTCMinutes",       CSF_getUTCMinutes      ),
C_METHOD_ENTRY( "getUTCSeconds",       CSF_getUTCSeconds      ),
C_METHOD_ENTRY( "getUTCMilliseconds",  CSF_getUTCMilliseconds ),
C_METHOD_ENTRY( "getUTCDay",           CSF_getUTCDay ),

C_METHOD_ENTRY( "setDate",          CSF_setDate         ),
C_METHOD_ENTRY( "setMonth",         CSF_setMonth        ),
C_METHOD_ENTRY( "setYear",          CSF_setYear         ),
C_METHOD_ENTRY( "setFullYear",      CSF_setFullYear     ),
C_METHOD_ENTRY( "setHours",         CSF_setHours        ),
C_METHOD_ENTRY( "setMinutes",       CSF_setMinutes      ),
C_METHOD_ENTRY( "setSeconds",       CSF_setSeconds      ),
C_METHOD_ENTRY( "setMilliseconds",  CSF_setMilliseconds ),

C_METHOD_ENTRY( "setUTCDate",          CSF_setUTCDate         ),
C_METHOD_ENTRY( "setUTCMonth",         CSF_setUTCMonth        ),
C_METHOD_ENTRY( "setUTCYear",          CSF_setUTCYear         ),
C_METHOD_ENTRY( "setUTCFullYear",      CSF_setUTCFullYear     ),
C_METHOD_ENTRY( "setUTCHours",         CSF_setUTCHours        ),
C_METHOD_ENTRY( "setUTCMinutes",       CSF_setUTCMinutes      ),
C_METHOD_ENTRY( "setUTCSeconds",       CSF_setUTCSeconds      ),
C_METHOD_ENTRY( "setUTCMilliseconds",  CSF_setUTCMilliseconds ),
*/

C_METHOD_ENTRY( "toString",       CSF_toString ),
C_METHOD_ENTRY( "toISOString",    CSF_toISOString ),
C_METHOD_ENTRY( "toUTCString",    CSF_toUTCString ),
C_METHOD_ENTRY( "toGMTString",    CSF_toGMTString ),
C_METHOD_ENTRY( "toLocaleString", CSF_toLocaleString ),
C_METHOD_ENTRY( "valueOf",        CSF_valueOf ),
C_METHOD_ENTRY( "parse",          CSF_parse ),
C_METHOD_ENTRY( "UTC",            CSF_UTC ),
C_METHOD_ENTRY( "setTime",        CSF_setTime ),
C_METHOD_ENTRY( "monthName",        CSF_monthName       ),
C_METHOD_ENTRY( "dayOfWeekName",    CSF_dayOfWeekName   ),

C_METHOD_ENTRY( "timeZoneOffset",  CSF_localOffset      ),
C_METHOD_ENTRY( "timeZoneName",    CSF_localTimeZone    ),
C_METHOD_ENTRY( "isDaylight",      CSF_isDaylight   ),
C_METHOD_ENTRY( "ticks",           CSF_ticks   ),
C_METHOD_ENTRY( "now",             CSF_now   ),



C_METHOD_ENTRY( 0,                  0                  )
};

/* file properties */
static vp_method properties[] = {
VP_METHOD_ENTRY( "day",             CSF_day,               CSF_set_day  ),
VP_METHOD_ENTRY( "dayOfWeek",       CSF_dayOfWeek,         0  ),
VP_METHOD_ENTRY( "month",           CSF_month,             CSF_set_month  ),
VP_METHOD_ENTRY( "year",            CSF_year,              CSF_set_year  ),
VP_METHOD_ENTRY( "hour",            CSF_hour,              CSF_set_hour  ),
VP_METHOD_ENTRY( "minute",          CSF_minute,            CSF_set_minute  ),
VP_METHOD_ENTRY( "second",          CSF_second,            CSF_set_second  ),
VP_METHOD_ENTRY( "millisecond",     CSF_millisecond,       CSF_set_millisecond  ),

VP_METHOD_ENTRY( "UTCday",          CSF_utc_day,           CSF_set_utc_day  ),
VP_METHOD_ENTRY( "UTCdayOfWeek",    CSF_utc_dayOfWeek,     0  ),
VP_METHOD_ENTRY( "UTCmonth",        CSF_utc_month,         CSF_set_utc_month  ),
VP_METHOD_ENTRY( "UTCyear",         CSF_utc_year,          CSF_set_utc_year  ),
VP_METHOD_ENTRY( "UTChour",         CSF_utc_hour,          CSF_set_utc_hour  ),
VP_METHOD_ENTRY( "UTCminute",       CSF_utc_minute,        CSF_set_utc_minute  ),
VP_METHOD_ENTRY( "UTCsecond",       CSF_utc_second,        CSF_set_utc_second  ),
VP_METHOD_ENTRY( "UTCmillisecond",  CSF_utc_millisecond,   CSF_set_utc_millisecond  ),

VP_METHOD_ENTRY( "firstDayOfWeek",  CSF_firstDayOfWeek, 0  ),

VP_METHOD_ENTRY( 0,                0,         0         )
};

/* CsInitDate - initialize the 'Date' obj */
void CsInitDate(VM *c)
{
    /* create the 'File' type */
    if (!(c->dateDispatch = CsEnterCObjectType(CsGlobalScope(c),NULL,"Date",
        methods,properties, 0, sizeof(tool::datetime_t))))
        CsInsufficientMemory(c);

}

bool CsDateP(VM *c, value obj)
{
  return CsIsType(obj,c->dateDispatch);
}

tool::datetime_t& CsDateValue(VM* c, value obj)
{
  assert( CsDateP(c, obj) );
  return *((tool::datetime_t*)CsCObjectDataAddress(obj));
}


/* CsMakeDate - make a 'Date' obj */
value CsMakeDate(VM *c, tool::datetime_t dta)
{
  value newo = CsMakeCObject(c,c->dateDispatch);
  CsDateValue(c,newo) = dta;
  return newo;
}

//inline void* CsCObjectValue(value o)        { return ((CsCPtrObject *)o)->ptr; }
//inline void  CsSetCObjectValue(value o,void* v) { ((CsCPtrObject *)o)->ptr = v; }

static bool ParseDateTime(value s, tool::datetime_t& utc);

static int64 ms1970()
{
  static int64 v = 0;
  if(!v)
  {
    tool::date_time dt;
    dt.set_date(1970,1,1); 
    v = dt.absolute_millis();
  }
  return v;
}

/* CSF_ticks - built-in method 'ticks' */
static value CSF_ticks(VM *c)
{
#ifdef WINDOWS
  return int_value(GetTickCount());
#else
  tms tm;
  return int_value(times(&tm));
#endif
}


/* CSF_ctor - built-in method 'initialize' */
static value CSF_ctor(VM *c)
{
    value val;
    value p1;

    int_t year, month, date, hours, minutes, seconds, ms; 

    int_t n = c->argc - 2;

    tool::datetime_t t;

    CsParseArguments(c,"V=*|V|i|i|i|i|i|i",&val,c->dateDispatch,&p1,
      &month, &date, &hours, &minutes, &seconds, &ms);

    switch( n )
    {
    case 0: 
  {
          t = tool::date_time::now(true).time();
  } break;
    case 1: 
        if( CsFloatP(p1) )
        {
          int64 n = int64(CsFloatValue(p1));
          t = (ms1970() + n) * 10000;
        }
        else if ( CsStringP(p1) )
        {
          if( !ParseDateTime(p1,t) )
             CsBadValue(c, p1);
        }
        else if ( CsDateP(c,p1) )
          t = CsDateValue(c,p1);
        else 
          CsTypeError(c, p1);
        break;
    case 3: case 4: case 5: case 6: case 7:
      {
        if( !CsIntegerP(p1) )
            CsTypeError(c, p1);
        year = CsIntegerValue(p1);
        
        tool::date_time ts(year, month, date);
        if( n >= 4 )
        {
          ts.hours(hours);
          if( n >= 5 )
          {
            ts.minutes(minutes);
            if( n >= 6 )
            {
              ts.seconds(seconds);
              if(n == 7)
              {
                ts.millis(ms);
              }
            }
          }
        }
        ts.to_utc();
  t = ts.time();
        //SystemTimeToFileTime(&ts,&t);
        break;
      }
    default:
        CsTooManyArguments(c);
        break;
    }
    
    CsDateValue(c,val) = t;
    CsCtorRes(c) = val;
    return val;
}

/* CSF_now - built-in method 'now' */
static value CSF_now(VM *c)
{
    bool  sequential = false;

    CsParseArguments(c,"**|B",&sequential);

    tool::datetime_t t = tool::date_time::now().time();
    if( sequential )
    {
      // this ensures that two consequtive calls of Date.now(true) return distinct values.
      static tool::mutex guard;
      static tool::datetime_t last_t = 0;
      tool::critical_section _(guard);
      if( t <= last_t )
        t = last_t + 1;
      last_t = t;
    }
    return CsMakeDate(c,t);
}



/* CSF_UTC - built-in method 'UTC' */
static value CSF_UTC(VM *c)
{
    value val;
    int_t year, month, date, hours, minutes, seconds, ms; 
    int_t n = c->argc - 2;

    tool::datetime_t t;

    CsParseArguments(c,"V=*iii|i|i|i|i",&val,c->dateDispatch,&year,
      &month, &date, &hours, &minutes, &seconds, &ms);

    tool::date_time ts(ushort(year), ushort(month + 1), ushort(date));
    if( n >= 4 )
    {
      ts.hours(ushort(hours));
      if( n >= 5 )
      {
        ts.minutes(ushort(minutes));
        if( n >= 6 )
        {
          ts.seconds(ushort(seconds));
          if(n == 7)
          {
            ts.millis(ushort(ms));
          }
        }
      }
    }
    t = ts.time();

/*        
    SYSTEMTIME ts; memset(&ts,0, sizeof(ts));
    ts.wYear = ushort(year);
    ts.wMonth = ushort(month + 1);
    ts.wDay = ushort(date);
    if( n >= 4 )
    {
      ts.wHour = ushort(hours);
      if( n >= 5 )
      {
        ts.wMinute = ushort(minutes);
        if( n >= 6 )
        {
          ts.wSecond = ushort(seconds);
          if(n == 7)
          {
            ts.wMilliseconds = ushort(ms);
          }
        }
      }
    }
    SystemTimeToFileTime(&ts,&t);


    SYSTEMTIME st; memset(&st,0, sizeof(st));
    st.wDay = 1;
    st.wMonth = 1;
    st.wYear = 1970;
    tool::datetime_t ftzero;
    SystemTimeToFileTime(&st,&ftzero);

    int64 n1 = (( ft64(t) - ft64(ftzero) ) / 10000);
    double d = double(n1);
    return CsMakeFloat(c, d );
*/
    return CsMakeFloat(c, float_t((ts.absolute_millis() - ms1970()) / 10000) );
    
}

/* CSF_setTime - built-in method 'initialize' */
static value CSF_setTime(VM *c)
{
    value val;
    //tool::datetime_t t;
    float_t  ms;
    CsParseArguments(c,"V=*d",&val,c->dateDispatch,&ms);

    tool::date_time ts;
    ts.absolute_millis( ms1970() + int64(ms) );
    //ft64(t) = (ms1970() + int64(ms)) * 10000;
    CsDateValue(c,val) = ts.time();
    return val;
}

/*
inline SYSTEMTIME get_local(VM *c, value d)
{
    tool::datetime_t ft = CsDateValue(c,d);
    FileTimeToLocalFileTime(&ft,&ft);
    SYSTEMTIME st;
    FileTimeToSystemTime(&ft,&st);
    return st;
}

inline int64 get_local_64(VM *c, value d)
{
    tool::datetime_t ft = CsDateValue(c,d);
    FileTimeToLocalFileTime(&ft,&ft);
    return *((int64*)&ft);
}

inline void set_local_64(VM *c, value d, int64 t)
{
    tool::datetime_t ft = *((tool::datetime_t*)&t);
    LocalFileTimeToFileTime(&ft,&ft);
    CsDateValue(c,d) = ft;
}

inline int64 get_utc_64(VM *c, value d)
{
    tool::datetime_t ft = CsDateValue(c,d);
    return *((int64*)&ft);
}

inline void set_utc_64(VM *c, value d, int64 t)
{
    tool::datetime_t ft = *((tool::datetime_t*)&t);
    CsDateValue(c,d) = ft;
}


inline SYSTEMTIME get_local(VM *c)
{
    value d;
    CsParseArguments(c,"V=*",&d,c->dateDispatch);
    return get_local(c,d);
}

inline SYSTEMTIME get_utc(VM *c, value d)
{
    tool::datetime_t ft = CsDateValue(c,d);
    SYSTEMTIME st;
    FileTimeToSystemTime(&ft,&st);
    return st;
}

inline SYSTEMTIME get_utc(VM *c)
{
    value d;
    CsParseArguments(c,"V=*",&d,c->dateDispatch);
    return get_utc(c,d);
}
*/

inline tool::date_time get_local(VM *c, value d)
{
    tool::datetime_t ft = CsDateValue(c,d);
    tool::date_time t(ft);
    t.to_local();
    return t;
}

inline tool::date_time get_utc(VM *c, value d)
{
    tool::datetime_t ft = CsDateValue(c,d);
    tool::date_time t(ft);
    return t;
}


static value CSF_year(VM *c,value obj)
{
    tool::date_time st = get_local(c,obj);
    return CsMakeInteger(st.year());
}
static value CSF_month(VM *c,value obj)
{
    tool::date_time st = get_local(c,obj);
    return CsMakeInteger(st.month());
}
static value CSF_day(VM *c,value obj)
{
    tool::date_time st = get_local(c,obj);
    return CsMakeInteger(st.day());
}
static value CSF_dayOfWeek(VM *c,value obj)
{
    tool::date_time st = get_local(c,obj);
    return CsMakeInteger(st.day_of_week());
}

static value CSF_firstDayOfWeek(VM *c,value obj)
{
    return CsMakeInteger(tool::date_time::first_day_of_week());
}

static value CSF_hour(VM *c,value obj)
{
    tool::date_time st = get_local(c,obj);
    return CsMakeInteger(st.hours());
}
static value CSF_minute(VM *c,value obj)
{
    tool::date_time st = get_local(c,obj);
    return CsMakeInteger(st.minutes());
}
static value CSF_second(VM *c,value obj)
{
    tool::date_time st = get_local(c,obj);
    return CsMakeInteger(st.seconds());
}

static value CSF_millisecond(VM *c,value obj)
{
    tool::date_time st = get_local(c,obj);
    return CsMakeInteger(st.millis());
}


//---
static value CSF_utc_year(VM *c,value obj)
{
    tool::date_time st = get_utc(c,obj);
    return CsMakeInteger(st.year());
}
static value CSF_utc_month(VM *c,value obj)
{
    tool::date_time st = get_utc(c,obj);
    return CsMakeInteger(st.month());
}
static value CSF_utc_day(VM *c,value obj)
{
    tool::date_time st = get_utc(c,obj);
    return CsMakeInteger(st.day());
}
static value CSF_utc_dayOfWeek(VM *c,value obj)
{
    tool::date_time st = get_utc(c,obj);
    return CsMakeInteger(st.day_of_week());
}
static value CSF_utc_hour(VM *c,value obj)
{
    tool::date_time st = get_utc(c,obj);
    return CsMakeInteger(st.hours());
}
static value CSF_utc_minute(VM *c,value obj)
{
    tool::date_time st = get_utc(c,obj);
    return CsMakeInteger(st.minutes());
}
static value CSF_utc_second(VM *c,value obj)
{
    tool::date_time st = get_utc(c,obj);
    return CsMakeInteger(st.seconds());
}
static value CSF_utc_millisecond(VM *c,value obj)
{
    tool::date_time st = get_utc(c,obj);
    return CsMakeInteger(st.millis());
}


//---


/*
static value CSF_getFullYear(VM *c)
{
    SYSTEMTIME st = get_local(c);
    return CsMakeInteger( st.wYear);
}

static value CSF_getYear(VM *c)
{
    SYSTEMTIME st = get_local(c);
    return CsMakeInteger(st.wYear - 1900);
}

static value CSF_getMonth(VM *c)
{
    SYSTEMTIME st = get_local(c);
    return CsMakeInteger( st.wMonth - 1);
}

static value CSF_getDate(VM *c)
{
    SYSTEMTIME st = get_local(c);
    return CsMakeInteger(st.wDay);
}

static value CSF_getDay(VM *c)
{
    SYSTEMTIME st = get_local(c);
    return CsMakeInteger(st.wDayOfWeek);
}

static value CSF_getHours(VM *c)
{
    SYSTEMTIME st = get_local(c);
    return CsMakeInteger(st.wHour);
}

static value CSF_getMinutes(VM *c)
{
  SYSTEMTIME st = get_local(c);
  return CsMakeInteger(st.wMinute);
}

static value CSF_getSeconds(VM *c)
{
  SYSTEMTIME st = get_local(c);
  return CsMakeInteger(st.wSecond);
}

static value CSF_getMilliseconds(VM *c)
{
  SYSTEMTIME st = get_local(c);
  return CsMakeInteger(st.wMilliseconds);
}

static value CSF_getUTCFullYear(VM *c)
{
    SYSTEMTIME st = get_utc(c);
    return CsMakeInteger(st.wYear);
}

static value CSF_getUTCYear(VM *c)
{
    SYSTEMTIME st = get_utc(c);
    return CsMakeInteger(st.wYear - 1900);
}

static value CSF_getUTCMonth(VM *c)
{
    SYSTEMTIME st = get_utc(c);
    return CsMakeInteger(st.wMonth - 1);
}

static value CSF_getUTCDate(VM *c)
{
    SYSTEMTIME st = get_utc(c);
    return CsMakeInteger(st.wDay);
}

static value CSF_getUTCDay(VM *c)
{
    SYSTEMTIME st = get_utc(c);
    return CsMakeInteger(st.wDayOfWeek);
}


static value CSF_getUTCHours(VM *c)
{
    SYSTEMTIME st = get_utc(c);
    return CsMakeInteger(st.wHour);
}

static value CSF_getUTCMinutes(VM *c)
{
    SYSTEMTIME st = get_utc(c);
    return CsMakeInteger(st.wMinute);
}

static value CSF_getUTCSeconds(VM *c)
{
    SYSTEMTIME st = get_utc(c);
    return CsMakeInteger(st.wSecond);
}

static value CSF_getUTCMilliseconds(VM *c)
{
    SYSTEMTIME st = get_utc(c);
    return CsMakeInteger(st.wMilliseconds);
}

*/

// setters

/*
inline void set_local(VM *c,value d, SYSTEMTIME &st)
{
    tool::datetime_t ft;
    SystemTimeToFileTime(&st,&ft);
    LocalFileTimeToFileTime(&ft,&ft);
    CsDateValue(c,d) = ft;
}

inline void set_local(VM *c,SYSTEMTIME &st)
{
    value d;
    CsParseArguments(c,"V=*",&d,c->dateDispatch);
    set_local(c,d,st);
}


inline ushort int_param(VM* c)
{
  int_t d;
  CsParseArguments(c,"**i",&d);
  return ushort(d);
}

inline int toInt(VM* c, value v)
{
  if( CsIntegerP(v) )
    return CsIntegerValue(v);
  CsTypeError(c,v);
  return 0;
}


int days_in_month(const SYSTEMTIME &st)
{
  if( st.wMonth == 2 )
  {
    bool is_leap_year = ( ( st.wYear & 3 )   == 0 ) &&
                          ( ( st.wYear % 100 ) != 0 ||
                            ( st.wYear % 400 ) == 0 );
    return is_leap_year? 29:28;
  }

  static int dmonth [ 12 ] =
  {
    31, 29, 31, 30, 31, 30,
    31, 31, 30, 31, 30, 31
  };

  return dmonth[ st.wMonth - 1 ];
}



inline int64& CsDateValue64(VM *c,value obj)
{
  return ft64( CsDateValue(c,obj) );
}

const int64 uPerMillisecond = __int64(10) * 1000L; 
const int64 uPerSecond = uPerMillisecond * 1000L; 
const int64 uPerMinute = uPerSecond * 60; 
const int64 uPerHour = uPerMinute * 60; 
const int64 uPerDay = uPerHour * 24; 

*/

inline int toInt(VM* c, value v)
{
  if( CsIntegerP(v) )
    return CsIntegerValue(v);
  CsTypeError(c,v);
  return 0;
}


inline void set_local(VM *c,value d, tool::date_time dt)
{
    dt.to_utc();
    CsDateValue(c,d) = dt.time();
}


static void CSF_set_day(VM *c,value obj,value val)
{
    tool::date_time st = get_local(c,obj);
    st.day(toInt(c,val));
    set_local(c,obj,st);
}
static void CSF_set_month(VM *c,value obj,value val)
{
    tool::date_time st = get_local(c,obj);
    st.month(toInt(c,val));
    set_local(c,obj,st);
}
static void CSF_set_year(VM *c,value obj,value val)
{
    tool::date_time st = get_local(c,obj);
    st.year(toInt(c,val));
    set_local(c,obj,st);
}
static void CSF_set_hour(VM *c,value obj,value val)
{
    tool::date_time st = get_local(c,obj);
    st.hours(toInt(c,val));
    set_local(c,obj,st);
}
static void CSF_set_minute(VM *c,value obj,value val)
{
    tool::date_time st = get_local(c,obj);
    st.minutes(toInt(c,val));
    set_local(c,obj,st);
}
static void CSF_set_second(VM *c,value obj,value val)
{
    tool::date_time st = get_local(c,obj);
    st.seconds(toInt(c,val));
    set_local(c,obj,st);
}

static void CSF_set_millisecond(VM *c,value obj,value val)
{
    tool::date_time st = get_local(c,obj);
    st.millis(toInt(c,val));
    set_local(c,obj,st);
}

inline void set_utc(VM *c,value d,  tool::date_time dt)
{
    CsDateValue(c,d) = dt.time();
}

//----
static void CSF_set_utc_day(VM *c,value obj,value val)
{
    tool::date_time st = get_utc(c,obj);
    st.day(toInt(c,val));
    set_utc(c,obj,st);
}
static void CSF_set_utc_month(VM *c,value obj,value val)
{
    tool::date_time st = get_utc(c,obj);
    st.month(toInt(c,val));
    set_utc(c,obj,st);
}
static void CSF_set_utc_year(VM *c,value obj,value val)
{
    tool::date_time st = get_utc(c,obj);
    st.year(toInt(c,val));
    set_utc(c,obj,st);
}
static void CSF_set_utc_hour(VM *c,value obj,value val)
{
    tool::date_time st = get_utc(c,obj);
    st.hours(toInt(c,val));
    set_utc(c,obj,st);
}
static void CSF_set_utc_minute(VM *c,value obj,value val)
{
    tool::date_time st = get_utc(c,obj);
    st.minutes(toInt(c,val));
    set_utc(c,obj,st);
}
static void CSF_set_utc_second(VM *c,value obj,value val)
{
    tool::date_time st = get_utc(c,obj);
    st.seconds(toInt(c,val));
    set_utc(c,obj,st);
}

static void CSF_set_utc_millisecond(VM *c,value obj,value val)
{
    tool::date_time st = get_utc(c,obj);
    st.millis(toInt(c,val));
    set_utc(c,obj,st);
}



//----
/*
static value CSF_setFullYear(VM *c)
{
    SYSTEMTIME st = get_local(c);   
    st.wYear = int_param(c);
    set_local(c,st);
    return UNDEFINED_VALUE;
}


static value CSF_setYear(VM *c)
{
    SYSTEMTIME st = get_local(c);   
    st.wYear = int_param(c) + 1900;
    set_local(c,st);
    return UNDEFINED_VALUE;
}


static value CSF_setMonth(VM *c)
{
    SYSTEMTIME st = get_local(c);   
    st.wMonth = int_param(c) + 1;
    set_local(c,st);
    return UNDEFINED_VALUE;
}

static value CSF_setDate(VM *c)
{
    SYSTEMTIME st = get_local(c);   
    st.wDay = int_param(c);
    set_local(c,st);
    return UNDEFINED_VALUE;
}

static value CSF_setHours(VM *c)
{
    SYSTEMTIME st = get_local(c);   
    st.wHour = int_param(c);
    set_local(c,st);
    return UNDEFINED_VALUE;
}

static value CSF_setMinutes(VM *c)
{
    SYSTEMTIME st = get_local(c);   
    st.wMinute = int_param(c);
    set_local(c,st);
    return UNDEFINED_VALUE;
}

static value CSF_setSeconds(VM *c)
{
    SYSTEMTIME st = get_local(c);   
    st.wSecond = int_param(c);
    set_local(c,st);
    return UNDEFINED_VALUE;
}

static value CSF_setMilliseconds(VM *c)
{
    SYSTEMTIME st = get_local(c);   
    st.wMilliseconds = int_param(c);
    set_local(c,st);
    return CsMakeInteger(0);
}

static value CSF_setUTCFullYear(VM *c)
{
    SYSTEMTIME st = get_utc(c);   
    st.wYear = int_param(c);
    set_utc(c,st);
    return UNDEFINED_VALUE;
}

static value CSF_setUTCYear(VM *c)
{
    SYSTEMTIME st = get_utc(c);   
    st.wYear = int_param(c) + 1900;
    set_utc(c,st);
    return UNDEFINED_VALUE;
}


static value CSF_setUTCMonth(VM *c)
{
    SYSTEMTIME st = get_utc(c);   
    st.wMonth = int_param(c) + 1;
    set_utc(c,st);
    return UNDEFINED_VALUE;
}

static value CSF_setUTCDate(VM *c)
{
    SYSTEMTIME st = get_utc(c);   
    st.wDay = int_param(c);
    set_utc(c,st);
    return UNDEFINED_VALUE;
}

static value CSF_setUTCHours(VM *c)
{
    SYSTEMTIME st = get_utc(c);   
    st.wHour = int_param(c);
    set_utc(c,st);
    return UNDEFINED_VALUE;
}

static value CSF_setUTCMinutes(VM *c)
{
    SYSTEMTIME st = get_utc(c);   
    st.wMinute = int_param(c);
    set_utc(c,st);
    return UNDEFINED_VALUE;
}

static value CSF_setUTCSeconds(VM *c)
{
    SYSTEMTIME st = get_utc(c);   
    st.wSecond = int_param(c);
    set_utc(c,st);
    return UNDEFINED_VALUE;
}

static value CSF_setUTCMilliseconds(VM *c)
{
    SYSTEMTIME st = get_utc(c);   
    st.wMilliseconds = int_param(c);
    set_utc(c,st);
    return CsMakeInteger(0);
}
*/

//The toGMTString method returns a String object that contains 
//the date formatted using GMT convention. The format of the return value 
//is as follows: "05 Jan 1996 00:00:00 GMT."

static char* week_days[7] = 
    {"Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"};

static char* short_months[12] =
    {"Jan", "Feb", "Mar", "Apr", "May", "Jun",
         "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"};


static value CSF_toGMTString(VM *c)
{
    value d;
    CsParseArguments(c,"V=*",&d,c->dateDispatch);

    tool::date_time st = get_utc(c,d); 

    char time_buf[48];
    sprintf(time_buf, "%s,%d %s %d %02d:%02d:%02d GMT",
      week_days[st.day_of_week()],
      st.day(), short_months[st.month() - 1], st.year(),
      st.hours(), st.minutes(), st.seconds() );

    return CsMakeCString(c,time_buf);
}

static value CSF_toUTCString(VM *c)
{
    value d;
    CsParseArguments(c,"V=*",&d,c->dateDispatch);

    tool::date_time st = get_utc(c,d); 

    char time_buf[48];
    sprintf(time_buf, "%s,%d %s %d %02d:%02d:%02d UTC",
      week_days[st.day_of_week()],
      st.day(), short_months[st.month() - 1], st.year(),
      st.hours(), st.minutes(), st.seconds() );

    return CsMakeCString(c,time_buf);
}

static value CSF_toString(VM *c)
{
    value d;
    CsParseArguments(c,"V=*",&d,c->dateDispatch);

    tool::date_time st = get_local(c,d); 

    char time_buf[48];
    sprintf(time_buf, "%s,%d %s %d %02d:%02d:%02d",
      week_days[st.day_of_week()],
      st.day(), short_months[st.month() - 1], st.year(),
      st.hours(), st.minutes(), st.seconds() );

    return CsMakeCString(c,time_buf);
}

static value CSF_toISOString(VM *c)
{
    value d;
    bool utc = false;
    CsParseArguments(c,"V=*|B",&d,c->dateDispatch,&utc);

    tool::date_time dt = utc? get_utc(c,d) : get_local(c,d); 

    uint flags = 0;
    if( utc )
      flags |= tool::date_time::DT_UTC;
    if( dt.has_date() )
      flags |= tool::date_time::DT_HAS_DATE;
    if( dt.has_time() )
      flags |= tool::date_time::DT_HAS_TIME | tool::date_time::DT_HAS_SECONDS;
    return CsMakeCString(c,dt.emit_iso(flags));
}

bool CsPrintDate(VM *c,value v, stream* s)
{
    tool::date_time st = get_utc(c,v); 
    uint flags = 0;//tool::date_time::DT_UTC;
    if( st.has_date() )
      flags |= tool::date_time::DT_HAS_DATE;
    if( st.has_time() )
      flags |= tool::date_time::DT_HAS_TIME | tool::date_time::DT_HAS_SECONDS;
    tool::string ds = st.emit_iso(flags);  
    return s->put_str(ds);

    //return s->printf(L"%d-%d-%d %02d:%02d:%02d UTC",
    //  st.year(), st.month(), st.day(),
    //  st.hours(), st.minutes(), st.seconds() );
}

static value CSF_toLocaleString(VM *c)
{
#ifndef WINDOWS
    #pragma TODO("need porting")
    return CSF_toUTCString(c);
#else
    bool longFmt = false;
    
    value d;
    CsParseArguments(c,"V=*|B",&d,c->dateDispatch,&longFmt);
    tool::datetime_t ft = CsDateValue(c,d);
    FileTimeToLocalFileTime((FILETIME*)&ft,(FILETIME*)&ft);
    SYSTEMTIME st;
    FileTimeToSystemTime((FILETIME*)&ft,&st);

    wchar str[64]; memset(str,0,sizeof(wchar)*64);
   
    int n = GetDateFormatW(
      LOCALE_USER_DEFAULT,       // locale
      longFmt?DATE_LONGDATE: DATE_SHORTDATE,     // options
      &st,    // date
      NULL,    // date format
      str,    // formatted string buffer
      63      // size of buffer
    );

    if(n == 0)
      return CsMakeCString(c,"");

    wcscat(str,L" ");
    n = wcslen(str);
    
    n = GetTimeFormatW(
      LOCALE_USER_DEFAULT,// locale
      0,                  // options
      &st,                // time
      NULL,               // time format string
      str + n,            // formatted string buffer
      63 - n              // size of string buffer
    );
    return CsMakeCString(c,str);
#endif
}

static value CSF_monthName(VM *c)
{
#ifndef WINDOWS
    #pragma TODO("need porting")
    value d;
    CsParseArguments(c,"V=*",&d,c->dateDispatch);

    tool::date_time st = get_utc(c,d); 

    return CsMakeCString(c,short_months[st.month() - 1]);
#else
    bool longFmt = false;
    
    value d;
    CsParseArguments(c,"V=*|B",&d,c->dateDispatch,&longFmt);
    tool::datetime_t ft = CsDateValue(c,d);
    //FileTimeToLocalFileTime(&ft,&ft);
    SYSTEMTIME st;
    FileTimeToSystemTime((FILETIME*)&ft,&st);

    wchar str[64];
   
    int n = GetDateFormatW(
      LOCALE_USER_DEFAULT,       // locale
      0,     // options
      &st,    // date
      longFmt?L"MMMM":L"MMM",    // date format
      str,    // formatted string buffer
      64      // size of buffer
    );

    if(n == 0)
      return CsMakeCString(c,"");

    return CsMakeCString(c,str);
#endif
}

static value CSF_dayOfWeekName(VM *c)
{
#ifndef WINDOWS
    #pragma TODO("need porting")
    value d;
    CsParseArguments(c,"V=*",&d,c->dateDispatch);

    tool::date_time st = get_utc(c,d); 

    return CsMakeCString(c,week_days[st.day_of_week()]);
#else
    bool longFmt = false;
    
    value d;
    CsParseArguments(c,"V=*|B",&d,c->dateDispatch,&longFmt);
    tool::datetime_t ft = CsDateValue(c,d);
    //FileTimeToLocalFileTime(&ft,&ft);
    SYSTEMTIME st;
    FileTimeToSystemTime((FILETIME*)&ft,&st);

    wchar str[64];
   
    int n = GetDateFormatW(
      LOCALE_USER_DEFAULT,       // locale
      0,     // options
      &st,    // date
      longFmt?L"dddd":L"ddd",    // date format
      str,    // formatted string buffer
      64      // size of buffer
    );

    if(n == 0)
      return CsMakeCString(c,"");

    return CsMakeCString(c,str);
#endif
}



//The stored time value in milliseconds since midnight, January 1, 1970 UTC.

static value CSF_valueOf(VM *c)
{
    value d; 
    CsParseArguments(c,"V=*|B",&d,c->dateDispatch);
    tool::datetime_t ft = CsDateValue(c,d);
    /*
    SYSTEMTIME st; memset(&st,0, sizeof(st));
    st.wDay = 1;
    st.wMonth = 1;
    st.wYear = 1970;
    tool::datetime_t ftzero;
    SystemTimeToFileTime(&st,&ftzero);
    */

    double n = double(int64(ft)) / 10000.0 - ms1970();
    return CsMakeFloat(c, n );
}

static value CSF_localOffset(VM *c)
{
  return CsMakeInteger( int_t(tool::date_time::local_offset_ms()));
/*
  TIME_ZONE_INFORMATION tzi;
  memset(&tzi,0, sizeof(tzi));
  switch( GetTimeZoneInformation( &tzi ))
  {
    case TIME_ZONE_ID_STANDARD:
      return CsMakeInteger(tzi.Bias + tzi.StandardBias);
    case TIME_ZONE_ID_DAYLIGHT:
      return CsMakeInteger(tzi.Bias + tzi.DaylightBias);
  }
  return UNDEFINED_VALUE;
*/
}

static value CSF_localTimeZone(VM *c)
{
#ifndef WINDOWS
  #pragma TODO("need porting")

// WHAT A HECK, why SUSE does not have it?
  /*::tzset();
  tool::string tzn(tzname,2);
  return CsMakeCString(c,tzn); */
#else
  TIME_ZONE_INFORMATION tzi;
  memset(&tzi,0, sizeof(tzi));
  switch( GetTimeZoneInformation( &tzi ))
  {
    case TIME_ZONE_ID_STANDARD:
      return CsMakeCString(c, tzi.StandardName);
    case TIME_ZONE_ID_DAYLIGHT:
      return CsMakeCString(c, tzi.DaylightName);
  }
#endif
  return UNDEFINED_VALUE;
}

static value CSF_isDaylight(VM *c)
{

// WHAT A HECK, why SUSE does not have it?
/*
  tzset();
  return daylight? c->trueValue: c->falseValue;
*/

  TIME_ZONE_INFORMATION tzi;
  memset(&tzi,0, sizeof(tzi));
  switch( GetTimeZoneInformation( &tzi ))
  {
    case TIME_ZONE_ID_STANDARD:
      return FALSE_VALUE;
    case TIME_ZONE_ID_DAYLIGHT:
      return TRUE_VALUE;
  }
  return UNDEFINED_VALUE;
}


/* CSF_parse - static method */
static value CSF_parse(VM *c)
{
    value str;
    CsParseArguments(c,"**V",&str);
    tool::datetime_t utc;
    if(ParseDateTime(str, utc))
    {
      //SYSTEMTIME st; memset(&st,0, sizeof(st));
      tool::date_time dtu(utc);
      int64 n = dtu.absolute_millis() - ms1970();
      return CsMakeFloat(c, float_t(n) );
    }
    return UNDEFINED_VALUE;
}

static int comment_length(const wchar *str)
{
    int ch, pos, level, quoteNext, done, len;

    level = 0;
    quoteNext = 0;
    pos = 0;
    len = 0;
    ch = str[pos];
    done = 0;
    while (1) {
        switch (ch) {
        case 0:
            len = pos;
            done = 1;
            break;
        case '\\':
            quoteNext = 1;
            break;
        case '(':
            if (!quoteNext) {
                ++level;
            }
            quoteNext = 0;
            break;
        case ')':
            if (!quoteNext) {
                --level;
                if (level == 0) {
                    len = pos + 1;
                    done = 1;
                }
            }
            quoteNext = 0;
            break;
        default:
            quoteNext = 0;
        }
        if (done) {
            break;
        }
        ++pos;
        ch = str[pos];
    }
    return len;
}


/*
 * ParseRfc822Date() -- Parse a date in RFC-822 (RFC-1123) format
 * 
 * If the parsing succeeds:
 *  - tms is set to contain the year, month, day, hour, minute, and second
 *  - z is set to contain the time zone in minutes offset from UTC
 *  - 0 is returned
 * If the parsing fails:
 *  - (-1) is returned
 *  - the information in tms and z is undefined
 */

int ParseRfc822Date(const wchar *str, tool::date_time::datetime_s *tms, int *z, wchar* name, int nameSize)
{
    int pos, ch, n, sgn, numDigits;
    int day=1, month=1, year=1970, hour=0, minute=0, second=0, zone=0;
    int isValid = 1;

    if (!str) {
        return -1;
    }
    if (name != 0) {
        name[0] = 0;
    }
    /*
     * Ignore optional day of the week.
     */

    /*-----------------------------------------------------------------*\
     * Day -- one or two digits
    \*-----------------------------------------------------------------*/
    /* -- skip over non-digits */
    pos = 0;
    ch = str[pos];
    while (ch && !('0' <= ch && ch <= '9')) {
        if (ch == '(') {
            pos += comment_length(&str[pos]);
        }
        else {
            ++pos;
        }
        ch = str[pos];
    }
    /* -- convert next one or two digits */
    n = -1;
    if ('0' <= ch && ch <= '9') {
        n = ch - '0';
        ++pos;
        ch = str[pos];
    }
    if ('0' <= ch && ch <= '9') {
        n *= 10;
        n += ch - '0';
        ++pos;
        ch = str[pos];
    }
    if (1 <= n && n <= 31) {
        day = n;
    }
    else {
        isValid = 0;
        goto FUNCTION_EXIT;
    }
    /*-----------------------------------------------------------------*\
     * Month.  Use case-insensitive string compare for added robustness
    \*-----------------------------------------------------------------*/
    /* -- skip over chars to first possible month char */
    while (ch && !('A' <= ch && ch <= 'S') && !('a' <= ch && ch <= 's')) {
        if (ch == '(') {
            pos += comment_length(&str[pos]);
        }
        else {
            ++pos;
        }
        ch = str[pos];
    }
    /* -- convert the month name */
    n = -1;
    switch (ch) {
    case 'A':
    case 'a':
        /* Apr */
        if ((str[pos+1] == 'p' || str[pos+1] == 'P')
            && (str[pos+2] == 'r' || str[pos+2] == 'R')) {
            n = 4;
            pos += 3;
            ch = str[pos];
        }
        /* Aug */
        else if ((str[pos+1] == 'u' || str[pos+1] == 'U')
            && (str[pos+2] == 'g' || str[pos+2] == 'G')) {
            n = 8;
            pos += 3;
            ch = str[pos];
        }
        break;
    case 'D':
    case 'd':
        /* Dec */
        if ((str[pos+1] == 'e' || str[pos+1] == 'E')
            && (str[pos+2] == 'c' || str[pos+2] == 'C')) {
            n = 12;
            pos += 3;
            ch = str[pos];
        }
        break;
    case 'F':
    case 'f':
        /* Feb */
        if ((str[pos+1] == 'e' || str[pos+1] == 'E')
            && (str[pos+2] == 'b' || str[pos+2] == 'B')) {
            n = 2;
            pos += 3;
            ch = str[pos];
        }
        break;
    case 'J':
    case 'j':
        /* Jan */
        if ((str[pos+1] == 'a' || str[pos+1] == 'A')
            && (str[pos+2] == 'n' || str[pos+2] == 'N')) {
            n = 1;
            pos += 3;
            ch = str[pos];
        }
        /* Jul */
        else if ((str[pos+1] == 'u' || str[pos+1] == 'U')
            && (str[pos+2] == 'l' || str[pos+2] == 'L')) {
            n = 7;
            pos += 3;
            ch = str[pos];
        }
        /* Jun */
        else if ((str[pos+1] == 'u' || str[pos+1] == 'U')
            && (str[pos+2] == 'n' || str[pos+2] == 'N')) {
            n = 6;
            pos += 3;
            ch = str[pos];
        }
        break;
    case 'M':
    case 'm':
        /* Mar */
        if ((str[pos+1] == 'a' || str[pos+1] == 'A')
            && (str[pos+2] == 'r' || str[pos+2] == 'R')) {
            n = 3;
            pos += 3;
            ch = str[pos];
        }
        /* May */
        else if ((str[pos+1] == 'a' || str[pos+1] == 'A')
            && (str[pos+2] == 'y' || str[pos+2] == 'Y')) {
            n = 5;
            pos += 3;
            ch = str[pos];
        }
        break;
    case 'N':
    case 'n':
        /* Nov */
        if ((str[pos+1] == 'o' || str[pos+1] == 'O')
            && (str[pos+2] == 'v' || str[pos+2] == 'V')) {
            n = 11;
            pos += 3;
            ch = str[pos];
        }
        break;
    case 'O':
    case 'o':
        /* Oct */
        if ((str[pos+1] == 'c' || str[pos+1] == 'c')
            && (str[pos+2] == 't' || str[pos+2] == 'T')) {
            n = 10;
            pos += 3;
            ch = str[pos];
        }
        break;
    case 'S':
    case 's':
        /* Sep */
        if ((str[pos+1] == 'e' || str[pos+1] == 'E')
            && (str[pos+2] == 'p' || str[pos+2] == 'P')) {
            n = 9;
            pos += 3;
            ch = str[pos];
        }
        break;
    }
    if (1 <= n && n <= 12) {
        month = n;
    }
    else {
        isValid = 0;
        goto FUNCTION_EXIT;
    }
    /*-----------------------------------------------------------------*\
     * Year -- two or four digits (four required per RFC 1123, two for
     *         compatibility with legacy systems)
    \*-----------------------------------------------------------------*/
    /* -- skip over non-digits */
    while (ch && !('0' <= ch && ch <= '9')) {
        if (ch == '(') {
            pos += comment_length(&str[pos]);
        }
        else {
            ++pos;
        }
        ch = str[pos];
    }
    /* -- convert up to four digits */
    n = -1;
    if ('0' <= ch && ch <= '9') {
        n = ch - '0';
        ++pos;
        ch = str[pos];
    }
    if ('0' <= ch && ch <= '9') {
        n *= 10;
        n += ch - '0';
        ++pos;
        ch = str[pos];
    }
    if ('0' <= ch && ch <= '9') {
        n *= 10;
        n += ch - '0';
        ++pos;
        ch = str[pos];
    }
    if ('0' <= ch && ch <= '9') {
        n *= 10;
        n += ch - '0';
        ++pos;
        ch = str[pos];
    }
    if (n != -1) {
        if (n < 50) {
            year = n + 2000;
        }
        else if (n < 100) {
            year = n + 1900;
        }
        else {
            year = n;
        }
    }
    else {
        isValid = 0;
        goto FUNCTION_EXIT;
    }
    /*-----------------------------------------------------------------*\
     * Hour -- two digits
    \*-----------------------------------------------------------------*/

    /* skip whitespaces  */
    while (' ' == ch) 
    {
        ch = str[++pos];
    }

    if( ch < '0' || ch > '9' )
      goto TZ;

    while (ch && !('0' <= ch && ch <= '9')) {
        if (ch == '(') {
            pos += comment_length(&str[pos]);
        }
        else {
            ++pos;
        }
        ch = str[pos];
    }
    /* -- convert next one or two digits */
    n = -1;
    if ('0' <= ch && ch <= '9') {
        n = ch - '0';
        ++pos;
        ch = str[pos];
    }
    if ('0' <= ch && ch <= '9') {
        n *= 10;
        n += ch - '0';
        ++pos;
        ch = str[pos];
    }
    if (0 <= n && n <= 23) {
        hour = n;
    }
    else {
        isValid = 0;
        goto FUNCTION_EXIT;
    }
    /*-----------------------------------------------------------------*\
     * Minute -- two digits
    \*-----------------------------------------------------------------*/
    /* -- scan for ':' */
    while (ch && ch != ':') {
        if (ch == '(') {
            pos += comment_length(&str[pos]);
        }
        else {
            ++pos;
        }
        ch = str[pos];
    }
    /* -- skip over non-digits */
    while (ch && !('0' <= ch && ch <= '9')) {
        if (ch == '(') {
            pos += comment_length(&str[pos]);
        }
        else {
            ++pos;
        }
        ch = str[pos];
    }
    /* -- convert next one or two digits */
    n = -1;
    if ('0' <= ch && ch <= '9') {
        n = ch - '0';
        ++pos;
        ch = str[pos];
    }
    if ('0' <= ch && ch <= '9') {
        n *= 10;
        n += ch - '0';
        ++pos;
        ch = str[pos];
    }
    if (0 <= n && n <= 59) {
        minute = n;
    }
    else {
        isValid = 0;
        goto FUNCTION_EXIT;
    }
    /*-----------------------------------------------------------------*\
     * Second (optional) -- two digits
    \*-----------------------------------------------------------------*/
    /* -- scan for ':' or start of time zone */
    while (ch && !(ch == ':' || ch == '+' || ch == '-' || isalpha(ch))) {
        if (ch == '(') {
            pos += comment_length(&str[pos]);
        }
        else {
            ++pos;
        }
        ch = str[pos];
    }
    /* -- get the seconds, if it's there */
    if (ch == ':') {
        ++pos;
        /* -- skip non-digits */
        ch = str[pos];
        while (ch && !('0' <= ch && ch <= '9')) {
            if (ch == '(') {
                pos += comment_length(&str[pos]);
            }
            else {
                ++pos;
            }
            ch = str[pos];
        }
        /* -- convert next one or two digits */
        n = -1;
        if ('0' <= ch && ch <= '9') {
            n = ch - '0';
            ++pos;
            ch = str[pos];
        }
        if ('0' <= ch && ch <= '9') {
            n *= 10;
            n += ch - '0';
            ++pos;
            ch = str[pos];
        }
        if (0 <= n && n <= 59) {
            second = n;
        }
        else {
            isValid = 0;
            goto FUNCTION_EXIT;
        }
        /* -- scan for start of time zone */
        while (ch && !(ch == '+' || ch == '-' || isalpha(ch))) {
            if (ch == '(') {
                pos += comment_length(&str[pos]);
            }
            else {
                ++pos;
            }
            ch = str[pos];
        }
    }
    else /* if (ch != ':') */ {
        second = 0;
    }
    /*-----------------------------------------------------------------*\
     * Time zone
     *
     * Note: According to RFC-1123, the military time zones are specified
     * incorrectly in RFC-822.  RFC-1123 then states that "military time
     * zones in RFC-822 headers carry no information."
     * Here, we follow the specification in RFC-822.  What else could we
     * do?  Military time zones should *never* be used!
    \*-----------------------------------------------------------------*/
TZ:
    sgn = 1;
    switch (ch) {
    case '-':
        sgn = -1;
        /* fall through */
    case '+':
        ++pos;
        /* -- skip non-digits */
        ch = str[pos];
        while (ch && !('0' <= ch && ch <= '9')) {
            ++pos;
            ch = str[pos];
        }
        /* -- convert next four digits */
        numDigits = 0;
        n = 0;
        if ('0' <= ch && ch <= '9') {
            n = (ch - '0')*600;
            ++pos;
            ch = str[pos];
            ++numDigits;
        }
        if ('0' <= ch && ch <= '9') {
            n += (ch - '0')*60;
            ++pos;
            ch = str[pos];
            ++numDigits;
        }
        if ('0' <= ch && ch <= '9') {
            n += (ch - '0')*10;
            ++pos;
            ch = str[pos];
            ++numDigits;
        }
        if ('0' <= ch && ch <= '9') {
            n += ch - '0';
            ++numDigits;
        }
        if (numDigits == 4) {
            zone = sgn*n;
        }
        else {
            isValid = 0;
        }
        break;
    case 'U':
    case 'u':
        if (str[pos+1] == 'T' || str[pos+1] == 't') {
            zone = 0;
        }
        else {
            /* Military time zone */
            zone = 480;
        }
        break;
    case 'G':
    case 'g':
        if ((str[pos+1] == 'M' || str[pos+1] == 'm')
            && (str[pos+2] == 'T' || str[pos+2] == 't')) {
            zone = 0;
        }
        else {
            /* Military time zone */
            zone = -420;
        }
        break;
    case 'E':
    case 'e':
        if ((str[pos+1] == 'S' || str[pos+1] == 's')
            && (str[pos+2] == 'T' || str[pos+2] == 't')) {
            zone = -300;
        }
        else if ((str[pos+1] == 'D' || str[pos+1] == 'd')
            && (str[pos+2] == 'T' || str[pos+2] == 't')) {
            zone = -240;
        }
        else {
            /* Military time zone */
            zone = -300;
        }
        break;
    case 'C':
    case 'c':
        if ((str[pos+1] == 'S' || str[pos+1] == 's')
            && (str[pos+2] == 'T' || str[pos+2] == 't')) {
            zone = -360;
        }
        else if ((str[pos+1] == 'D' || str[pos+1] == 'd')
            && (str[pos+2] == 'T' || str[pos+2] == 't')) {
            zone = -300;
        }
        else {
            /* Military time zone */
            zone = -180;
        }
        break;
    case 'M':
    case 'm':
        if ((str[pos+1] == 'S' || str[pos+1] == 's')
            && (str[pos+2] == 'T' || str[pos+2] == 't')) {
            zone = -420;
        }
        else if ((str[pos+1] == 'D' || str[pos+1] == 'd')
            && (str[pos+2] == 'T' || str[pos+2] == 't')) {
            zone = -360;
        }
        else {
            /* Military time zone */
            zone = -720;
        }
        break;
    case 'P':
    case 'p':
        if ((str[pos+1] == 'S' || str[pos+1] == 's')
            && (str[pos+2] == 'T' || str[pos+2] == 't')) {
            zone = -480;
        }
        else if ((str[pos+1] == 'D' || str[pos+1] == 'd')
            && (str[pos+2] == 'T' || str[pos+2] == 't')) {
            zone = -420;
        }
        else {
            /* Military time zone */
            zone = 180;
        }
        break;
    case 'Z':
        /* Military time zone */
        zone = 0;
        break;
    default:
        /* Military time zone */
        if ('A' <= ch && ch <= 'I') {
            zone = 'A' - 1 - ch;
        }
        else if ('K' <= ch && ch <= 'M') {
            zone = 'A' - ch;
        }
        else if ('N' <= ch && ch <= 'Y') {
            zone = ch - 'N' + 1;
        }
        else {
            isValid = 0;
        }
        break;
    }
    /*
     * Last comment is taken to be the time zone name
     */
    /* -- Advance to end of string or to comment */
    ch = str[pos];
    while (ch && ch != '(') {
        ++pos;
        ch = str[pos];
    }
    /* -- If comment is present, get the time zone name */
    if (ch == '(' && name != 0) {
        n = comment_length(&str[pos]);
        if (n > 2) {
            n -= 2;
            n = (n < nameSize-1) ? n : nameSize-1;
            ++pos;
            wcsncpy(name, &str[pos], n);
            name[n] = 0;
        }
    }
FUNCTION_EXIT:
    if (isValid) {
        if (tms) {
            tms->year   = year;
            tms->month  = month;
            tms->day    = day;
            tms->hour   = hour;
            tms->minute = minute;
            tms->second = second;
        }
        if (z) {
            *z = zone;
        }
    }
    else {
        if (tms) {
            tms->year   = 1970;
            tms->month  = 1;
            tms->day    = 1;
            tms->hour   = 0;
            tms->minute = 0;
            tms->second = 0;
        }
        if (z) {
            *z = 0;
        }
    }
    return isValid ? 0 : -1;
}


static bool ParseDateTime(value s, tool::datetime_t& utc)
{
  const wchar* str = CsStringAddress(s);
  wchar tz_name[20];
  tool::date_time::datetime_s tms; memset(&tms,0,sizeof(tms));
  int tz_minutes;

  if (ParseRfc822Date(str, &tms, &tz_minutes, tz_name, 20) == 0)
  {
    //Rfc 822 Date
    tool::date_time::cvt(utc,tms);
    utc -= int64(tz_minutes) * 60 * 1000 * 1000 * 10;
    return true;
  }

  // try to parse ISO
    uint dcomponents = 0;
    tool::date_time dt = tool::date_time::parse_iso(CsStringChars(s),dcomponents);
    if(dcomponents == tool::date_time::DT_UNKNOWN)
      return false;
    utc = dt.time();
    if( (dcomponents & tool::date_time::DT_UTC) == 0 )
      utc -= tool::date_time::local_offset();
      
    return true;

}



}
