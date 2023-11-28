/* sedate.c   Implements the ECMAScript date object 
 */

/* (c) COPYRIGHT 1993-98           NOMBAS, INC.
 *                                 64 SALEM ST.
 *                                 MEDFORD, MA 02155  USA
 * 
 * ALL RIGHTS RESERVED
 * 
 * This software is the property of Nombas, Inc. and is furnished under
 * license by Nombas, Inc.; this software may be used only in accordance
 * with the terms of said license.  This copyright notice may not be removed,
 * modified or obliterated without the prior written permission of Nombas, Inc.
 * 
 * This software is a Trade Secret of Nombas, Inc.
 * 
 * This software may not be copied, transmitted, provided to or otherwise made
 * available to any other person, company, corporation or other entity except
 * as specified in the terms of said license.
 * 
 * No right, title, ownership or other interest in the software is hereby
 * granted or transferred.
 * 
 * The information contained herein is subject to change without notice and
 * should not be construed as a commitment by Nombas, Inc.
 */

#include "jseopt.h"

// seb 98.11.12 -- Added
#if defined(__JSE_BEOS__)
#include <math.h>
#endif

#ifdef JSE_ECMA_DATE

/* JSE_MILLENIUM - flag that if set (!=0) turns off any functionality that will represent
 *                 two-digite dates with the assumption that they're between 1900
 *                 and 1999 (inclusive).
 */
#if !defined(JSE_MILLENIUM)
#  define JSE_MILLENIUM 0  /* By default this flag is off for pure ECMASCRIPT behavior */
#endif

/* Prototypes to shut up compiler */
static long NEAR_CALL DaysInYear(long y);
static long NEAR_CALL YearFromTime(jsenumber t);
static int NEAR_CALL InLeapYear(jsenumber t);
static int NEAR_CALL DayWithinYear(jsenumber t);
static int NEAR_CALL MonthFromTime(jsenumber t);
static int NEAR_CALL DateFromTime(jsenumber t);
static jsenumber NEAR_CALL millisec_from_gmtime(void);
static long NEAR_CALL DaylightSavingTA(jsenumber t);
static jsenumber NEAR_CALL MakeDay(jsenumber year,jsenumber month,jsenumber date);
static jsenumber NEAR_CALL MakeDate(jsenumber day,jsenumber time);
static jsebool NEAR_CALL IsFinite(jsenumber val);


/* All of these defines are used by the spec. I might optimize them later,
 * but using them this way means its more likely to be correct.
 */

/* All of these found in the docs */

#  define sign(x) ((x<0)?-1.0:1.0)
#  define msPerHour (1000.0 * 60.0 * 60.0)
#  define msPerMinute (1000.0 * 60.0)
#  define msPerDay 86400000.0
#  define msPerSecond 1000.0

#  define SecondsPerMinute 60.0
#  define MinutesPerHour 60.0
#  define HoursPerDay 24.0

#  define Day(t) floor(t/msPerDay)
#  define TimeWithinDay(t) fmod(t,msPerDay)



/* Each platform needs a function to tell the current time, in milliseconds, since
 * jan 1 1970.  On most systems the underlying library can alread handle
 * this.  If all else fails then just use time()*msPerSecond
 */
#if defined(__JSE_NWNLM__)
   /* for these systems, we don't yet know how to get milli-second accuracy */
#  define __DEFAULT_TIME_C_FUNCTION__
#endif
#if defined(__JSE_MAC__)
#  include <Timer.h>
#elif defined(__JSE_UNIX__)
#  include <sys/timeb.h>
#elif defined(__DEFAULT_TIME_C_FUNCTION__)
   /* for systems without ftime */
#else
#  if !defined(__JSE_WINCE__)
#     include <sys/timeb.h>
#  endif /* !(__JSE_WINCE__) */
#endif

   static jsenumber NEAR_CALL
msElapsedSince1970()
{
#if defined(__JSE_MAC__)
   /* macintosh does not support ftime */
   UnsignedWide   ms;
   time_t t = time(NULL);
   Microseconds(&ms);
   return ( ((jsenumber)t * msPerSecond)
          + ((unsigned short)((ms.lo/1000) % 1000)) );
#elif defined(__DEFAULT_TIME_C_FUNCTION__)
   /* a system without ftime() may use this version, but it will not return
    * millisecond accuracy.
    */
   time_t t = time(NULL);
   return ( ((jsenumber)t * msPerSecond) );
#else
   struct timeb tb;
   ftime(&tb);
   return ( ((jsenumber)(tb.time) * msPerSecond)
          + tb.millitm );
#endif
}




/* looking at the spec, it seems to think that modulos of negative numbers
 * result in positive numbers, but they do not - that is not how modulo works!
 */

   static int NEAR_CALL
HourFromTime(double t)
{
   assert( IsFinite(t) );
   return (int)(fmod(fmod(floor(t/msPerHour),HoursPerDay)+((t<0)?24.0:0.0),24));
}

   static int NEAR_CALL
MinFromTime(double t)
{
   assert( IsFinite(t) );
   return (int)(fmod(fmod(floor(t/msPerMinute),MinutesPerHour)+((t<0)?60.0:0.0),60));
}

   static int NEAR_CALL
SecFromTime(double t)
{
   assert( IsFinite(t) );
   return (int)(fmod(fmod(floor(t/msPerSecond),SecondsPerMinute)+((t<0)?60.0:0.0),60));
}


/* This is the 'builtin' date constructor. */
static jseLibFunc(BuiltinDateConstructor)
{
   // seb 98.11.12 -- Added cast to jseVariable
   jseReturnVar(jsecontext,(jseVariable)CreateNewObject(jsecontext,DATE_PROPERTY),jseRetTempVar);
}

   static slong NEAR_CALL
msFromTime(double t)
{
   assert( IsFinite(t) );
   return (slong)(fmod(fmod(t,msPerSecond)+((t<0)?1000.0:0.0),1000));
}

/* ---------------------------------------------------------------------- */

   static long NEAR_CALL
DaysInYear(long y)
{
   if((y%4)!=0 ) return 365;
   if((y%100)!=0 ) return 366;
   if((y%400)!=0 ) return 365;
   return 366;
}

   static double NEAR_CALL
DayFromYear(slong y)
{
   return (365.0*(y-1970.0)+floor((y-1969.0)/4.0)-floor((y-1901.0)/100.0)+floor((y-1601.0)/400.0));
}
#  define TimeFromYear(y) (msPerDay * DayFromYear(y))

   static long NEAR_CALL
YearFromTime(jsenumber t)
{
   /* NYI: this ought to get a guess at the year (using 365 days per year)
    *      and then search up or down from there.
    */
   long year;

   assert( IsFinite(t) );
   if( t>=0 )
   {
      for( year = 1970;;year++ )
      {
         if( TimeFromYear(year)>t ) return year-1;
      }
   }
   else
   {
      for( year=1969;;year-- )
      {
         if( TimeFromYear(year)<t ) return year;
      }
   }
   /* shouldn't reach here */
}


