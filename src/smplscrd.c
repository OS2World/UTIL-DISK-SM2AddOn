/***********************************************************************
*                      MODULE PROLOG                                   *
************************************************************************
*                                                                      *
* Module:       SMPLSCRD                                               *
*                                                                      *
* Title:        Library Services for OS/2 Database Manager Sample      *
*               (methods used by client only)                          *
*                                                                      *
* Entry point:  LIB_get_doc_attr_values                                *
*               LIB_get_doc_group_attr_values                          *
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

/* 	$Id: smplscrd.c,v 1.11 1998/03/23 17:23:45 siebert Exp $	 */

#ifndef lint
static char vcid[] = "$Id: smplscrd.c,v 1.11 1998/03/23 17:23:45 siebert Exp $";
#endif /* lint */

/*--------------------------------------------------------------------*/
/* Includes                                                           */
/*--------------------------------------------------------------------*/
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#define INCL_DOS
#define INCL_DOSFILEMGR
#define INCL_ERRORS
#include <os2.h>

#include "EHWLSDEF.H"               /* Library Services definitions   */
#include "EHWLSPRO.H"               /* Library Services profiles      */
#include "EHWLANG.H"                /* Language definitions           */
                                    /* Library Services sample        */
#include "SMPLSGRD.H"               /*    definitions                 */
                                    /* Library Services datastream    */
#include "SMPLSDSH.H"               /*    handling                    */

#include "rmail/rmail.h"

#include "sm_debug.h"

/*--------------------------------------------------------------------*/
/* Structures                                                         */
/*--------------------------------------------------------------------*/
                                    /* Attribute Values List Handle   */
                                    /*   structure (used by           */
                                    /*   LIB_get_attr_values)         */
struct AttrValListHandle {          /*   in case of continuation)     */
   UCHAR AVLHandleEyecatcher[8];    /*   eyecatcher "LSAVLHDL"        */
   USHORT usWorkBufferOffset;       /*   current offset in workbuffer */
   PCHAR pWorkBuffer;               /*   pointer to workbuffer        */
   USHORT usDLBufferLength;         /*   Length of doclist buffer     */
   USHORT usDLOffset;               /*   current offset in doclist    */
                                    /*      buffer                    */
   PCHAR pDocList;                  /*   pointer to doclist buffer    */
   };



/*--------------------------------------------------------------------*/
/* Database Manager declarations and hostvariables                    */
/*--------------------------------------------------------------------*/

                                    /* document                       */
   static unsigned char hostvar_pDocument[_MAX_PATH];
                                    /* date and time last modified    */
   static FILEFINDBUF3 hostvar_pDateLMod;
   static short DateLMod_Ind;

/* End of Database Manager declarations ------------------------------*/


