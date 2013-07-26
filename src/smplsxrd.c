/***********************************************************************
*                      MODULE PROLOG                                   *
************************************************************************
*                                                                      *
* Module:       SMPLSXRD                                               *
*                                                                      *
* Title:        Library Services for OS/2 Database Manager Sample      *
*               (common methods used by both client and server)        *
*                                                                      *
* Entry points: LIB_init                                               *
*               LIB_end                                                *
*               LIB_access_doc                                         *
*               LIB_read_doc_content                                   *
*               LIB_close_doc                                          *
*               LIB_list_doc_groups                                    *
*               LIB_list_documents                                     *
*                                                                      *
***********************************************************************/
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
/*  COPYRIGHT:                                                        */
/*  ----------                                                        */
/*   Copyright (C) International Business Machines Corp., 1993        */
/*                                                                    */
/*  DISCLAIMER OF WARRANTIES:                                         */
/*  -------------------------                                         */
/*   The following [enclosed] code is sample code created by IBM      */
/*   Corporation. This sample code is not part of any standard IBM    */
/*   product and is provided to you solely for the purpose of         */
/*   assisting you in the development of your applications.           */
/*   The code is provided "AS IS", without warranty of any kind.      */
/*   IBM shall not be liable for any damages arising out of your      */
/*   use of the sample code, even if they have been advised of the    */
/*   possibility of such damages.                                     */
/*                                                                    */
/**********************************************************************/

/* 	$Id: smplsxrd.c,v 1.24 1998/05/06 14:46:52 siebert Exp $	 */

#ifndef lint
static char vcid[] = "$Id: smplsxrd.c,v 1.24 1998/05/06 14:46:52 siebert Exp $";
static char vcname[] = "$Name: release_06_05_1998_v7 $";
#endif /* lint */

/*--------------------------------------------------------------------*/
/* Includes                                                           */
/*--------------------------------------------------------------------*/
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>

#define INCL_DOS
#define INCL_DOSFILEMGR
#define INCL_ERRORS
#define INCL_DOSDATETIME
#include <os2.h>

#include "EHWLSDEF.H"               /* Library Services definitions   */
#include "EHWLSPRO.H"               /* Library Services profiles      */
#include "EHWLANG.H"                /* Language definitions           */
#include "SMPLSGRD.H"               /* Sample DBM definitions         */
                                    /* Library Services datastream    */
#include "SMPLSDSH.H"               /*    handling                    */

#include "zlib.h"
#include "rmail/rmail.h"

#include "sm_debug.h"

/* Functions for decompressing gzipped files
/* ===========================================================================
 * Uncompress input to output then close both files.
 */
#define BUFLEN      16384

int gz_uncompress(gzFile in, FILE *out) {
  char buf[BUFLEN];
  int len;
  int err;
  int fehler = 0;

  for (;;) {
    len = gzread(in, &buf, sizeof(buf));
    if (len < 0) {
      fehler = 1;
      break;
    }
    if (len == 0) {
      break;
    }
    if ((int)fwrite(buf, 1, (unsigned)len, out) != len) {
      fehler = 1;
      break;
    }
  }
  if (fclose(out)) {
    fehler = 1;
  }
  
  if (gzclose(in) != Z_OK) {
    fehler = 1;
  }
  return fehler;
}


/* ===========================================================================
 * Uncompress the given file into the given outfile
 */
int file_uncompress(char *file, char *outfile) {
  FILE  *out;
  gzFile in;
  
  DMSG2("file = %s\n",file);
  in = gzopen(file, "rb");
  DMSG2("gzopen() = %i\n",in);
  if (in == NULL) {
    return 1;
  }
  DMSG2("outfile = %s\n",outfile);
  out = fopen(outfile, "wb");
  DMSG2("fopen() = %i\n",out);
  if (out == NULL) {
    gzclose(in);
    return 1;
  }
  DMSG("calling gz_uncompress\n");
  return gz_uncompress(in, out);

}

/*--------------------------------------------------------------------*/
/* Structures                                                         */
/*--------------------------------------------------------------------*/
                                    /* Document handle structure      */
                                    /*   (allocated by LIB_access_doc */
                                    /*   and passed to                */
                                    /*   LIB_read_doc_content and     */
struct DocHandle {                  /*   LIB_close_doc)               */
   UCHAR DocHandleEyecatcher[8];    /*   eyecatcher "LSDOCHDL"        */
   USHORT usTextBufOffset;          /*   current offset in buffer     */
   USHORT usTextLength;             /*   current text length          */
   UCHAR StartReadDocument;         /*   indicator whether first read */
   FILE *fh;
   UCHAR type;                      /* r = rmail, g = gzip,           */
   UCHAR tempFileName[L_tmpnam];
   };

                                    /* Document group list handle     */
                                    /*   structure                    */
                                    /*   (used by LIB_list_doc_groups */
struct DocGroupListHandle {         /*   in case of continuation)     */
   UCHAR DGLHandleEyecatcher[8];    /*   eyecatcher "LSDGLHDL"        */
   ULONG ulWorkBufferOffset;        /*   current offset in workbuffer */
   PCHAR pWorkBuffer;               /*   pointer to workbuffer        */
   HDIR  GroupHandle;
   };

                                    /* Document list handle structure */
                                    /*   (used by LIB_list_documents  */
struct DocListHandle {              /*   in case of continuation)     */
   UCHAR DLHandleEyecatcher[8];     /*   eyecatcher "LSDOLHDL"        */
   ULONG ulWorkBufferOffset;        /*   current offset in workbuffer */
   PCHAR pWorkBuffer;               /*   pointer to workbuffer        */
   HDIR  GroupHandle;               /*   Handle for DosFindNext()     */
   mailidx MailGroupHandle;         /*   Handle for getmailnext()     */
   UCHAR GroupPath[_MAX_PATH];
   short GroupIsRmail;              /*   == 1 fuer RMAIL File         */
   };


/*--------------------------------------------------------------------*/
/* Database Manager declarations and hostvariables                    */
/*--------------------------------------------------------------------*/

/*    EXEC SQL INCLUDE SQLCA;          /* define and declare SQLCA area  */

/*    EXEC SQL BEGIN DECLARE SECTION; */
/*                                     /* document group                 */
   static APIRET os2_rc;

   static FILEFINDBUF3 hostvar_pDocGroup;
                                    /* document                       */
   static FILEFINDBUF3 hostvar_pDocument;
   static short hostvar_usSeqNo;           /* document sequence number      */
   static short hostvar_CCSID;             /* CCSID                         */
   static short CCSID_Ind;                 /* CCSID NULL-indicator          */
   static short hostvar_Language;          /* language                      */
   static short Language_Ind;              /* language NULL-indicator       */
   static short hostvar_Format;            /* format                        */
   static short Format_Ind;                /* format NULL-indicator         */
   static short Default_CCSID_Ind;         /* CCSID NULL-indicator          */
   static short Default_Language_Ind;      /* language NULL-indicator       */
   static short Default_Format_Ind;        /* format NULL-indicator         */
/*                                     /* date and time last modified    */
/*    unsigned char hostvar_pDateLMod[26]; */
   static struct Text {                    /* text                           */
      short Len;
      long Size;
      unsigned char *Data;
      } hostvar_pText;

   static char sm2_html_filter[_MAX_PATH];
   static char sm2_rc_filename[_MAX_PATH];
   static char sm2_ignore_ext[_MAX_PATH];

/* end of Database Manager declarations ------------------------------*/


/***********************************************************************
*                                                                      *
* Entry point:  LIB_init                                               *
*                                                                      *
* Function:     Establishes Library Services session.                  *
*                                                                      *
* Input parms:  1.Index name  (not used)                               *
*                                                                      *
* Output parms: 1.Pointer to Library Services anchor                   *
*               2.Reserved length  (set to zero)                       *
*               3.Reserved area    (buffer provided by caller)         *
*                   (not used)                                         *
*               4.Diagnosis information (buffer provided by caller;    *
*                   is used by caller only for return code not RC_OK)  *
*                                                                      *
* Return codes: Normal end         RC_OK                               *
*               Termination error  RC_TERMINATION_ERROR                *
*                                                                      *
* Input files:  -                                                      *
*                                                                      *
* Output files: -                                                      *
*                                                                      *
* Calls:        OS/2 Database Manager functions                        *
*                                                                      *
* Exits:        Always returns to caller.                              *
*                                                                      *
* -------------------------------------------------------------------- *
*                                                                      *
* Program logic:                                                       *
*                                                                      *
* - Allocate structure needed to anchor all required information       *
*   within the session, and point to that structure with the           *
*   Library Services anchor.                                           *
*                                                                      *
* - Start Database Manager.                                            *
*         sqlestar()                                                   *
*   In case of errors, terminate (RC_TERMINATION_ERROR)                *
*   If already started, continue.                                      *
*                                                                      *
* - Connect application to database in shared access (database name is *
*   a compile variable).                                               *
*   DB2/2:                                                             *
*         EXEC SQL CONNECT TO IBMSM2                                   *
*   ES/2 1.0 DBM (set to comment):                                     *
*         sqlestrd(database name,SQL_USE_SHR,sqlca)                    *
*     If restart required, restart database.                           *
*         sqlerest(database name,sqlca)                                *
*       In case of errors, terminate (RC_TERMINATION_ERROR)            *
*       Otherwise again connect application to database.               *
*         sqlestrd(database name,SQL_USE_SHR,sqlca)                    *
*       In case of errors, terminate (RC_TERMINATION_ERROR)            *
*                                                                      *
* - Return to caller (RC_OK).                                          *
*                                                                      *
***********************************************************************/

ULONG LSCALL
LIB_init                            /* Initialize Library Services ---*/
 (
 USHORT usLenSessInfo,
 PCHAR pIndexName,                  /* In:  Index name (length=8)     */
                                    /*      (not used within sample)  */
 PPVOID ppAnchor,                   /* Out: pointer that can be used  */
                                    /*      as anchor                 */
 PUSHORT pusReserved,               /* -- Reserved for future use --- */
 PCHAR pReserved,                   /* -- Reserved for future use --- */
 PCHAR pDiagInfo                    /* I/O: buffer to receive         */
                                    /*      diagnosis information     */
 )

{
   ULONG rc = RC_OK;                /* Return code                    */
   struct LSData *pLSData;          /* LS Data area pointer           */
   CHAR pDatabase[9];               /* Database name                  */
   char *p;
   DATETIME dt;

   pusReserved;                     /* these pointers are reserved    */
   pReserved;                       /*    for future use              */
   pIndexName;                      /* this pointer is not used       */
                                    /* initialize Diagnosis Info area */
                                    /*    and SQLCODE                 */

   DMSG("Betrete LIB_Init!\n");
   memset(pDiagInfo,' ',LEN_DIAGNOSIS_INFO);

/*--------------------------------------------------------------------*/
/* Allocate LS Data, address it via anchor and initialize it.         */
/* Allocate all areas addressed by LS Data.                 .         */
/*--------------------------------------------------------------------*/
                                    /* allocate LS Data area          */
   pLSData = (struct LSData *)malloc(sizeof(struct LSData));
   if (!pLSData)
   {
      sprintf(pDiagInfo,"LSD ALLOC ERR");
      return RC_TERMINATION_ERROR;
   }
   *ppAnchor = (void *)pLSData;
                                    /* initialize LS Data area        */
   memcpy(pLSData->LSDataEyecatcher,"LSDATA  ",LSLenEyeCatcher);

   pLSData->fh = NULL;

   DosGetDateTime(&dt);

   pLSData->date[0] = (UCHAR)(dt.year>>8);
   pLSData->date[1] = (UCHAR)(dt.year&0x00ff);
   pLSData->date[2] = dt.month;
   pLSData->date[3] = dt.day;
   /* Da die aktuelle Uhrzeit geringfuegig spaeter sein kann,
      als die Zeit, die SM/2 als Indexierungsstart annimmt,
      setzen wir die Sekunden auf 0 und gehen eine Minute
      zurueck, sofern wir dadurch nicht bei einem anderen
      Tag landen. */
   if (dt.minutes > 0) {
     pLSData->date[4] = dt.hours;
     pLSData->date[5] = dt.minutes-1;
   } else {
     if (dt.hours > 0) {
       pLSData->date[4] = dt.hours-1;
       pLSData->date[5] = 59;
     } else {
       pLSData->date[4] = dt.hours;
       pLSData->date[5] = 0;
     }
   }
   pLSData->date[6] = 0;
   pLSData->date[7] = 0;



   /* Query Environment Settings */

   p = getenv("SM2_HTML_FILTER");
   if (p != NULL) {
     DMSG2("SM2_HTML_FILTER = %s\n",p);
     strncpy(sm2_html_filter,p,_MAX_PATH);
     sm2_html_filter[_MAX_PATH-1] = '\0';
   } else {
     DMSG("SM2_HTML_FILTER not set!\n");
     sm2_html_filter[0] = '\0';
   }
   p = getenv("SM2_RC_FILENAME");
   if (p != NULL) {
     DMSG2("SM2_RC_FILENAME = %s\n",p);
     strncpy(sm2_rc_filename,p,_MAX_PATH);
     sm2_rc_filename[_MAX_PATH-1] = '\0';
   } else {
     DMSG("SM2_RC_FILENAME not set!\n");
     sm2_rc_filename[0] = '\0';
   }
   p = getenv("SM2_IGNORE_EXT");
   if (p != NULL) {
     DMSG2("SM2_IGNORE_EXT = %s\n",p);
     strncpy(sm2_ignore_ext,p,_MAX_PATH);
     sm2_ignore_ext[_MAX_PATH-1] = '\0';
   } else {
     DMSG("SM2_IGNORE_EXT not set!\n");
     sm2_ignore_ext[0] = '\0';
   }

/*--------------------------------------------------------------------*/
/* Start Database Manager: if it is already started, continue         */
/*--------------------------------------------------------------------*/


/*--------------------------------------------------------------------*/
/* End of LIB_init: normal return to caller                           */
/*--------------------------------------------------------------------*/

   return rc;


/*--------------------------------------------------------------------*/
/* Free all allocated areas pointed to by the LS anchor               */

   terminate:

   if (*ppAnchor != NULL)           /* free LS Data area and reset    */
      free(*ppAnchor);              /*    anchor itself               */

   return RC_TERMINATION_ERROR;

}


/***********************************************************************
*                                                                      *
* Entry point:  LIB_end                                                *
*                                                                      *
* Function:     Ends Library Services session.                         *
*                                                                      *
* Input parms:  1.Pointer to Library Services anchor                   *
*                                                                      *
* Output parms: 1.Diagnosis information (buffer provided by caller;    *
*                   is used by caller only for return code not RC_OK)  *
*                                                                      *
* Return codes: Normal end         RC_OK                               *
*               Termination error  RC_TERMINATION_ERROR                *
*                                                                      *
* Input files:  -                                                      *
*                                                                      *
* Output files: -                                                      *
*                                                                      *
* Calls:        OS/2 Database Manager functions                        *
*                                                                      *
* Exits:        Always returns to caller.                              *
*                                                                      *
* -------------------------------------------------------------------- *
*                                                                      *
* Program logic:                                                       *
*                                                                      *
* - Free all areas still allocated.                                    *
*                                                                      *
* - Return to caller with RC_OK.                                       *
*                                                                      *
***********************************************************************/