/* These next two must become functions because 'YearFromTime' is a function
 * call and cannot be successfully inserted into macros 
 */
   static int NEAR_CALL
InLeapYear(jsenumber t)
{
   long year;
   assert( IsFinite(t) );
   year = YearFromTime(t);
   return (DaysInYear(year)==366) ? 1 : 0 ;
}

   static int NEAR_CALL
DayWithinYear(jsenumber t)
{
   long year;
   assert( IsFinite(t) );
   year = YearFromTime(t);
   return ((int) (Day(t)-DayFromYear(year)));
}

   static int NEAR_CALL
MonthFromTime(jsenumber t)
{
   long i, i2;

   assert( IsFinite(t) );
   i = DayWithinYear(t);
   i2 = InLeapYear(t);
   if( i<31 ) return 0;
   if( i<59+i2 ) return 1;
   if( i<90+i2 ) return 2;
   if( i<120+i2 ) return 3;
   if( i<151+i2 ) return 4;
   if( i<181+i2 ) return 5;
   if( i<212+i2 ) return 6;
   if( i<243+i2 ) return 7;
   if( i<273+i2 ) return 8;
   if( i<304+i2 ) return 9;
   if( i<334+i2 ) return 10;
   return 11;
}

   static int NEAR_CALL
DateFromTime(jsenumber t)
{
   int i, d, m;

   assert( IsFinite(t) );
   d = DayWithinYear(t);
   m = MonthFromTime(t);
   i = InLeapYear(t);
   switch( m )
   {
      case 0: return d+1;
      case 1: return d-30;
      case 2: return d-58-i;
      case 3: return d-89-i;
      case 4: return d-119-i;
      case 5: return d-150-i;
      case 6: return d-180-i;
      case 7: return d-211-i;
      case 8: return d-242-i;
      case 9: return d-272-i;
      case 10: return d-303-i;
      case 11: return d-333-i;
   }

   /* Assertion means it should never get here and will trigger if it does. */
   assert( m>=0 && m<=11 );
   return 0;
}

   static int NEAR_CALL
WeekDay(double tm)
{
   int weekday;
   assert( IsFinite(tm) );
   weekday = (int)(fmod(Day(tm)+4,7));
   if ( weekday < 0 )
      weekday += 7;
   return weekday;
}

   static jsenumber NEAR_CALL
millisec_from_gmtime(void)
{
   /* this computation can be a tad slow and doesn't change once computed,
    * so do it once and save the result */
   static VAR_DATA(jsebool) first_time = True;
   static VAR_DATA(jsenumber) diff = 0.0;

   if( first_time )
   {
      time_t now = time(NULL);

      struct tm *tmstr = localtime(&now);
      time_t ourtime = mktime(gmtime(&now));
      time_t utctime = now;

      diff = (double)utctime - (double)ourtime -

         /* localtime is adjusted for DST, so we unadjust it */
         (tmstr->tm_isdst?1.0:0.0)*(SecondsPerMinute*MinutesPerHour);
      diff *= msPerSecond;

      first_time = False;
   }

   return diff;
}


#  define LocalTZA (millisec_from_gmtime())

   static long NEAR_CALL
DaylightSavingTA(jsenumber t)
{
   /* pick a known 1990's year with monday falling on the same day as that year
    * and determine DST as if it were that 90's year
    */
   static CONST_DATA(uword8) March1Years[7] = 
      /* years in 19th century with March 1 falling on sun, moon, tues, etc... */
      { 92, 93, 94, 95, 90, 91, 97 };
   int weekday;
   double March1Date;
   long year;
   struct tm buildTime, *st;
   time_t timeIn90s;

   assert( IsFinite(t) );
   year = YearFromTime(t);
   March1Date = MakeDate(MakeDay(year,2,1),0);
   assert( year == YearFromTime(March1Date) );
   
   weekday = WeekDay(March1Date);
   assert( 0 <= weekday  &&  weekday < 7 );

   buildTime.tm_sec = SecFromTime(t);
   buildTime.tm_min = MinFromTime(t);
   buildTime.tm_hour = HourFromTime(t);
   buildTime.tm_mday = DateFromTime(t);
   buildTime.tm_mon = MonthFromTime(t);
   buildTime.tm_year = March1Years[weekday];
   buildTime.tm_wday = 0;
   buildTime.tm_yday = 0;
   buildTime.tm_isdst = -1; /* let C library figure this out */
   timeIn90s = mktime(&buildTime);
   
   st = localtime(&timeIn90s);
   assert( NULL != st);
   return (long)((st->tm_isdst?1:0)*msPerHour);
}

   static double NEAR_CALL
LocalTime(double t)
{
   if ( !IsFinite(t) )
      return jseNaN;
   else
      return t + LocalTZA + DaylightSavingTA(t);
}

   static double NEAR_CALL
UTC(double t)
{
   if ( !IsFinite(t) )
      return jseNaN;
   else
      return t-LocalTZA - DaylightSavingTA(t-LocalTZA);
}
   
static CONST_DATA(jsechar *) monthnames[12] = {
   UNISTR("Jan"), UNISTR("Feb"), UNISTR("Mar"), UNISTR("Apr"), UNISTR("May"), UNISTR("Jun"), UNISTR("Jul"), UNISTR("Aug"), UNISTR("Sep"),
   UNISTR("Oct"), UNISTR("Nov"), UNISTR("Dec")
};

static CONST_DATA(jsechar *) daynames[7] = {
   UNISTR("Sun"), UNISTR("Mon"), UNISTR("Tue"), UNISTR("Wed"), UNISTR("Thu"), UNISTR("Fri"), UNISTR("Sat")
};

#define MY_ASCTIME_BUFFER_SIZE 200
   static void
my_asctime(jsechar buffer[MY_ASCTIME_BUFFER_SIZE],double tm,jsebool ConvertToLocaltime)
{
   long dt;

   assert( IsFinite(tm) );
   if ( ConvertToLocaltime )
   {
      tm = LocalTime(tm);
   }
   dt = DateFromTime(tm);

   sprintf_jsechar(buffer,UNISTR("%s %s %s%ld %02d:%02d:%02d %04d"),
           daynames[WeekDay(tm)],
           monthnames[MonthFromTime(tm)],
           dt<10?UNISTR(" "):UNISTR(""),dt,
           HourFromTime(tm),MinFromTime(tm),SecFromTime(tm),
           (int)YearFromTime(tm));
   if ( !ConvertToLocaltime )
      strcat_jsechar(buffer,UNISTR(" GMT"));
   assert( strlen_jsechar(buffer) < (MY_ASCTIME_BUFFER_SIZE-1)/sizeof(jsechar) );
}

