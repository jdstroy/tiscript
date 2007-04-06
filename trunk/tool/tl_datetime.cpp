//|
//|
//| Copyright (c) 2001-2005
//| Andrew Fedoniouk - andrew@terra-informatica.org
//|
//| date & time implementation
//|
//|
#include <stdlib.h>
#include <time.h> 
#include "tl_datetime.h"

#ifdef WIN32
#include <TCHAR.H> 
#endif

namespace tool
{

#define LLABS(i) (((i)<0)?-(i):(i))

  //////////////////////////////////////////////////////////////////////
  // Construction/Destruction
  //////////////////////////////////////////////////////////////////////

#define MAX_TIME_BUFFER_SIZE 128

  // Constant array with months # of days of year
  int date_time::month_day_in_year [ 13 ] =
  {
    0, 31, 59, 90, 120, 151, 181,
    212, 243, 273, 304, 334, 365
  };



  // Static member for getting the current time
  date_time
    date_time::now (bool utc)
  {
    date_time dt;
#ifdef WIN32
    SYSTEMTIME st;
    if(utc)
      GetSystemTime(&st);
    else
      GetLocalTime(&st);
    
    SystemTimeToFileTime( &st, (FILETIME*)&dt._time);
    
#else
    struct timeval tv;
    gettimeofday(&tv, NULL); 
    dt = tv; // cvt needs to be implemented 
#endif    

    /*
    time_t t;
    ::time ( &t );
    struct tm syst = utc? *gmtime( &t ): *localtime ( &t );
    
    dt = syst;
    */
    return dt;
  }

  datetime_t date_time::local_offset()
  {
    time_t t = 0;
    struct tm u = *gmtime( &t );
    struct tm l = *localtime ( &t );
    date_time dtu = u;
    date_time dtl = l;
    return dtl.time() - dtu.time();
  }

  void date_time::to_local()
  {
    _time += local_offset();
  }
  void date_time::to_utc()
  {
    _time -= local_offset();
  }


  datetime_t date_time::local_offset_ms() { return local_offset() / 10000L; }

  date_time::date_time ( int year, int month, int day,
                         int hours, int minutes, int seconds,
                         int millis , int micros ,int nanos )
  // nMilli, nMicro & nNano default = 0
  {
    set ( year, month, day, hours, minutes, seconds, millis, micros, nanos );
  }

  // date_time operators

  date_time::date_time( const tm& syst )
  {
    datetime_s src;
    src.year   = syst.tm_year + 1900;
    src.month  = syst.tm_mon + 1;
    src.day    = syst.tm_mday;
    src.hour   = syst.tm_hour;
    src.minute = syst.tm_min;
    src.second = syst.tm_sec;
    src.millis = 0;
    src.micros = 0;
    src.nanos  = 0;
    cvt ( _time, src );
  }


  bool
    date_time::set ( int year, int month, int day,
                     int hours, int minutes, int seconds,
                     int millis, int micros, int nanos )
  {
    datetime_s src;
    src.year   = year;
    src.month  = month;
    src.day    = day;
    src.hour   = hours;
    src.minute = minutes;
    src.second = seconds;
    src.millis = millis;
    src.micros = micros;
    src.nanos  = nanos;

    return cvt ( _time, src );
  }

  // HighTime helper function, static function
  bool
    date_time::cvt ( datetime_t& dst, const datetime_s &src )
  {
    datetime_t date;

    int days    = src.day;
    int hours   = src.hour;
    int minutes = src.minute;
    int seconds = src.second;
    int millis  = src.millis;
    int micros  = src.micros;

    int nanos100 = ( src.nanos + 50 ) / 100;

    if ( src.year > 29000 || src.year < -29000 ||
         src.month < 1 || src.month > 12 )
      return false;

    bool is_leap_year = ( ( src.year & 3 )   == 0 ) &&
                        ( ( src.year % 100 ) != 0 ||
                          ( src.year % 400 ) == 0 );

    /*int nDaysInMonth =
    anMonthDayInYear[SrcTime.nMonth] - anMonthDayInYear[SrcTime.nMonth-1] +
    ((bIsLeapYear && SrcTime.nDay == 29 && SrcTime.nMonth == 2) ? 1 : 0);*/

    // Adjust time and frac time
    micros   += nanos100 / 10;
    nanos100 %= 10;
    millis   += micros   / 1000;
    micros   %= 1000;
    seconds  += millis   / 1000;
    millis   %= 1000;
    minutes  += seconds  / 60;
    seconds  %= 60;
    hours    += minutes  / 60;
    minutes  %= 60;
    days     += hours    / 24;
    hours    %= 24;

    //It is a valid date; make Jan 1, 1AD be 1
    date = src.year * 365L + src.year / 4 - src.year / 100 + src.year / 400 +
           month_day_in_year [ src.month - 1 ] + days;

    //  If leap year and it's before March, subtract 1:
    if ( src.month <= 2 && is_leap_year )
      --date;

    //  Offset so that 01/01/1601 is 0
    date -= 584754L;

    // Change date to seconds
    date *= 86400L;
    date += hours * 3600L + minutes * 60L + seconds;

    // Change date to hundreds of nanoseconds
    date *= 10000000L;
    date += ( millis * 10000L ) + ( micros * 10L ) + nanos100;

    dst = date;

    return true;
  }