ULONG LSCALL
LIB_end                             /* End Library Services ----------*/
 (
 PVOID pAnchor,                     /* In:  anchor                    */
 PCHAR pDiagInfo                    /* I/O: buffer to receive         */
                                    /*      diagnosis information     */
 )

{
   ULONG rc = RC_OK;                /* Return code                    */
   struct LSData *pLSData =         /* LS Data area pointer           */
      (void *)pAnchor;

   DMSG("Betrete LIB_End!\n");

/*--------------------------------------------------------------------*/
/* Free all allocated areas pointed to by the LS anchor               */
/*--------------------------------------------------------------------*/

   end:

   if (pAnchor != NULL) {           /* free LS Data area and reset    */
     if (((struct LSData *)pAnchor)->fh != NULL) {
       fclose(((struct LSData *)pAnchor)->fh);
     }
     free(pAnchor);                 /*    anchor itself               */
   }
   return rc;
}


/***********************************************************************
*                                                                      *
* Entry point:  LIB_access_doc                                         *
*                                                                      *
* Function:     Accesses the document for reading.                     *
*                                                                      *
*               Checks whether the document exists, and returns        *
*               the document information CCSID, language and           *
*               document format.                                       *
*                                                                      *
* Input parms:  1.Pointer to Library Services anchor                   *
*               2.Length of document identifier                        *
*               3.Pointer to document identifier                       *
*                                                                      *
* Output parms: 1.Actual length of document information                *
*               2.Document information (datastream format; buffer      *
*                    provided by caller)                               *
*               3.Document handle                                      *
*               4.Diagnosis information (buffer provided by caller;    *
*                   is used by caller only for return code not RC_OK)  *
*                                                                      *
* Return codes: Normal end         RC_OK                               *
*               Not found          RC_DOCUMENT_NOT_FOUND               *
*               Document error     RC_DOCUMENT_IN_ERROR                *
*               Termination error  RC_TERMINATION_ERROR                *
*                                                                      *
* Input files:  Table containing the documents                         *
*                                                                      *
* Output files: -                                                      *
*                                                                      *
* Calls:        WriteDataStream                                        *
*               OS/2 Database Manager functions                        *
*                                                                      *
* Exits:        Always returns to caller.                              *
*                                                                      *
* -------------------------------------------------------------------- *
*                                                                      *
* Program logic:                                                       *
*                                                                      *
* - Read document identifier and anchor it.                            *
*                                                                      *
* - Get document format, CCSID, and language:                          *
*         SELECT CCSID, LANGUAGE, FORMAT                               *
*           INTO :hostvar_CCSID :CCSID_Ind,                            *
*                :hostvar_Language :Language_Ind,                      *
*                :hostvar_Format :Format_Ind                           *
*           FROM IBMSM2.DOCTABLE                                       *
*          WHERE DOCUMENT = :hostvar_pDocument AND SEQNO = 1           *
*   If document is not found, return with RC_DOCUMENT_NOT_FOUND.       *
*   In case of any errors related to the document return with          *
*   RC_DOCUMENT_IN_ERROR.                                              *
*                                                                      *
* - For any Null-value of CCSID, LANGUAGE or FORMAT (shown by the      *
*   indicator variables :CCSID_Ind etc.) use the default value         *
*   (compile constant).                                                *
*                                                                      *
* - Prepare output datastream.                                         *
*                                                                      *
* - Set text offset to zero, and anchor it.  Allocate buffer to        *
*   hold text of a row in maximal length and anchor pointer.           *
*   Set text length to zero (text buffer structure is text length      *
*   followed by text).                                                 *
*   Set current document sequence number to zero and anchor it.        *
*   Use the document handle to point to this information.              *
*                                                                      *
* - Return with RC_OK.                                                 *
*                                                                      *
***********************************************************************/

ULONG LSCALL
LIB_access_doc                      /* Access Document ---------------*/
 (
 PVOID pAnchor,                     /* In:  anchor (not used here)    */
 USHORT usLenDocId,                 /* In:  length of document iden-  */
                                    /*      tifier                    */
 PCHAR pDocIdIn,                    /* In:  document id               */
 PUSHORT pusLenDocInfo,             /* Out: actual length of document */
                                    /*      related information       */
 PCHAR pDocInfo,                    /* I/O: buffer to receive doc.    */
                                    /*      related information       */
 PPVOID ppDocHandle,                /* Out: document handle           */
 PCHAR pDiagInfo                    /* I/O: buffer to receive         */
                                    /*      diagnosis information     */
 )

{
   ULONG rc = RC_OK;                /* Return code                    */
   struct LSData *pLSData =         /* LSData area pointer            */
      (void *)pAnchor;
   struct DocHandle *pDocHdl = NULL;/* Document handle pointer        */
   CHAR *pDataStream = NULL;        /* Pointer to datastream          */
   char pDocId[_MAX_PATH];
   char buffer[_MAX_PATH];
   char buffer2[_MAX_PATH];
   char filename[_MAX_PATH];
   char zeile[512], *trenner, *p;
   FILE *ifh, *fh;
   int name_len;
   char command[_MAX_PATH+_MAX_PATH+_MAX_PATH+_MAX_PATH];
   char oldTempFileName[L_tmpnam];
   mail m;

   DMSG("Betrete LIB_Access_Doc!\n");
   DMSG2("pDocIdIn <%s>\n",pDocIdIn);
   DMSG2("usLenDocId = %i\n",usLenDocId);
   strncpy(pDocId,pDocIdIn,usLenDocId);
   pDocId[usLenDocId] = '\0';
   DMSG2("pDocId = <%s>\n",pDocId);
                                    /* initialize Diagnosis Info area */
   memset(pDiagInfo,' ',LEN_DIAGNOSIS_INFO);
                                    /* initialize host variable       */
/*    memset(hostvar_pDocument,0x00,LSLenDocColumn); */

                                    /* allocate document handle area  */
   pDocHdl = (struct DocHandle *)malloc(sizeof(struct DocHandle));
   if (!pDocHdl)
   {
      sprintf(pDiagInfo,"DOCHD ALLOC ERR");
      return RC_TERMINATION_ERROR;
   }
                                    /* initialize document handle     */
   memcpy(pDocHdl->DocHandleEyecatcher,"LSDOCHDL",LSLenEyeCatcher);
   pDocHdl->usTextBufOffset= 0;
   pDocHdl->usTextLength= 0;
   pDocHdl->StartReadDocument = 'Y';
   pDocHdl->tempFileName[0] = '\0';
   pDocHdl->type = ' ';

/*--------------------------------------------------------------------*/
/* Get document information by reading the document table             */
/*--------------------------------------------------------------------*/
                                    /* get document identifier        */

   Format_Ind = -1;
   CCSID_Ind = -1;
   Language_Ind = -1;
   
   Default_Format_Ind = -1;
   Default_CCSID_Ind = -1;
   Default_Language_Ind = -1;

   if (strstr(pDocId,RMAIL_EXTPATH) != NULL) {
     /* Rmail File */
     memcpy(buffer,pDocId,usLenDocId);
     buffer[usLenDocId] = '\0';
     strcpy(buffer2,buffer);
     trenner = strrchr(buffer2,'\\');
     if (trenner == NULL) {
       /* error */
       sprintf(pDiagInfo,"Kein Trenner");
       return RC_TERMINATION_ERROR;
     }
     *trenner++ = '\0';
     /* Buffer2 enth„lt Filename vom RMAIL-File,
	trenner die MSGID */
     if (findmail(buffer2,trenner,&m) != 0) {
       return RC_DOCUMENT_NOT_FOUND;
     }
     if (hostvar_pText.Size == 0) {
       hostvar_pText.Size = m.header.maillen+1;
       if ((hostvar_pText.Data = (char *)malloc(hostvar_pText.Size)) == NULL) {
	 sprintf(pDiagInfo,"Malloc Failure");
	 free(m.msgid);
	 free(m.subject);
	 return RC_TERMINATION_ERROR;
       }
     } else if (hostvar_pText.Size < m.header.maillen+1) {
       hostvar_pText.Size = m.header.maillen+1;
       if ((hostvar_pText.Data = (char *)realloc(hostvar_pText.Data,hostvar_pText.Size)) == NULL) {
	 sprintf(pDiagInfo,"Realloc Failure");
	 free(m.msgid);
	 free(m.subject);
	 return RC_TERMINATION_ERROR;
       }
     }
     if ((fh = ((struct LSData *)pAnchor)->fh) != NULL) {
       if (strcmp(buffer2,((struct LSData *)pAnchor)->fname) != 0) {
	 /* Altes File schliessen, neues File oeffnen */
	 fclose(fh);
	 fh = fopen(buffer2,"rb");
	 ((struct LSData *)pAnchor)->fh = fh;
	 strcpy(((struct LSData *)pAnchor)->fname,buffer2);
       }
     } else {
       fh = fopen(buffer2,"rb");
       ((struct LSData *)pAnchor)->fh = fh;
       strcpy(((struct LSData *)pAnchor)->fname,buffer2);
     }
	 
     if (_readmail(fh,m,hostvar_pText.Data) != 0) {
       sprintf(pDiagInfo,"readmail err");
       free(m.msgid);
       free(m.subject);
       return RC_DOCUMENT_IN_ERROR;
     }
     hostvar_pText.Len = m.header.maillen;
     free(m.msgid);
     free(m.subject);
     pDocHdl->type = 'r';
     pDocHdl->usTextLength = hostvar_pText.Len;
     pDocHdl->usTextBufOffset = 0;
     Format_Ind = TDS; /* ascii */
     CCSID_Ind = CCSID_00819; /* ISO Latin 1 = CP819 */
     if (m.header.language == 'd') {
       Language_Ind = LANG_DEU;
     } else if (m.header.language == 'e') {
       Language_Ind = LANG_ENU;
     }
   } else {
     DMSG2("sm2_rc_filename = <%s>\n",sm2_rc_filename);
     if (strcmp(sm2_rc_filename,"") != 0) {
       memcpy(buffer,pDocId,usLenDocId);
       buffer[usLenDocId] = '\0';
       strcpy(buffer2,buffer);
       trenner = strrchr(buffer2,'\\');
       if (trenner == NULL) {
	 /* error */
	 sprintf(pDiagInfo,"Kein Trenner");
	 return RC_TERMINATION_ERROR;
       }
       *trenner = '\0';
       strcpy(filename,++trenner);
       strcat(buffer2,"\\");
       strcat(buffer2,sm2_rc_filename);
       DMSGSTART2("Filename = <%s>\n",filename);
       DMSGEND2("rc = <%s>\n",buffer2);
       if ((ifh = fopen(buffer2,"r")) != NULL) {
	 DMSG2("ifh = %i\n",ifh);
	 while (!feof(ifh)) {
	   fgets(zeile, sizeof(zeile), ifh);
	   if (strncmp(zeile,"* ",2) == 0) {
	     /* Setze default Werte */
	     p = strtok(&zeile[1]," \t\n");
	     if (p != NULL) {
	       if (strcmp(p,"d") == 0) {
		 Default_Language_Ind = LANG_DEU;
	       } else if (strcmp(p,"e") == 0) {
		 Default_Language_Ind = LANG_ENU;
	       } else {
		 DMSG2("Falsche Sprache bei Default = <%s>\n",p);
		 sprintf(pDiagInfo,"Falsche Sprache");
		 return RC_TERMINATION_ERROR;
	       }
	     }
	     p = strtok(NULL," \t\n");
	     if (p != NULL) {
	       if (strcmp(p,"ascii") == 0) {
		 Default_Format_Ind = TDS;
	       } else if (strcmp(p,"word") == 0) {
		 Default_Format_Ind = MSWORD;
	       } else if (strcmp(p,"swriter") == 0) {
		 Default_Format_Ind = STARWRITER;
	       } else {
		 DMSG2("Falsches Format bei Default = <%s>\n",p);
		 sprintf(pDiagInfo,"Falsche Sprache");
		 return RC_TERMINATION_ERROR;
	       }
	     }
	   } else if (strnicmp(zeile,filename,strlen(filename)) == 0) {
	     /* File gefunden */
	     p = strtok(&zeile[strlen(filename)]," \t\n");
	     if (p != NULL) {
	       if (strcmp(p,"d") == 0) {
		 Language_Ind = LANG_DEU;
	       } else if (strcmp(p,"e") == 0) {
		 Language_Ind = LANG_ENU;
	       } else {
		 DMSG2("Falsche Sprache = <%s>\n",p);
		 sprintf(pDiagInfo,"Falsche Sprache");
		 return RC_TERMINATION_ERROR;
	       }
	     }
	     p = strtok(NULL," \t\n");
	     if (p != NULL) {
	       if (stricmp(p,"ascii") == 0) {
		 Format_Ind = TDS;
	       } else if (stricmp(p,"word") == 0) {
		 Format_Ind = MSWORD;
	       } else if (stricmp(p,"swriter") == 0) {
		 Format_Ind = STARWRITER;
	       } else {
		 DMSG2("Falsches Format = <%s>\n",p);
		 sprintf(pDiagInfo,"Falsches Format");
		 return RC_TERMINATION_ERROR;
	       }
	     }
	     break;
	   }
	 } /* while */
	 fclose(ifh);
	 if (Language_Ind == -1) {
	   Language_Ind = Default_Language_Ind;
	 }
       } /* if */
     }
   }
   DMSG2("Language_Ind = %i\n",Language_Ind);
                                    /* read document table with docu- */
                                    /*    ment identifier as key      */
/*    EXEC SQL SELECT CCSID, LANGUAGE, FORMAT */
/*        INTO :hostvar_CCSID :CCSID_Ind, */
/*             :hostvar_Language :Language_Ind, */
/*             :hostvar_Format :Format_Ind */
/*        FROM IBMSM2.DOCTABLE */
/*       WHERE DOCUMENT = :hostvar_pDocument */
/*         AND SEQNO = 1; */

/*    if (SQLCODE != 0)                /* check for errors:              */
/*    { */
/*       if (SQLCODE == 100)           /* check whether document is      */
/*                                     /*   not found                    */
/*          return RC_DOCUMENT_NOT_FOUND; */
/*       else */
/*       { */
/*          sprintf(pDiagInfo,"SQLERR: %d",SQLCODE); */
/*          return RC_DOCUMENT_IN_ERROR; */
/*       } */
/*    } */

/*--------------------------------------------------------------------*/
/* Build datastream: take default if no value is specified            */
/*--------------------------------------------------------------------*/
   pDataStream = pDocInfo;
   *pusLenDocInfo = 0;
   *pusLenDocInfo = LSLenDocInfo;   /* get minimal length required    */
                                    /* (compile constant)             */

/* Item: Document format                                              */
                                    /* if no document format is       */
   if (Format_Ind < 0) {            /*    specified, take default     */
     hostvar_Format = DefaultFormat;
   } else {
     hostvar_Format = Format_Ind;
   }     

   (void) WriteDataStream((ULONG *)pusLenDocInfo,
                   &pDataStream,
                   ID_DOCF,
                   IT_ATOMIC,
                   2,
                   (PCHAR)&hostvar_Format);

/* Item: Document CCSID                                               */
                                    /* if no document CCSID is        */
   if (CCSID_Ind < 0) {             /*    specified, take default     */
     hostvar_CCSID = DefaultCCSID;
   } else {
     hostvar_CCSID = CCSID_Ind;
   }     

   (void) WriteDataStream((ULONG *)pusLenDocInfo,
                   &pDataStream,
                   ID_CCSID,
                   IT_ATOMIC,
                   2,
                   (PCHAR)&hostvar_CCSID);

/* Item: Document language                                            */
                                    /* if no document language is     */
   if (Language_Ind < 0) {          /*    specified, take default     */
     hostvar_Language = DefaultLanguage;
   } else {
     hostvar_Language = Language_Ind;
   }     

   DMSGSTART2("Language_Ind = %i\n",hostvar_Language);
   DMSGEND2("Format_Ind = %i\n",hostvar_Format);

   (void) WriteDataStream((ULONG *)pusLenDocInfo,
                   &pDataStream,
                   ID_LANG,
                   IT_ATOMIC,
                   2,
                   (PCHAR)&hostvar_Language);
                                    /* set actual length of items     */
   *pusLenDocInfo = pDataStream - pDocInfo;


/*--------------------------------------------------------------------*/
/* Allocate and initialize document handle area                       */
/*--------------------------------------------------------------------*/

   if (pDocHdl->type == 'r') {
     *ppDocHandle = (void *)pDocHdl;
     return rc;
   }

   strncpy(buffer,pDocId,usLenDocId);
   buffer[usLenDocId] = '\0';

   name_len = strlen(buffer);
   DMSG2("extension: %s\n",&buffer[name_len-3]);
   if ((name_len > 3) && (stricmp(&buffer[name_len-3],".gz") == 0)) {
     /* gzipped file */
     DMSG("entzippe file\n");
     tmpnam(pDocHdl->tempFileName);
     DMSG2("tmpname = %s\n",pDocHdl->tempFileName);
     os2_rc = file_uncompress(buffer,pDocHdl->tempFileName);
     DMSG2("os2_rc: %i\n",os2_rc);
     if (os2_rc != 0) {
       sprintf(pDiagInfo,"file_uncompress");
       return RC_DOCUMENT_IN_ERROR;
     }
     pDocHdl->type = 'g'; /* GZIP-File */
   }

#define HTML_COMMAND "%s \"%s\" %s"

   if (strcmp(sm2_html_filter,"") != 0) { /* HTML Filter definiert? */
     DMSG("Teste auf HTML\n");
     /* HTML-Files */
     DMSG2("pDocHdl->type = %c\n",pDocHdl->type);
     DMSG2("buffer = %s\n",buffer);
     if (pDocHdl->type == 'g') {
       /* File war gezipped, daher das TMP_FILE nehmen und .gz von der Endung
	  abziehen */
       DMSG("gzipped HTML-File?\n");
       strcpy(oldTempFileName,pDocHdl->tempFileName);
       DMSG2("Test1: %s\n",&buffer[name_len-7]);
       DMSG2("Test2: %s\n",&buffer[name_len-8]);
       DMSG2("Test3: %s\n",&buffer[name_len-9]);
       if (((name_len > 7) && (strnicmp(&buffer[name_len-7],".htm",4) == 0)) || 
	   ((name_len > 8) && (strnicmp(&buffer[name_len-8],".html",5) == 0)) ||
	   ((name_len > 9) && (strnicmp(&buffer[name_len-9],".shtml",6) == 0))) {
	 sprintf(command,HTML_COMMAND,sm2_html_filter,oldTempFileName,tmpnam(pDocHdl->tempFileName));
	 DMSG2("HTML-Commandozeile: %s\n",command);
	 os2_rc = system(command);
	 unlink(oldTempFileName);
	 if (os2_rc != 0) {
	   sprintf(pDiagInfo,"unh fehler");
	   return RC_DOCUMENT_IN_ERROR;
	 }
	 pDocHdl->type = 'c'; /* HTML-File gezipped */
       }
     } else {
       DMSG2("Test1: %s\n",&buffer[name_len-4]);
       DMSG2("Test2: %s\n",&buffer[name_len-5]);
       DMSG2("Test3: %s\n",&buffer[name_len-6]);
       if (((name_len > 4) && (stricmp(&buffer[name_len-4],".htm") == 0)) ||
	   ((name_len > 5) && (stricmp(&buffer[name_len-5],".html") == 0)) ||
	   ((name_len > 6) && (stricmp(&buffer[name_len-6],".shtml") == 0))) {
	 sprintf(command,HTML_COMMAND,sm2_html_filter,buffer,tmpnam(pDocHdl->tempFileName));
	 DMSG2("HTML-Commandozeile: %s\n",command);
	 os2_rc = system(command);
	 if (os2_rc != 0) {
	   sprintf(pDiagInfo,"unh fehler");
	   return RC_DOCUMENT_IN_ERROR;
	 }
	 pDocHdl->type = 'h'; /* HTML-File */
       }
     }
   }
   if (pDocHdl->tempFileName[0] != '\0') {
     pDocHdl->fh = fopen(pDocHdl->tempFileName,"rb");
     DMSG3("fopen(%s)=%i\n",pDocHdl->tempFileName,pDocHdl->fh);
   } else {
     pDocHdl->fh = fopen(buffer,"rb");
     pDocHdl->type = 'f'; /* Normales File */
     DMSG3("fopen(%s)=%i\n",buffer,pDocHdl->fh);
   }
   if (pDocHdl->fh == NULL) {
     sprintf(pDiagInfo,"fopen failed");
     return RC_DOCUMENT_IN_ERROR;
   }
   *ppDocHandle = (void *)pDocHdl;
   return rc;
}