/* ---------------------------------------------------------------------- */

/* all of these function are documented in the spec in section 15.9.x
 *
 * note: some of these functions do various casts from float->int->float.
 * this is needed to meet the spec.
 *
 * some utility functions to ease implementing as below 
 */


/*
 * Determine if the input is finite. Infinite, -Infinite, or NaN are bad
 */
static jsebool NEAR_CALL IsFinite(jsenumber val)
{
   return jseIsFinite(val);
}


static jsenumber NEAR_CALL MakeTime(jsenumber hour,jsenumber min,jsenumber sec,jsenumber ms)
{
   if( !IsFinite(hour) || !IsFinite(min) ||
       !IsFinite(sec) || !IsFinite(ms))
      return jseNaN;

   return
      hour*msPerHour +
      min*msPerMinute +
      sec*msPerSecond +
      ms;
}


static CONST_DATA(double) days_in_month[12] =
{
   31,28,31,30,31,30,31,31,30,31,30,31
};


   static jsenumber NEAR_CALL
MakeDay(jsenumber year,jsenumber month,jsenumber date)
{
   long y,d,r5,r6;
   int x, m;
   jsenumber t;

   if( !IsFinite(year) || !IsFinite(month) ||
       !IsFinite(date))
      return jseNaN;

   y = (long)year;
   m = (int)month;
   d = (long)date;

   r5 = y + m/12;
   r6 = m%12;

   t = TimeFromYear(r5);
   for( x=0;x<r6;x++ )
   {
      t += (days_in_month[x] * msPerDay);
      if( x==1 && InLeapYear(t)) t += msPerDay;
   }
   return Day(t) + d - 1;
}


   static jsenumber NEAR_CALL
MakeDate(jsenumber day,jsenumber time)
{
   if( !IsFinite(day) || !IsFinite(time))
      return jseNaN;

   return day*msPerDay + time;
}

   static jsenumber NEAR_CALL
TimeClip(jsenumber time)
{
   if( !IsFinite(time) || fabs(time)>8.64e15 )
      return jseNaN;

   return floor(fabs(time)) * sign(time);
}

   static void NEAR_CALL
ensure_valid_date(jseContext jsecontext,jseVariable what)
{
   ensure_type(jsecontext,what,DATE_PROPERTY);
}

/* ---------------------------------------------------------------------- */

static CONST_DATA(jsechar *) month_names[12] =
{
UNISTR("jan"), UNISTR("feb"), UNISTR("mar"), UNISTR("apr"), UNISTR("may"), UNISTR("jun"),
UNISTR("jul"), UNISTR("aug"), UNISTR("sep"), UNISTR("oct"), UNISTR("nov"), UNISTR("dec")
};

   static int NEAR_CALL
GetNumbersFromString(jsechar *str,jsechar separator,int nums[3])
   /* return 0 for failure, else 2 or 3 if filled 2 or 3 of the numbers */
{
   int i;
   for ( i = 0; i < 3; i++ )
   {
      int num = 0;
      jsebool neg = False;
      if ( *str == '+' )
      {
         *(str++) = ' ';
      }
      else if ( *str == '-' )
      {
         *(str++) = ' ';
         neg = True;
      }
      if ( !isdigit_jsechar(*str) )
         return 0;
      while ( isdigit_jsechar(*str) )
      {
         num = (num*10) + (*str - '0');
         *(str++) = ' ';
      }
      nums[i] = neg ? -num : num ;
      if ( *str == separator )
      {
         /* bad to have separator even after third number */
         *(str++) = ' ';
         if ( i == 2 )
            return 0;
      }
      else
      {
         if ( i == 0 )
            /* error not to get separator after first number */
            return 0;
         if ( i == 1 )
         {
            /* no separator after second number, so 3rd is 0 */
            nums[2] = 0;
            i++;
            break;
         }
      }
   }
   return i;
}

   static jsenumber NEAR_CALL
do_parse(jseContext jsecontext,jseVariable string)
{
   jsenumber t = jseNaN; /* assume failure */

   const jsechar *str = jseGetString(jsecontext,string,NULL);

   /* date comes in many formats.  Here we do our best of parsing in
    * the possible dates and figuring out from clues.
    */
   jsechar * DateBuf = StrCpyMalloc(str);
   jsechar *colon, *slash;
   jsebool ParseOK = True;
   int time[3]; /* hour, min, sec */
   int date[3]; /* month, day, year */
   int i;

   strlwr_jsechar(DateBuf);
   /* easiest bit to parse is time, which always has colons in it */
   colon = strchr_jsechar(DateBuf,':');
   if ( NULL == colon )
   {
      /* no time specified */
      time[0] = time[1] = time[2] = 0;
   }
   else
   {
      while ( DateBuf < colon  &&  !IS_WHITESPACE(colon[-1]) )
         colon--;
      if ( 2 <= GetNumbersFromString(colon,':',time) )
      {
         /* if there is a PM anywhere in string and <= 12 then adjust to PM */
         if ( time[0] < 12  &&  strstr_jsechar(DateBuf,UNISTR("pm")) )
            time[0] += 12;
      }
      else
      {
         ParseOK = False;
      }
   }

   if ( ParseOK )
   {
      ParseOK = False;
      /* try to find date in convenient m/d/y format */
      slash = strchr_jsechar(DateBuf,'/');
      date[2] = 0;
      if ( NULL != slash )
      {
         int slashmatch;
         while ( DateBuf < slash  &&  !IS_WHITESPACE(slash[-1]) )
            slash--;
         slashmatch = GetNumbersFromString(slash,'/',date);
         if ( 1 < slashmatch )
         {
            /* found convenient month/day/year format */
            date[0]--;
            if ( 2 == slashmatch )
            {
               /* year not found, make lastditch effort */
               goto LastDitchNumberSearch;
            }
            ParseOK = True;
         }
      }
      if ( !ParseOK )
      {
         /* look for names of month */
         for ( i = 0; i < 12; i++ )
         {
            if ( strstr_jsechar(DateBuf,month_names[i]) )
               break;
         }
         if ( i < 12 )
         {
            jsechar *c;
            date[0] = i;
            date[1] = 0; /* indicate that not yet found */
            /* there must be two more numbers, one is date other
             * is year.  year is bigger
             */
            LastDitchNumberSearch:
            for ( c = DateBuf; c[0] && (!date[1] || !date[2]); c++ )
            {
               int num;
               if ( isdigit_jsechar(*c) )
               {
                  num = atoi_jsechar(c);
                  while ( isdigit_jsechar(*c)  &&  c[1] )
                     c++;
                  /* year is 70 or greater */
                  if ( num < 70 )
                     date[1] = num;
                  else
                     date[2] = num;
               }
            }
            if ( date[1] && date[2] )
            {
               ParseOK = True;
            }
         }
      }

      if ( ParseOK )
      {
         const jsechar *gmt;

#        if !JSE_MILLENIUM
            /* do silly 1900 addition */
            if ( 0 <= date[2]  &&  date[2] <= 99 )
               date[2] += 1900;
#        endif

         /* convert to ecmascript date */
         t = TimeClip(MakeDate(MakeDay(date[2],date[0],date[1]),
                               MakeTime(time[0],time[1],time[2],0)));

         gmt = strstr_jsechar(DateBuf,UNISTR("gmt"));
         if ( gmt == NULL )
         {
            /* convert to local time */
            t = UTC(t);
         }
         else
         {
            /* time is already GMT, but there may be numbers following that
             * to adjust from GMT
             */
            int adjust;
            double fadjust;
            adjust = atoi_jsechar(gmt+3);
            if ( adjust )
            {
               /* adjust time number of hours and/or seconds */
               if ( 100 < adjust )
               {
                  /* high part is hours, low part is minutes */
                  fadjust = (adjust / 100.0) + ((adjust % 100) / 60.0);
               }
               else
               {
                  fadjust = adjust;
               }
               /* adjust time by fadjust hours */
               t -= fadjust * msPerHour;
            }
         }
      }
   }
   jseMustFree(DateBuf);

   return t;
}


