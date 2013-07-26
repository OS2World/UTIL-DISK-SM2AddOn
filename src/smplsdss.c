/***********************************************************************
*                      MODULE PROLOG                                   *
************************************************************************
*                                                                      *
* Module:       SMPLSDSS                                               *
*                                                                      *
* Title:        Library Services for OS/2 Database Manager Sample      *
*               Datastream Handling                                    *
*                                                                      *
* Entry points: CheckDataStream                                        *
*               WriteDataStream                                        *
*               TSConversion                                           *
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

/* 	$Id: smplsdss.c,v 1.5 1998/02/24 18:05:01 siebert Exp $	 */

#ifndef lint
static char vcid[] = "$Id: smplsdss.c,v 1.5 1998/02/24 18:05:01 siebert Exp $";
#endif /* lint */

/*--------------------------------------------------------------------*/
/* Includes                                                           */
/*--------------------------------------------------------------------*/

#include <os2.h>
#include <string.h>
#include <stdlib.h>

                                    /* Library Services: general      */
#include "EHWLSDEF.H"               /*    definitions                 */
                                    /* Library Services: Sample       */
#include "SMPLSGRD.H"               /*    definitions                 */
#include "SMPLSDSH.H"               /* datastream handling include    */

#include "sm_debug.h"

/***********************************************************************
*                                                                      *
* Entry point:  CheckDataStream                                        *
*                                                                      *
* Function:     Checks the given datastream item                       *
*               - whether it fits into the available space (if yes,    *
*                 available space and pointer to datastream item       *
*                 are updated)                                         *
*               - whether item identifier and type are matching the    *
*                 given values                                         *
*               - whether the given value and its length matches the   *
*                 given value and length (optional: if no value or     *
*                 length are given, no checking is done)               *
*                                                                      *
*               Conversion of USHORT variables between Intel and       *
*               'Big-endian' formats is done within this function      *
*               for two-byte identifiers, lengths, and values using    *
*               CONV_US macro.                                         *
*                                                                      *
* Input parms:  1.Available space                                      *
*               2.Pointer to datastream item to be checked             *
*               3.Item identifier                                      *
*               4.Item type                                            *
*               5.Length of item value (optional)                      *
*               6.Item value (optional)                                *
*                                                                      *
* Output parms: 1.Updated length of available space (for RC_TRUE)      *
*               2.Updated pointer to datastream item (for RC_TRUE)     *
*                                                                      *
* Return codes: Check ok           RC_TRUE                             *
*               Check not ok       RC_FALSE                            *
*                                                                      *
* Exits:        Always returns to caller.                              *
*                                                                      *
***********************************************************************/

ULONG CheckDataStream
 (
 PULONG pRemSpace,                  /* I/O: Pointer to remaining space*/
 PPCHAR ppArea,                     /* I/O: Pointer to free           */
                                    /*         datastream area        */
 USHORT ItemId,                     /* In:  Item identifier           */
 UCHAR  ItemIt,                     /* In:  Item type                 */
 USHORT ValueLen,                   /* In:  Length of item value      */
 PCHAR  pValue                      /* In:  Pointer to item value     */
 )

{
  USHORT ItemLL = 0;                /* length of item value           */
  struct DataItem *pItem =          /* pointer to data stream item    */
     (struct DataItem *)*ppArea;
                                    /* check remaining space versus   */
                                    /*    length                      */
  if (*pRemSpace < (ULONG)CONV_US(&(pItem->usLL)))
     return (RC_FALSE);
  if (CONV_US(&(pItem->usLL)) < OffsetOf(struct DataItem,pValue[0]))
     return (RC_FALSE);
                                    /* check item identifier          */
  if (CONV_US(&(pItem->usId)) != ItemId)
     return (RC_FALSE);
                                    /* check item type                */
  if (pItem->It != ItemIt)
     return (RC_FALSE);
                                    /* calculate length of item value */
  ItemLL =
     CONV_US(&(pItem->usLL)) - OffsetOf(struct DataItem, pValue[0]);


  switch(ValueLen)
  {
     case ZERO_LEN:
        if (ValueLen != ItemLL) return (RC_FALSE);
        break;

     case ANY_LEN:
        if (ItemLL == 0) return (RC_FALSE);
        break;

     default:
        if (ValueLen != ItemLL) return (RC_FALSE);
        if (pValue != NULL)
           if (memcmp(pItem->pValue,pValue,ValueLen))
              return (RC_FALSE);
  }

                                    /* update current pointer of      */
                                    /*    datastream area and their   */
                                    /*    remaining free space        */
  *ppArea    += (ULONG)CONV_US(&(pItem->usLL));
  *pRemSpace -= (ULONG)CONV_US(&(pItem->usLL));

  return (RC_TRUE);
}                                   /* end of CheckDataStream         */