/***********************************************************************
*                                                                      *
* Entry point:  LIB_get_doc_attr_values                                *
*                                                                      *
* Function:     Lists for a given list of documents and for given      *
*               attributes there values.                               *
*                                                                      *
* Input parms:  1.Pointer to Library Services anchor (not used here)   *
*               2.Length of attribute / document list                  *
*               3.Pointer to attribute / document list (datastream     *
*                    format)                                           *
*               4.Length of buffer to receive requested values         *
*               5.Type of the request (LS_FIRST/LS_NEXT/LS_CANCEL)     *
*               6.Attribute values handle (set to zero for LS_FIRST)   *
*                                                                      *
* Output parms: 1.Actual length of requested values                    *
*               2.List of document ids and requested values            *
*                    (datastream format; buffer provided by caller)    *
*               3.Requested values handle (only for LS_FIRST and       *
*                    LS_NEXT if return code is RC_CONT)                *
*               4.Diagnosis information (buffer provided by caller;    *
*                   is used by caller only for return code not RC_OK)  *
*                                                                      *
* Return codes: Normal end         RC_OK                               *
*               Not found          RC_DOCUMENT_NOT_FOUND               *
*               Syntax error       RC_DATASTREAM_SYNTAX_ERROR          *
*               Termination error  RC_TERMINATION_ERROR                *
*                                                                      *
* Input files:  Table containing the documents                         *
*                                                                      *
* Output files: -                                                      *
*                                                                      *
* Calls:        CheckDataStream                                        *
*               WriteDataStream                                        *
*               TSConversion                                           *
*               OS/2 Database Manager functions                        *
*                                                                      *
* Exits:        Always returns to caller.                              *
*                                                                      *
* -------------------------------------------------------------------- *
*                                                                      *
* Program logic:                                                       *
*                                                                      *
* - If the request type is FIRST:                                      *
*                                                                      *
*   Get list of documents and attributes and save it. In case of any   *
*   error, terminate.                                                  *
*                                                                      *
*   Prepare writing to output buffer:                                  *
*   - Set offset for requested values to zero and the remaining space  *
*     to length of buffer.                                             *
*   - Allocate work buffer with the given buffer length, and set work  *
*     offset to zero and remaining work space to the given buffer      *
*     length.                                                          *
*     This work buffer will receive the datastream entry for one       *
*     document.                                                        *
*                                                                      *
*   Write output datastream:                                           *
*   - Do while (documents in list):                                    *
*     - get the DATELMOD value:                                        *
*         SELECT DATELMOD                                              *
*           INTO :hostvar_pDateLMod :DateLMod_Ind                      *
*           FROM IBMSM2.DOCTABLE                                       *
*          WHERE DOCUMENT = :hostvar_pDocument AND SEQNO = 1           *
*     - If the document is not found, put the document id together     *
*       with the DNFND indicator to the datastream and skip attribute  *
*       id and value.                                                  *
*     - For any other errors put the document id together with the     *
*       DAERR indicator to the datastream and skip attribute id and    *
*       value.                                                         *
*     - If the returned value is NULL (shown by the indicator variable *
*       :DateLMod_Ind), put the document id and attribute id to the    *
*       datastream (the value is skipped). Otherwise put document id,  *
*       attribute id and attribute value to the datastream.            *
*     - If the remaining space is greater than or equal to the length  *
*       of the complete datastream entry in the work buffer, move      *
*       the datastream item to the output buffer, initialize work      *
*       offset and remaining work space, increase offset for requested *
*       values and decrease remaining space by item length.            *
*     - If the remaining space is less than the length of the complete *
*       datastream entry in the work buffer, allocate the attribute    *
*       values list handle, the work buffer, save work offset, work    *
*       buffer with the requested values, as well as the document list *
*       buffer together with ist offset and total length using the     *
*       handle.                                                        *
*       Set return code to RC_CONTINUATION_MODE_ENTERED and return.    *
*     - Point to next document in list and continue.                   *
*                                                                      *
*   - If there are no more documents, free all areas anchored in the   *
*     requested values handle, free handle itself, set return code to  *
*     RC_OK and return.                                                *
*                                                                      *
* - If the request type is NEXT:                                       *
*                                                                      *
*   Prepare writing to output buffer:                                  *
*   - Set offset for requested values to zero and the remaining space  *
*     to length of buffer.                                             *
*   - Get work buffer, work offset, document list buffer, its offset   *
*     and length (anchored in the attribute values list handle).       *
*   - Get document buffer with the documents to be processed, and the  *
*     next document to process.                                        *
*                                                                      *
*   - If there is a datastream item in the work buffer to be moved,    *
*     move it to the output buffer, initialize work offset and         *
*     remaining work space, increase offset for requested values and   *
*     decrease remaining space by item length.                         *
*                                                                      *
*   Write output datastream:                                           *
*   - Do while (documents in list):                                    *
*     - get the DATELMOD value:                                        *
*         SELECT DATELMOD                                              *
*           INTO :hostvar_pDateLMod :DateLMod_Ind                      *
*           FROM IBMSM2.DOCTABLE                                       *
*          WHERE DOCUMENT = :hostvar_pDocument AND SEQNO = 1           *
*     - If the document is not found, put the document id together     *
*       with the DNFND indicator to the datastream and skip attribute  *
*       id and value.                                                  *
*     - For any other errors put the document id together with the     *
*       DAERR indicator to the datastream and skip attribute id and    *
*       value.                                                         *
*     - If the returned value is NULL (shown by the indicator variable *
*       :DateLMod_Ind), put the document id and attribute id to the    *
*       datastream (the value is skipped). Otherwise put document id,  *
*       attribute id and attribute value to the datastream.            *
*     - If the remaining space is greater than or equal to the length  *
*       of the complete datastream entry in the work buffer, move      *
*       the datastream item to the output buffer, initialize work      *
*       offset and remaining work space, increase offset for requested *
*       values and decrease remaining space by item length.            *
*     - If the remaining space is less than the length of the complete *
*       datastream entry in the work buffer, allocate the attribute    *
*       values list handle, the work buffer, save work offset, work    *
*       buffer with the requested values, as well as the document list *
*       buffer together with its offset and total length using the     *
*       handle.                                                        *
*       Set return code to RC_CONTINUATION_MODE_ENTERED and return.    *
*     - Point to next document in list and continue.                   *
*                                                                      *
*   - If there are no more documents, free all areas anchored in the   *
*     requested values handle, free handle itself, set return code to  *
*     RC_OK and return.                                                *
*                                                                      *
* - If the request type is CANCEL:                                     *
*                                                                      *
*   free all areas anchored in the requested values handle, free       *
*   handle itself and return with RC_OK.                               *
*                                                                      *
***********************************************************************/
ULONG LSCALL
LIB_get_doc_attr_values             /* Get Document Attribute Values -*/
 (
 PVOID pAnchor,                     /* In:  anchor (not used)         */
 USHORT usLenDocAttrList,           /* In:  length of input           */
 PCHAR pDocAttrList,                /* In:  attribute / document list */
 ULONG ulLenOutBuffer,              /* In:  length of buffer to re-   */
                                    /*      ceive requested values    */
 CHAR ReqType,                      /* In:  type of the request       */
 PPVOID ppAttrValListHandle,        /* I/O: attribute value list      */
                                    /*      handle                    */
 PULONG pulLenAttrValues,           /* Out: actual length of          */
                                    /*      attribute values          */
 PCHAR pAttrValues,                 /* I/O: buffer to receive reques- */
                                    /*      ted attribute values      */
 PCHAR pDiagInfo                    /* I/O: buffer to receive         */
                                    /*      diagnosis information     */
 )

