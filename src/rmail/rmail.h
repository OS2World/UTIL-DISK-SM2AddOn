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

#ifndef _RMAIL_H_
#define _RMAIL_H_

/* 	$Id: rmail.h,v 1.5 1998/04/04 19:55:14 siebert Exp $	 */

#ifndef lint
static char vcid_rmail_h[] = "$Id: rmail.h,v 1.5 1998/04/04 19:55:14 siebert Exp $";
#endif /* lint */

typedef struct {
  short msgidlen;
  char language;
  short subjectlen;
  char date[8];
  char indexdate[8];
  long mailstart;
  long maillen;
} mailheader;

typedef struct {
  mailheader header;
  char *msgid;
  char *subject;
} mail;

typedef struct {
  FILE* fh;
} mailidx;

#define RMAIL_IDXFILE_EXT ".rdx"
#define RMAIL_NEWIDXFILE_EXT ".rdx.new"
#define RMAIL_EXT ".rmail"
#define RMAIL_EXT_LEN 6
#define RMAIL_EXTPATH ".rmail\\"
#define RMAIL_EXTPATH_LEN 7
#define RMAIL_RDX_EXT ".rmail.rdx"
#define RMAIL_RDX_EXT_LEN 10

int getmailfirst(char *rmail, mailidx *mi, mail *m);
int getmailnext(mailidx mi, mail *m);
int getmailclose(mailidx mi);
int findmail(char *rmail, char *msgid, mail *m);
int _findmail(FILE *fh, char *msgid, mail *m);
int readmail(const char *rmail, mail m, char *buffer);
int _readmail(FILE *fh, mail m, char *buffer);
int parse_date(char *datein, char *dateout);
int rmail_index(char *rmailfname, char *date);

#endif /* _RMAIL_H_ */