/***********************************************************************
*                                                                      *
* Entry point:  LIB_read_doc_content                                   *
*                                                                      *
* Function:     Reads a part of the document content.                  *
*                                                                      *
* Input parms:  1.Pointer to Library Services anchor (not used here)   *
*               2.Document handle                                      *
*               3.Number of bytes to be skipped before reading         *
*               4.Number of bytes to be read                           *
*                                                                      *
* Output parms: 1.Number of bytes actually read                        *
*               2.Document text (buffer provided by caller)            *
*               3.Diagnosis information (buffer provided by caller;    *
*                   is used by caller only for return code not RC_OK)  *
*                                                                      *
* Return codes: Normal end         RC_OK                               *
*               End of file        RC_END_OF_FILE                      *
*               Document error     RC_DOCUMENT_IN_ERROR                *
*               Termination error  RC_TERMINATION_ERROR                *
*                                                                      *
* Input files:  Table containing the documents                         *
*                                                                      *
* Output files: -                                                      *
*                                                                      *
* Calls:        OS/2 Database Manager functions                        *
*                                                                      *
* Exits:        Always returns to caller.                              *
*                                                                      *
* -------------------------------------------------------------------- *
*                                                                      *
* Program logic:                                                       *
*                                                                      *
* - If 'number of bytes to skip' and 'number of bytes to read' are     *
*   both zero, return with RC_TERMINATION_ERROR.                       *
*                                                                      *
* - If actual text length, i.e. that part of the text already received *
*   from DBM, but not passed to the caller, is zero, read next row     *
*   (i.e. with sequence number  = current sequence number + 1):        *
*   -     SELECT TEXT                                                  *
*           INTO :hostvar_pText                                        *
*           FROM IBMSM2.DOCTABLE                                       *
*          WHERE DOCUMENT = :hostvar_pDocument AND                     *
*                SEQNO = :hostvar_usSeqNo                              *
*   - In case of NOT FOUND, return with RC_END_OF_FILE.                *
*     In case of any errors related to the document, return            *
*     RC_DOCUMENT_IN_ERROR.                                            *
*                                                                      *
* - Do While (still number of bytes to be skipped)                     *
*   - Get length of text (hostvar_pText is a structure consisting of   *
*     the length and the not null-terminated text).                    *
*     - If 'number of bytes to skip' is less than the length of text,  *
*       increase text offset and decrease text length by this number.  *
*       Then leave the loop.                                           *
*     - Otherwise decrease 'number of bytes to skip' by the length of  *
*       text, set text offset to zero and read next row (sequence      *
*       number = current sequence number + 1):                         *
*   -     SELECT TEXT                                                  *
*           INTO :hostvar_pText                                        *
*           FROM IBMSM2.DOCTABLE                                       *
*          WHERE DOCUMENT = :hostvar_pDocument AND                     *
*                SEQNO = :hostvar_usSeqNo                              *
*       In case of NOT FOUND, return with RC_END_OF_FILE.              *
*       In case of any errors related to the document, return          *
*       RC_DOCUMENT_IN_ERROR.                                          *
*                                                                      *
* - If there are bytes to read:                                        *
*   - Set number of bytes actually read to zero, and decrease length   *
*     of text by text offset.                                          *
*   - Do while (length of text is greater than zero):                  *
*     - If 'number of bytes to read' is less than length of text, move *
*       the requested number of bytes to the document text buffer      *
*       starting at offset 'bytes actually read'. Increase 'bytes      *
*       actually read' and text offset by 'number of bytes to read'    *
*       and leave loop.                                                *
*     - If 'number of bytes to read' is greater than or equals the     *
*       length of text, move the whole text to the document text       *
*       buffer starting at offset 'bytes actually read'. Set text      *
*       offset to zero, decrease 'number of bytes to read' and         *
*       increase 'bytes actually read' by the length of the text.      *
*       Read next row (sequence number = current sequence number + 1): *
*   -     SELECT TEXT                                                  *
*           INTO :hostvar_pText                                        *
*           FROM IBMSM2.DOCTABLE                                       *
*          WHERE DOCUMENT = :hostvar_pDocument AND                     *
*                SEQNO = :hostvar_usSeqNo                              *
*     - In case of NOT FOUND, return with RC_END_OF_FILE.              *
*       In case of any errors related to the document, return          *
*       RC_DOCUMENT_IN_ERROR.                                          *
*                                                                      *
* - Anchor new document sequence number, the text offset and the       *
*   buffer containing the partly read text, and return with RC_OK.     *
*                                                                      *
***********************************************************************/

ULONG LSCALL
LIB_read_doc_content                /* Read document content ---------*/
 (
 PVOID pAnchor,                     /* In:  anchor                    */
 PVOID pDocHandle,                  /* In:  document handle           */
 USHORT usSkipBytes,                /* In:  number of bytes to skip   */
                                    /*      from current position     */
 USHORT usReadBytes,                /* In:  bytes to read then from   */
                                    /*      current position          */
 PUSHORT pusBytesRead,              /* Out: number of bytes actually  */
                                    /*      read                      */
 PCHAR pDocText,                    /* I/O: buffer to receive text    */
 PCHAR pDiagInfo                    /* I/O: buffer to receive         */
                                    /*      diagnosis information     */
 )