/* ---------------------------------------------------------------------- */



   static void NEAR_CALL
do_date_construction(jseContext jsecontext,jseVariable thisvar)
{
   long args = (long) jseFuncVarCount(jsecontext);
   jsenumber value;
   jseVariable yearsvar,monthsvar,datevar,hoursvar,minutesvar,secondsvar,millisvar;
   jseVariable da,tmp;

   if( args>1 )
   {
      /* all of these perform more or less the same way. The values that are */
      /* not included are all set to 0 (1 for date). */

      double years,months,date = 1,hours = 0,minutes = 0,seconds = 0,millis = 0;

      JSE_FUNC_VAR_NEED(yearsvar,jsecontext,0,JSE_VN_CONVERT(JSE_VN_ANY,JSE_VN_NUMBER));
      years = jseGetNumber(jsecontext,yearsvar);
#     if !JSE_MILLENIUM
         if( years>=0 && years<=99 ) years += 1900;
#     endif
      JSE_FUNC_VAR_NEED(monthsvar,jsecontext,1,JSE_VN_CONVERT(JSE_VN_ANY,JSE_VN_NUMBER));
      months = jseGetNumber(jsecontext,monthsvar);

      if ( args>2 )
      {
         JSE_FUNC_VAR_NEED(datevar,jsecontext,2,JSE_VN_CONVERT(JSE_VN_ANY,JSE_VN_NUMBER));
         date = jseGetNumber(jsecontext,datevar);
      }
      if( args>3 )
      {
         JSE_FUNC_VAR_NEED(hoursvar,jsecontext,3,JSE_VN_CONVERT(JSE_VN_ANY,JSE_VN_NUMBER));
         hours = jseGetNumber(jsecontext,hoursvar);
      }
      if( args>4 )
      {
         JSE_FUNC_VAR_NEED(minutesvar,jsecontext,4,JSE_VN_CONVERT(JSE_VN_ANY,JSE_VN_NUMBER));
         minutes = jseGetNumber(jsecontext,minutesvar);
      }
      if( args>5 )
      {
         JSE_FUNC_VAR_NEED(secondsvar,jsecontext,5,JSE_VN_CONVERT(JSE_VN_ANY,JSE_VN_NUMBER));
         seconds = jseGetNumber(jsecontext,secondsvar);
      }

      if( args>6 )
      {
         JSE_FUNC_VAR_NEED(millisvar,jsecontext,6,JSE_VN_CONVERT(JSE_VN_ANY,JSE_VN_NUMBER));
         millis = jseGetNumber(jsecontext,millisvar);
      }

      value = TimeClip(UTC(MakeDate(MakeDay(years,months,date),
                                    MakeTime(hours,minutes,seconds,millis))));
   }

   else if( args==1 )
   {
      jseVariable cv = jseCreateConvertedVariable(jsecontext,jseFuncVar(jsecontext,0),
                                                  jseToPrimitive);

      /* We can call Ecma_Date_parse directly because in this case it takes
       * one parameter just like we do, the same type, and returns the result
       */
      if( jseGetType(jsecontext,cv)==jseTypeString )
      {
         value = do_parse(jsecontext,cv);
      }
      else
      {
         jseVariable cv2 = jseCreateConvertedVariable(jsecontext,cv,jseToNumber);
         value = jseGetNumber(jsecontext,cv2);
         jseDestroyVariable(jsecontext,cv2);
      }
      jseDestroyVariable(jsecontext,cv);
   }
   else
   {
      assert( args == 0 );
      value = msElapsedSince1970();
   }

   jseConvert(jsecontext,thisvar,jseTypeObject);

   /* First assign our prototype to the prototype from the original date object */
   da = jseFindVariable(jsecontext,DATE_PROPERTY,0);
   if( da )
   {
      jseVariable pr = jseGetMember(jsecontext,da,ORIG_PROTOTYPE_PROPERTY);
      if( pr )
      {
		 // seb 98.11.12 -- Added cast to jseVariable
         jseVariable me = (jseVariable)MyjseMember(jsecontext,thisvar,PROTOTYPE_PROPERTY,jseTypeObject);
         jseAssign(jsecontext,me,pr);
         jseSetAttributes(jsecontext,me,jseDontEnum);
      }
   }

   /* next assign our class to DATE_PROPERTY */
   da = jseMember(jsecontext,thisvar,CLASS_PROPERTY,jseTypeString);
   jseConvert(jsecontext,da,jseTypeString);
   jsePutString(jsecontext,da,DATE_PROPERTY);
   jseSetAttributes(jsecontext,da,jseDontEnum);

   /* assign the value. */
   // seb 98.11.12 -- Added cast to jseVariable
   jsePutNumber(jsecontext,
                ((jseVariable)tmp = (jseVariable)MyjseMember(jsecontext,thisvar,VALUE_PROPERTY,jseTypeNumber)),
                value);
   jseSetAttributes(jsecontext,tmp,jseDontEnum);
}


static jseLibFunc(DateConstruct)
{
   /* 'new' has already created the variable to work with as 'this' */
   do_date_construction(jsecontext,jseGetCurrentThisVariable(jsecontext));
}


/* In this case, we construct a new variable */
static jseLibFunc(DateCall)
{
   jseVariable newobj = jseCreateVariable(jsecontext,jseTypeObject);
   jseVariable strvar;

   do_date_construction(jsecontext,newobj);

   /* return this variable as a string */
   strvar = jseCreateConvertedVariable(jsecontext,newobj,jseToString);
   jseDestroyVariable(jsecontext,newobj);
   jseReturnVar(jsecontext,strvar,jseRetTempVar);
}


