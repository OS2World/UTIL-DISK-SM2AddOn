/**********************************************************************/
/*                                                                    */
/*  SM2ADDON                                                          */
/*  ========                                                          */
/*                                                                    */
/*  COPYRIGHT:                                                        */
/*  ----------                                                        */
/*   Copyright (c) 1996-98 Steffen Siebert (siebert@logware.de)       */
/*                                                                    */  
/*   This program is free software; you can redistribute it and/or    */
/*   modify it under the terms of the GNU General Public License as   */
/*   published by the Free Software Foundation; either version 2 of   */
/*   the License, or (at your option) any later version.              */
/*                                                                    */
/*   This program is distributed in the hope that it will be useful,  */
/*   but WITHOUT ANY WARRANTY; without even the implied warranty of   */
/*   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the    */
/*   GNU General Public License for more details.                     */
/*                                                                    */  
/*   You should have received a copy of the GNU General Public License*/
/*   along with this program; if not, write to the Free Software      */
/*   Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.        */
/*                                                                    */
/**********************************************************************/
/*                                                                    */
/* This file is based on and contains code from                       */
/*                                                                    */
/* b2m - a filter for Babyl -> Unix mail files                        */
/*                                                                    */
/* Written by                                                         */
/*   Ed Wilkinson                                                     */
/*   E.Wilkinson@massey.ac.nz                                         */
/*   Mon Nov 7 15:54:06 PDT 1988                                      */
/*                                                                    */
/**********************************************************************/

/* 	$Id: rmailidx.c,v 1.13 1998/04/04 19:54:50 siebert Exp $	 */

#ifndef lint
static char vcid[] = "$Id: rmailidx.c,v 1.13 1998/04/04 19:54:50 siebert Exp $";
#endif /* lint */

#include <stdio.h>
#include <stdlib.h>
#include <io.h>
#include <string.h>
#include <time.h>
#include <sys/types.h>
#include <sys/utime.h>
#include <sys/stat.h>

#include "rmail.h"

#include "../sm_debug.h"

#ifdef USE_REGEXP
/* #include "rxposix.h" */
#include "regex.h"
#endif

#define INCL_DOSDATETIME
#include <os2.h>

#define streq(s,t)	(stricmp (s, t) == 0)
#define strneq(s,t,n)	(strnicmp (s, t, n) == 0)

/*
 * A `struct linebuffer' is a structure which holds a line of text.
 * `readline' reads a line from a stream into a linebuffer and works
 * regardless of the length of the line.
 */
struct linebuffer
{
  long size;
  char *buffer;
};

long readline (struct linebuffer *linebuffer, register FILE *stream);
int parse_date(char *datein, char *dateout);