{
   ULONG rc = RC_OK;                /* Return code                    */
                                    /* Document Handle area pointer   */
   struct DocHandle *pDocHdl = (struct DocHandle *)pDocHandle;

   DMSGSTART("Betrete LIB_Read_Doc_Content!\n");
   DMSGLINE2("usReadBytes = %i\n",usReadBytes);
   DMSGLINE2("usSkipBytes = %i\n",usSkipBytes);
   DMSGLINE2("pAnchor = %i\n",pAnchor);
   DMSGLINE2("pDocHandle = %i\n",pDocHandle);
   DMSGLINE2("pDocHdl->fh = %i\n",pDocHdl->fh);
   DMSGLINE2("pDocHdl->usTextBufOffset = %i\n",pDocHdl->usTextBufOffset);
   DMSGEND2("pDocHdl->usTextLength = %i\n",pDocHdl->usTextLength);
                                    /* initialize DiagnosisInfo area  */
   memset(pDiagInfo,' ',LEN_DIAGNOSIS_INFO);
   pAnchor;

   *pusBytesRead = 0;               /* initialize parameter           */
                                    /* check whether first read of    */
                                    /*    document, if yes: reset     */
                                    /*    document sequence number    */
   if (pDocHdl->StartReadDocument == 'Y')
   {
      hostvar_usSeqNo = 0;
/*       pDocHdl->fh = fopen(pDocHdl->,"rt"); */
      /* TODO: Fehlerbehandlung fopen() */
      pDocHdl->StartReadDocument = 'N';
   }

   if (hostvar_pText.Size == 0) {
     hostvar_pText.Size = 3900;
     if ((hostvar_pText.Data = (char *)malloc(hostvar_pText.Size)) == NULL) {
       return RC_TERMINATION_ERROR;
     }
   }

/*--------------------------------------------------------------------*/
/* Check input parameters                                             */
/*--------------------------------------------------------------------*/
   if (usSkipBytes == 0 && usReadBytes == 0)
   {
        sprintf(pDiagInfo,"SKIP+READ ZERO");
        return RC_TERMINATION_ERROR;
   }

   if ((pDocHdl->type != 'r') && (pDocHdl->fh == NULL)) {
     sprintf(pDiagInfo,"FH == NULL");
     return RC_DOCUMENT_IN_ERROR;
   }

/*--------------------------------------------------------------------*/
/* Read next document row, if no text is pending                      */
/*--------------------------------------------------------------------*/
                                    /* No text pending: (length = 0)  */
   if (pDocHdl->usTextLength == 0)
   {                                /* Increase sequence number       */
     if (pDocHdl->type == 'r') {
       /* Rmail File wurde bereits komplett gelesen */
       DMSG("Versuche weitere Daten bei RMAIL zu lesen!\n");
       DMSG2("pusBytesRead = %i\n",*pusBytesRead);
       return RC_END_OF_FILE;
     }
      ++hostvar_usSeqNo;
                                    /*    and read next document row  */
      hostvar_pText.Len = fread(hostvar_pText.Data,sizeof(char),hostvar_pText.Size,pDocHdl->fh);
      DMSGSTART2("hostvar_pText.Len = %i\n",hostvar_pText.Len);
      DMSGLINE2("hostvar_pText.Size = %i\n",hostvar_pText.Size);
      DMSGLINE2("ferror = %i\n",ferror(pDocHdl->fh));
      DMSGEND2("feof = %i\n",feof(pDocHdl->fh));
/*       EXEC SQL SELECT TEXT */
/*           INTO :hostvar_pText */
/*           FROM IBMSM2.DOCTABLE */
/*          WHERE DOCUMENT = :hostvar_pDocument */
/*            AND SEQNO = :hostvar_usSeqNo; */


/*       if (feof(pDocHdl->fh)) { */
/*         return RC_END_OF_FILE; */
/*       } */
      if (ferror(pDocHdl->fh)) {
        return RC_DOCUMENT_IN_ERROR;
      }
                                    /* set current text length and    */
                                    /*    offset                      */
      pDocHdl->usTextLength = hostvar_pText.Len;
      pDocHdl->usTextBufOffset = 0;
   }


/*--------------------------------------------------------------------*/
/* Process skipping of text                                           */
/*--------------------------------------------------------------------*/
                                    /* Do while there are still bytes */
                                    /*    to be skipped               */
   while (usSkipBytes > 0)
   {                                /* More text available than to be */
                                    /*    skipped:                    */
                                    /*  - increase offset             */
                                    /*  - decrease remaining text     */
                                    /*    length                      */
                                    /*  - reset 'bytes to skip'       */
                                    /*  - leave loop                  */
      if (usSkipBytes < pDocHdl->usTextLength)
      {
         pDocHdl->usTextBufOffset += usSkipBytes;
         pDocHdl->usTextLength -= usSkipBytes;
         usSkipBytes = 0;
         break;
      }
      else                          /* More text to be skipped than   */
                                    /*    available:                  */
      {                             /*  - decrease bytes to skip      */
                                    /*  - increase sequence number    */
         usSkipBytes -= pDocHdl->usTextLength;
	if (pDocHdl->type == 'r') {
	  /* Rmail File wurde bereits komplett gelesen */
	  DMSG("Versuche weitere Daten bei RMAIL zu lesen!\n");
	  DMSG2("pusBytesRead = %i\n",*pusBytesRead);
	  return RC_END_OF_FILE;
	}
         ++hostvar_usSeqNo;
                                    /*  - and read next document row  */
         hostvar_pText.Len = fread(hostvar_pText.Data,sizeof(char),hostvar_pText.Size,pDocHdl->fh);
	 DMSG2("hostvar_pText.Len = %i\n",hostvar_pText.Len);
/*          EXEC SQL SELECT TEXT */
/*              INTO :hostvar_pText */
/*              FROM IBMSM2.DOCTABLE */
/*             WHERE DOCUMENT = :hostvar_pDocument */
/*               AND SEQNO = :hostvar_usSeqNo; */

/*          if (feof(pDocHdl->fh)) { */
/*            return RC_END_OF_FILE; */
/*          } */
         if (ferror(pDocHdl->fh)) {
           return RC_DOCUMENT_IN_ERROR;
         }

         pDocHdl->usTextLength = hostvar_pText.Len;
         pDocHdl->usTextBufOffset = 0;
      }

   } /* end of while loop (still bytes to skip) */


/*--------------------------------------------------------------------*/
/* Process reading of text                                            */
/*--------------------------------------------------------------------*/
                                    /* if there are bytes to be read  */
   if (usReadBytes > 0)
   {                                /* No text pending: (length = 0)  */
      if (pDocHdl->usTextLength == 0)
      {                             /* Increase sequence number       */
	if (pDocHdl->type == 'r') {
	  /* Rmail File wurde bereits komplett gelesen */
	  DMSG("Versuche weitere Daten bei RMAIL zu lesen!\n");
	  DMSG2("pusBytesRead = %i\n",*pusBytesRead);
	  return RC_END_OF_FILE;
	}
         ++hostvar_usSeqNo;
                                    /*    and read next document row  */
         hostvar_pText.Len = fread(hostvar_pText.Data,sizeof(char),hostvar_pText.Size,pDocHdl->fh);
	 DMSG2("hostvar_pText.Len = %i\n",hostvar_pText.Len);
/*          EXEC SQL SELECT TEXT */
/*              INTO :hostvar_pText */
/*              FROM IBMSM2.DOCTABLE */
/*             WHERE DOCUMENT = :hostvar_pDocument */
/*               AND SEQNO = :hostvar_usSeqNo; */

/*          if (feof(pDocHdl->fh)) { */
/*            return RC_END_OF_FILE; */
/*          } */
         if (ferror(pDocHdl->fh)) {
           return RC_DOCUMENT_IN_ERROR;
         }

                                    /* set current text length and    */
                                    /*    offset                      */
         pDocHdl->usTextLength = hostvar_pText.Len;
         pDocHdl->usTextBufOffset = 0;
      }
                                    /* do while there is text         */
                                    /*    available (i.e. to be read) */
      while (pDocHdl->usTextLength > 0)
      {
                                    /* more text available than to be */
                                    /*    read:                       */
         if (usReadBytes < pDocHdl->usTextLength)
         {                          /* move all text to be read and   */
            memcpy(pDocText + *pusBytesRead,
                   hostvar_pText.Data + pDocHdl->usTextBufOffset,
                   usReadBytes);
                                    /*    update lengths and offsets  */
            *pusBytesRead += usReadBytes;
            pDocHdl->usTextBufOffset += usReadBytes;
            pDocHdl->usTextLength -= usReadBytes;
                                    /* then leave loop                */
         break;
         }
                                    /* more text to be read than      */
                                    /*    available:                  */
         else
         {
            memcpy(pDocText + *pusBytesRead,
                   hostvar_pText.Data + pDocHdl->usTextBufOffset,
                   pDocHdl->usTextLength);
                                    /*    update lengths and offsets  */
            usReadBytes -= pDocHdl->usTextLength;
            *pusBytesRead += pDocHdl->usTextLength;
                                    /* increase sequence number       */
	   if (pDocHdl->type == 'r') {
	     /* Rmail File wurde bereits komplett gelesen */
	     DMSG("Versuche weitere Daten bei RMAIL zu lesen!\n");
	     DMSG2("pusBytesRead = %i\n",*pusBytesRead);
	     return RC_END_OF_FILE;
	   }
            ++hostvar_usSeqNo;
                                    /*  - and read next document row  */
            hostvar_pText.Len = fread(hostvar_pText.Data,sizeof(char),hostvar_pText.Size,pDocHdl->fh);
	    DMSG2("hostvar_pText.Len = %i\n",hostvar_pText.Len);
/*             EXEC SQL SELECT TEXT */
/*                 INTO :hostvar_pText */
/*                 FROM IBMSM2.DOCTABLE */
/*                WHERE DOCUMENT = :hostvar_pDocument */
/*                  AND SEQNO = :hostvar_usSeqNo; */

                                    /* set current text length and    */
                                    /*    offset                      */

/*             if (feof(pDocHdl->fh)) { */
/*               return RC_END_OF_FILE; */
/*             } */
            if (ferror(pDocHdl->fh)) {
              return RC_DOCUMENT_IN_ERROR;
            }

            pDocHdl->usTextLength = hostvar_pText.Len;
            pDocHdl->usTextBufOffset = 0;

         } /* endif */

      } /* end of while loop (still text available) */

   } /* endif */

   if ((rc == RC_OK) && (*pusBytesRead < usReadBytes)) {
     rc = RC_END_OF_FILE;
   }

#ifdef DEBUG_LOG_SM2
   if ((debug_fh = fopen(DEBUG_FNAME,"a")) != NULL) {
     time(&debug_time);
     fprintf(debug_fh,"==== %s",ctime(&debug_time));
     fflush(debug_fh);
     fprintf(debug_fh,"Verlasse LIB_Read_Doc_Content!\n");
     fflush(debug_fh);
     fprintf(debug_fh,"pDocText = <");
     fwrite(pDocText,1,*pusBytesRead,debug_fh);
     fprintf(debug_fh,">\n");
     fflush(debug_fh);
     if (rc == RC_END_OF_FILE) {
       fprintf(debug_fh,"RC_END_OF_FILE\n");
       fflush(debug_fh);
     }
     fclose(debug_fh);
   }
#endif /* DEBUG_LOG_SM2 */

   return rc;
}


/***********************************************************************
*                                                                      *
* Entry point:  LIB_close_doc                                          *
*                                                                      *
* Function:     Ends access to a document for reading.                 *
*                                                                      *
* Input parms:  1.Pointer to Library Services anchor (not used here)   *
*               2.Document handle                                      *
*                                                                      *
* Output parms: 1.Diagnosis information (not used here)                *
*                                                                      *
* Return codes: Normal end         RC_OK                               *
*                                                                      *
* Input files:  -                                                      *
*                                                                      *
* Output files: -                                                      *
*                                                                      *
* Exits:        Always returns to caller.                              *
*                                                                      *
* -------------------------------------------------------------------- *
*                                                                      *
* Program logic:                                                       *
*                                                                      *
* - Free all anchored document information pointed to by the document  *
*   handle and the handle itself.                                      *
*                                                                      *
* - Return RC_OK.                                                      *
*                                                                      *
***********************************************************************/

ULONG LSCALL
LIB_close_doc                       /* Close Document ----------------*/
 (
 PVOID pAnchor,                     /* In:  anchor                    */
 PVOID pDocHandle,                  /* In:  document handle to be     */
                                    /*      released by LS            */
 PCHAR pDiagInfo                    /* I/O: buffer to receive         */
                                    /*      diagnosis information     */
 )

{
   ULONG rc = RC_OK;                /* Return code                    */
                                    /* Document Handle area pointer   */
   struct DocHandle *pDocHdl = (struct DocHandle *)pDocHandle;
   char buffer[_MAX_PATH];

   pAnchor;                         /* these pointers are not used    */
   pDiagInfo;

   DMSGSTART("Betrete LIB_Close_Doc!\n");
   DMSGEND2("pDocHdl->fh = %i\n",pDocHdl->fh);
   
   if (pDocHdl->type != 'r') {
     fclose(pDocHdl->fh);
     if (pDocHdl->tempFileName[0] != '\0') {
       /* Tempor„res File l”schen */
       unlink(pDocHdl->tempFileName);
     }
   }

/*--------------------------------------------------------------------*/
/* Free all the Document Handle area                                  */
/*--------------------------------------------------------------------*/

   if (pDocHdl != NULL)             /* free area                      */
      free(pDocHdl);

   return rc;
}

/***********************************************************************
*                                                                      *
* Entry point:  LIB_list_doc_groups                                    *
*                                                                      *
* Function:     Lists for the entire library or for a certain document *
*               group all document groups of the first level.          *
*                                                                      *
* Input parms:  1.Pointer to Library Services anchor (not used here)   *
*               2.Length of library / document group identifier        *
*               3.Pointer to library / document group identifier       *
*               4.Length of buffer to receive document group list      *
*               5.Type of the request (LS_FIRST/LS_NEXT/LS_CANCEL)     *
*               6.Doc group list handle (set to zero for LS_FIRST)     *
*                                                                      *
* Output parms: 1.Actual length of document group list                 *
*               2.Document group list (buffer provided by caller)      *
*               3.Document group list handle (only for LS_FIRST and    *
*                    LS_NEXT if return code is RC_CONT)                *
*               4.Diagnosis information (buffer provided by caller;    *
*                   is used by caller only for return code not RC_OK)  *
*                                                                      *
* Return codes: Normal end         RC_OK                               *
*               Contin. requested  RC_CONTINUATION_MODE_ENTERED        *
*               Empty list         RC_EMPTY_LIST                       *
*               Group not found    RC_DOCUMENT_GROUP_NOT_FOUND         *
*               Termination error  RC_TERMINATION_ERROR                *
*                                                                      *
* Input files:  Table containing the documents                         *
*                                                                      *
* Output files: -                                                      *
*                                                                      *
* Calls:        WriteDataStream                                        *
*               OS/2 Database Manager functions                        *
*                                                                      *
* Exits:        Always returns to caller.                              *
*                                                                      *
* -------------------------------------------------------------------- *
*                                                                      *
* Program logic:                                                       *
*                                                                      *
* - If request type is FIRST:                                          *
*   - Set grouplist offset to zero, remaining buffer space to buffer   *
*     length                                                           *
*                                                                      *
*   - If LIBRARY identifier:                                           *
*     - Return all distinct entries of column DOCGROUP                 *
*       - Start                                                        *
*         DECLARE clib CURSOR FOR                                      *
*          SELECT DISTINCT DOCGROUP                                    *
*            FROM IMBSM2.DOCTABLE                                      *
*         OPEN clib                                                    *
*       - Do until there are no more docgroups or the buffer is full:  *
*         FETCH clib INTO :hostvar_pDocGroup                           *
*         If there are no more document groups, close cursor and       *
*           return with RC_OK.                                         *
*         Build up document group entry in the output buffer. Check    *
*         whether the entry fits in the output buffer.                 *
*         If yes: put it to the buffer, increase offset and decrease   *
*           remaining space of the output buffer by the length of the  *
*           entry. Continue.                                           *
*         If no: allocate work buffer and write datastream item to it. *
*           Allocate document group list handle, anchor the work       *
*           buffer and length of entry, and return with                *
*           RC_CONTINUATION_MODE_ENTERED.                              *
*                                                                      *
*   - If not LIBRARY identifier: return RC_EMPTY_LIST                  *
*                                                                      *
* - If request type is NEXT:                                           *
*   - Get entry and its length from the document group list handle     *
*     and write it to the output buffer.                               *
*   - Do until there are no more docgroups or the buffer is full:      *
*     Read next document group identifier:                             *
*         FETCH clib INTO :hostvar_pDocGroup                           *
*     If there are no more document groups, close cursor and           *
*       return with RC_OK.                                             *
*     Check whether the entry fits in the buffer. If yes, put it       *
*       to the buffer, increase offset, decrease remaining space       *
*       of the buffer by the length of the entry.                      *
*       If no, anchor entry and length of entry by the document group  *
*       list handle, and return output buffer with                     *
*       RC_CONTINUATION_MODE_ENTERED.                                  *
*     Continue.                                                        *
*                                                                      *
* - If request type is CANCEL:                                         *
*         CLOSE clib                                                   *
*   Free all allocated areas and the document group list handle        *
*     itself; return with RC_OK.                                       *
*                                                                      *
***********************************************************************/

int isPointDir(char *name) {
  DMSGSTART("Betrete isPointDir!\n");
  DMSGEND2("name = <%s>\n",name);
  if (strcmp(name,".") == 0) {
    return 1;
  }
  if (strcmp(name,"..") == 0) {
    return 1;
  }
  return 0;
}

ULONG LSCALL
LIB_list_doc_groups                 /* List Doc Group Identifiers ----*/
 (
 PVOID pAnchor,                     /* In:  anchor (not used)         */
 USHORT usLenDocGroupId,            /* In:  length of input (not used)*/
 PCHAR pDocGroupId,                 /* In:  library/document group    */
                                    /*      identifier                */
 ULONG ulLenOutBuffer,              /* In:  length of buffer to       */
                                    /*      receive doc group list    */
 CHAR ReqType,                      /* In:  type of the request       */
 PPVOID ppDocGroupListHandle,       /* I/O: document group list       */
                                    /*      handle                    */
 PULONG pulLenDocGroupList,         /* Out: actual length of document */
                                    /*      group list                */
 PCHAR pDocGroupList,               /* I/O: buffer to receive docum-  */
                                    /*      ent group list            */
 PCHAR pDiagInfo                    /* I/O: buffer to receive         */
                                    /*      diagnosis information     */
 )

