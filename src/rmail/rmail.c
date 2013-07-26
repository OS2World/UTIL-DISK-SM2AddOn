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

/* 	$Id: rmail.c,v 1.6 1998/04/04 19:55:03 siebert Exp $	 */

#ifndef lint
static char vcid[] = "$Id: rmail.c,v 1.6 1998/04/04 19:55:03 siebert Exp $";
#endif /* lint */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "rmail.h"

int getmailfirst(char *rmail, mailidx *mi, mail *m) {
  char indexname[_MAX_PATH];
  int linelen;

  strcpy(indexname,rmail);
  linelen = strlen(indexname);
  if (indexname[linelen-1] == '\\') {
    indexname[linelen-1] = '\0';
  }
  strcat(indexname,RMAIL_IDXFILE_EXT);
  if ((mi->fh = fopen(indexname,"rb")) == NULL) {
    return 1;
  }
  return getmailnext(*mi,m);
}

int getmailnext(mailidx mi, mail *m) {
  if (fread(&(m->header),sizeof(mailheader),1,mi.fh) != 1) {
    return 18; /* == ERROR_NO_MORE_FILES */
  }
  if ((m->msgid = (char *)malloc(m->header.msgidlen+1)) == NULL) {
    return 1;
  }
  if ((m->subject = (char *)malloc(m->header.subjectlen+1)) == NULL) {
    free(m->msgid);
    return 1;
  }
  if (fread(m->msgid,sizeof(char),m->header.msgidlen,mi.fh) != m->header.msgidlen) {
    free(m->msgid);
    free(m->subject);
    return 1;
  }
  m->msgid[m->header.msgidlen] = '\0';
  if (fread(m->subject,sizeof(char),m->header.subjectlen,mi.fh) != m->header.subjectlen) {
    free(m->msgid);
    free(m->subject);
    return 1;
  }
  m->subject[m->header.subjectlen] = '\0';
  return 0;
}

int getmailclose(mailidx mi) {
  if (mi.fh != NULL) {
    return fclose(mi.fh);
  } else {
    return 0;
  }
}

int findmail(char *rmail, char *msgid, mail *m) {
  FILE* fh;
  char indexname[_MAX_PATH];
  int ret;
  
  strcpy(indexname,rmail);
  strcat(indexname,RMAIL_IDXFILE_EXT);
  if ((fh = fopen(indexname,"rb")) == NULL) {
    return 1;
  }

  ret = _findmail(fh,msgid,m);

  fclose(fh);

  return ret;
}

int _findmail(FILE *fh, char *msgid, mail *m) {
  char *buffer, *subject;
  int bufsize = 80;
  int subjectsize = 80;
  mailheader mh;
  int found=0;
  int searchlen=strlen(msgid);
  
  buffer = (char *)malloc(bufsize);
  subject = (char *)malloc(subjectsize);

  while ((!found) && !feof(fh) && !ferror(fh)) {
    if (fread(&mh,sizeof(mh),1,fh) != 1) {
      break;
    }
    if (mh.msgidlen != searchlen) {
      /* Skip Mail */
      if (fseek(fh,mh.msgidlen+mh.subjectlen,SEEK_CUR)) {
	break;
      }
    } else {
      if (mh.msgidlen+1 > bufsize) {
	buffer = (char *)realloc(buffer,mh.msgidlen+1);
	bufsize = mh.msgidlen+1;
      }
      if (fread(buffer,sizeof(char),mh.msgidlen,fh) != mh.msgidlen) {
	break;
      }
      buffer[mh.msgidlen] = '\0';
      if (strcmp(buffer,msgid) == 0) {
	/* Mail found */
	if (mh.subjectlen+1 > subjectsize) {
	  subject = (char *)realloc(subject,mh.subjectlen+1);
	  subjectsize = mh.subjectlen+1;
	}
	if (fread(subject,sizeof(char),mh.subjectlen,fh) != mh.subjectlen) {
	  break;
	}
	subject[mh.subjectlen] = '\0';
	found = 1;
      } else {
	/* Skip rest of Mail */
	if (fseek(fh,mh.subjectlen,SEEK_CUR)) {
	  break;
	}
      }
    }
  }

  if (!found) {
    free(buffer);
    free(subject);
    return 1;
  } else {
    m->header = mh;
    m->msgid = buffer;
    m->subject = subject;
    return 0;
  }
}

int readmail(const char *rmail, mail m, char *mbuffer) {
  FILE* fh;

  if ((fh = fopen(rmail,"rb")) == NULL) {
    return 1;
  }
  if (fseek(fh,m.header.mailstart,SEEK_SET) != 0) {
    fclose(fh);
    return 1;
  }
  if ((fseek(fh,m.header.mailstart,SEEK_SET) != 0) ||
      (fread(mbuffer,m.header.maillen,1,fh) != 1)) {
    fclose(fh);
    return 1;
  }
  mbuffer[m.header.maillen] = '\0';
  fclose(fh);
  return 0;
}

int _readmail(FILE* fh, mail m, char *mbuffer) {
  if (fseek(fh,m.header.mailstart,SEEK_SET) != 0) {
    return 1;
  }
  if ((fseek(fh,m.header.mailstart,SEEK_SET) != 0) ||
      (fread(mbuffer,m.header.maillen,1,fh) != 1)) {
    return 1;
  }
  mbuffer[m.header.maillen] = '\0';
  return 0;
}

#ifdef TEST_LIBRARY
#if 0
void main(int argc, char *argv[]) {
  mailidx mi;
  mail m, m2;
  char *mbuf;
  char *rmailname;

  if (argc != 2) {
    printf("Usage: %s <rmail-Filename>\n",argv[0]);
    exit(1);
  }

  if ((rmailname = (char *)malloc(strlen(argv[1])+1)) == NULL) {
    printf("Malloc Fehler!\n");
    exit(1);
  }
  strcpy(rmailname,argv[1]);

  if (getmailfirst(rmailname,&mi,&m) != 0) {
    printf("Mail-Index not found!\n");
    getmailclose(mi);
    exit (1);
  } else {
    printf("<%s>\n",m.msgid);
  }
  while (getmailnext(mi,&m2) == 0) {
    printf("%s\n",m2.msgid);
    free(m2.subject);
    free(m2.msgid);
  }
  getmailclose(mi);

  if (findmail(rmailname,m.msgid,&m2) != 0) {
    printf("Mail not found!\n");
    exit (1);
  }

  free(m.subject);
  free(m.msgid);

  mbuf = (char *)malloc(m.header.maillen);
  if (readmail(rmailname,m2,mbuf) != 0) {
    printf("Kann Mail nicht lesen!\n");
  } else {
    printf("<%s>\n",mbuf);
    free(mbuf);
  }
}
#endif
#if 1
void main(int argc, char *argv[]) {
  mail m;
  char *mbuf;

  if (argc != 3) {
    printf("Usage: %s <rmail-Filename> <MSG-Id>\n",argv[0]);
    exit(1);
  }

  if (findmail(argv[1],argv[2],&m) != 0) {
    printf("Mail not found!\n");
    exit (1);
  }
  mbuf = (char *)malloc(m.header.maillen);
  if (readmail(argv[1],m,mbuf) != 0) {
    printf("Kann Mail nicht lesen!\n");
  } else {
    printf("<%s>\n",mbuf);
    free(mbuf);
  }
}
#endif
#endif