int rmail_index(char *rmailfn, char *idate) {
  int printing, subject, date, msgid, mailsearched;
  char *p;
  struct linebuffer data;
  char rmailfname[_MAX_PATH];
  char indexname[_MAX_PATH];
  char newindexname[_MAX_PATH];
  char fname[_MAX_PATH];
  char msgidstr[_MAX_PATH];
  char datetmp[_MAX_PATH];
  char subjectstr[_MAX_PATH];
  FILE *fhin, *fhout, *fhoin;
  mailheader mh;
  struct utimbuf ut;
  struct stat rmailstat, rmailidxstat;
  long mend;
  long oldIndexPos;
  mail oldMail;
  int linelen;
  int useOldIndex, detectLanguage;
#ifdef USE_REGEXP
  int count_de, count_engl;

  regex_t expr_de, expr_engl;

  if (regcomp(&expr_de,"\\<\\(der\\|das\\|die\\|es\\|ist\\)\\>",REG_ICASE|REG_NOSUB|REG_NEWLINE) != 0) {
    return 1;
  }
  if (regcomp(&expr_engl,"\\<\\(the\\|if\\|it\\|is\\)\\>",REG_ICASE|REG_NOSUB|REG_NEWLINE) != 0) {
    return 1;
  }
#endif

  strcpy(rmailfname,rmailfn);
  linelen = strlen(rmailfname);
  if (rmailfname[linelen-1] == '\\') {
    rmailfname[linelen-1] = '\0';
  }
  strcpy(indexname,rmailfname);
  strcat(indexname,RMAIL_IDXFILE_EXT);

  strcpy(newindexname,rmailfname);
  strcat(newindexname,RMAIL_NEWIDXFILE_EXT);

  if (stat(rmailfname,&rmailstat) != 0) {
    return 1;
  }

  if (stat(indexname,&rmailidxstat) == 0) {
    if (rmailidxstat.st_mtime >= rmailstat.st_mtime) {
      /* Index is up-to-date, nothing to do */
      DMSG("Index Up-To-Date, exiting\n");
      return 0;
    }
    useOldIndex = 1;
  } else {
    useOldIndex = 0;
  }
  if (stat(newindexname,&rmailidxstat) == 0) {
    /* Temporaeres Indexfile loeschen */
    DMSG2("Deleting %s\n",newindexname);
    unlink(newindexname);
  }
  
  DMSG2("usOldIndex = %i\n",useOldIndex);

  if ((fhin = fopen(rmailfname,"rb")) == NULL) {
    return 1;
  }

  if ((fhout = fopen(newindexname,"wb")) == NULL) {
    fclose(fhin);
    return 1;
  }

  if (useOldIndex == 1) {
    if ((fhoin = fopen(indexname,"rb")) == NULL) {
      DMSG2("Can't open %s\n",indexname);
      useOldIndex = 0;
    } else {
      oldIndexPos = ftell(fhoin);
    }
  }

  printing = subject = msgid = date = mailsearched = 0;
  data.size = 200;
  if ((data.buffer = (char *)malloc(200)) == NULL) {
    fclose(fhin);
    fclose(fhout);
    if (useOldIndex) {
      fclose(fhoin);
    }
    unlink(newindexname);
    return 1;
  }

  if (readline (&data, fhin) <= 0
      || !strneq (data.buffer, "BABYL OPTIONS:", 14)) {
    /* Input is not a Babyl mailfile */
    fclose(fhin);
    fclose(fhout);
    if (useOldIndex) {
      fclose(fhoin);
    }
    unlink(newindexname);
    return 1;
  }

  while ((linelen = readline (&data, fhin)) > 0) {
    if (!subject && (strneq (data.buffer, "Subject:",8))) {
      p = &data.buffer[8];
      while (*p==' ') {
	p++;
      }
      strncpy(subjectstr,p,_MAX_PATH);
      subjectstr[_MAX_PATH-1] = '\0';
      subject = 1;
    }
    if (!msgid && (strneq (data.buffer, "Message-Id:",11))) {
      p = &data.buffer[11];
      while ((*p==' ') || (*p=='<')) {
	p++;
      }
      strncpy(msgidstr,p,_MAX_PATH);
      if (msgidstr[strlen(msgidstr)-1] == '>') {
	msgidstr[strlen(msgidstr)-1] = '\0';
      }
      msgidstr[_MAX_PATH-1] = '\0';
      msgid = 1;
    }
    if (!date && (strneq (data.buffer, "Date:",5))) {
      p = &data.buffer[5];
      while (*p==' ') {
	p++;
      }
      strncpy(datetmp,p,_MAX_PATH);
      datetmp[_MAX_PATH-1] = '\0';
      if (parse_date(datetmp,mh.date) == 0) {
	date = 1;
      } else {
	printf("Date <%s> not parsable!\n",datetmp);
      }
    }
    
    if (streq (data.buffer, "*** EOOH ***") && !printing) {
#ifdef USE_REGEXP
      count_engl = count_de = 0;
#endif
      printing = 1;
      mh.mailstart = ftell(fhin);
      continue;
    }
    
    /* Recognize header start */
    if (data.buffer[0] == '\037') {
      if (data.buffer[1] == '\0') {
	continue;
      } else if (data.buffer[1] == '\f') {
	/* Save Information about previous mail */
	if (msgid) {
	  if (!date) {
	    /* Fehler - Kein Datum gefunden */
	    /* Mail ignorieren */
	    printing = msgid = subject = date = mailsearched = 0;
	    continue;
	  }
	  if (!subject) {
	    strcpy(subjectstr,"Kein Subject!");
	  }
	  mend = ftell(fhin) - (linelen+2);
	  mh.maillen = mend - mh.mailstart;
	  mh.msgidlen = strlen(msgidstr);
	  mh.subjectlen = strlen(subjectstr);
	  memcpy(mh.indexdate,idate,8);
#ifdef USE_REGEXP
          if (detectLanguage == 1) {
            if ((count_de > 0) && (count_engl > 0)) {
              if (count_de > count_engl) {
                mh.language = 'd';
              } else {
                mh.language = 'e';
              }
            } else if ((count_de == 0) && (count_engl == 0)) {
              mh.language = '?'; /* FIXME */
            } else if (count_de > 0) {
              mh.language = 'd';
            } else {
              mh.language = 'e';
            }
          } else {
	    memcpy(mh.indexdate,oldMail.header.indexdate,8);
	    mh.language = oldMail.header.language;
            if (mh.maillen != oldMail.header.maillen) {
              printf("mail <%s>:\n",msgidstr);
              printf("Error! Maillen differs\n");
            }
            if (memcmp(mh.date,oldMail.header.date,8) != 0) {
              printf("mail <%s>:\n",msgidstr);
              printf("Error! Date differs\n");
            }
            if (strcmp(subjectstr,oldMail.subject) != 0) {
              printf("mail <%s>:\n",msgidstr);
              printf("Error! Subject differs\n");
            }
            free(oldMail.subject);
            free(oldMail.msgid);
          }
#else
	  mh.language = '?';
#endif
	  fwrite(&mh,sizeof(mh),1,fhout);
	  fputs(msgidstr,fhout);
	  fputs(subjectstr,fhout);
	}
	/* Skip labels. */
	if ((linelen = readline (&data, fhin)) < 0) {
	  fclose(fhin);
	  fclose(fhout);
          if (useOldIndex) {
            fclose(fhoin);
          }
	  unlink(newindexname);
	  return 1;
	}
	printing = msgid = subject = date = mailsearched = 0;
	continue;
      }
    }

#ifdef USE_REGEXP
    if (printing) {
      if (!mailsearched && msgid) {
	mailsearched = 1;
	if (useOldIndex) {
	  if(_findmail(fhoin,msgidstr,&oldMail) == 0) {
	    detectLanguage = 0;
	    oldIndexPos = ftell(fhoin);
	  } else {
	    printf("mail <%s> not found!\n",msgidstr);
	    fseek(fhoin,oldIndexPos,SEEK_SET);
	    detectLanguage = 1;
	  }
	} else {
	  detectLanguage = 1;
	}
      }

      if (msgid && printing && (detectLanguage == 1)) {
/* 	printf("%s\n",data.buffer); */
	if (regexec(&expr_de,data.buffer,0,NULL,0) == 0) {
	  count_de++;
	}
	if (regexec(&expr_engl,data.buffer,0,NULL,0) == 0) {
	  count_engl++;
	}
      }
    }
#endif
  }

  if (linelen < 0) {
    fclose(fhin);
    fclose(fhout);
    if (useOldIndex) {
      fclose(fhoin);
    }
    unlink(newindexname);
    return 1;
  }
  mend = ftell(fhin) - (linelen+2);
  mh.maillen = mend - mh.mailstart;
  mh.msgidlen = strlen(msgidstr);
  mh.subjectlen = strlen(subjectstr);
  memcpy(mh.indexdate,idate,8);
#ifdef USE_REGEXP
  if (detectLanguage == 1) {
    if ((count_de > 0) && (count_engl > 0)) {
      if (count_de > count_engl) {
        mh.language = 'd';
      } else {
        mh.language = 'e';
      }
    } else if ((count_de == 0) && (count_engl == 0)) {
      mh.language = '?'; /* FIXME */
    } else if (count_de > 0) {
      mh.language = 'd';
    } else {
      mh.language = 'e';
    }
  } else {
    mh.language = oldMail.header.language;
    if (mh.maillen != oldMail.header.maillen) {
      printf("Error! Maillen differs\n");
    }
    if (memcmp(mh.date,oldMail.header.date,8) != 0) {
      printf("Error! Date differs\n");
    }
    if (strcmp(subjectstr,oldMail.subject) != 0) {
      printf("Error! Subject differs\n");
    }
  }
#else
  mh.language = '?';
#endif
  fwrite(&mh,sizeof(mh),1,fhout);
  fputs(msgidstr,fhout);
  fputs(subjectstr,fhout);
  fclose(fhin);
  fclose(fhout);
  if (useOldIndex) {
    DMSG2("Closing and deleting %s\n",indexname);
    fclose(fhoin);
    unlink(indexname);
  }
  DMSG3("Renaming %s to %s\n",newindexname,indexname);
  rename(newindexname,indexname);
  ut.actime = rmailstat.st_atime;
  ut.modtime = rmailstat.st_mtime;
  utime(indexname,&ut);
  return 0;
}