{

   ULONG rc = RC_OK;                /* Return code                    */
                                    /* Document Group List Handle     */
   ULONG anzDocuments;
   HDIR  GroupHandle;
   struct DocGroupListHandle *pDGLHandle = NULL;
   PCHAR pDataStream = NULL,        /* work buffer pointers           */
          pDSItem = NULL;
   ULONG ulDSOffset = 0,            /* work and output buffer         */
          ulDSRemSpace = 0,         /*    lengths                     */
          ulDGLRemSpace = 0,
          ulDSInLength = 0;
   UCHAR FirstEntry ='Y';           /* indicate first docgroup id     */
   UCHAR NewEntry ='Y';           /* indicate first docgroup id     */
   USHORT usDGIDLength;
   char buffer[_MAX_PATH];
   char buffer2[_MAX_PATH];
   char buffer3[_MAX_PATH];
   BYTE               fsqBuffer[sizeof(FSQBUFFER2) + (3 * CCHMAXPATH)] = {0};
   ULONG              cbBuffer = sizeof(fsqBuffer);
   PFSQBUFFER2        pfsq2 = (PFSQBUFFER2) fsqBuffer;
   ULONG              ulLen, ulInx;
   CHAR               szDrive[3] = " :";
   CHAR               szDriveSL[4];
   int fnlen;
   int groupIsRmail;
   struct stat filestat;
                                    /* input parameters not used      */
   usLenDocGroupId;
   pAnchor;
                                    /* initialize DiagnosisInfo area  */
   DMSGSTART("Betrete LIB_List_Doc_Groups!\n");
   DMSGLINE2("ulLenOutBuffer = %i\n",ulLenOutBuffer);
   DMSGEND2("pDocGroupId = <%s>\n",pDocGroupId);

   memset(pDiagInfo,' ',LEN_DIAGNOSIS_INFO);
   *pulLenDocGroupList = 0;         /* initialize length of output    */
                                    /* switch dependent on request:   */

   switch (ReqType)
   {

/*--------------------------------------------------------------------*/
/* Request type = LS_FIRST                                            */
/*--------------------------------------------------------------------*/
   case (LS_FIRST):
                                    /* initialize hostvariable        */
/*       memset(hostvar_pDocGroup,0x00,LSLenDocGroupColumn); */
                                    /* initialize offset and remain-  */
                                    /*    ing space of output buffer  */
      ulDGLRemSpace = ulLenOutBuffer;
                                    /* initialize pointer to          */
      pDSItem = pDocGroupList;        /*    datastream item             */
                                    /* check whether request is for   */
                                    /*    entire library or specific  */
                                    /*    document group:             */
      if (!memcmp(pDocGroupId,LIBRARY,sizeof(LIBRARY)-1)) {

   /*-----------------------------------------------------------------*/
   /* Request for entire library:                                     */

	DMSG("Betrete LIB_List_Doc_Groups (ges. Lib)!\n");
	strcpy(szDrive," :");
         for (ulInx = 0; ulInx < 24; ulInx++)
         {                          /*  - read document group value   */
	   ulLen = cbBuffer;
	   szDrive[0] = (CHAR)('C' + ulInx);
	   os2_rc = DosQueryFSAttach(szDrive, 0L, FSAIL_QUERYNAME, pfsq2, &ulLen);
	   DMSG3("Query <%s> = %i",szDrive,os2_rc);
	   if (os2_rc == NO_ERROR) {  /*    check for errors:           */
	     strcpy(szDriveSL,szDrive);
	     strcat(szDriveSL,"\\");
	     if (WriteDataStream(&ulDGLRemSpace,
				 &pDSItem,
				 ID_DGID,
				 IT_ATOMIC,
				 (USHORT)strlen(szDriveSL),
				 szDriveSL) != RC_TRUE)
                                    /*    no more space in output     */
                                    /*    buffer: allocate buffer to  */
                                    /*    save datastream item        */
                                    /*    (length is compile constant)*/
                                    /*    check 'FirstEntry': in this */
               {                    /*         case terminate         */
		 if (FirstEntry == 'Y')
		   {
                     sprintf(pDiagInfo,"DGBUF TOO SMALL");
		     /*         close cursor (do not   */
		     /*         check the return code; */
		     /*         passing it would over- */
		     /*         write the current      */
		     /*         diagnosis information) */
                     return RC_TERMINATION_ERROR;
		   }
		 else              /*    allocate work buffer to     */
		   {                 /*    save datastream item        */
                                    /*    (length is compile constant)*/
                     pDataStream = (PCHAR)malloc(LSLenDocGroupItem);
                     if (!pDataStream)
		       {
			 sprintf(pDiagInfo,"LDG ALLOC ERROR");
			 return RC_TERMINATION_ERROR;
		       }
                                    /*    initialize lengths of work  */
                                    /*    buffer                      */
                     ulDSRemSpace = (ULONG)LSLenDocGroupItem;
                     pDSItem = pDataStream;
                                    /*    and write datastream item   */
                                    /*    to work buffer              */
                     (void) WriteDataStream(&ulDSRemSpace,
                                  &pDSItem,
                                  ID_DGID,
                                  IT_ATOMIC,
                                  (USHORT)strlen(szDriveSL),
                                  szDriveSL);
                     ulDSOffset =   /*    calculate workbuffer offset */
                        (ULONG)LSLenDocGroupItem - ulDSRemSpace;
                                    /*    allocate group list handle  */
                                    /*    area to hold the data       */
                     pDGLHandle = (struct DocGroupListHandle *)
                        malloc(sizeof(struct DocGroupListHandle));
                                    /*    if allocation failed,       */
                                    /*    terminate                   */
                     if (!pDGLHandle)
                     {
                        sprintf(pDiagInfo,"DGLH_ALLOC_ERROR");
                        return RC_TERMINATION_ERROR;
                     }
                                    /*         set DGL handle values  */
                     *ppDocGroupListHandle = (void *)pDGLHandle;
                     memcpy(pDGLHandle->DGLHandleEyecatcher,
                        "LSDGLHDL",LSLenEyeCatcher);
                     pDGLHandle->pWorkBuffer = pDataStream;
                     pDGLHandle->ulWorkBufferOffset = ulDSOffset;
		     pDGLHandle->GroupHandle = GroupHandle;
                                    /*         set 'continuation      */
                                    /*         requested' and return  */
                     return RC_CONTINUATION_MODE_ENTERED;
                  }
               } /* endif (no output buffer space available)          */

               else                 /*    datastream item written to  */
               {                    /*    output buffer: adjust       */
                                    /*    length                      */
                  *pulLenDocGroupList = ulLenOutBuffer - ulDGLRemSpace;
               } /* endif (output buffer space available)             */

                                    /*    reset 'FirstEntry'          */
               if (FirstEntry == 'Y') FirstEntry = 'N';

            } /* endif (fetch cursor)                                 */

         } /* endfor (as long as document group identifiers are avail-*/
                                    /* able and output buffer is not  */
                                    /* full)                          */
	 if (FirstEntry == 'Y') {
	   return RC_EMPTY_LIST;
	 }
       break;
      }
	else
   /*-----------------------------------------------------------------*/
   /* Request for single document group                               */
      {
	int blen;
	strncpy(buffer,pDocGroupId,usLenDocGroupId);
	buffer[usLenDocGroupId] = '\0';
	blen = strlen(buffer);
	strcpy(buffer2,buffer);
	
	DMSG2("blen=%i\n",blen);
	DMSG2("usLenDocGroupId=%i\n",usLenDocGroupId);
	DMSG("schaun'mer mal\n");
	if ((blen > RMAIL_EXTPATH_LEN) &&
	    stricmp(buffer+blen-RMAIL_EXTPATH_LEN,RMAIL_EXTPATH) == 0) {
	  /* Rmail Files enthalten keine weiteren Gruppen */
	  buffer[blen-1] = '\0';
	  DMSG2("stat(%s)\n",buffer);
	  if (stat(buffer,&filestat) == -1) {
	    sprintf(pDiagInfo,"stat = -1");
	    return RC_TERMINATION_ERROR;
	  }
	  DMSG2("filestat.st_mode&S_IFREG = %i\n",filestat.st_mode&S_IFREG);
	  if ((filestat.st_mode&S_IFREG) != 0) {
	    DMSG("RMAIL File erkannt -> enthaelt keine weiteren Gruppen\n");
	    return RC_EMPTY_LIST;
	  } else {
	    DMSG("File hat .rmail Extension, ist aber ein Verzeichnis\n");
	  }
	}
	strcpy(buffer,buffer2);
	strcat(buffer,"*");

	pDSItem = pDocGroupList;        /*    datastream item             */
                             /* prepare reading of DOCGROUP    */
                                    /*    values from document table: */
                                    /*  - declare cursor              */
/*          EXEC SQL DECLARE clib CURSOR FOR */
/*            SELECT DISTINCT DOCGROUP */
/*              FROM IBMSM2.DOCTABLE; */
                                    /*  - open cursor                 */
/*          EXEC SQL OPEN clib; */
                                    /*    do until no more document   */
                                    /*    group values are available  */
         for (;;)
         {                          /*  - read document group value   */
/*             EXEC SQL FETCH clib INTO :hostvar_pDocGroup; */
	   anzDocuments = 1;

	   if (NewEntry == 'Y') {
	     NewEntry = 'N';
	     GroupHandle = HDIR_CREATE;
	     DMSG2("GroupHandle = (%i)\n",GroupHandle);
	     os2_rc = DosFindFirst(buffer,&GroupHandle,FILE_NORMAL|FILE_DIRECTORY,&hostvar_pDocGroup,sizeof(FILEFINDBUF3),&anzDocuments,FIL_STANDARD);
	     if (os2_rc != NO_ERROR) {
	       GroupHandle=-1;
	     }
	   } else {
	     DMSG2("GroupHandle = (%i)\n",GroupHandle);
	     os2_rc = DosFindNext(GroupHandle,&hostvar_pDocGroup,sizeof(FILEFINDBUF3),&anzDocuments);
	   }
	   DMSG2("os2_rc = (%i)\n",os2_rc);
	   if (os2_rc != NO_ERROR) {  /*    check for errors:           */
	     if (os2_rc != ERROR_NO_MORE_FILES) {
	       sprintf(pDiagInfo,"DosFindFirst/Next %d",os2_rc);
	       return RC_TERMINATION_ERROR;
	     }
	     DMSGSTART2("Beende ListDocGroups; FirstEntry = (%c)\n",FirstEntry);
	     DMSGEND2("GroupHandle = (%i)\n",GroupHandle);
	     if (FirstEntry == 'Y') {
	       rc = RC_EMPTY_LIST;
	     }              /*    no more document group      */
                                    /*    values: close cursor and    */
                                    /*    return                      */
	     if (GroupHandle != -1) {
	       os2_rc = DosFindClose(GroupHandle);
	       if (os2_rc != NO_ERROR) {
		 sprintf(pDiagInfo,"DosFindClose = %i",os2_rc);
		 rc = RC_TERMINATION_ERROR;
	       }
	     }
	     return rc;
	   } /* NO_ERROR */
            else                    /*  - otherwise:                  */
	      {                       /*    build document group entry  */
                                    /*    in output buffer            */
		DMSG2("Teste %s\n",hostvar_pDocGroup.achName);
		if ((hostvar_pDocGroup.attrFile & FILE_DIRECTORY) == 0) {
		  /* Ist das File ein Rmail File? */
		  DMSG("Kein Verzeichnis, sondern File\n");
		  if (((fnlen = strlen(hostvar_pDocGroup.achName)) > RMAIL_EXT_LEN) &&
		      stricmp(hostvar_pDocGroup.achName+fnlen-RMAIL_EXT_LEN,RMAIL_EXT) == 0) {
		    DMSG("RMAIL File gefunden!\n");
		    groupIsRmail = 1;
		  } else {
		    DMSG("Ignoriere File!\n");
		    continue;
		  }
		} else {
		  DMSG("Ist ein Verzeichnis!\n");
		  groupIsRmail = 0;
		}						  
		DMSG2("WriteDataStream(%s)\n",hostvar_pDocGroup.achName);
		if (isPointDir(hostvar_pDocGroup.achName)) {
		  continue;
		} else {
		  strcpy(buffer3,buffer2);
		  strcat(buffer3,hostvar_pDocGroup.achName);
		  strcat(buffer3,"\\");
		  if (WriteDataStream(&ulDGLRemSpace,
				      &pDSItem,
				      ID_DGID,
				      IT_ATOMIC,
				      (USHORT)strlen(buffer3),
				      buffer3) != RC_TRUE)
                                    /*    no more space in output     */
                                    /*    buffer: allocate buffer to  */
                                    /*    save datastream item        */
                                    /*    (length is compile constant)*/
                                    /*    check 'FirstEntry': in this */
		    {                    /*         case terminate         */
		      if (FirstEntry == 'Y')
			{
			  sprintf(pDiagInfo,"DGBUF TOO SMALL");
                                    /*         close cursor (do not   */
                                    /*         check the return code; */
                                    /*         passing it would over- */
                                    /*         write the current      */
                                    /*         diagnosis information) */

			  os2_rc = DosFindClose(GroupHandle);
			  return RC_TERMINATION_ERROR;
			}
		      else              /*    allocate work buffer to     */
			{                 /*    save datastream item        */
			  /*    (length is compile constant)*/
			  pDataStream = (PCHAR)malloc(LSLenDocGroupItem);
			  if (!pDataStream)
			    {
			      sprintf(pDiagInfo,"LDG ALLOC ERROR");
			      return RC_TERMINATION_ERROR;
			    }
                                    /*    initialize lengths of work  */
                                    /*    buffer                      */
			  ulDSRemSpace = (ULONG)LSLenDocGroupItem;
			  pDSItem = pDataStream;
                                    /*    and write datastream item   */
                                    /*    to work buffer              */
			  DMSG2("WriteDataStream(%s) in temp-Buffer\n",hostvar_pDocGroup.achName);
			  (void) WriteDataStream(&ulDSRemSpace,
						 &pDSItem,
						 ID_DGID,
						 IT_ATOMIC,
						 (USHORT)strlen(buffer3),
						 buffer3);
			  ulDSOffset =   /*    calculate workbuffer offset */
			    (ULONG)LSLenDocGroupItem - ulDSRemSpace;
                                    /*    allocate group list handle  */
                                    /*    area to hold the data       */
			  pDGLHandle = (struct DocGroupListHandle *)
			    malloc(sizeof(struct DocGroupListHandle));
                                    /*    if allocation failed,       */
                                    /*    terminate                   */
			  if (!pDGLHandle)
			    {
			      sprintf(pDiagInfo,"DGLH_ALLOC_ERROR");
			      return RC_TERMINATION_ERROR;
			    }
                                    /*         set DGL handle values  */
			  *ppDocGroupListHandle = (void *)pDGLHandle;
			  memcpy(pDGLHandle->DGLHandleEyecatcher,
				 "LSDGLHDL",LSLenEyeCatcher);
			  pDGLHandle->pWorkBuffer = pDataStream;
			  pDGLHandle->ulWorkBufferOffset = ulDSOffset;
			  pDGLHandle->GroupHandle = GroupHandle;
			  /*         set 'continuation      */
			  /*         requested' and return  */
			  return RC_CONTINUATION_MODE_ENTERED;
			}
		    } /* endif (no output buffer space available)          */

		  else                 /*    datastream item written to  */
		    {                    /*    output buffer: adjust       */
                                    /*    length                      */
		      *pulLenDocGroupList = ulLenOutBuffer - ulDGLRemSpace;
		    } /* endif (output buffer space available)             */

#ifdef asdf
		  /* Schreibe group_name */
		  if (WriteDataStream(&ulDGLRemSpace,
				      &pDSItem,
				      ID_DGNAM,
				      IT_ATOMIC,
				      (USHORT)strlen(hostvar_pDocGroup.achName),
				      hostvar_pDocGroup.achName) != RC_TRUE)
                                    /*    no more space in output     */
                                    /*    buffer: allocate buffer to  */
                                    /*    save datastream item        */
                                    /*    (length is compile constant)*/
                                    /*    check 'FirstEntry': in this */
		    {                    /*         case terminate         */
		      if (FirstEntry == 'Y')
			{
			  sprintf(pDiagInfo,"DGBUF TOO SMALL");
                                    /*         close cursor (do not   */
                                    /*         check the return code; */
                                    /*         passing it would over- */
                                    /*         write the current      */
                                    /*         diagnosis information) */

			  os2_rc = DosFindClose(GroupHandle);
			  return RC_TERMINATION_ERROR;
			}
		      else              /*    allocate work buffer to     */
			{                 /*    save datastream item        */
			  /*    (length is compile constant)*/
			  pDataStream = (PCHAR)malloc(LSLenDocGroupItem);
			  if (!pDataStream)
			    {
			      sprintf(pDiagInfo,"LDG ALLOC ERROR");
			      return RC_TERMINATION_ERROR;
			    }
                                    /*    initialize lengths of work  */
                                    /*    buffer                      */
			  ulDSRemSpace = (ULONG)LSLenDocGroupItem;
			  pDSItem = pDataStream;
                                    /*    and write datastream item   */
                                    /*    to work buffer              */
			  DMSG2("WriteDataStream(%s) in temp-Buffer\n",hostvar_pDocGroup.achName);
			  (void) WriteDataStream(&ulDSRemSpace,
						 &pDSItem,
						 ID_DGNAM,
						 IT_ATOMIC,
						 (USHORT)strlen(hostvar_pDocGroup.achName),
						 hostvar_pDocGroup.achName);
			  ulDSOffset =   /*    calculate workbuffer offset */
			    (ULONG)LSLenDocGroupItem - ulDSRemSpace;
                                    /*    allocate group list handle  */
                                    /*    area to hold the data       */
			  pDGLHandle = (struct DocGroupListHandle *)
			    malloc(sizeof(struct DocGroupListHandle));
                                    /*    if allocation failed,       */
                                    /*    terminate                   */
			  if (!pDGLHandle)
			    {
			      sprintf(pDiagInfo,"DGLH_ALLOC_ERROR");
			      return RC_TERMINATION_ERROR;
			    }
                                    /*         set DGL handle values  */
			  *ppDocGroupListHandle = (void *)pDGLHandle;
			  memcpy(pDGLHandle->DGLHandleEyecatcher,
				 "LSDGLHDL",LSLenEyeCatcher);
			  pDGLHandle->pWorkBuffer = pDataStream;
			  pDGLHandle->ulWorkBufferOffset = ulDSOffset;
			  pDGLHandle->GroupHandle = GroupHandle;
			  /*         set 'continuation      */
			  /*         requested' and return  */
			  return RC_CONTINUATION_MODE_ENTERED;
			}
		    } /* endif (no output buffer space available)          */

		  else                 /*    datastream item written to  */
		    {                    /*    output buffer: adjust       */
                                    /*    length                      */
		      *pulLenDocGroupList = ulLenOutBuffer - ulDGLRemSpace;
		    } /* endif (output buffer space available)             */
#endif
                                    /*    reset 'FirstEntry'          */
		  if (FirstEntry == 'Y') FirstEntry = 'N';

		} /* !PointDir */

            } /* endif (fetch cursor)                                 */

         } /* endfor (as long as document group identifiers are avail-*/
                                    /* able and output buffer is not  */
                                    /* full)                          */
         break;

      } /* endif (entire library)                                     */



/*--------------------------------------------------------------------*/
/* Request type = LS_NEXT                                             */
/*--------------------------------------------------------------------*/
   case (LS_NEXT):
     /* <siebert> hier muesste korrekterweise noch eine Unterscheidung
	hin, ob die gesamte Library (LW-Namen) oder Unterverzeichnisse
	gesucht werden. Da die LW-Namen wohl in den ersten Block passen
	sollten, funktioniert es hier auch so. */

                                    /* initialize offset and remain-  */
                                    /*    ing space of work buffer    */
      pDGLHandle = (struct DocGroupListHandle *)*ppDocGroupListHandle;
      ulDSOffset = pDGLHandle->ulWorkBufferOffset;
      pDataStream = pDGLHandle->pWorkBuffer;
      GroupHandle = pDGLHandle->GroupHandle;
      pDSItem = pDocGroupList;
                                    /* initialize offset and remain-  */
                                    /*    ing space of output buffer  */
      ulDGLRemSpace = ulLenOutBuffer;
      *pulLenDocGroupList = 0;

                                    /* do until there are no more     */
                                    /*    document group values       */
                                    /*    available:                  */
      for (;;)
      {
	anzDocuments = 1;
                                    /* check whether the entry fits   */
                                    /*    in the output buffer:       */
         if (ulDSOffset > 0)
         {
            if (ulDGLRemSpace >= ulDSOffset)
            {                       /*    yes: copy it                */
               memcpy(pDocGroupList,pDataStream,ulDSOffset);
                                    /*         increase output        */
                                    /*         buffer offset          */
               *pulLenDocGroupList = ulDSOffset;
                                    /*         decrease output        */
                                    /*         buffer remaining space */
               ulDGLRemSpace -= ulDSOffset;
                                    /*         increase pointer to    */
                                    /*         remaining space        */
               pDSItem += ulDSOffset;
               ulDSOffset = 0;      /*         reset lengths          */
               ulDSRemSpace = (ULONG)LSLenDocGroupItem;
            }
            else                    /*    no:  indicate output buffer */
            {                       /*         too small              */
               sprintf(pDiagInfo,"DGBUF TOO SMALL");
                                    /*         close cursor (do not   */
                                    /*         check the return code; */
                                    /*         passing it would over- */
                                    /*         write the current      */
                                    /*         diagnosis information) */
	       os2_rc = DosFindClose(GroupHandle);
               return RC_TERMINATION_ERROR;
            } /* endif (output buffer space available)                */
         }
         else                       /* no saved entry available, get  */
         {                          /*    next document group item    */
	   os2_rc = DosFindNext(GroupHandle,&hostvar_pDocGroup,sizeof(FILEFINDBUF3),&anzDocuments);

            if (os2_rc != NO_ERROR)       /* check for errors               */
            {
	      if (os2_rc != ERROR_NO_MORE_FILES) {
		sprintf(pDiagInfo,"DosFindNext %d",os2_rc);
		return RC_TERMINATION_ERROR;
	      }
	      os2_rc = DosFindClose(GroupHandle);
	      if (os2_rc != NO_ERROR)
		{
		  sprintf(pDiagInfo,"DosFindClose =  %i",os2_rc);
		  rc = RC_TERMINATION_ERROR;
		}
	      return rc;
	    }
            else
	      {                       /* otherwise: build document      */
                                    /*    group entry in output       */
                                    /*    buffer                      */
		DMSG2("Teste %s\n",hostvar_pDocGroup.achName);
		if ((hostvar_pDocGroup.attrFile & FILE_DIRECTORY) == 0) {
		  /* Ist das File ein Rmail File? */
		  DMSG("Kein Verzeichnis, sondern File\n");
		  if (((fnlen = strlen(hostvar_pDocGroup.achName)) > RMAIL_EXT_LEN) &&
		      stricmp(hostvar_pDocGroup.achName+fnlen-RMAIL_EXT_LEN,RMAIL_EXT) == 0) {
		    DMSG("RMAIL File gefunden!\n");
		    groupIsRmail = 1;
		  } else {
		    DMSG("Ignoriere File!\n");
		    continue;
		  }
		} else {
		  DMSG("Ist ein Verzeichnis!\n");
		  groupIsRmail = 0;
		}
		DMSG2("WriteDataStream(%s)\n",hostvar_pDocGroup.achName);
		if (isPointDir(hostvar_pDocGroup.achName)) {
		  continue;
		} else {
		  strcpy(buffer3,buffer2);
		  strcat(buffer3,hostvar_pDocGroup.achName);
		  strcat(buffer3,"\\");
		  if (WriteDataStream(&ulDGLRemSpace,
				      &pDSItem,
				      ID_DGID,
				      IT_ATOMIC,
				      (USHORT)strlen(buffer3),
				      buffer3) != RC_TRUE)
                                    /*    no more space in output     */
                                    /*    buffer:                     */
                                    /*    check 'FirstEntry': in this */
		    {                    /*         case terminate         */
		      if (FirstEntry == 'Y')
			{
			  sprintf(pDiagInfo,"DGBUF TOO SMALL");
                                    /*         close cursor (do not   */
                                    /*         check the return code; */
                                    /*         passing it would over- */
                                    /*         write the current      */
                                    /*         diagnosis information) */
			  os2_rc = DosFindClose(GroupHandle);
			  return RC_TERMINATION_ERROR;
			}
		      else              /*    write datastream item to    */
			{                 /*    work buffer                 */
			  pDSItem = pDataStream;
			  DMSG2("WriteDataStream(%s) in temp-Buffer\n",hostvar_pDocGroup.achName);
			  (void) WriteDataStream(&ulDSRemSpace,
						 &pDSItem,
						 ID_DGID,
						 IT_ATOMIC,
						 (USHORT)strlen(buffer3),
						 buffer3);
			  ulDSOffset =   /*    calculate workbuffer offset */
			    (ULONG)LSLenDocGroupItem - ulDSRemSpace;
                                    /*    set work buffer offset      */
			  pDGLHandle->ulWorkBufferOffset = ulDSOffset;
                                    /*         set 'continuation      */
                                    /*         requested' and return  */
			  return RC_CONTINUATION_MODE_ENTERED;

			}
		    } /* endif (no output buffer space available)          */

		  else                 /*    datastream item written to  */
		    {                    /*    output buffer: adjust       */
                                    /*    length                      */
		      *pulLenDocGroupList = ulLenOutBuffer - ulDGLRemSpace;
		    } /* endif (output buffer space available)             */

                                    /*    reset 'FirstEntry'          */
		  if (FirstEntry == 'Y') FirstEntry = 'N';

		} /* endif (!PointDir) */

            } /* endif (output space available)                       */

         } /* endif (saved entry available)                           */

      } /* endfor (as long as docgroup identifiers are available      */

      break;


/*--------------------------------------------------------------------*/
/* Request type = LS_CANCEL                                           */
/*--------------------------------------------------------------------*/
   case (LS_CANCEL):
                                    /* free all allocated areas and   */
                                    /*    the handle itself           */
      if (pDGLHandle != NULL)
      {
         if (pDGLHandle->pWorkBuffer != NULL)
            free(pDGLHandle->pWorkBuffer);
         free(pDGLHandle);
      }

      break;


/*--------------------------------------------------------------------*/
/* Default type                                                       */
/*--------------------------------------------------------------------*/
   default:
      {
         sprintf(pDiagInfo,"INVALID REQTYPE");
         return RC_TERMINATION_ERROR;
      }

   } /* end switch (ReqType) */

   return rc;
}