  bool
    date_time::cvt ( datetime_s& dst, const datetime_t &src )
  {
    datetime_t temptime;
    long days_absolute;     // Number of days since 1/1/0
    long secs_in_day;       // Time in seconds since midnight

    long years400;          // Number of 400 year increments since 1/1/0
    long century400;        // Century within 400 year block (0,1,2 or 3)
    long years4;            // Number of 4 year increments since 1/1/0
    long day4;              // Day within 4 year block
    //  (0 is 1/1/yr1, 1460 is 12/31/yr4)
    int  year4;             // Year within 4 year block (0,1,2 or 3)
    bool leap4 = true;      // TRUE if 4 year block includes leap year
    long nanos100_this_day;

    temptime = src;

    nanos100_this_day  = (long) ( temptime % 10000000L );
    temptime          /= 10000000L;
    secs_in_day        = (long) ( temptime % 86400L );
    temptime          /= 86400L;
    days_absolute      = (long) ( temptime );
    days_absolute     += 584754L;	//  adjust days from 1/1/0 to 01/01/1601

    // Calculate the day of week (mon=0...sun=6)
    //   -2 because 1/1/0 is Sat.  
    dst.day_of_week = (int) (( days_absolute - 2 ) % 7L );

    // Leap years every 4 yrs except centuries not multiples of 400.
    years400 = (long) ( days_absolute / 146097L );

    // Set days_absolute to day within 400-year block
    days_absolute %= 146097L;

    // -1 because first century has extra day
    century400 = (long) ( ( days_absolute - 1 ) / 36524L );

    // Non-leap century
    if ( century400 != 0 )
    {
      // Set days_absolute to day within century
      days_absolute = ( days_absolute - 1 ) % 36524L;

      // +1 because 1st 4 year increment has 1460 days
      years4 = (long) ( ( days_absolute + 1 ) / 1461L );

      if ( years4 != 0 )
        day4 = (long) ( ( days_absolute + 1 ) % 1461L );
      else
      {
        leap4 = false;
        day4 = (long) days_absolute;
      }
    }
    else
    {
      // Leap century - not special case!
      years4 = (long) ( days_absolute / 1461L );
      day4 = (long) ( days_absolute % 1461L );
    }

    if ( leap4 )
    {
      // -1 because first year has 366 days
      year4 = ( day4 - 1 ) / 365;
      if ( year4 != 0 )
        day4 = ( day4 - 1 ) % 365;
    }
    else
    {
      year4 = day4 / 365;
      day4 %= 365;
    }

    // day4 is now 0-based day of year. Save 1-based day of year, year number
    dst.day_of_year = (int) day4;
    dst.year = years400 * 400 + century400 * 100 + years4 * 4 + year4;

    // Handle leap year: before, on, and after Feb. 29.
    if ( year4 == 0 && leap4 )
    {
      // Leap Year
      if ( day4 == 59 )
      {
        /* Feb. 29 */
        dst.month = 2;
        dst.day   = 29;
        goto DoTime;
      }

      // Pretend it's not a leap year for month/day comp.
      if ( day4 >= 60 )
        --day4;
    }

    // Make day4 a 1-based day of non-leap year and compute
    // month/day for everything but Feb. 29.
    ++day4;

    // Month number always >= n/32, so save some loop time */
    for ( dst.month = ( day4 >> 5 ) + 1;
          day4 > month_day_in_year [ dst.month ];
          dst.month++ );

    dst.day = (int) ( day4 - month_day_in_year [ dst.month - 1 ] );

  DoTime:
    if ( secs_in_day == 0 )
      dst.hour = dst.minute = dst.second = 0;
    else
    {
      dst.second = (unsigned int) secs_in_day % 60L;
      long minutes_in_day = secs_in_day / 60L;
      dst.minute = (unsigned int) minutes_in_day % 60;
      dst.hour   = (unsigned int) minutes_in_day / 60;
    }

    if ( nanos100_this_day == 0 )
      dst.millis = dst.micros = dst.nanos = 0;
    else
    {
      dst.nanos =  (unsigned int) ( ( nanos100_this_day % 10L ) * 100L );
      long millis_this_day = nanos100_this_day / 10L;
      dst.micros = (unsigned int) millis_this_day % 1000;
      dst.millis = (unsigned int) millis_this_day / 1000;
    }
    return true;
  }