{
   ULONG rc = RC_OK;                /* Return code                    */
                                    /* Document List Handle           */
   ULONG os2_rc;

   struct AttrValListHandle *pAVLHandle = NULL;
   PCHAR  pDataStream = NULL,       /* work buffer pointers           */
          pDSItem = NULL,
          pDocList = NULL,
          pDLItem = NULL;
   USHORT usDSOffset = 0,           /* work and output buffer lengths */
          usItemLength = 0,
          usDLLength = 0,
          usDLOffset = 0;
   ULONG  ulDSRemSpace = 0,
          ulDSInLength = 0,
          ulDLRemSpace = 0,
          ulAVLRemSpace = 0;
   UCHAR  FirstEntry = 'Y',         /* indicate first document id     */
          pAIDT[3],
          pAINM[3],
          pAILC[3],
          pAIDS[3],
          pDaTime[sizeof(LS_DATIME)];
   HDIR  GroupHandle;
   ULONG anzDocuments;
   char buffer[_MAX_PATH], *pname;
   int documentIsRmail;
   int blen;
   mail m;

   DMSGSTART("Betrete LIB_get_doc_attr_values!\n");
   DMSGLINE2("usLenDocAttrList = <%i>\n",usLenDocAttrList);
#ifdef DEBUG_LOG_SM2
   fprintf(debug_fh,"pDocAttrList = <");
   fwrite(pDocAttrList,1,usLenDocAttrList,debug_fh);
   fprintf(debug_fh,">\n");
   fflush(debug_fh);
#endif /* DEBUG_LOG_SM2 */
   DMSGLINE2("pDiagInfo = <%s>\n",pDiagInfo);
   DMSGEND2("ReqType = <%x>\n",ReqType);
                                    /* initialize DiagnosisInfo area  */
   if (ReqType != LS_CANCEL) {
     memset(pDiagInfo,' ',LEN_DIAGNOSIS_INFO);
   }
   DMSG("memset done\n");
                                    /* initialize attribute identif-  */
                                    /*    ier DT                      */
   memcpy(pAIDT,ATID_DTIME,sizeof(ATID_DTIME));
   memcpy(pAINM,ATID_NAME,sizeof(ATID_NAME));
   memcpy(pAILC,ATID_LOC,sizeof(ATID_LOC));
   memcpy(pAIDS,ATID_DESC,sizeof(ATID_DESC));

                                    /* switch dependent on request:   */
   switch (ReqType)
   {

/*--------------------------------------------------------------------*/
/* Request type = LS_FIRST                                            */
/*--------------------------------------------------------------------*/
   case (LS_FIRST):

   /*-----------------------------------------------------------------*/
   /* Check input datastream and get list of documents:               */

                                    /* initialize work lengths and    */
                                    /*    pointers (function is       */
                                    /*    always called for all 4     */
                                    /*    attributes):                */
      ulDSInLength = (ULONG)(4 * LSLenATIDItem);
      pDSItem = pDocAttrList;
      *pulLenAttrValues = 0;

      DMSGSTART("request:\n");
      DMSGLINE2("pDSItem[0] = %x\n",pDSItem[0]);
      DMSGLINE2("pDSItem[1] = %x\n",pDSItem[1]);
      DMSGLINE2("pDSItem[2] = %x\n",pDSItem[2]);
      DMSGLINE2("pDSItem[3] = %x\n",pDSItem[3]);
      DMSGLINE2("pDSItem[4] = %x\n",pDSItem[4]);
      DMSGLINE2("pDSItem[5] = %x\n",pDSItem[5]);
      DMSGLINE2("pDSItem[6] = %x\n",pDSItem[6]);
      DMSGLINE2("pDSItem[7] = %x\n",pDSItem[7]);
      DMSGLINE2("pDSItem[8] = %x\n",pDSItem[8]);
      DMSGLINE2("pDSItem[9] = %x\n",pDSItem[9]);
      DMSGLINE2("pDSItem[10] = %x\n",pDSItem[10]);
      DMSGLINE2("pDSItem[11] = %x\n",pDSItem[11]);
      DMSGLINE2("pDSItem[12] = %x\n",pDSItem[12]);
      DMSGLINE2("pDSItem[13] = %x\n",pDSItem[13]);
      DMSGLINE2("pDSItem[14] = %x\n",pDSItem[14]);
      DMSGLINE2("pDSItem[15] = %x\n",pDSItem[15]);
      DMSGLINE2("pDSItem[16] = %x\n",pDSItem[16]);
      DMSGLINE2("pDSItem[17] = %x\n",pDSItem[17]);
      DMSGLINE2("pDSItem[18] = %x\n",pDSItem[18]);
      DMSGLINE2("pDSItem[19] = %x\n",pDSItem[19]);
      DMSGEND2("pDSItem[20] = %x\n",pDSItem[20]);
                                    /* check Attribute List           */
      while (ulDSInLength > 0)
      {
         if (CheckDataStream(&ulDSInLength,
                   &pDSItem,
                   ID_ATID,
                   IT_ATOMIC,
                   LSLenATIDItem-LSLenLLIdIt,
                                    /* value is not checked           */
                   NULL) == RC_FALSE)
         {                          /* in case of error, terminate    */
                                    /*    with 'programming error'    */
            sprintf(pDiagInfo,"INVALID ATID");
            return RC_TERMINATION_ERROR;
         }
      }
                                    /* check whether document list    */
                                    /*    exists                      */
      ulDSInLength = (ULONG)(usLenDocAttrList - (4 * LSLenATIDItem));
      if (ulDSInLength == 0)
      {
         sprintf(pDiagInfo,"NO DOCLIST");
         return RC_DATASTREAM_SYNTAX_ERROR;
      }
                                    /* save length of document list   */
      usDLLength = (USHORT)ulDSInLength;
      pDataStream = pDSItem;        /* save pointer to document list  */

                                    /* check Document List:           */
      while (ulDSInLength > 0)
      {
         if (CheckDataStream(&ulDSInLength,
                   &pDSItem,
                   ID_DID,
                   IT_ATOMIC,
                   ANY_LEN,         /*    length is variable          */
                                    /*    value is not checked        */
                   NULL) == RC_FALSE)
         {                          /* in case of error, terminate    */
            sprintf(pDiagInfo,"INVALID DOC ITEM");
            return RC_DATASTREAM_SYNTAX_ERROR;
         }
      }
                                    /* allocate buffer to hold list   */
                                    /*    of documents:               */
      pDocList = malloc(usDLLength);
      if (!pDocList)
      {
         sprintf(pDiagInfo,"DL ALLOC ERROR");
         return RC_TERMINATION_ERROR;
      }

      usDLOffset = 0;               /* copy document list and         */
                                    /*    initialize lengths          */
      ulDLRemSpace = (ULONG)usDLLength;
      memcpy(pDocList, pDataStream, usDLLength);


   /*-----------------------------------------------------------------*/
   /* Prepare writing of output datastream:                           */

                                    /* initialize offset and remain-  */
                                    /*    ing space of output buffer  */
      ulAVLRemSpace = ulLenOutBuffer;

                                    /* allocate work buffer           */
                                    /*    (length is compile constant)*/
      pDataStream = (PCHAR)malloc(LSLenDocAttrItem);
      if (!pDataStream)
      {
         sprintf(pDiagInfo,"LDA ALLOC ERROR");
         return RC_TERMINATION_ERROR;
      }
                                    /* initialize lengths of work     */
      usDSOffset = 0;               /*    buffer and pointers         */
      ulDSRemSpace = (ULONG)LSLenDocAttrItem;
      pDSItem = pDataStream;
      pDLItem = pDocList;
      usItemLength = 0;


   /*-----------------------------------------------------------------*/
   /* Write output datastream:                                        */

                                    /*    do until no more document   */
                                    /*    values are available:       */
      while (usDLOffset < usDLLength)
      {                             /*  - get document identifier:    */
         (void) CheckDataStream(&ulDLRemSpace,
                     &pDLItem,
                     ID_DID,
                     IT_ATOMIC,
                     ANY_LEN,       /*    length is variable          */
                                    /*    value is not checked        */
                     NULL);

         usItemLength =
            usDLLength - (USHORT)ulDLRemSpace - usDLOffset;
                                    /*    check item length           */
         if ((usItemLength - LSLenLLIdIt) < 0
            || (usItemLength - LSLenLLIdIt) > LSLenDocColumn)
         {
            sprintf(pDiagInfo,"INVALID DID");
            return RC_TERMINATION_ERROR;
         }
                                    /*    provide document identifier */
         memset(hostvar_pDocument,0x00,LSLenDocColumn);
/* 	 strcpy(hostvar_pDocument,"d:\\sm2test\\"); */
/*          strncat(hostvar_pDocument, */
/*                 pDocList + usDLOffset + LSLenLLIdIt, */
/*                 usItemLength - LSLenLLIdIt); */
         memcpy(hostvar_pDocument,
                pDocList + usDLOffset + LSLenLLIdIt,
                usItemLength - LSLenLLIdIt);
                                    /*    and increase offset         */
	 DMSG2("pDocList + usDLOffset + LSLenLLIdIt = %i\n",pDocList + usDLOffset + LSLenLLIdIt);
	 DMSG2("usItemLength-LSLenLLIdIt = %i\n",usItemLength-LSLenLLIdIt)
         usDLOffset += usItemLength;
                                    /*  - get attribute value:        */
/*          EXEC SQL SELECT DATELMOD */
/*              INTO :hostvar_pDateLMod :DateLMod_Ind */
/*              FROM IBMSM2.DOCTABLE */
/*             WHERE DOCUMENT = :hostvar_pDocument */
/*               AND SEQNO = 1; */

	 strcpy(buffer,hostvar_pDocument);
	 DMSGSTART2("hostvar_pDocument = <%s>\n",hostvar_pDocument);
	 DMSGEND2("buffer = <%s>\n",buffer);
	 pname = strrchr(buffer,'\\');
	 *pname = '\0';
	 pname++;
	 DMSGSTART2("pname = <%s>\n",pname);
	 DMSGEND2("buffer = <%s>\n",buffer);
	 blen = strlen(buffer);
	 if ((blen > RMAIL_EXTPATH_LEN) &&
	     stricmp(buffer+blen-RMAIL_EXT_LEN,RMAIL_EXT) == 0) {
	   /* Rmail File */
	   documentIsRmail = 1;
	   os2_rc = findmail(buffer,pname,&m);
	 } else {
	   documentIsRmail = 0;
	   GroupHandle = HDIR_SYSTEM;
	   anzDocuments = 1;
	   os2_rc = DosFindFirst(hostvar_pDocument,&GroupHandle,FILE_NORMAL,&hostvar_pDateLMod,sizeof(FILEFINDBUF3),&anzDocuments,FIL_STANDARD);
	   if (os2_rc != NO_ERROR) {
	     GroupHandle=-1;
	   }
	 }
	 DMSG2("rc DosFindFirst = %i!\n",os2_rc);

	 DateLMod_Ind = 1;
                                    /*    Check for errors:           */
         if (os2_rc != NO_ERROR)
         {                          /*    not found: write document   */
            if (os2_rc == ERROR_FILE_NOT_FOUND)     /*       item with DNFND and skip */
            {                       /*       attribute item           */
               (void) WriteDataStream(&ulDSRemSpace,
                           &pDSItem,
                           ID_DNFND,
                           IT_ATOMIC,
                           (USHORT)strlen(hostvar_pDocument),
                           hostvar_pDocument);
            }
            else
            {                       /*    any other error: write doc. */
                                    /*       item with DAERR and skip */
                                    /*       attribute item           */
               (void) WriteDataStream(&ulDSRemSpace,
                           &pDSItem,
                           ID_DAERR,
                           IT_ATOMIC,
                           (USHORT)strlen(hostvar_pDocument),
                           hostvar_pDocument);
            }
         }
         else                       /*    document found: write doc.  */
         {                          /*       item with DID            */
	   if (!documentIsRmail && (GroupHandle != -1)) {
	     os2_rc = DosFindClose(GroupHandle);
	   }

	   DMSG2("rc DosFindClose = %i!\n",os2_rc);

            (void) WriteDataStream(&ulDSRemSpace,
                         &pDSItem,
                         ID_DID,
                         IT_ATOMIC,
                         (USHORT)strlen(hostvar_pDocument),
                         hostvar_pDocument);

                                    /*    write attribute identifier  */
                                    /*       item                     */
	       /* Name Attribut */
/*             (void) WriteDataStream(&ulDSRemSpace, */
/*                          &pDSItem, */
/*                          ID_DID, */
/*                          IT_ATOMIC, */
/*                          (USHORT)strlen(hostvar_pDocument), */
/*                          hostvar_pDocument); */

                                    /*    write attribute identifier  */
                                    /*       item                     */
            (void) WriteDataStream(&ulDSRemSpace,
                         &pDSItem,
                         ID_ATID,
                         IT_ATOMIC,
                         2,
                         pAINM);

	    DMSGSTART2("pname = <%s>\n",pname);
	    DMSGEND2("pname = <%i>\n",strlen(pname));

	    if (documentIsRmail) {
	      (void) WriteDataStream(&ulDSRemSpace,
				     &pDSItem,
				     ID_AVAL,
				     IT_ATOMIC,
				     (USHORT)m.header.subjectlen+1,
				     m.subject);
	      free(m.msgid);
	      free(m.subject);
	    } else {
	      (void) WriteDataStream(&ulDSRemSpace,
				     &pDSItem,
				     ID_AVAL,
				     IT_ATOMIC,
				     (USHORT)strlen(pname)+1,
				     /* sehr seltsam, eigenlich sollte
					strlen(pname) genuegen, aber
					dann fehlt leider immer der
					letzte Buchstabe! */
				     pname);
	    }
	       /* Location Attribut */
/*             (void) WriteDataStream(&ulDSRemSpace, */
/*                          &pDSItem, */
/*                          ID_DID, */
/*                          IT_ATOMIC, */
/*                          (USHORT)strlen(hostvar_pDocument), */
/*                          hostvar_pDocument); */

                                    /*    write attribute identifier  */
                                    /*       item                     */
            (void) WriteDataStream(&ulDSRemSpace,
                         &pDSItem,
                         ID_ATID,
                         IT_ATOMIC,
                         2,
                         pAILC);

	    (void) WriteDataStream(&ulDSRemSpace,
				   &pDSItem,
				   ID_AVAL,
				   IT_ATOMIC,
				   (USHORT)strlen(buffer),
				   buffer);

	    /* Description Attribut */
/*             (void) WriteDataStream(&ulDSRemSpace, */
/*                          &pDSItem, */
/*                          ID_ATID, */
/*                          IT_ATOMIC, */
/*                          2, */
/*                          pAIDS); */
	    /* DateTime Attribut */
            (void) WriteDataStream(&ulDSRemSpace,
                         &pDSItem,
                         ID_ATID,
                         IT_ATOMIC,
                         2,
                         pAIDT);


/*             if (DateLMod_Ind >= 0) { */
                                    /*    value exists: write attrib. */
                                    /*       item with given value    */
                                    /*       convert to datastream    */
                                    /*       format (LS_DATIME)       */
	    if (documentIsRmail) {
	      memcpy(pDaTime,m.header.date,sizeof(LS_DATIME));
	    } else {
	      DMSG("Starte TSConversion\n");
	      TSConversion(&hostvar_pDateLMod,pDaTime);
	      DMSG2("TSConversion = <%s>!\n",pDaTime);
	    }
               (void) WriteDataStream(&ulDSRemSpace,
                            &pDSItem,
                            ID_AVAL,
                            IT_ATOMIC,
                            (USHORT)sizeof(LS_DATIME),
                            pDaTime);

/*             } */
         }
                                    /*    get workbuffer offset       */


         usDSOffset = LSLenDocAttrItem - (USHORT)ulDSRemSpace;
                                    /*    check whether it fits in    */
                                    /*    output buffer               */
         if (ulAVLRemSpace >= (ULONG)usDSOffset)
         {                          /*    yes: copy it                */
	   DMSG2("return = <%s>!\n",pDataStream);
            memcpy(pAttrValues + *pulLenAttrValues,
                   pDataStream,
                   usDSOffset);
                                    /*         increase output        */
                                    /*         buffer offset          */
            *pulLenAttrValues += (ULONG)usDSOffset;
                                    /*         decrease output        */
                                    /*         buffer remaining space */
            ulAVLRemSpace -= (ULONG)usDSOffset;
                                    /*         reset work buffer      */
                                    /*         lengths and pointer    */
            pDSItem = pDataStream;
            usDSOffset = 0;
            ulDSRemSpace = LSLenDocAttrItem;
         }
         else
         {                          /*    no:  allocate AVL Handle    */
                                    /*         and anchor required    */
                                    /*         values                 */
                                    /*    it must not be 'FirstEntry' */
                                    /*         if yes, terminate      */
	   DMSG("Buffer zu klein\n");
            if (FirstEntry == 'Y')
            {
               sprintf(pDiagInfo,"AVLBUF TOO SMALL");
               return RC_TERMINATION_ERROR;
            }
            else
            {
               pAVLHandle = (struct AttrValListHandle *)
                  malloc(sizeof(struct AttrValListHandle));
               if (!pAVLHandle)
               {
                  sprintf(pDiagInfo,"AVL ALLOC ERROR");
                  return RC_TERMINATION_ERROR;
               }
                                    /*         set AVL handle values  */
               *ppAttrValListHandle = (void *)pAVLHandle;
               memcpy(pAVLHandle->AVLHandleEyecatcher,"LSAVLHDL",
                  LSLenEyeCatcher);
               pAVLHandle->pWorkBuffer = pDataStream;
               pAVLHandle->usWorkBufferOffset = usDSOffset;
               pAVLHandle->usDLBufferLength = usDLLength;
               pAVLHandle->usDLOffset = usDLOffset;
               pAVLHandle->pDocList = pDocList;
                                    /*         set 'continuation      */
                                    /*         requested' and return  */
               return RC_CONTINUATION_MODE_ENTERED;
            } /* endif ('FirstEntry' == Y)                            */

         } /* endif (output buffer space available)                   */
         if (FirstEntry == 'Y') FirstEntry = 'N';

      } /* end while (documents to be processed)                      */

      break;

/*--------------------------------------------------------------------*/
/* Request type = LS_NEXT                                             */
/*--------------------------------------------------------------------*/
   case (LS_NEXT):
                                    /* initialize offset and remain-  */
                                    /*    ing space of output buffer  */
      ulAVLRemSpace = ulLenOutBuffer;
      *pulLenAttrValues = 0;
                                    /* initialize offset and remain-  */
                                    /*    ing space of work buffers   */
      pAVLHandle = (struct AttrValListHandle *)*ppAttrValListHandle;
      pDataStream = pAVLHandle->pWorkBuffer;
      usDSOffset = pAVLHandle->usWorkBufferOffset;
      usDLLength = pAVLHandle->usDLBufferLength;
      usDLOffset = pAVLHandle->usDLOffset;
      pDocList = pAVLHandle->pDocList;

                                    /* do as long as there are        */
                                    /*    documents to process:       */
      while (usDLOffset <= usDLLength)
      {
                                    /*    check whether item fits in  */
                                    /*    output buffer               */
         if (ulAVLRemSpace >= (ULONG)usDSOffset)
         {                          /*    yes: copy it                */
            memcpy(pAttrValues + *pulLenAttrValues,
                   pDataStream,
                   usDSOffset);
                                    /*         increase output        */
                                    /*         buffer offset          */
            *pulLenAttrValues += (ULONG)usDSOffset;
                                    /*         decrease output        */
                                    /*         buffer remaining space */
            ulAVLRemSpace -= (ULONG)usDSOffset;
                                    /*         reset work buffer      */
                                    /*         lengths and pointer    */
            pDSItem = pDataStream;
            usDSOffset = 0;
            ulDSRemSpace = LSLenDocAttrItem;
         }
         else
         {
            if (FirstEntry == 'Y')
            {
               sprintf(pDiagInfo,"AVLBUF TOO SMALL");
               return RC_TERMINATION_ERROR;
            }
            else
            {                       /*    no: anchor required values  */
                                    /*         set AVL handle values  */
            pAVLHandle->pWorkBuffer = pDataStream;
            pAVLHandle->usWorkBufferOffset = usDSOffset;
            pAVLHandle->usDLBufferLength = usDLLength;
            pAVLHandle->usDLOffset = usDLOffset;
            pAVLHandle->pDocList = pDocList;
                                    /*         set 'continuation      */
                                    /*         requested' and return  */
            return RC_CONTINUATION_MODE_ENTERED;

            } /* endif (FirstEntry == 'Y')                            */

         } /* endif (output buffer space available)                   */

                                    /* reset indicator                */
         if (FirstEntry == 'Y') FirstEntry = 'N';

                                    /* initialize lengths of work     */
         usDSOffset = 0;            /*    buffer and pointers         */
         ulDSRemSpace = (ULONG)LSLenDocAttrItem;
         pDSItem = pDataStream;
         pDLItem = pDocList+usDLOffset;
         usItemLength = 0;
         ulDLRemSpace = (ULONG)(usDLLength - usDLOffset);
                                    /* check whether all documents    */
                                    /*    have been processed and the */
                                    /*    datastream items are        */
                                    /*    written                     */
         if (ulDLRemSpace == 0) break;

                                    /*  - get document identifier:    */
	 DMSGSTART("Vor CheckDataStream\n");
#ifdef DEBUG_LOG_SM2
	 fprintf(debug_fh,"pDLItem = <");
	 fwrite(pDLItem,1,ulDLRemSpace,debug_fh);
	 fprintf(debug_fh,">\n");
	 fflush(debug_fh);
#endif /* DEBUG_LOG_SM2 */
	 DMSGEND2("ulDLRemSpace = %i\n",ulDLRemSpace);
         (void) CheckDataStream(&ulDLRemSpace,
                     &pDLItem,
                     ID_DID,
                     IT_ATOMIC,
                     (SHORT)ANY_LEN,/*    length is variable          */
                                    /*    value is not checked        */
                     NULL);
	 DMSGSTART("Nach CheckDataStream\n");
#ifdef DEBUG_LOG_SM2
	 fprintf(debug_fh,"pDLItem = <");
	 fwrite(pDLItem,1,ulDLRemSpace,debug_fh);
	 fprintf(debug_fh,">\n");
	 fflush(debug_fh);
#endif /* DEBUG_LOG_SM2 */
	 DMSGLINE2("usDLLength= %i\n",usDLLength);
	 DMSGLINE2("usDLOffset= %i\n",usDLOffset);
	 DMSGEND2("ulDLRemSpace = %i\n",ulDLRemSpace);
         usItemLength =
            usDLLength - (USHORT)ulDLRemSpace - usDLOffset;
                                    /*    check item length           */
         if ((usItemLength - LSLenLLIdIt) < 0
            || (usItemLength - LSLenLLIdIt) > LSLenDocColumn)
         {
            sprintf(pDiagInfo,"INVALID DID");
            return RC_TERMINATION_ERROR;
         }
                                    /*    provide document identifier */
         memset(hostvar_pDocument,0x00,LSLenDocColumn);
/* 	 strcpy(hostvar_pDocument,"d:\\sm2test\\"); */
/*          strncat(hostvar_pDocument, */
/*                 pDocList + usDLOffset + LSLenLLIdIt, */
/*                 usItemLength - LSLenLLIdIt); */
         memcpy(hostvar_pDocument,
                pDocList + usDLOffset + LSLenLLIdIt,
                usItemLength - LSLenLLIdIt);
                                    /*    and increase offset         */
	 DMSG2("pDocList + usDLOffset + LSLenLLIdIt = %i\n",pDocList + usDLOffset + LSLenLLIdIt);
	 DMSG2("usItemLength-LSLenLLIdIt = %i\n",usItemLength-LSLenLLIdIt)
         usDLOffset += usItemLength;
                                    /*  - get attribute value:        */
/*          EXEC SQL SELECT DATELMOD */
/*              INTO :hostvar_pDateLMod :DateLMod_Ind */
/*              FROM IBMSM2.DOCTABLE */
/*             WHERE DOCUMENT = :hostvar_pDocument */
/*               AND SEQNO = 1; */

	 strcpy(buffer,hostvar_pDocument);
	 DMSGSTART2("hostvar_pDocument = <%s>\n",hostvar_pDocument);
	 DMSGEND2("buffer = <%s>\n",buffer);
	 pname = strrchr(buffer,'\\');
	 *pname = '\0';
	 pname++;
	 DMSGSTART2("pname = <%s>\n",pname);
	 DMSGEND2("buffer = <%s>\n",buffer);
	 blen = strlen(buffer);
 	 if ((blen > RMAIL_EXTPATH_LEN) &&
 	     stricmp(buffer+blen-RMAIL_EXT_LEN,RMAIL_EXT) == 0) {
 	   /* Rmail File */
 	   documentIsRmail = 1;
 	   os2_rc = findmail(buffer,pname,&m);
 	 } else {
 	   documentIsRmail = 0;
	   GroupHandle = HDIR_SYSTEM;
	   anzDocuments = 1;
	   os2_rc = DosFindFirst(hostvar_pDocument,&GroupHandle,FILE_NORMAL,&hostvar_pDateLMod,sizeof(FILEFINDBUF3),&anzDocuments,FIL_STANDARD);
	   if (os2_rc != NO_ERROR) {
	     GroupHandle=-1;
	   }
	 }

	 DateLMod_Ind = 1;
                                    /*    Check for errors:           */
         if (os2_rc != NO_ERROR)
         {                          /*    not found: write document   */
            if (os2_rc == ERROR_FILE_NOT_FOUND)     /*       item with DNFND and skip */
            {                       /*       attribute item           */
               (void) WriteDataStream(&ulDSRemSpace,
                           &pDSItem,
                           ID_DNFND,
                           IT_ATOMIC,
                           (USHORT)strlen(hostvar_pDocument),
                           hostvar_pDocument);
            }
            else
            {                       /*    any other error: write doc. */
                                    /*       item with DAERR and skip */
                                    /*       attribute item           */
               (void) WriteDataStream(&ulDSRemSpace,
                           &pDSItem,
                           ID_DAERR,
                           IT_ATOMIC,
                           (USHORT)strlen(hostvar_pDocument),
                           hostvar_pDocument);
            }
         }
         else                       /*    document found: write doc.  */
         {                          /*       item with DID            */
	   if (!documentIsRmail && (GroupHandle != -1)) {
	     DosFindClose(GroupHandle);
	   }

            (void) WriteDataStream(&ulDSRemSpace,
                         &pDSItem,
                         ID_DID,
                         IT_ATOMIC,
                         (USHORT)strlen(hostvar_pDocument),
                         hostvar_pDocument);

                                    /*    write attribute identifier  */
                                    /*       item                     */
	       /* Name Attribut */
/*             (void) WriteDataStream(&ulDSRemSpace, */
/*                          &pDSItem, */
/*                          ID_DID, */
/*                          IT_ATOMIC, */
/*                          (USHORT)strlen(hostvar_pDocument), */
/*                          hostvar_pDocument); */

                                    /*    write attribute identifier  */
                                    /*       item                     */
            (void) WriteDataStream(&ulDSRemSpace,
                         &pDSItem,
                         ID_ATID,
                         IT_ATOMIC,
                         2,
                         pAINM);

 	    if (documentIsRmail) {
 	      (void) WriteDataStream(&ulDSRemSpace,
 				     &pDSItem,
 				     ID_AVAL,
 				     IT_ATOMIC,
 				     (USHORT)m.header.subjectlen+1,
 				     m.subject);
	      free(m.msgid);
	      free(m.subject);
 	    } else {
	      (void) WriteDataStream(&ulDSRemSpace,
				     &pDSItem,
				     ID_AVAL,
				     IT_ATOMIC,
				     (USHORT)strlen(pname)+1,
				     /* sehr seltsam, eigenlich sollte
					strlen(pname) genuegen, aber
					dann fehlt leider immer der
					letzte Buchstabe! */
				     pname);
	    }
	       /* Location Attribut */
/*             (void) WriteDataStream(&ulDSRemSpace, */
/*                          &pDSItem, */
/*                          ID_DID, */
/*                          IT_ATOMIC, */
/*                          (USHORT)strlen(hostvar_pDocument), */
/*                          hostvar_pDocument); */

                                    /*    write attribute identifier  */
                                    /*       item                     */
            (void) WriteDataStream(&ulDSRemSpace,
                         &pDSItem,
                         ID_ATID,
                         IT_ATOMIC,
                         2,
                         pAILC);

	    (void) WriteDataStream(&ulDSRemSpace,
				   &pDSItem,
				   ID_AVAL,
				   IT_ATOMIC,
				   (USHORT)strlen(buffer),
				   buffer);
	    /* Description Attribut */
/*             (void) WriteDataStream(&ulDSRemSpace, */
/*                          &pDSItem, */
/*                          ID_ATID, */
/*                          IT_ATOMIC, */
/*                          2, */
/*                          pAIDS); */
	    /* DateTime Attribut */
            (void) WriteDataStream(&ulDSRemSpace,
                         &pDSItem,
                         ID_ATID,
                         IT_ATOMIC,
                         2,
                         pAIDT);


/*             if (DateLMod_Ind >= 0){ */
                                    /*    value exists: write attrib. */
                                    /*       item with given value    */
                                    /*       convert to datastream    */
                                    /*       format (LS_DATIME)       */
	      if (documentIsRmail) {
		memcpy(pDaTime,m.header.date,sizeof(LS_DATIME));
	      } else {
		TSConversion(&hostvar_pDateLMod,pDaTime);
	      }
               (void) WriteDataStream(&ulDSRemSpace,
                            &pDSItem,
                            ID_AVAL,
                            IT_ATOMIC,
                            (USHORT)sizeof(LS_DATIME),
                            pDaTime);
/*             } */
         }
                                    /* get workbuffer offset          */
                                    /*    and continue                */
         usDSOffset = LSLenDocAttrItem - (USHORT)ulDSRemSpace;

      } /* end while (documents to be processed)                      */

      break;

/*--------------------------------------------------------------------*/
/* Request type = LS_CANCEL                                           */
/*--------------------------------------------------------------------*/
   case (LS_CANCEL):
                                    /* free all allocated areas and   */
                                    /*    the handle itself           */
     DMSG("LS_CANCEL\n");
      if (pAVLHandle != NULL)
      {
	 if (pAVLHandle->pWorkBuffer != NULL) {
	    DMSG("FREE pWorkBuffer");
	    free(pAVLHandle->pWorkBuffer);
	 } 
         if (pAVLHandle->pDocList != NULL) {
	    DMSG("FREE pDocList");
            free(pAVLHandle->pDocList);
	 }
	 DMSG("FREE pAVLHandle");
         free(pAVLHandle);
      }
      DMSG("LS_CANCEL COMPLETE\n");

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
* Entry point:  LIB_get_doc_group_attr_values                          *
*                                                                      *
* Function:     Not implemented.                                       *
*                                                                      *
* Input parms:  1.Pointer to Library Services anchor (not used here)   *
*               2.Length of attribute / document group list            *
*               3.Pointer to attribute / document group list           *
*                    datastream format)                                *
*               4.Length of buffer to receive requested values         *
*               5.Type of the request (LS_FIRST/LS_NEXT/LS_CANCEL)     *
*               6.Attribute values handle (set to zero for LS_FIRST)   *
*                                                                      *
* Output parms: 1.Actual length of requested values = 0                *
*                                                                      *
* Return codes: No attributes      RC_NO_ATTRIBUTES_DEFINED            *
*                                                                      *
* Input files:  Table containing the document groups                   *
*                                                                      *
* Output files: -                                                      *
*                                                                      *
* Calls:        -                                                      *
*                                                                      *
* Exits:        Always returns to caller.                              *
*                                                                      *
* -------------------------------------------------------------------- *
*                                                                      *
* Program logic:                                                       *
*                                                                      *
* - Returns RC_NO_ATTRIBUTES_DEFINED.                                  *
*                                                                      *
***********************************************************************/

ULONG LSCALL
LIB_get_doc_group_attr_values       /* Get Docgroup Attribute Values -*/
 (
 PVOID pAnchor,                     /* In:  anchor                    */
 USHORT usLenDGrpAttrList,          /* In:  length of input           */
 PCHAR pDGrpAttrList,               /* In:  attribute / docgroup list */
 ULONG ulLenOutBuffer,              /* In:  length of buffer to rec-  */
                                    /*      eive requested values     */
 CHAR ReqType,                      /* In:  type of the request       */
 PPVOID ppAttrValListHandle,        /* I/O: attribute value list      */
                                    /*      handle                    */
 PULONG pulLenAttrValues,           /* Out: actual length of          */
                                    /*      attribute values          */
 PCHAR pAttrValues,                 /* I/O: buffer to receive reques- */
                                    /*      ted attribute values      */
 PCHAR pDiagInfo                    /* I/O: buffer to receive         */
                                    /*      diagnosis information     */
 )

{
   *pulLenAttrValues = 0;           /* set length to zero and return  */
   return RC_NO_ATTRIBUTES_DEFINED;
}