#if defined(JSE_ECMA_DATE_FROMSYSTEM)
/* Date.fromSystem() */
static jseLibFunc(Ecma_Date_fromSystem)
{
   jseVariable newobj = jseCreateVariable(jsecontext,jseTypeObject);
   jseVariable mem, sysDateVar;
   jsenumber t;

   JSE_FUNC_VAR_NEED(sysDateVar,jsecontext,0,JSE_VN_CONVERT(JSE_VN_ANY,JSE_VN_NUMBER));
   t = jseGetNumber(jsecontext,sysDateVar);
   if ( IsFinite(t) )
      /* system date was in seconds, so convert ot milliseconds */
      t *= msPerSecond; 

   do_date_construction(jsecontext,newobj);
   mem = jseMember(jsecontext,newobj,VALUE_PROPERTY,jseTypeNumber);
   jsePutNumber(jsecontext,mem,t);
   jseReturnVar(jsecontext,newobj,jseRetTempVar);
}
#endif

#if defined(JSE_ECMA_DATE_TOSYSTEM)
/* Date.toSystem() */
static jseLibFunc(Ecma_Date_toSystem)
{
   jseVariable thisvar = jseGetCurrentThisVariable(jsecontext);
   jsenumber units;

   ensure_valid_date(jsecontext,thisvar);

   units = jseGetNumber(jsecontext,
                                  jseMember(jsecontext,thisvar,VALUE_PROPERTY,jseTypeNumber));
   jseReturnNumber(jsecontext,units/msPerSecond);
}
#endif

static jseLibFunc(Ecma_Date_UTC)
{
   jseVariable yearsvar,monthsvar,datevar,hoursvar,minutesvar,secondsvar;
   jseVariable millisvar;

   long args = (long) jseFuncVarCount(jsecontext);

   /* all of these perform more or less the same way. The values that are
    * not included are all set to 0. 
    */

   double years,months,date,hours = 0,minutes = 0,seconds = 0,millis = 0;

   JSE_FUNC_VAR_NEED(yearsvar,jsecontext,0,JSE_VN_CONVERT(JSE_VN_ANY,JSE_VN_NUMBER));
   years = jseGetNumber(jsecontext,yearsvar);
#  if !JSE_MILLENIUM
      if( years>=0 && years<=99 ) years += 1900;
#  endif
   JSE_FUNC_VAR_NEED(monthsvar,jsecontext,1,JSE_VN_CONVERT(JSE_VN_ANY,JSE_VN_NUMBER));
   months = jseGetNumber(jsecontext,monthsvar);
   JSE_FUNC_VAR_NEED(datevar,jsecontext,2,JSE_VN_CONVERT(JSE_VN_ANY,JSE_VN_NUMBER));
   date = jseGetNumber(jsecontext,datevar);

   if( args>3 )
   {
      JSE_FUNC_VAR_NEED(hoursvar,jsecontext,3,JSE_VN_CONVERT(JSE_VN_ANY,JSE_VN_NUMBER));
      hours = jseGetNumber(jsecontext,hoursvar);
   }
   if( args>4 )
   {
      JSE_FUNC_VAR_NEED(minutesvar,jsecontext,4,JSE_VN_CONVERT(JSE_VN_ANY,JSE_VN_NUMBER));
      minutes = jseGetNumber(jsecontext,minutesvar);
   }
   if( args>5 )
   {
      JSE_FUNC_VAR_NEED(secondsvar,jsecontext,5,JSE_VN_CONVERT(JSE_VN_ANY,JSE_VN_NUMBER));
      seconds = jseGetNumber(jsecontext,secondsvar);
   }

   if( args>6 )
   {
      JSE_FUNC_VAR_NEED(millisvar,jsecontext,6,JSE_VN_CONVERT(JSE_VN_ANY,JSE_VN_NUMBER));
      millis = jseGetNumber(jsecontext,millisvar);
   }

   jseReturnNumber(jsecontext,TimeClip(MakeDate(MakeDay(years,months,date),
                                     MakeTime(hours,minutes,seconds,millis))));
}


static jseLibFunc(Ecma_Date_parse)
{
   /* This is how the spec demands it be done. */
   jseVariable string = jseCreateConvertedVariable(jsecontext,jseFuncVar(jsecontext,0),
                                                   jseToString);

   jsenumber t = do_parse(jsecontext,string);

   jseReturnNumber(jsecontext,t);

   jseDestroyVariable(jsecontext,string);
}



static jseLibFunc(Ecma_Date_toString)
{
   jseVariable thisvar = jseGetCurrentThisVariable(jsecontext);
   jsenumber units;
   jsechar *value;
   jseVariable ret;
   jsechar buffer[MY_ASCTIME_BUFFER_SIZE];

   ensure_valid_date(jsecontext,thisvar);

   units = jseGetNumber(jsecontext,
                        jseMember(jsecontext,thisvar,VALUE_PROPERTY,jseTypeNumber));

   if( jseIsNaN(units) )
      value = UNISTR("invalid date");
   else
      my_asctime(value=buffer,units,True);

   ret = jseCreateVariable(jsecontext,jseTypeString);
   jsePutString(jsecontext,ret,value);
   jseReturnVar(jsecontext,ret,jseRetTempVar);
}


static jseLibFunc(Ecma_Date_valueOf)
{
   jseVariable thisvar = jseGetCurrentThisVariable(jsecontext);
   ensure_valid_date(jsecontext,thisvar);

   jseReturnNumber(jsecontext,jseGetNumber(jsecontext,
                                jseMember(jsecontext,thisvar,VALUE_PROPERTY,jseTypeNumber)));
}


#define GET_MILLI 1
#define GET_SEC   2
#define GET_MIN   3
#define GET_HOUR  4
#define GET_DATE  5
#define GET_DAY   6
#define GET_MONTH 7
#define GET_YEAR  8
#if !JSE_MILLENIUM
#  define GET_YEAR_1900   9
#endif

   static void NEAR_CALL
DateGet(jseContext jsecontext,int WhichGet,jsebool Local)
{
   jseVariable thisvar = jseGetCurrentThisVariable(jsecontext);
   jsenumber t;

   ensure_valid_date(jsecontext,thisvar);

   t = jseGetNumber(jsecontext,
                    jseMember(jsecontext,thisvar,VALUE_PROPERTY,jseTypeNumber));
   if( !jseIsNaN(t))
   {
      if ( Local )
      {
         t = LocalTime(t);
      }
      switch ( WhichGet )
      {
         case GET_MILLI:
            t = msFromTime(t);
            break;
         case GET_SEC:
            t = SecFromTime(t);
            break;
         case GET_MIN:
            t = MinFromTime(t);
            break;
         case GET_HOUR:
            t = HourFromTime(t);
            break;
         case GET_DATE:
            t = DateFromTime(t);
            break;
         case GET_DAY:
            t = WeekDay(t);
            break;
         case GET_MONTH:
            t = MonthFromTime(t);
            break;
         case GET_YEAR:
            t = YearFromTime(t);
            break;
#        if !JSE_MILLENIUM
         case GET_YEAR_1900:
            t = YearFromTime(t);
            if ( 1900 <= t  &&  t <= 1999 )
               t -= 1900;
            break;
#        endif
      }
   }

   jseReturnNumber(jsecontext,t);
}