  bool
    date_time::cvt ( struct tm& dst, const datetime_s& src )
  {
    // Convert internal tm to format expected by runtimes (sfrtime, etc)
    dst.tm_year  = src.year  - 1900;  // year is based on 1900
    dst.tm_mon   = src.month - 1;      // month of year is 0-based
    dst.tm_wday  = src.day_of_week;     // day of week is 0-based
    dst.tm_yday  = src.day_of_year;     // day of year is 0-based
    dst.tm_mday  = src.day;
    dst.tm_hour  = src.hour;
    dst.tm_min   = src.minute;
    dst.tm_sec   = src.second;
    dst.tm_isdst = 0;
    return true;
  }

  int
    date_time::year () const
  {
    datetime_s ts;
    cvt ( ts, _time );
    return ts.year;
  }

  int
    date_time::month () const       // month of year (1 = Jan)
  {
    datetime_s ts;
    cvt ( ts, _time );
    return ts.month;
  }

  int
    date_time::day () const         // day of month (0-31)
  {
    datetime_s ts;
    cvt ( ts, _time );
    return ts.day;
  }

  int
    date_time::hours () const        // hour in day (0-23)
  {
    datetime_s ts;
    cvt ( ts, _time );
    return ts.hour;
  }

  int
    date_time::minutes () const      // minute in hour (0-59)
  {
    datetime_s ts;
    cvt ( ts, _time );
    return ts.minute;
  }

  int
    date_time::seconds () const      // second in minute (0-59)
  {
    datetime_s ts;
    cvt ( ts, _time );
    return ts.second;
  }

  int
    date_time::millis () const // millisecond in second (0-999)
  {
    datetime_s ts;
    cvt ( ts, _time );
    return ts.millis;
  }

  int
    date_time::micros () const // microsecond in millisecond (0-999)
  {
    datetime_s ts;
    cvt ( ts, _time );
    return ts.micros;
  }

  int
    date_time::nanos () const  // nanosecond in millisecond (0-999), step of 100ns
  {
    datetime_s ts;
    cvt ( ts, _time );
    return ts.nanos;
  }


  int
    date_time::day_of_week () const   // 0=Sun, 1=Mon, ..., 6=Sat
  {
    datetime_s ts;
    cvt ( ts, _time );
    return ts.day_of_week;
  }

  int
    date_time::day_of_year () const   // days since start of year, Jan 1 = 0
  {
    datetime_s ts;
    cvt ( ts, _time );
    return ts.day_of_year;
  }


  void
    date_time::nanos ( int nv )
  {
    datetime_s dts;
    cvt ( dts, _time );
    dts.nanos += ( nv - dts.nanos );
    cvt ( _time, dts );
  }

  void
    date_time::micros ( int nv )
  {
    datetime_s dts;
    cvt ( dts, _time );
    dts.micros += ( nv - dts.micros );
    cvt ( _time, dts );
  }


  void
    date_time::millis ( int nv )
  {
    datetime_s dts;
    cvt ( dts,_time );
    dts.millis += ( nv - dts.millis );
    cvt ( _time, dts );
  }

  void
    date_time::seconds ( int nv )
  {
    datetime_s dts;
    cvt ( dts, _time );
    dts.second += ( nv - dts.second );
    cvt ( _time, dts );
  }

  void
    date_time::minutes ( int nv )
  {
    datetime_s dts;
    cvt ( dts, _time );
    dts.minute += ( nv - dts.minute );
    cvt ( _time, dts );
  }

  void
    date_time::hours ( int nv )
  {
    datetime_s dts;
    cvt ( dts, _time );
    dts.hour += ( nv - dts.hour );
    cvt ( _time, dts );
  }

  void
    date_time::day ( int nv )
  {
    datetime_s dts;
    cvt ( dts, _time );
    dts.day += ( nv - dts.day );
    cvt ( _time, dts );
  }

