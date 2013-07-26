/*  SM2ADDON
    Copyright (c) 1996-98 Steffen Siebert (siebert@logware.de)

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
*/
#ifndef SM_DEBUG_H
#define SM_DEBUG_H

#ifdef DEBUG_LOG_SM2
#include <time.h>
#include <stdio.h>
FILE *debug_fh;
time_t debug_time;
#define DEBUG_FNAME "d:\\sm2test\\debug.log"

#define DMSG(x) if ((debug_fh = fopen(DEBUG_FNAME,"a")) != NULL) {\
  time(&debug_time);\
  fprintf(debug_fh,"==== %s",ctime(&debug_time));\
  fflush(debug_fh);\
  fprintf(debug_fh,x);\
  fflush(debug_fh);\
  fclose(debug_fh);\
}

#define DMSG2(x,y) if ((debug_fh = fopen(DEBUG_FNAME,"a")) != NULL) {\
  time(&debug_time);\
  fprintf(debug_fh,"==== %s",ctime(&debug_time));\
  fflush(debug_fh);\
  fprintf(debug_fh,x,y);\
  fflush(debug_fh);\
  fclose(debug_fh);\
}

#define DMSG3(x,y,z) if ((debug_fh = fopen(DEBUG_FNAME,"a")) != NULL) {\
  time(&debug_time);\
  fprintf(debug_fh,"==== %s",ctime(&debug_time));\
  fflush(debug_fh);\
  fprintf(debug_fh,x,y,z);\
  fflush(debug_fh);\
  fclose(debug_fh);\
}

#define DMSGSTART(x) if ((debug_fh = fopen(DEBUG_FNAME,"a")) != NULL) {\
  time(&debug_time);\
  fprintf(debug_fh,"==== %s",ctime(&debug_time));\
  fflush(debug_fh);\
  fprintf(debug_fh,x);\
  fflush(debug_fh)

#define DMSGLINE(x) fprintf(debug_fh,x);\
  fflush(debug_fh)

#define DMSGEND(x) fprintf(debug_fh,x);\
  fflush(debug_fh);\
  fclose(debug_fh);\
}

#define DMSGSTART2(x,y) if ((debug_fh = fopen(DEBUG_FNAME,"a")) != NULL) {\
  time(&debug_time);\
  fprintf(debug_fh,"==== %s",ctime(&debug_time));\
  fflush(debug_fh);\
  fprintf(debug_fh,x,y);\
  fflush(debug_fh)

#define DMSGLINE2(x,y) fprintf(debug_fh,x,y);\
  fflush(debug_fh)

#define DMSGEND2(x,y) fprintf(debug_fh,x,y);\
  fflush(debug_fh);\
  fclose(debug_fh);\
}

#else /* DEBUG_LOG_SM2 */

#define DMSG(x)
#define DMSG2(x,y)
#define DMSG3(x,y,z)
#define DMSGSTART(x)
#define DMSGLINE(x)
#define DMSGEND(x)
#define DMSGSTART2(x,y)
#define DMSGLINE2(x,y)
#define DMSGEND2(x,y)

#endif /* DEBUG_LOG_SM2 */

#endif /* SM_DEBUG_H */