#if !JSE_MILLENIUM
static jseLibFunc(Ecma_Date_getYear)
{
   DateGet(jsecontext,GET_YEAR_1900,True);
}
#endif

static jseLibFunc(Ecma_Date_getFullYear)
{
   DateGet(jsecontext,GET_YEAR,True);
}

static jseLibFunc(Ecma_Date_getUTCFullYear)
{
   DateGet(jsecontext,GET_YEAR,False);
}

static jseLibFunc(Ecma_Date_getMonth)
{
   DateGet(jsecontext,GET_MONTH,True);
}

static jseLibFunc(Ecma_Date_getUTCMonth)
{
   DateGet(jsecontext,GET_MONTH,False);
}

static jseLibFunc(Ecma_Date_getDate)
{
   DateGet(jsecontext,GET_DATE,True);
}

static jseLibFunc(Ecma_Date_getUTCDate)
{
   DateGet(jsecontext,GET_DATE,False);
}

static jseLibFunc(Ecma_Date_getDay)
{
   DateGet(jsecontext,GET_DAY,True);
}

static jseLibFunc(Ecma_Date_getUTCDay)
{
   DateGet(jsecontext,GET_DAY,False);
}

static jseLibFunc(Ecma_Date_getHours)
{
   DateGet(jsecontext,GET_HOUR,True);
}

static jseLibFunc(Ecma_Date_getUTCHours)
{
   DateGet(jsecontext,GET_HOUR,False);
}

static jseLibFunc(Ecma_Date_getMinutes)
{
   DateGet(jsecontext,GET_MIN,True);
}

static jseLibFunc(Ecma_Date_getUTCMinutes)
{
   DateGet(jsecontext,GET_MIN,False);
}

static jseLibFunc(Ecma_Date_getSeconds)
{
   DateGet(jsecontext,GET_SEC,True);
}

static jseLibFunc(Ecma_Date_getUTCSeconds)
{
   DateGet(jsecontext,GET_SEC,False);
}

static jseLibFunc(Ecma_Date_getMilliseconds)
{
   DateGet(jsecontext,GET_MILLI,True);
}

static jseLibFunc(Ecma_Date_getUTCMilliseconds)
{
   DateGet(jsecontext,GET_MILLI,False);
}


static jseLibFunc(Ecma_Date_getTimezoneOffset)
{
   jseVariable thisvar = jseGetCurrentThisVariable(jsecontext);
   jsenumber t;

   ensure_valid_date(jsecontext,thisvar);

   t = jseGetNumber(jsecontext,
                    jseMember(jsecontext,thisvar,VALUE_PROPERTY,jseTypeNumber));
   if( !jseIsNaN(t))
      t = (t-LocalTime(t))/msPerMinute;

   jseReturnNumber(jsecontext,t);
}


static jseLibFunc(Ecma_Date_setTime)
{
   jseVariable thisvar = jseGetCurrentThisVariable(jsecontext);
   jsenumber t;
   jseVariable n;

   ensure_valid_date(jsecontext,thisvar);


   n = jseCreateConvertedVariable(jsecontext,jseFuncVar(jsecontext,0),jseToNumber);
   t = TimeClip(jseGetNumber(jsecontext,n));

   jseDestroyVariable(jsecontext,n);

   jsePutNumber(jsecontext,jseMember(jsecontext,thisvar,VALUE_PROPERTY,jseTypeNumber),t);

   jseReturnNumber(jsecontext,t);
}


   static double NEAR_CALL
ForceNumberFromFuncVar(jseContext jsecontext,int WhichParameter)
{
   jseVariable var;
   double ret;
   var = jseCreateConvertedVariable(jsecontext,jseFuncVar(jsecontext,(uint)WhichParameter),jseToNumber);
   ret = jseGetNumber(jsecontext,var);
   jseDestroyVariable(jsecontext,var);
   return ret;
}

   static void NEAR_CALL
SetHourMinSecMilli(jseContext jsecontext,int MaxParams/*hourm,min,sec,milli*/,
                   jsebool Local)
{
   jseVariable thisvar = jseGetCurrentThisVariable(jsecontext);
   double hour,min,sec,milli;
   jsenumber t,val;
   int ParmCount = (int) jseFuncVarCount(jsecontext);

   ensure_valid_date(jsecontext,thisvar);

   t = jseGetNumber(jsecontext,
                    jseMember(jsecontext,thisvar,VALUE_PROPERTY,jseTypeNumber));
   if ( Local )
   {
      t = LocalTime(t);
   }

   /* HOUR */
   if ( MaxParams == 4 )
   {
      hour = ForceNumberFromFuncVar(jsecontext,0);
      ParmCount--;
   }
   else
   {
      hour = HourFromTime(t);
   }

   /* MINUTE */
   if ( 3 <= MaxParams  &&  ParmCount )
   {
      min = ForceNumberFromFuncVar(jsecontext,MaxParams-3);
      ParmCount--;
   }
   else
   {
      min = MinFromTime(t);
   }

   /* SECOND */
   if ( 2 <= MaxParams  &&  ParmCount )
   {
      sec = ForceNumberFromFuncVar(jsecontext,MaxParams-2);
      ParmCount--;
   }
   else
   {
      sec = SecFromTime(t);
   }

   /* MILLI */
   if ( ParmCount )
   {
      milli = ForceNumberFromFuncVar(jsecontext,MaxParams-1);
   }
   else
   {
      milli = msFromTime(t);
   }

   val = MakeDate(Day(t),MakeTime(hour,min,sec,milli));
   if ( Local )
   {
      val = UTC(val);
   }

   jsePutNumber(jsecontext,jseMember(jsecontext,thisvar,VALUE_PROPERTY,jseTypeNumber),val);

   jseReturnNumber(jsecontext,val);
}

static jseLibFunc(Ecma_Date_setMilliseconds)
{
   SetHourMinSecMilli(jsecontext,1,True);
}

static jseLibFunc(Ecma_Date_setUTCMilliseconds)
{
   SetHourMinSecMilli(jsecontext,1,False);
}

static jseLibFunc(Ecma_Date_setSeconds)
{
   SetHourMinSecMilli(jsecontext,2,True);
}

static jseLibFunc(Ecma_Date_setUTCSeconds)
{
   SetHourMinSecMilli(jsecontext,2,False);
}