int parse_date(char *datein, char *dateout) {
  char *cp, *tp;
  int sl, mo, i;
  short year_i;
  char month[4];
  char date[3];
  char year[5];
  char time[9];
  char *months[13] = {"","Jan","Feb","Mar","Apr","May","Jun","Jul","Aug","Sep",
		     "Oct","Nov","Dec"};

  cp = strchr(datein,','); /* Skip Day of Week */
  if (cp == NULL) {
    cp = datein;
  } else {
    cp++;
  }
  while (*cp == ' ') { /* Skip Blanks */
    cp++;
  }
  cp = strtok(cp," ");
  if ((*cp < '0') || (*cp > '9')) {
    /* Month. Must be 3 chars long */
    if (strlen(cp) != 3) {
      return 1;
    }
    strcpy(month,cp);
  } else {
    /* Day. Must be one or two chars long */
    if (((sl = strlen(cp)) > 2) || (sl < 1)){
      return 1;
    }
    strcpy(date,cp);
  }

  cp = strtok(NULL," ");

  if ((*cp < '0') || (*cp > '9')) {
    /* Month. Must be 3 chars long */
    if (strlen(cp) != 3) {
      return 1;
    }
    strcpy(month,cp);
  } else {
    /* Day. Must be one or two chars long */
    if (((sl = strlen(cp)) > 2) || (sl < 1)){
      return 1;
    }
    strcpy(date,cp);
  }

  dateout[3] = (char) atoi(date);
  mo = 0;
  for (i=1; i<=12; i++) {
    if (strcmp(months[i],month) == 0) {
      mo = i;
      break;
    }
  }
  if (mo == 0) {
    return 1;
  } 
  dateout[2] = (char) mo;
  cp = strtok(NULL," ");
  if ((*cp < '0') || (*cp > '9')) {
    /* Skip Timezone Identifier */
    cp = strtok(NULL," ");
  }    

  if (strchr(cp,':') != NULL) { /* Time */
    if (strlen(cp) > 8) {
      return 1;
    }
    strcpy(time,cp);
  } else { /* Year */
    if (((sl = strlen(cp)) != 4) && (sl != 2)) {
      return 1;
    }
    strcpy(year,cp);
  }

  cp = strtok(NULL," ");
  if ((*cp < '0') || (*cp > '9')) {
    /* Skip Timezone Identifier */
    cp = strtok(NULL," ");
  }    

  if (strchr(cp,':') != NULL) { /* Time */
    if (strlen(cp) > 8) {
      return 1;
    }
    strcpy(time,cp);
  } else { /* Year */
    if (((sl = strlen(cp)) != 4) && (sl != 2)) {
      return 1;
    }
    strcpy(year,cp);
  }

  year_i = atoi(year);
  if (sl == 2) {
    if (year_i > 60) {
      year_i += 1900;
    } else {
      year_i += 2000;
    }
  }
  dateout[0] = (char) (year_i >> 8);
  dateout[1] = (char) (year_i & 0x00ff);

  tp = strtok(time,":"); /* Get Hours */
  if ((tp == NULL) || ((sl = strlen(tp)) < 1) || (sl > 2)) {
    return 1;
  }
  dateout[4] = (char)atoi(tp);
  tp = strtok(NULL,": "); /* Get Minutes */
  if ((tp == NULL) || ((sl = strlen(tp)) < 1) || (sl > 2)) {
    return 1;
  }
  dateout[5] = (char)atoi(tp);
  tp = strtok(NULL," "); /* Get Seconds */
  if ((tp != NULL) && (((sl = strlen(tp)) < 1) || (sl > 2))) {
    return 1;
  }
  if (tp == NULL) {
    dateout[6] = 0;
  } else {
    dateout[6] = (char)atoi(tp);
  }
  dateout[7] = 0;
  return 0;
}