/***********************************************************************
*                                                                      *
* Entry point:  LIB_list_documents                                     *
*                                                                      *
* Function:     Lists for a given document group all documents         *
*               contained therein.                                     *
*                                                                      *
* Input parms:  1.Pointer to Library Services anchor (not used here)   *
*               2.Length of document group identifier datastream       *
*               3.Pointer to document group identifier datastream      *
*               4.Length of buffer to receive document list            *
*               5.Type of the request (LS_FIRST/LS_NEXT/LS_CANCEL)     *
*               6.Document list handle (set to zero for LS_FIRST)      *
*                                                                      *
* Output parms: 1.Actual length of document list                       *
*               2.Document list (buffer provided by caller)            *
*               3.Document list handle (only for LS_FIRST and LS_NEXT  *
*                    if return code is RC_CONTINUATION_MODE_ENTERED)   *
*               4.Diagnosis information (buffer provided by caller;    *
*                   is used by caller only for return code not RC_OK)  *
*                                                                      *
* Return codes: Normal end         RC_OK                               *
*               Contin. requested  RC_CONTINUATION_MODE_ENTERED        *
*               Group not found    RC_DOCUMENT_GROUP_NOT_FOUND         *
*               Document error     RC_DOCUMENT_IN_ERROR                *
*               Syntax error       RC_DATASTREAM_SYNTAX_ERROR          *
*               Termination error  RC_TERMINATION_ERROR                *
*                                                                      *
* Input files:  Table containing the documents                         *
*                                                                      *
* Output files: -                                                      *
*                                                                      *
* Calls:        CheckDataStream                                        *
*               WriteDataStream                                        *
*               OS/2 Database Manager functions                        *
*               C functions                                            *
*                                                                      *
* Exits:        Always returns to caller.                              *
*                                                                      *
* -------------------------------------------------------------------- *
*                                                                      *
* Program logic:                                                       *
*                                                                      *
* - If request type is FIRST:                                          *
*   - Analyze input datastream and get document group identifier.      *
*     If there is any error, return with RC_DATASTREAM_SYNTAX_ERROR.   *
*   - Set document offset to zero, remaining buffer space to buffer    *
*     length                                                           *
*   - Return all distinct entries of column DOCUMENT for given         *
*     document group:                                                  *
*     - Start                                                          *
*         DECLARE cdoc CURSOR FOR                                      *
*          SELECT DISTINCT DOCUMENT                                    *
*            FROM IBMSM2.DOCTABLE                                      *
*            WHERE DOCGROUP = :hostvar_pDocGroup                       *
*       Open cursor and get first document identifier:                 *
*         OPEN cdoc                                                    *
*         FETCH cdoc INTO :hostvar_pDocument                           *
*       If there is no such document group, return                     *
*         RC_DOCUMENT_GROUP_NOT_FOUND.                                 *
*     - Do until there are no more documents or the buffer is full:    *
*       If there are no more documents, close cursor and return        *
*         with RC_OK.                                                  *
*       Otherwise build entry in work buffer: the entry consists of    *
*       the document item plus the data/time last modified or deleted  *
*       if a requested time is given and the date and time fulfills    *
*       the condition (if not, the entire entry is skipped).           *
*       Check whether the entry fits in the buffer. If yes, put it     *
*         to the buffer, increase offset, decrease remaining space     *
*         of the buffer by the length of the entry.                    *
*         If no, anchor document identifier, offset and remaining      *
*         space by the document list handle, and return buffer with    *
*         RC_CONTINUATION_MODE_ENTERED.                                *
*         FETCH cdoc INTO :hostvar_pDocument                           *
*       Continue.                                                      *
*                                                                      *
* - If request type is NEXT:                                           *
*   - Get offset, remaining space and last document identifier from    *
*     the document handle.                                             *
*   - Do until there are no more documents or the buffer is full:      *
*     If there are no more documents, close cursor and return with     *
*       RC_OK.                                                         *
*     Otherwise build entry in work buffer: the entry consists of the  *
*       document item plus the data/time last modified or deleted      *
*       if a requested time is given and the date and time fulfills    *
*       the condition (if not, the entire entry is skipped).           *
*     Check whether the entry fits in the buffer. If yes, put it       *
*       to the buffer, increase offset, decrease remaining space       *
*       of the buffer by the length of the entry.                      *
*       If no, anchor document identifier, offset and remaining space  *
*       by the document list handle, and return buffer with RC_CONT.   *
*         FETCH cdoc INTO :hostvar_pDocument                           *
*     Continue.                                                        *
*                                                                      *
* - If request type is CANCEL:                                         *
*         CLOSE cdoc                                                   *
*   Free all allocated areas and the document list handle itself and   *
*     return with RC_OK.                                               *
*                                                                      *
***********************************************************************/

ULONG LSCALL
LIB_list_documents                  /* List Document Identifiers -----*/
 (
 PVOID pAnchor,                     /* In:  anchor (not used)         */
 USHORT usLenDocGroupId,            /* In:  length of input           */
 PCHAR pDocGroupId,                 /* In:  document group identifier */
 ULONG ulLenOutBuffer,              /* In:  length of buffer to re-   */
                                    /*      ceive document identifier */
 CHAR ReqType,                      /* In:  type of the request       */
 PPVOID ppDocListHandle,            /* I/O: document list handle      */
 PULONG pulLenDocList,              /* Out: actual length of document */
                                    /*      identifier list           */
 PCHAR pDocList,                    /* I/O: buffer to receive docum-  */
                                    /*      ent identifiers           */
 PCHAR pDiagInfo                    /* I/O: buffer to receive         */
                                    /*      diagnosis information     */
 )