/***********************************************************************
*                                                                      *
* Entry point:  WriteDataStream                                        *
*                                                                      *
* Function:     Writes the given datastream item to the given space.   *
*                                                                      *
*               It is checked, whether it fits into the given          *
*               remaining buffer space (if yes, the remaining space    *
*               and its pointer are updated).                          *
*                                                                      *
*               Conversion of USHORT variables between Intel and       *
*               'Big-endian' formats is done within this function      *
*               for two-byte identifiers, lengths, and values using    *
*               CONV_US macro.                                         *
*                                                                      *
* Input parms:  1.Length of remaining space                            *
*               2.Remaining space                                      *
*               3.Item identifier                                      *
*               4.Item type                                            *
*               5.Length of item value (must be >= 0)                  *
*               6.Item value (optional)                                *
*                                                                      *
* Output parms: 1.Updated length of remaining space (for RC_TRUE)      *
*               2.Updated pointer to remaining space (for RC_TRUE)     *
*                                                                      *
* Return codes: Check ok           RC_TRUE                             *
*               Check not ok       RC_FALSE                            *
*                                                                      *
* Exits:        Always returns to caller.                              *
*                                                                      *
***********************************************************************/

ULONG WriteDataStream
 (
 PULONG pRemSpace,                  /* I/O: Pointer to remaining      */
                                    /*         space                  */
 PPCHAR ppArea,                     /* I/O: Pointer to free           */
                                    /*         datastream area        */
 USHORT ItemId,                     /* In:  Item identifier           */
 UCHAR  ItemIt,                     /* In:  Item type                 */
 USHORT ValueLen,                   /* In:  Length of item value      */
 PCHAR  pValue                      /* In:  Pointer to item value     */
 )

{
  USHORT ItemLL = 0;                /* total length of data item      */
  struct DataItem *pItem =          /* pointer to data stream item    */
     (struct DataItem *)*ppArea;
                                    /* calculate total item length    */
  ItemLL = OffsetOf(struct DataItem,pValue[0]) + ValueLen;
                                    /* check remaining length of      */
                                    /*    datastream area             */
  DMSG3("ItemLL = %i, *pRemSpace = %i\n",ItemLL,*pRemSpace);
  if ((ULONG)ItemLL > *pRemSpace)  return (RC_FALSE);
                                    /* put item ll/id/it to           */
                                    /*    datastream area             */
  pItem->usLL = CONV_US(&(ItemLL));
  pItem->usId = CONV_US(&(ItemId));
  pItem->It = ItemIt;
                                    /* if applicable, write item      */
                                    /*    value to datastream area    */
                                    /* Note: for the USHORT-type      */
                                    /*    values CCSID, format and    */
                                    /*    language a conversion to    */
                                    /*    Big-endian format is done   */
  if (ItemId == ID_CCSID || ItemId == ID_LANG || ItemId == ID_DOCF)
     *(USHORT *)pItem->pValue = CONV_US(pValue);
  else
     memcpy (pItem->pValue,pValue,ValueLen);

  *ppArea    += (ULONG)ItemLL;      /* update current pointer of      */
                                    /*    datastream area and their   */
  *pRemSpace -= (ULONG)ItemLL;      /*    remaining free space        */

  return (RC_TRUE);
}                                   /* end of WriteDataStream         */


/***********************************************************************
*                                                                      *
* Entry point:  TSConversion                                           *
*                                                                      *
* Function:     Converts the SQL timestamp datatype to the LS_DATIM    *
*               format used within the Library Services datastream.    *
*                                                                      *
* Input parms:  1.SQL timestamp                                        *
*               2.Pointer to space to receive the converted timestamp  *
*                                                                      *
* Output parms: 1.Converted timestamp in the LS_DATIM format           *
*                                                                      *
* Exits:        Always returns to caller.                              *
*                                                                      *
***********************************************************************/

ULONG TSConversion
 (
 FILEFINDBUF3  *pTimestamp,         /* In:  OS/2 timestamp            */
 PCHAR  pDTFormat                   /* I/O: pointer to converted      */
 )                                  /*         timestamp              */

{

/* convert year: -----------------------------------------------------*/

  pDTFormat[0] = (UCHAR)(((USHORT)pTimestamp->fdateLastWrite.year+1980)>>8);
  pDTFormat[1] = (UCHAR)(pTimestamp->fdateLastWrite.year+1980) & 0x00ff;

/* convert month: ----------------------------------------------------*/

  pDTFormat[2] = (UCHAR)pTimestamp->fdateLastWrite.month;

/* convert day: ------------------------------------------------------*/

  pDTFormat[3] = (UCHAR)pTimestamp->fdateLastWrite.day;

/* convert hours: ----------------------------------------------------*/

  pDTFormat[4] = (UCHAR)pTimestamp->ftimeLastWrite.hours;

/* convert minutes: --------------------------------------------------*/

  pDTFormat[5] = (USHORT)pTimestamp->ftimeLastWrite.minutes;

/* convert seconds: --------------------------------------------------*/

  pDTFormat[6] = (USHORT)pTimestamp->ftimeLastWrite.twosecs;

/* convert hundreds of seconds:                                       */

  pDTFormat[7] = (UCHAR)0;

  return RC_OK;

}
