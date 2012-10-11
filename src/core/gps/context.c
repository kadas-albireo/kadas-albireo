/*
* Copyright Tim (xtimor@gmail.com)
*
* NMEA library is free software; you can redistribute it and/or modify
* it under the terms of the GNU Lesser General Public License as published by
* the Free Software Foundation; either version 2 of the License, or
* (at your option) any later version.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU Lesser General Public License for more details.
*
* You should have received a copy of the GNU Lesser General Public License
* along with this program.  If not, see <http://www.gnu.org/licenses/>
*/
/*
 *
 * NMEA library
 * URL: http://nmea.sourceforge.net
 * Author: Tim (xtimor@gmail.com)
 * Licence: http://www.gnu.org/licenses/lgpl.html
 * $Id: context.c 17 2008-03-11 11:56:11Z xtimor $
 *
 */

#include "context.h"

#include <string.h>
#include <stdarg.h>
#include <stdio.h>

nmeaPROPERTY * nmea_property()
{
  static nmeaPROPERTY prop =
  {
    0, 0, NMEA_DEF_PARSEBUFF
  };

  return &prop;
}

void nmea_trace( const char *str, ... )
{
  int size;
  va_list arg_list;
  char buff[NMEA_DEF_PARSEBUFF];
  nmeaTraceFunc func = nmea_property()->trace_func;

  if ( func )
  {
    va_start( arg_list, str );
    size = NMEA_POSIX( vsnprintf )( &buff[0], NMEA_DEF_PARSEBUFF - 1, str, arg_list );
    va_end( arg_list );

    if ( size > 0 )
      ( *func )( &buff[0], size );
  }
}

void nmea_trace_buff( const char *buff, int buff_size )
{
  nmeaTraceFunc func = nmea_property()->trace_func;
  if ( func && buff_size )
    ( *func )( buff, buff_size );
}

void nmea_error( const char *str, ... )
{
  int size;
  va_list arg_list;
  char buff[NMEA_DEF_PARSEBUFF];
  nmeaErrorFunc func = nmea_property()->error_func;

  if ( func )
  {
    va_start( arg_list, str );
    size = NMEA_POSIX( vsnprintf )( &buff[0], NMEA_DEF_PARSEBUFF - 1, str, arg_list );
    va_end( arg_list );

    if ( size > 0 )
      ( *func )( &buff[0], size );
  }
}