  void
    date_time::month ( int nv )
  {
    datetime_s dts;
    cvt ( dts, _time );
    dts.month += ( nv - dts.month );
    dts.year  += dts.month / 12;
    dts.month %= 12;
    cvt ( _time, dts );
  }

  void
    date_time::year ( int nv )
  {
    datetime_s dts;
    cvt ( dts, _time );
    dts.year += ( nv - dts.year );
    cvt ( _time, dts );
  }

  datetime_t
    date_time::absolute_millis ()
  {
    return _time / 10000L;
  }

  void
    date_time::absolute_millis ( datetime_t t )
  {
    _time = t * 10000L;
  }

  bool
    date_time::systemtime ( struct tm& syst ) const
  {
    date_time::datetime_s dts;
    cvt ( dts, _time );
    memset ( &syst, 0, sizeof syst );
    syst.tm_year  = dts.year  - 1900;
    syst.tm_mon   = dts.month - 1;
    syst.tm_mday  = dts.day;
    syst.tm_hour  = dts.hour;
    syst.tm_min   = dts.minute;
    syst.tm_sec   = dts.second;
    syst.tm_wday  = dts.day_of_week;
    syst.tm_yday  = dts.day_of_year;
    return true;
  }

  string date_time::format(const char* fmt) const
  {
    struct tm syst;
    systemtime ( syst );
    char buf[128];
    strftime( buf, sizeof(buf) , fmt, &syst );
    return buf;
  }

  ustring date_time::format(const wchar* fmt) const
  {
    struct tm syst;
    systemtime ( syst );
    wchar buf[128];
    wcsftime( buf, sizeof(buf)/sizeof(wchar) , fmt, &syst );
    return buf;
  }


  ustring date_time::locale_format(const wchar* fmt) const 
  {
#ifdef WIN32
    //tool::datetime_t ft;
    //FileTimeToLocalFileTime((FILETIME*)&_time,(FILETIME*)&ft);
    SYSTEMTIME st;
    FileTimeToSystemTime((FILETIME*)&_time,&st);

    wchar str[64]; str[0] = 0;
    //uint flags = LOCALE_USER_DEFAULT;
    //if( !fmt ) flags |= DATE_SHORTDATE;
      
    int n = GetDateFormatW(
      LOCALE_USER_DEFAULT,  // locale
      0,      // options
      &st,    // date
      fmt,    // date format
      str,    // formatted string buffer
      64      // size of buffer
    );
    return str;
#else
    return format(fmt);
#endif
  }

  ustring date_time::default_format(bool full) const
  {
#ifdef WIN32
    //tool::datetime_t ft;
    //FileTimeToLocalFileTime((FILETIME*)&_time,(FILETIME*)&ft);
    SYSTEMTIME st;
    FileTimeToSystemTime((FILETIME*)&_time,&st);

    wchar str[64]; str[0] = 0;
    uint flags = full? DATE_LONGDATE: DATE_SHORTDATE;
      
    int n = GetDateFormatW(
      LOCALE_USER_DEFAULT,  // locale
      flags,  // options
      &st,    // date
      0,      // date format
      str,    // formatted string buffer
      64      // size of buffer
    );
    return str;
#else
    return L"";
#endif
  }

  int date_time::first_day_of_week() // returns first day of week for the current locale
  {
#ifdef WIN32
    wchar sz[2];
    BOOL bResult = GetLocaleInfoW(LOCALE_USER_DEFAULT, LOCALE_IFIRSTDAYOFWEEK, sz, 2);
    return sz[0]-'0';
#else
    return 0;
#endif
  }

  ustring date_time::week_day_name(int n, int maxlength)
  {
#ifdef WIN32
    wchar sz[64]; sz[0] = 0;
    GetLocaleInfoW(LOCALE_USER_DEFAULT, LOCALE_SABBREVDAYNAME1 + n, sz, 64);
    if(maxlength && maxlength < 64)
      sz[maxlength] = 0;
    return sz;
#else
    return "";
#endif
  }

  void date_time::date_format(date_format_order& order, wchar& separator )
  {
#ifdef WIN32
    wchar sz[4];
    BOOL bResult = GetLocaleInfoW(LOCALE_USER_DEFAULT, LOCALE_ILDATE, sz, 2);
    order = date_format_order(sz[0]-'0');
    bResult = GetLocaleInfoW(LOCALE_USER_DEFAULT, LOCALE_SDATE, sz, 4);
    separator = sz[0];
#else
    return;
#endif
  }


};