{
   ULONG rc = RC_OK;                /* Return code                    */
                                    /* Document List Handle           */
   ULONG anzDocuments;
   HDIR GroupHandle;
   struct DocListHandle *pDLHandle = NULL;
   PCHAR pDataStream = NULL,        /* work buffer pointers           */
         pDSItem = NULL,
         pDaTimRequested = NULL;
   USHORT usDGIDLength = 0;         /* work and output buffer lengths */
   ULONG ulDSRemSpace = 0,
         ulDSOffset = 0,
         ulDSInLength = 0,
         ulDLRemSpace = 0;
   UCHAR FirstEntry = 'Y',          /* indicate first document id     */
         FirstRead = 'Y',
         pDaTime[sizeof(LS_DATIME)];
   int  ignore, namelen;
   int  groupIsRmail;
   mailidx mi;
   mail m;
   char *pstr;
   char buffer[_MAX_PATH];
   char buffer2[_MAX_PATH];
   char buffer3[_MAX_PATH];
   struct stat filestat;

   DMSGSTART("Betrete LIB_List_Documents!\n");
   DMSGEND2("ulLenOutBuffer = %i\n",ulLenOutBuffer);
                                    /* initialize DiagnosisInfo and   */
                                    /*    date/time area              */
   memset(pDiagInfo,' ',LEN_DIAGNOSIS_INFO);
   memset(pDaTime,0x00,sizeof(LS_DATIME));
   *pulLenDocList = 0;              /* initialize output length       */
/*    pAnchor; */
                                    /* switch dependent on request:   */
   switch (ReqType) {

     /*--------------------------------------------------------------------*/
     /* Request type = LS_FIRST                                            */
     /*--------------------------------------------------------------------*/
   case (LS_FIRST): {

     /*-----------------------------------------------------------------*/
     /* Check input datastream:                                         */

                                    /* initialize work lengths and    */
                                    /*    pointers                    */
     ulDSInLength = (ULONG)usLenDocGroupId;
     pDSItem = pDocGroupId;
   
   /* save start of Document Group   */
     pDataStream = pDSItem;        /*    identifier item             */
                                    /* check Document Group ID item:  */
     if (CheckDataStream(&ulDSInLength,
                         &pDSItem,
                         ID_DGID,
                         IT_ATOMIC,
                         ANY_LEN,            /*    length is variable   */
                         NULL) != RC_TRUE) { /*    value is not checked */
       /* in case of error, terminate    */
       sprintf(pDiagInfo,"INVALID DGID");
       return RC_DATASTREAM_SYNTAX_ERROR;
     }

                                    /* calculate length of Document   */
                                    /*    Group identifier item       */
     usDGIDLength = (USHORT)(pDSItem - pDataStream) - LSLenLLIdIt;
                                    /* check length of Document Group */
                                    /*    identifier item:            */
     if (usDGIDLength <= 0 || usDGIDLength > LSLenDocGroupColumn) {
       sprintf(pDiagInfo,"DGID LEN ERROR");
       return RC_TERMINATION_ERROR;
     }

     pDataStream = pDSItem;
                                    /* check optional Date/Time       */
                                    /*    Requested item:             */
     if (CheckDataStream(&ulDSInLength,
                         &pDSItem,
                         ID_DTRQ,
                         IT_ATOMIC,
                         (USHORT)sizeof(LS_DATIME),
                         NULL) == RC_TRUE) {
                                    /* found: save date/time value    */
       pDaTimRequested = malloc(sizeof(LS_DATIME));
       if (!pDaTimRequested) {
         sprintf(pDiagInfo,"DTMR ALLOC ERROR");
         return RC_TERMINATION_ERROR;
       }
       memcpy(pDaTimRequested,pDataStream+LSLenLLIdIt,
              sizeof(LS_DATIME));
       DMSG2("Time requested: %i\n",(char)pDaTimRequested[4]);
       DMSG2("Time requested: %i\n",(char)pDaTimRequested[5]);
       DMSG2("Month requested: %i\n",(char)pDaTimRequested[2]);
       DMSG2("Day requested: %i\n",(char)pDaTimRequested[3]);
     }


   /*-----------------------------------------------------------------*/
   /* Prepare writing of output datastream:                           */

                                    /* initialize offset and remain-  */
                                    /*    ing space of output buffer  */
     ulDLRemSpace = ulLenOutBuffer;
                                    /* allocate work buffer (length   */
                                    /*    is compile constant)        */
     pDataStream = (PCHAR)malloc(LSLenDocItem);
     if (!pDataStream) {
       sprintf(pDiagInfo,"LD ALLOC ERROR");
       return RC_TERMINATION_ERROR;
     }
                                    /* initialize lengths of work     */
     ulDSOffset = 0;               /*    buffer                      */
     ulDSRemSpace = (ULONG)LSLenDocItem;
     pDSItem = pDataStream;        /* initialize pointer             */
                                    /* prepare document group id      */
/*      memset(hostvar_pDocGroup,0x00,LSLenDocGroupColumn); */
     strncpy(buffer,pDocGroupId+LSLenLLIdIt,usDGIDLength);
     buffer[usDGIDLength] = '\0';
     strcpy(buffer2,buffer);

     if ((usDGIDLength > RMAIL_EXTPATH_LEN) &&
	 stricmp(buffer+usDGIDLength-RMAIL_EXTPATH_LEN,RMAIL_EXTPATH) == 0) {
       /* Ist dies ein Verzeichnis oder ein echtes RMAIL File? */
       buffer[usDGIDLength-1] = '\0';
       DMSG2("stat(%s)\n",buffer);
       if (stat(buffer,&filestat) == -1) {
	 sprintf(pDiagInfo,"stat = -1");
	 return RC_TERMINATION_ERROR;
       }
       strcpy(buffer,buffer2);
       if ((filestat.st_mode&S_IFREG) != 0) {
	 /* Rmail Files enthalten keine weiteren Gruppen */
#ifdef NO_RMAIL_INDEX
	 DMSG("Dies ist ein Rmail\n");
#else       
	 DMSG("Dies ist ein Rmail - rufe rmail_index() auf\n");
	 if (rmail_index(buffer,((struct LSData *)pAnchor)->date) != 0) {
	   DMSG("rmail_index != 0\n");
	   return RC_DOCUMENT_IN_ERROR;
	 }
	 DMSG("rmail_index == 0\n");
#endif
	 groupIsRmail = 1;
       } else {
	 groupIsRmail = 0;
	 strcat(buffer,"*");
       }
     } else {
       groupIsRmail = 0;
       strcat(buffer,"*");
     }
/*      strcat(buffer2,"\\"); */

                                    /* prepare reading of DOCUMENT    */
                                    /*    values from document table: */
                                    /*  - declare cursor              */

/*       EXEC SQL DECLARE cdoc CURSOR FOR */
/*         SELECT DISTINCT DOCUMENT */
/*           FROM IBMSM2.DOCTABLE */
/*          WHERE DOCGROUP = :hostvar_pDocGroup; */

/*                                     /*  - open cursor                 */
/*       EXEC SQL OPEN cdoc; */


   /*-----------------------------------------------------------------*/
   /* Write output datastream:                                        */

                                    /*    do until no more document   */
                                    /*    values are available:       */
     FirstRead = 'Y';
     for (;;) {
       anzDocuments = 1;
       if (FirstRead == 'Y') {
	 DMSGSTART("FirstEntry = Y\n");
	 DMSGEND2("buffer = %i\n",buffer);
	 FirstRead = 'N';
	 if (groupIsRmail) {
	   DMSG2("Buffer = <%s>\n",buffer);
	   os2_rc = getmailfirst(buffer,&mi,&m);
	   DMSG2("getmailfirst() = %i\n",os2_rc);
	 } else {
	   GroupHandle = HDIR_CREATE;
	   os2_rc = DosFindFirst(buffer,&GroupHandle,FILE_NORMAL,&hostvar_pDocument,sizeof(FILEFINDBUF3),&anzDocuments,FIL_STANDARD);
	   DMSG2("DosFindFirst() = %i\n",os2_rc);
	 }
	 if (os2_rc != NO_ERROR) {
	   GroupHandle=-1;
	 }
	 DMSGSTART2("GroupHandle = %i\n",GroupHandle);
	 DMSGEND2("rc DosFindFirst = %i\n",os2_rc);
       } else {
	 DMSGSTART("FirstEntry != Y\n");
	 DMSGEND2("GroupHandle = %i\n",GroupHandle);
	 if (groupIsRmail) {
	   os2_rc = getmailnext(mi,&m);
	 } else {
	   os2_rc = DosFindNext(GroupHandle,&hostvar_pDocument,sizeof(FILEFINDBUF3),&anzDocuments);
	 }
	 DMSG2("rc DosReadNext = %i\n",os2_rc);
       }
       if (os2_rc != NO_ERROR) {        /*  - check for errors:           */
	 if (os2_rc != ERROR_NO_MORE_FILES) {
#ifdef NO_RMAIL_INDEX
	   if (groupIsRmail) {
	     return RC_EMPTY_LIST;
	   }
#endif
           sprintf(pDiagInfo,"DosFindFirst/Next %d",os2_rc);
	   return RC_TERMINATION_ERROR;
	 }
         if (FirstEntry == 'Y') {
/*            rc = RC_DOCUMENT_GROUP_NOT_FOUND; */
	   return RC_EMPTY_LIST;
                                    /*    not found and already       */
                                    /*    entries found:              */
                                    /*    return 'group not found'    */
         }
         if (FirstEntry == 'E') {
           rc = RC_EMPTY_LIST;
                                    /*    no more document values:    */
                                    /*    close cursor and return     */
         }

	 if (groupIsRmail) {
	   os2_rc = getmailclose(mi);
	 } else if (GroupHandle != -1) {
	   os2_rc = DosFindClose(GroupHandle);
	 }
	 DMSGSTART2("GroupHandle = %i\n",GroupHandle);
	 DMSGEND2("rc DosFindClose = %i\n",os2_rc);
	 if (os2_rc != NO_ERROR) {
	   sprintf(pDiagInfo,"DosFindClose1 %d",os2_rc);
	   rc = RC_TERMINATION_ERROR;
	 }
	 DMSGSTART("Beende LIB_List_Documents!\n");
	 DMSGLINE2("pulLenDocList = %i\n",pulLenDocList);
	 DMSGEND2("pDocList = <%s>\n",pDocList);
         return rc;
       } else /* NO_ERROR */ {
          /*    otherwise:                  */
          /*    build entry in work buffer: */
          /*    - write document item       */

	 if (!groupIsRmail) {
	   /* Spezial File; wird nicht angezeigt! */
	   if (strcmp(hostvar_pDocument.achName,sm2_rc_filename) == 0) {
	     continue;
	   }
	   
	   /* Rmail-File; wird nicht angezeigt! */
	   namelen = strlen(hostvar_pDocument.achName);
	   if ((namelen > RMAIL_EXT_LEN) &&
	       stricmp(hostvar_pDocument.achName+namelen-RMAIL_EXT_LEN,RMAIL_EXT) == 0) {
	     continue;
	   }

	   /* Rmail-Index-File; wird nicht angezeigt! */
	   if ((namelen > RMAIL_RDX_EXT_LEN) &&
	       stricmp(hostvar_pDocument.achName+namelen-RMAIL_RDX_EXT_LEN,RMAIL_RDX_EXT) == 0) {
	     continue;
	   }

	   /* Filenamen mit extension aus SM2_IGNORE_EXT nicht anzeigen! */
	   if (strcmp(sm2_ignore_ext,"") != 0) {
	     strcpy(buffer3,sm2_ignore_ext);
	     pstr = strtok(buffer3,";");
	     ignore = 0;
	     while (!ignore && (pstr != NULL)) {
	       if ((namelen >= strlen(pstr)) && (stricmp(&hostvar_pDocument.achName[namelen-strlen(pstr)],pstr) == 0)) {
		 ignore = 1;
	       } else {
		 pstr = strtok(NULL,";");
	       }
	     }
	     if (ignore) {
	       continue;
	     }
	   }
	 }

	 strcpy(buffer3,buffer2);
	 if (groupIsRmail) {
	   strcat(buffer3,m.msgid);
	   free(m.msgid);
	 } else {
	   strcat(buffer3,hostvar_pDocument.achName);
	 }
	 DMSG2("WriteDataStream(%s)\n",buffer3);
         (void) WriteDataStream(&ulDSRemSpace,
                                &pDSItem,
                                ID_DID,
                                IT_ATOMIC,
                                (USHORT)strlen(buffer3),
                                buffer3);
/* 	 DMSG2("nach WriteDataStream(%s)\n",buffer3); */
	 if (groupIsRmail) {
	   (void) WriteDataStream(&ulDSRemSpace,
				  &pDSItem,
				  ID_DNAM,
				  IT_ATOMIC,
				  (USHORT)m.header.subjectlen,
				  m.subject);
	   free(m.subject);
	 } else {
	   (void) WriteDataStream(&ulDSRemSpace,
				  &pDSItem,
				  ID_DNAM,
				  IT_ATOMIC,
				  (USHORT)strlen(hostvar_pDocument.achName),
				  hostvar_pDocument.achName);
	 }

                                    /*    - check whether there is a  */
                                    /*      requested date/time:      */
                                    /*      if yes, get date/time last*/
                                    /*         modified and compare   */
                                    /*         dates: if the entry is */
                                    /*         to be included, write  */
                                    /*         it, otherwise skip     */
                                    /*         entire entry           */

         if (pDaTimRequested != NULL) {
                                    /*      compare date/time reques- */
                                    /*      ted with date/time last   */
                                    /*      modified: if older, take  */
                                    /*      item, otherwise skip entry*/


	   DMSG("Mache Zeitvergleich!\n");
	   /* TODO: Zeitvergleich mit FILEFINDBUF3 */
	   if (groupIsRmail) {
	     memcpy(pDaTime,m.header.indexdate,sizeof(LS_DATIME));
	   } else {
	     TSConversion(&hostvar_pDocument,pDaTime);
	   }
	   DMSG( "Zeitvergleich Req  Akt\n");
	   DMSG3("Jahr          %i   %i\n",(short)*pDaTimRequested,(short)*pDaTime);
	   DMSG3("Month         %i   %i\n",(char)pDaTimRequested[2],(char)pDaTime[2]);
	   DMSG3("Day           %i   %i\n",(char)pDaTimRequested[3],(char)pDaTime[3]);
	   DMSG3("Hour          %i   %i\n",(char)pDaTimRequested[4],(char)pDaTime[4]);
	   DMSG3("Minute        %i   %i\n",(char)pDaTimRequested[5],(char)pDaTime[5]);
	   DMSG3("Seconds       %i   %i\n",(char)pDaTimRequested[6],(char)pDaTime[6]);
	   DMSG3("Hundrets      %i   %i\n",(char)pDaTimRequested[7],(char)pDaTime[7]);
           if (memcmp(pDaTimRequested,pDaTime,sizeof(LS_DATIME))
               <= 0) {
                                    /*      write date/time last mod- */
                                    /*      ified item:               */
	     DMSG("Item is newer\n");
             (void) WriteDataStream(&ulDSRemSpace,
                                    &pDSItem,
                                    ID_DTLM,
                                    IT_ATOMIC,
                                    (USHORT)sizeof(LS_DATIME),
                                    pDaTime);
	   } else {
	     /*      skip documents with no    */
	     /*      timestamps                */
	     /*      reset work buffer         */
	     DMSG("Item is older, skipping\n");
	     ulDSOffset = 0;
	     ulDSRemSpace = (ULONG)LSLenDocItem;
	     pDSItem = pDataStream;
	     if (FirstEntry == 'Y') FirstEntry = 'E';
	     continue;         /*      skip entire entry         */
	     
	   } /* endif (date/time requested)                          */
	 }
	 DMSGSTART("Kopiere Buffer!\n");
	 DMSGLINE2("LSLenDocItem = %i\n",LSLenDocItem);
	 DMSGEND2("ulDSRemSpace = %i\n",ulDSRemSpace);
                                    /*    get workbuffer offset       */
         ulDSOffset = (ULONG)LSLenDocItem - ulDSRemSpace;
                                    /*    check whether it fits in    */
                                    /*    output buffer               */
	 DMSGSTART2("ulDLRemSpace = %i\n",ulDLRemSpace);
	 DMSGEND2("ulDSOffset = %i\n",ulDSOffset);
         if (ulDLRemSpace >= ulDSOffset) {
                                    /*    yes: copy it                */
	   DMSG2("memcpy pDataStream= <%s>\n",pDataStream);
           memcpy(pDocList + *pulLenDocList,
                  pDataStream,
                  ulDSOffset);
                                    /*         increase output        */
                                    /*         buffer offset          */
           *pulLenDocList += ulDSOffset;
                                    /*         decrease output        */
                                    /*         buffer remaining space */
           ulDLRemSpace -= ulDSOffset;
	   DMSGSTART2("memcpy pDocList= <%s>\n",pDocList);
	   DMSGLINE2("LSLenDocItem = %i\n",LSLenDocItem);
	   DMSGEND2("ulDSRemSpace = %i\n",ulDSRemSpace);
         } else /* Output Buffer Space Available */ {
                                    /*    no:  allocate DL Handle     */
                                    /*         and anchor entry and   */
                                    /*         its length             */
                                    /*    it must not be 'FirstEntry' */
                                    /*         if yes, terminate      */
           if (FirstEntry == 'Y') {
             sprintf(pDiagInfo,"DBUF TOO SMALL");
                                    /*         close cursor (do not   */
                                    /*         check the return code; */
                                    /*         passing it would over- */
                                    /*         write the current      */
                                    /*         diagnosis information) */
	     if (groupIsRmail) {
	       os2_rc = getmailclose(mi);
	     } else {
	       os2_rc = DosFindClose(GroupHandle);
	     }
             return RC_TERMINATION_ERROR;
           } else {
             pDLHandle = (struct DocListHandle *)
               malloc(sizeof(struct DocListHandle));
             if (!pDLHandle) {
               sprintf(pDiagInfo,"DLH_ALLOC_ERROR");
               return RC_TERMINATION_ERROR;
             }
                                    /*         set DL handle values  */
             *ppDocListHandle = (void *)pDLHandle;
             memcpy(pDLHandle->DLHandleEyecatcher,
                    "LSDOLHDL",LSLenEyeCatcher);
             pDLHandle->pWorkBuffer = pDataStream;
             pDLHandle->ulWorkBufferOffset = ulDSOffset;
	     pDLHandle->GroupIsRmail = groupIsRmail;
	     strcpy(pDLHandle->GroupPath,buffer2);
	     if (groupIsRmail) {
	       pDLHandle->MailGroupHandle = mi;
	     } else {
	       pDLHandle->GroupHandle = GroupHandle;
	     }
                                    /*         set 'continuation      */
                                    /*         requested' and return  */
             return RC_CONTINUATION_MODE_ENTERED;
           } /* endif (FirstEntry == 'Y')                         */

         } /* endif (output buffer space available)                */
         FirstEntry = 'N';       /*    reset 'FirstEntry'          */

       } /* endif (fetch cursor)                                    */

       pDSItem = pDataStream;  /* reset lengths and pointer         */
       ulDSOffset = 0;
       ulDSRemSpace = (ULONG)LSLenDocItem;

     } /* endfor (as long as document identifiers are available      */
                                    /* and the output buffer is not   */
     break;                        /* full)                          */
   }


/*--------------------------------------------------------------------*/
/* Request type = LS_NEXT                                             */
/*--------------------------------------------------------------------*/
   case (LS_NEXT):
     {
       strncpy(buffer,pDocGroupId+LSLenLLIdIt,usDGIDLength);
       buffer[usDGIDLength] = '\0';
       strcpy(buffer2,buffer);
                                    /* initialize offset and remain-  */
                                    /*    ing space of work buffer    */
       pDLHandle = (struct DocListHandle *)*ppDocListHandle;
       ulDSOffset = pDLHandle->ulWorkBufferOffset;
       pDataStream = pDLHandle->pWorkBuffer;
       groupIsRmail = pDLHandle->GroupIsRmail;
       if (groupIsRmail) {
	 mi = pDLHandle->MailGroupHandle;
       } else {
	 GroupHandle = pDLHandle->GroupHandle;
       }
       pDSItem = pDataStream;
                                    /* initialize offset and remain-  */
                                    /*    ing space of output buffer  */
       ulDLRemSpace = ulLenOutBuffer;
       *pulLenDocList = 0;

                                    /* do until there are no more     */
                                    /*    document values available:  */
       for (;;) {
                                    /* check whether the entry fits   */
                                    /*    in the output buffer:       */
         if (ulDLRemSpace >= ulDSOffset) {
           /*    yes: copy it                */
           memcpy(pDocList + *pulLenDocList,
                  pDataStream,
                  ulDSOffset);
                                    /*         increase output        */
                                    /*         buffer offset          */
           *pulLenDocList += ulDSOffset;
                                    /*         decrease output        */
                                    /*         buffer remaining space */
           ulDLRemSpace -= ulDSOffset;
           pDSItem = pDataStream;
           ulDSOffset = 0;
           ulDSRemSpace = (ULONG)LSLenDocItem;
         } else {
           if (FirstEntry == 'Y') {
             sprintf(pDiagInfo,"DBUF TOO SMALL");
                                    /*         close cursor (do not   */
                                    /*         check the return code; */
                                    /*         passing it would over- */
                                    /*         write the current      */
                                    /*         diagnosis information) */
	     if (groupIsRmail) {
	       os2_rc = getmailclose(mi);
	     } else {
	       os2_rc = DosFindClose(GroupHandle);
	     }
             return RC_TERMINATION_ERROR;
           } else {
                                    /*    no:  anchor entry and its   */
                                    /*         length                 */
                                    /*         set DL handle values   */
             pDLHandle->ulWorkBufferOffset = ulDSOffset;
                                    /*         set 'continuation      */
                                    /*         requested' and return  */
             return RC_CONTINUATION_MODE_ENTERED;
           } /* endif ('FirstEntry' == 'Y')                          */

         } /* endif (output buffer space available)                   */

         FirstEntry = 'N';          /* reset 'FirstEntry'             */
                                    /* get next document value        */
	 anzDocuments = 1;

	 DMSGSTART("FirstEntry != Y\n");
	 DMSGEND2("GroupHandle = %i\n",GroupHandle);
 	 if (groupIsRmail) {
 	   os2_rc = getmailnext(mi,&m);
 	 } else {
	   os2_rc = DosFindNext(GroupHandle,&hostvar_pDocument,sizeof(FILEFINDBUF3),&anzDocuments);
	 }
	 DMSGSTART2("rc DosReadNext = %i\n",os2_rc);
	 DMSGEND2("rc anzDocuments = %i\n",anzDocuments);
         if (os2_rc != NO_ERROR) {        /*  - check for errors:           */
	   if (os2_rc != ERROR_NO_MORE_FILES) {
	     sprintf(pDiagInfo,"DosFindNext %d",os2_rc);
	     return RC_TERMINATION_ERROR;
	   }
/*            rc = RC_EMPTY_LIST; */
                                    /*    no more document values:    */
                                    /*    close cursor and return     */
 	   if (groupIsRmail) {
 	     os2_rc = getmailclose(mi);
 	   } else {
	     os2_rc = DosFindClose(GroupHandle);
	   }
           if (os2_rc != NO_ERROR) {
             sprintf(pDiagInfo,"DosFindClose2 %d",os2_rc);
             rc = RC_TERMINATION_ERROR;
           }
           return rc;
         } else {                       /*    otherwise:                  */
           /*    build entry in work buffer: */
          /*    - write document item       */

	   /* Spezial File; wird nicht angezeigt! */
	   if (!groupIsRmail) {
	     if (strcmp(hostvar_pDocument.achName,sm2_rc_filename) == 0) {
	       continue;
	     }

	     namelen = strlen(hostvar_pDocument.achName);
	     /* Rmail-File; wird nicht angezeigt! */
	     if ((namelen > RMAIL_EXT_LEN) &&
		 stricmp(hostvar_pDocument.achName+namelen-RMAIL_EXT_LEN,RMAIL_EXT) == 0) {
	       continue;
	     }

	     /* Rmail-Index-File; wird nicht angezeigt! */
	     if ((namelen > RMAIL_RDX_EXT_LEN) &&
		 stricmp(hostvar_pDocument.achName+namelen-RMAIL_RDX_EXT_LEN,RMAIL_RDX_EXT) == 0) {
	       continue;
	     }

	     /* Filenamen mit extension aus SM2_IGNORE_EXT nicht anzeigen! */
	     if (strcmp(sm2_ignore_ext,"") != 0) {
	       strcpy(buffer3,sm2_ignore_ext);
	       pstr = strtok(buffer3,";");
	       ignore = 0;
	       while (!ignore && (pstr != NULL)) {
		 if ((namelen >= strlen(pstr)) && (stricmp(&hostvar_pDocument.achName[namelen-strlen(pstr)],pstr) == 0)) {
		   ignore = 1;
		 } else {
		   pstr = strtok(NULL,";");
		 }
	       }
	       if (ignore) {
		 continue;
	       }
	     }
	   }
	   strcpy(buffer3,pDLHandle->GroupPath);
	   if (groupIsRmail) {
	     strcat(buffer3,m.msgid);
	     free(m.msgid);
	   } else {
	     strcat(buffer3,hostvar_pDocument.achName);
	   }
	   DMSG2("WriteDataStream(%s)\n",buffer3);
	   (void) WriteDataStream(&ulDSRemSpace,
				  &pDSItem,
				  ID_DID,
				  IT_ATOMIC,
				  (USHORT)strlen(buffer3),
				  buffer3);
	   if (groupIsRmail) {
	     (void) WriteDataStream(&ulDSRemSpace,
				    &pDSItem,
				    ID_DNAM,
				    IT_ATOMIC,
				    (USHORT)m.header.subjectlen,
				    m.subject);
	     free(m.subject);
	   } else {
	     (void) WriteDataStream(&ulDSRemSpace,
				    &pDSItem,
				    ID_DNAM,
				    IT_ATOMIC,
				    (USHORT)strlen(hostvar_pDocument.achName),
				    hostvar_pDocument.achName); /* ss */
	   }
                                    /*    - check whether there is a  */
                                    /*      requested date/time:      */
                                    /*      if yes, get date/time last*/
                                    /*         modified and compare   */
                                    /*         dates: if the entry is */
                                    /*         to be included, write  */
                                    /*         it, otherwise skip     */
                                    /*         entire entry           */

	   if (pDaTimRequested != NULL) {

/*                EXEC SQL SELECT DATELMOD */
/*                    INTO :hostvar_pDateLMod */
/*                    FROM IBMSM2.DOCTABLE */
/*                   WHERE DOCUMENT = :hostvar_pDocument */
/*                     AND SEQNO = 1; */

              /* TODO: */

	     /*      compare date/time reques- */
	     /*      ted with date/time last   */
	     /*      modified: if older, take  */
	     /*      item, otherwise skip entry*/
	     if (groupIsRmail) {
	       memcpy(pDaTime,m.header.indexdate,sizeof(LS_DATIME));
	     } else {
	       TSConversion(&hostvar_pDocument,pDaTime);
	     }
	     DMSG( "Zeitvergleich Req  Akt\n");
	     DMSG3("Jahr          %i   %i\n",(short)*pDaTimRequested,(short)*pDaTime);
	     DMSG3("Month         %i   %i\n",(char)pDaTimRequested[2],(char)pDaTime[2]);
	     DMSG3("Day           %i   %i\n",(char)pDaTimRequested[3],(char)pDaTime[3]);
	     DMSG3("Hour          %i   %i\n",(char)pDaTimRequested[4],(char)pDaTime[4]);
	     DMSG3("Minute        %i   %i\n",(char)pDaTimRequested[5],(char)pDaTime[5]);
	     DMSG3("Seconds       %i   %i\n",(char)pDaTimRequested[6],(char)pDaTime[6]);
	     DMSG3("Hundrets      %i   %i\n",(char)pDaTimRequested[7],(char)pDaTime[7]);
	     if (memcmp(pDaTimRequested,pDaTime,sizeof(LS_DATIME))
		 <= 0) {
	       /*      write date/time last mod- */
	       /*      ified item:               */
	       DMSG("Item is newer\n");
	       (void) WriteDataStream(&ulDSRemSpace,
				      &pDSItem,
				      ID_DTLM,
				      IT_ATOMIC,
				      (USHORT)sizeof(LS_DATIME),
				      pDaTime);
	     } else {
	       DMSG("Item is older, skipping\n");
	       ulDSOffset = 0;
	       ulDSRemSpace = (ULONG)LSLenDocItem;
	       pDSItem = pDataStream;
	       continue;
	     }
             
           } /* endif (date/time requested)                          */


                                    /*    get workbuffer offset       */
           ulDSOffset = (ULONG)LSLenDocItem - ulDSRemSpace;
           
         } /* endif (fetch cursor)                                    */
        
       } /* endfor (as long as document identifiers are available      */
                                    /* and the output buffer is not   */
      break;                        /* full)                          */
     } /* case: */

/*--------------------------------------------------------------------*/
/* Request type = LS_CANCEL                                           */
/*--------------------------------------------------------------------*/
case (LS_CANCEL):
  {
                                    /* free all allocated areas and   */
                                    /*    the handle itself           */
    if (pDLHandle != NULL) {
      if (pDLHandle->GroupIsRmail) {
	os2_rc = getmailclose(pDLHandle->MailGroupHandle);
      } else {
	os2_rc = DosFindClose(pDLHandle->GroupHandle);
      }
      if (pDLHandle->pWorkBuffer != NULL) {
        free(pDLHandle->pWorkBuffer);
      }
      free(pDLHandle);
    }
    if (os2_rc != 0) {
      sprintf(pDiagInfo,"DosFindClose3: %d",os2_rc);
      rc = RC_TERMINATION_ERROR;
    }

    break;
  }

/*--------------------------------------------------------------------*/
/* Default type                                                       */
/*--------------------------------------------------------------------*/
default:
  {
    sprintf(pDiagInfo,"INVALID REQTYPE");
    return RC_TERMINATION_ERROR;
  }

} /* end switch (ReqType) */

   DMSGSTART("Beende LIB_List_Documents!\n");
   DMSGLINE2("pulLenDocList = %i\n",*pulLenDocList);
   DMSGEND2("pDocList = <%s>\n",pDocList);

   return rc;

}