/*
 * Read a line of text from `stream' into `linebuffer'.
 * Return the number of characters read from `stream',
 * which is the length of the line including the newline, if any.
 */
long readline (struct linebuffer *linebuffer, register FILE *stream) {
  char *buffer = linebuffer->buffer;
  register char *p = linebuffer->buffer;
  register char *pend;
  int chars_deleted;
  int poff;

  pend = p + linebuffer->size;	/* Separate to avoid 386/IX compiler bug.  */

  while (1)
    {
      register int c = getc (stream);
      if (p == pend)
	{
	  linebuffer->size *= 2;
	  poff = (p-buffer);
	  if ((buffer = (char *)realloc(buffer, linebuffer->size)) == NULL) {
	    return -1;
	  }
	  p = buffer + poff;
	  pend = buffer + linebuffer->size;
	  linebuffer->buffer = buffer;
	}
      if (c == EOF)
	{
	  chars_deleted = 0;
	  break;
	}
      if (c == '\n')
	{
	  if (p > linebuffer->buffer && p[-1] == '\r' && p > buffer)
	    {
	      *--p = '\0';
	      chars_deleted = 2;
	    }
	  else
	    {
	      *p = '\0';
	      chars_deleted = 1;
	    }
	  break;
	}
      *p++ = c;
    }
  
  return (p - buffer + chars_deleted);
}
#ifdef TEST_LIBRARY
int main (int argc, char **argv) {
  DATETIME dt;
  char date[8];
  if (argc != 2) {
    printf("Usage: %s <rmail-Filename>\n",argv[0]);
    exit(1);
  }
   DosGetDateTime(&dt);

  date[0] = (UCHAR)(dt.year>>8);
  date[1] = (UCHAR)(dt.year&0x00ff);
  date[2] = dt.month;
  date[3] = dt.day;
  date[4] = dt.hours;
  date[5] = dt.minutes;
  date[6] = dt.seconds;
  date[7] = dt.hundredths;
  return rmail_index(argv[1],date);
}
#endif
