/*====================================================================*/
/*                                                                    */
/*  Library Services:  Datastream Handling                            */
/*                                                                    */
/*                     (definitions, declarations, prototype          */
/*                     statements)                                    */
/*                                                                    */
/*====================================================================*/
/*                                                                    */
/*  Name:      SMPLSDSH.H                                             */
/*  -----                                                             */
/*====================================================================*/
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
/*====================================================================*/
#if !defined(SMPLSDSH_DEFINED)      /* definitions already included ? */
#define SMPLSDSH_DEFINED            /* if not, include them           */

/* 	$Id: smplsdsh.h,v 1.4 1998/02/24 18:05:19 siebert Exp $	 */

#ifndef lint
static char vcid_smplsdsh_h[] = "$Id: smplsdsh.h,v 1.4 1998/02/24 18:05:19 siebert Exp $";
#endif /* lint */

/* Definitions -------------------------------------------------------*/

#define RC_TRUE         1L
#define RC_FALSE        0L

#define ANY_LEN     65535           /* x'ffff'                        */
#define ZERO_LEN        0

#define OffsetOf(s,m) (USHORT)&(((s *)0)->m)

#if defined(__OS2__) || defined(_M_I386)
   #define  CONV_US(ptr)  (USHORT)                         \
                          (((UCHAR *)(ptr))[0] * 256 +     \
                          ((UCHAR *)(ptr))[1])
 #else
   #define  CONV_US(ptr)  *((USHORT *)(ptr))
#endif

typedef PCHAR *PPCHAR;


/* Declarations ------------------------------------------------------*/

struct DataItem {                   /* Datastream Item                */
  USHORT usLL;                      /*    item length                 */
  USHORT usId;                      /*    item identifier             */
  UCHAR  It;                        /*    item type                   */
  UCHAR  pValue[1];                 /*    item value                  */
  };


/* Prototypes (Library Services datastream handling)------------------*/

ULONG WriteDataStream
 (
 PULONG pRemSpace,                  /* I/O: Pointer to remaining space*/
 PPCHAR ppArea,                     /* I/O: Pointer to free           */
                                    /*         datastream area        */
 USHORT usItemId,                   /* In:  Item identifier           */
 UCHAR  ItemIt,                     /* In:  Item type                 */
 USHORT usValueLen,                 /* In:  Length of item value      */
 PCHAR  pValue                      /* In:  Pointer to item value     */
 );

ULONG CheckDataStream
 (
 PULONG pRemSpace,                  /* I/O: Pointer to available space*/
 PPCHAR ppArea,                     /* I/O: Pointer to datastream     */
                                    /*         item to be checked     */
 USHORT usItemId,                   /* In:  Item identifier           */
 UCHAR  ItemIt,                     /* In:  Item type                 */
 USHORT usValueLen,                 /* In:  Length of item value      */
 PCHAR  pValue                      /* In:  Pointer to item value     */
 );

ULONG TSConversion
 (
 FILEFINDBUF3 *pTimestamp,                 /* In:  Pointer to SQL timestamp  */
 PCHAR  pDTFormat                   /* I/O: Pointer to converted      */
 );                                 /*         timestamp              */

#endif                              /* end of include switch          */