static jseLibFunc(Ecma_Date_setMinutes)
{
   SetHourMinSecMilli(jsecontext,3,True);
}

static jseLibFunc(Ecma_Date_setUTCMinutes)
{
   SetHourMinSecMilli(jsecontext,2,False);
}

static jseLibFunc(Ecma_Date_setHours)
{
   SetHourMinSecMilli(jsecontext,4,True);
}

static jseLibFunc(Ecma_Date_setUTCHours)
{
   SetHourMinSecMilli(jsecontext,4,False);
}

   static void NEAR_CALL
SetYearMonDay(jseContext jsecontext,int MaxParams/*year,mon,day*/,
              jsebool Local,jsebool CheckMonAndDay,jsebool century1900)
{
   jseVariable thisvar = jseGetCurrentThisVariable(jsecontext);
   double year,mon,day;
   jsenumber t,val;
   int ParmCount = (int) jseFuncVarCount(jsecontext);

   ensure_valid_date(jsecontext,thisvar);

   t = jseGetNumber(jsecontext,
                    jseMember(jsecontext,thisvar,VALUE_PROPERTY,jseTypeNumber));
   if ( Local )
   {
      t = LocalTime(t);
   }

   /* YEAR */
   if ( MaxParams == 3 )
   {
      if( jseIsNaN(t)) t = 0.0;
      year = ForceNumberFromFuncVar(jsecontext,0);
#     if !JSE_MILLENIUM
         if ( century1900 )
         {
            if ( 0 <= year  &&  year <= 99 )
               year += 1900;
         }
#     endif
      ParmCount--;
   }
   else
   {
      year = YearFromTime(t);
   }

   /* MONTH */
   if ( CheckMonAndDay  &&  2 <= MaxParams  &&  ParmCount )
   {
      mon = ForceNumberFromFuncVar(jsecontext,MaxParams-2);
      ParmCount--;
   }
   else
   {
      mon = MonthFromTime(t);
   }

   /* DAY */
   if ( CheckMonAndDay  &&  ParmCount )
   {
      day = ForceNumberFromFuncVar(jsecontext,MaxParams-1);
   }
   else
   {
      day = DateFromTime(t);
   }

   val = MakeDate(MakeDay(year,mon,day),TimeWithinDay(t));
   if ( Local )
   {
      val = UTC(val);
   }

   jsePutNumber(jsecontext,jseMember(jsecontext,thisvar,VALUE_PROPERTY,jseTypeNumber),val);

   jseReturnNumber(jsecontext,val);
}

static jseLibFunc(Ecma_Date_setDate)
{
   SetYearMonDay(jsecontext,1,True,True,False);
}

static jseLibFunc(Ecma_Date_setUTCDate)
{
   SetYearMonDay(jsecontext,1,False,True,False);
}

static jseLibFunc(Ecma_Date_setMonth)
{
   SetYearMonDay(jsecontext,2,True,True,False);
}

static jseLibFunc(Ecma_Date_setUTCMonth)
{
   SetYearMonDay(jsecontext,2,False,True,False);
}

static jseLibFunc(Ecma_Date_setFullYear)
{
   SetYearMonDay(jsecontext,3,True,True,False);
}

static jseLibFunc(Ecma_Date_setUTCFullYear)
{
   SetYearMonDay(jsecontext,3,False,True,False);
}

static jseLibFunc(Ecma_Date_setYear)
{
   SetYearMonDay(jsecontext,3,True,False,True);
}

   static void NEAR_CALL
DateToString(jseContext jsecontext,jsebool toLocale)
{
   jseVariable thisvar = jseGetCurrentThisVariable(jsecontext);
   jsenumber t;
   jseVariable ret;
   jsechar buf[MY_ASCTIME_BUFFER_SIZE];

   ensure_valid_date(jsecontext,thisvar);

   t = jseGetNumber(jsecontext,
                    jseMember(jsecontext,thisvar,VALUE_PROPERTY,jseTypeNumber));

   my_asctime(buf,t,toLocale);

   ret = jseCreateVariable(jsecontext,jseTypeString);

   jsePutString(jsecontext,ret,buf);

   jseReturnVar(jsecontext,ret,jseRetTempVar);
}


static jseLibFunc(Ecma_Date_toLocaleString)
{
   DateToString(jsecontext,True);
}


static jseLibFunc(Ecma_Date_toUTCString)
{
   DateToString(jsecontext,False);
}


/* ---------------------------------------------------------------------- */

