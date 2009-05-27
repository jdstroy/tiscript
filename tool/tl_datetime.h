//|
//|
//| Copyright (c) 2001-2005
//| Andrew Fedoniouk - andrew@terra-informatica.org
//|
//| Date & time class
//|
//|

#ifndef __tl_datetime_h__
#define __tl_datetime_h__

#include <time.h>
#include "tl_string.h"
#include "tl_ustring.h"
#include "tl_slice.h"

namespace tool
{
  typedef int64  datetime_t;

  class date_time
  {
  public:
    static date_time now (bool utc = true);
    static datetime_t local_offset();
    static datetime_t local_offset_ms();
    static int first_day_of_week(); // returns first day of week for the current locale
    static ustring week_day_name(int n, int maxlength = 0); // returns week day name  for the current locale

    enum date_format_order
    {
      MDY = 0, 
      DMY = 1, 
      YMD = 2
    };

    enum time_format_hours
    {
      H_12 = 0,
      H_24 = 1
    };

    enum time_format_marker_pos
    {
      MP_AFTER = 0,
      MP_BEFORE = 1,
    };

    static void date_format(date_format_order& order, wchar& separator );
    static void time_format(time_format_hours&      hours, 
                            time_format_marker_pos& marker_pos,
                            ustring& am_text,
                            ustring& pm_text);

  public:

    date_time ();
    date_time ( const date_time &dt );
    date_time ( datetime_t dt );
    date_time ( const struct tm& syst );

    //date_time ( time_t time );
    date_time (int year, int month, int day,
    int hours = 0, int minutes = 0, int seconds = 0,
    int milli = 0, int micro = 0, int nano = 0 );

    void to_local();
    void to_utc();

    // operators
  public:
    const date_time& operator= ( const date_time& dt );
    const date_time& operator= ( datetime_t dts );
    //const date_time& operator= ( const struct tm& syst );

    bool operator== ( const date_time& date ) const;
    bool operator!= ( const date_time& date ) const;
    bool operator< ( const date_time& date ) const;
    bool operator> ( const date_time& date ) const;
    bool operator<= ( const date_time& date ) const;
    bool operator>= ( const date_time& date ) const;

    
    
    string  format(const char *fmt) const;
    ustring format(const wchar *fmt) const;
    ustring locale_format(const wchar* fmt) const; 
    ustring default_format(bool full = false) const; 

    enum type
    { 
      DT_UNKNOWN          = 0x00,
      DT_HAS_DATE         = 0x01,
      DT_HAS_TIME         = 0x02,
      DT_HAS_SECONDS      = 0x04,
      DT_UTC              = 0x10, 
    };

    static  date_time parse_iso /*8601*/( tool::wchars str, uint& dt ); 
    static  date_time parse_iso /*8601*/( tool::chars str, uint& dt ); // utc == true if it is known to be UTC time
     
    string  emit_iso /*8601*/( uint dt ) const; // utc == true if it is known to be UTC time

    // Operations
  public:
    bool set ( int year, int month, int day,
    int hours, int minutes, int seconds,
    int milli = 0, int micro = 0, int nano = 0 );
    bool set_date ( int year, int month, int day );
    bool set_time ( int hours, int minutes, int seconds );
    bool set_frac_time ( int millis, int micros, int nanos );

    bool systemtime ( struct tm& syst ) const;
    

    // getters
    int year    () const;
    int month   () const;     // month of year (1 = Jan)
    int day     () const;     // day of month (1-31)
    int hours   () const;     // hour in day (0-23)
    int minutes () const;     // minute in hour (0-59)
    int seconds () const;     // second in minute (0-59)
    int millis  () const;     // millisecond in minute (0-999)
    int micros  () const;     // microsecond in minute (0-999)
    int nanos   () const;     // nanosecond in minute (0-999), step of 100ns
    int day_of_week () const; // (mon=0...sun=6)
    int day_of_year () const; // days since start of year, Jan 1 = 1

    // setters
    void nanos   ( int nv );
    void micros  ( int nv );
    void millis  ( int nv );
    void seconds ( int nv );
    void minutes ( int nv );
    void hours   ( int nv );
    void day     ( int nv );
    void month   ( int nv );
    void year    ( int nv );

    datetime_t  absolute_millis ();
    void        absolute_millis ( datetime_t t );

    // formatting
    // ....

  public:
    datetime_t         time() const { return _time; }
  private:
    datetime_t         _time;
  public:
    struct datetime_s
    {
      int  year;
      unsigned int month;
      unsigned int day;
      unsigned int hour;
      unsigned int minute;
      unsigned int second;
      unsigned int millis;
      unsigned int micros;
      unsigned int nanos;
      unsigned int day_of_year;
      unsigned int day_of_week;
    };
    static int month_day_in_year [ 13 ];

    static bool cvt ( datetime_t& dst, const datetime_s& src );
    static bool cvt ( datetime_s& dst, const datetime_t& src );
    static bool cvt (  struct tm& dst, const datetime_s& src );
    static bool cvt ( datetime_s& dst, const  struct tm& src );

  };


  inline
    date_time::date_time () : _time ( 0 )
  {
    ;
  }  // 1 Jan-1601, 00:00, 100ns clicks

  inline
    date_time::date_time ( const date_time &src )
  {
    _time = src._time;
  }

  inline
    date_time::date_time ( datetime_t t )
  {
    _time = t;
  }

  /*
  inline
    date_time::date_time ( time_t timeSrc )
  {
    struct tm _tm = *( localtime ( &timeSrc ) );
    *this = _tm;
  }*/

  inline const
    date_time& date_time::operator= ( const date_time& src )
  {
    _time = src._time;
    return *this;
  }

  inline const
    date_time& date_time::operator= ( const datetime_t src )
  {
    _time = src;
    return *this;
  }

  inline bool
    date_time::set_date ( int year, int month, int day )
  {
    return set ( year, month, day, 0, 0, 0, 0, 0, 0 );
  }

  inline bool
    date_time::set_time ( int hour, int min, int sec )
  {
    return set ( 1601, 1, 1, hour, min, sec, 0, 0, 0 );
  }

  inline bool
    date_time::set_frac_time ( int millis, int micros, int nanos )
  {
    datetime_s d;
    cvt(d,_time);
    return set ( d.year, d.month, d.day, d.hour, d.minute, d.second, millis, micros, nanos );
  }

  inline bool
    date_time::operator== ( const date_time& date ) const
  {
    return ( _time == date._time );
  }

  inline bool
    date_time::operator!= ( const date_time& date ) const
  {
    return ( _time != date._time );
  }

  inline bool
    date_time::operator< ( const date_time& date ) const
  {
    return ( _time < date._time );
  }

  inline bool
    date_time::operator> ( const date_time& date ) const
  {
    return ( _time > date._time );
  }

  inline bool
    date_time::operator<= ( const date_time& date ) const
  {
    return ( _time <= date._time );
  }

  inline bool
    date_time::operator>= ( const date_time& date ) const
  {
    return ( _time >= date._time );
  }

}

#endif