static CONST_DATA(struct jseFunctionDescription) DateObjectFunctionList[] =
{
JSE_LIBOBJECT( DATE_PROPERTY,                DateCall,                0,       7,      jseDontEnum , jseFunc_Secure ),
JSE_LIBMETHOD( CONSTRUCT_PROPERTY,    DateConstruct,           0,       7,      jseDontEnum , jseFunc_Secure ),
JSE_PROTOMETH( CONSTRUCTOR_PROPERTY,    BuiltinDateConstructor,  0,       0,      jseDontEnum , jseFunc_Secure ),
JSE_LIBMETHOD( UNISTR("parse"),               Ecma_Date_parse,              1 ,      1,      jseDontEnum , jseFunc_Secure ),
JSE_LIBMETHOD( UNISTR("UTC"),                 Ecma_Date_UTC,                3 ,      7,      jseDontEnum , jseFunc_Secure ),
#if defined(JSE_ECMA_DATE_FROMSYSTEM)
   JSE_LIBMETHOD( UNISTR("fromSystem"),       Ecma_Date_fromSystem,         1 ,      1,      jseDontEnum , jseFunc_Secure ),
#endif
JSE_PROTOMETH( UNISTR("toString"),            Ecma_Date_toString,           0,       0,      jseDontEnum , jseFunc_Secure ),
/* These two share the same function */
JSE_PROTOMETH( UNISTR("valueOf"),             Ecma_Date_valueOf,            0,       0,      jseDontEnum , jseFunc_Secure ),
JSE_PROTOMETH( UNISTR("getTime"),             Ecma_Date_valueOf,            0,       0,      jseDontEnum , jseFunc_Secure ),

#if !JSE_MILLENIUM
   JSE_PROTOMETH( UNISTR("getYear"),          Ecma_Date_getYear,            0,       0,      jseDontEnum , jseFunc_Secure ),
#endif   
JSE_PROTOMETH( UNISTR("getFullYear"),         Ecma_Date_getFullYear,        0,       0,      jseDontEnum , jseFunc_Secure ),
JSE_PROTOMETH( UNISTR("getUTCFullYear"),      Ecma_Date_getUTCFullYear,     0,       0,      jseDontEnum , jseFunc_Secure ),
JSE_PROTOMETH( UNISTR("getMonth"),            Ecma_Date_getMonth,           0,       0,      jseDontEnum , jseFunc_Secure ),
JSE_PROTOMETH( UNISTR("getUTCMonth"),         Ecma_Date_getUTCMonth,        0,       0,      jseDontEnum , jseFunc_Secure ),
JSE_PROTOMETH( UNISTR("getDate"),             Ecma_Date_getDate,            0,       0,      jseDontEnum , jseFunc_Secure ),
JSE_PROTOMETH( UNISTR("getUTCDate"),          Ecma_Date_getUTCDate,         0,       0,      jseDontEnum , jseFunc_Secure ),
JSE_PROTOMETH( UNISTR("getDay"),              Ecma_Date_getDay,             0,       0,      jseDontEnum , jseFunc_Secure ),
JSE_PROTOMETH( UNISTR("getUTCDay"),           Ecma_Date_getUTCDay,          0,       0,      jseDontEnum , jseFunc_Secure ),
JSE_PROTOMETH( UNISTR("getHours"),            Ecma_Date_getHours,           0,       0,      jseDontEnum , jseFunc_Secure ),
JSE_PROTOMETH( UNISTR("getUTCHours"),         Ecma_Date_getUTCHours,        0,       0,      jseDontEnum , jseFunc_Secure ),
JSE_PROTOMETH( UNISTR("getMinutes"),          Ecma_Date_getMinutes,         0,       0,      jseDontEnum , jseFunc_Secure ),
JSE_PROTOMETH( UNISTR("getUTCMinutes"),       Ecma_Date_getUTCMinutes,      0,       0,      jseDontEnum , jseFunc_Secure ),
JSE_PROTOMETH( UNISTR("getSeconds"),          Ecma_Date_getSeconds,         0,       0,      jseDontEnum , jseFunc_Secure ),
JSE_PROTOMETH( UNISTR("getUTCSeconds"),       Ecma_Date_getUTCSeconds,      0,       0,      jseDontEnum , jseFunc_Secure ),
JSE_PROTOMETH( UNISTR("getMilliseconds"),     Ecma_Date_getMilliseconds,    0,       0,      jseDontEnum , jseFunc_Secure ),
JSE_PROTOMETH( UNISTR("getUTCMilliseconds"),  Ecma_Date_getUTCMilliseconds, 0,       0,      jseDontEnum , jseFunc_Secure ),
JSE_PROTOMETH( UNISTR("getTimezoneOffset"),   Ecma_Date_getTimezoneOffset,  0,       0,      jseDontEnum , jseFunc_Secure ),
JSE_PROTOMETH( UNISTR("setTime"),             Ecma_Date_setTime,            1,       1,      jseDontEnum , jseFunc_Secure ),
JSE_PROTOMETH( UNISTR("setMilliseconds"),     Ecma_Date_setMilliseconds,    1,       1,      jseDontEnum , jseFunc_Secure ),
JSE_PROTOMETH( UNISTR("setUTCMilliseconds"),  Ecma_Date_setUTCMilliseconds, 1,       1,      jseDontEnum , jseFunc_Secure ),
JSE_PROTOMETH( UNISTR("setSeconds"),          Ecma_Date_setSeconds,         1,       2,      jseDontEnum , jseFunc_Secure ),
JSE_PROTOMETH( UNISTR("setUTCSeconds"),       Ecma_Date_setUTCSeconds,      1,       2,      jseDontEnum , jseFunc_Secure ),
JSE_PROTOMETH( UNISTR("setMinutes"),          Ecma_Date_setMinutes,         1,       3,      jseDontEnum , jseFunc_Secure ),
JSE_PROTOMETH( UNISTR("setUTCMinutes"),       Ecma_Date_setUTCMinutes,      1,       3,      jseDontEnum , jseFunc_Secure ),
JSE_PROTOMETH( UNISTR("setHours"),            Ecma_Date_setHours,           1,       4,      jseDontEnum , jseFunc_Secure ),
JSE_PROTOMETH( UNISTR("setUTCHours"),         Ecma_Date_setUTCHours,        1,       4,      jseDontEnum , jseFunc_Secure ),
JSE_PROTOMETH( UNISTR("setDate"),             Ecma_Date_setDate,            1,       1,      jseDontEnum , jseFunc_Secure ),
JSE_PROTOMETH( UNISTR("setUTCDate"),          Ecma_Date_setUTCDate,         1,       1,      jseDontEnum , jseFunc_Secure ),
JSE_PROTOMETH( UNISTR("setMonth"),            Ecma_Date_setMonth,           1,       2,      jseDontEnum , jseFunc_Secure ),
JSE_PROTOMETH( UNISTR("setUTCMonth"),         Ecma_Date_setUTCMonth,        1,       2,      jseDontEnum , jseFunc_Secure ),
JSE_PROTOMETH( UNISTR("setFullYear"),         Ecma_Date_setFullYear,        1,       3,      jseDontEnum , jseFunc_Secure ),
JSE_PROTOMETH( UNISTR("setUTCFullYear"),      Ecma_Date_setUTCFullYear,     1,       3,      jseDontEnum , jseFunc_Secure ),
JSE_PROTOMETH( UNISTR("setYear"),             Ecma_Date_setYear,            1,       1,      jseDontEnum , jseFunc_Secure ),
JSE_PROTOMETH( UNISTR("toLocaleString"),      Ecma_Date_toLocaleString,     0,       0,      jseDontEnum , jseFunc_Secure ),
/* These two share the same function */
JSE_PROTOMETH( UNISTR("toUTCString"),         Ecma_Date_toUTCString,        0,       0,      jseDontEnum , jseFunc_Secure ),
JSE_PROTOMETH( UNISTR("toGMTString"),         Ecma_Date_toUTCString,        0,       0,      jseDontEnum , jseFunc_Secure ),
#if defined(JSE_ECMA_DATE_TOSYSTEM)
   JSE_PROTOMETH( UNISTR("toSystem"),         Ecma_Date_toSystem,           0 ,      0,      jseDontEnum , jseFunc_Secure ),
#endif

JSE_FUNC_END
};



   static void * JSE_CFUNC
DateLibInitFunction(jseContext jsecontext,void *unused)
{
   jseVariable date = jseMemberEx(jsecontext,NULL,DATE_PROPERTY,jseTypeObject,jseCreateVar);
   jseVariable t = jseMemberEx(jsecontext,date,PROTOTYPE_PROPERTY,jseTypeObject,jseCreateVar);

   UNUSED_PARAMETER(unused);

   jseSetAttributes(jsecontext,t,jseDontEnum|jseDontDelete|jseReadOnly);
   jseDestroyVariable(jsecontext,t);
   jseDestroyVariable(jsecontext,date);
   return NULL;
}


   void NEAR_CALL
InitializeLibrary_Ecma_Date(jseContext jsecontext)
{
   jseAddLibrary(jsecontext,NULL,DateObjectFunctionList,NULL,DateLibInitFunction,NULL);
}

#endif

ALLOW_EMPTY_FILE
