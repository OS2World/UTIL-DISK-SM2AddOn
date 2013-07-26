/*====================================================================*/
/*                                                                    */
/*  Library Services:  Sample Definitions                             */
/*                                                                    */
/*====================================================================*/
/*                                                                    */
/*  Name:      SMPLSGRD.H                                             */
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
#if !defined(SMPLSGRD_DEFINED)      /* definitions already included ? */
#define SMPLSGRD_DEFINED            /* if not, include them           */

/* 	$Id: smplsgrd.h,v 1.7 1998/03/17 18:53:50 siebert Exp $	 */

#ifndef lint
static char vcid_smplsgrd_h[] = "$Id: smplsgrd.h,v 1.7 1998/03/17 18:53:50 siebert Exp $";
#endif /* lint */

#include <stdio.h>

/* General defaults --------------------------------------------------*/

#define DefaultCCSID       4946     /* PC Data-190: Latin-1 (CP850)   */
#define DefaultLanguage    6011     /* US English                     */
#define DefaultFormat    0x000E     /* Plain ASCII                    */


/* General constants -------------------------------------------------*/

#define LSLenEyeCatcher       8     /* length of eyecatcher fields    */

#define LSLenDocColumn       _MAX_PATH     /* length of 'document' column    */

#define LSLenDocGroupColumn  _MAX_PATH     /* length of 'document group'     */
                                    /*    column                      */

#define LSLenDocInfo         21     /* minimal length of document     */
                                    /*    information datastream      */

#define LSLenLLIdIt           5     /* length of datastream part      */
                                    /*    LL, Id, It                  */

#define LSLenATIDItem         7     /* length of Attribute identifier */
                                    /*    datastream item             */

#define LSLenDocGroupItem    (5+_MAX_PATH)     /* length of document group       */
                                    /*    datastream item:            */
                                    /* ( 5+30(value) )                */

#define LSLenDocItem         (18+_MAX_PATH)     /* length of document item plus   */
                                    /*    date/time requested item:   */
                                    /* 5+30 + 5+8                     */

#define LSLenDocAttrItem     (35+(3*_MAX_PATH))     /* length of document item plus   */
                                    /*    attribute identifier item   */
                                    /*    plus date/time item:        */
                                    /* 35 + 7 + 13                    */

#define LSLenDDTItem         13     /* length of document's date/time */
                                    /*    item:                       */
                                    /* 5 + 8                          */

/* #define LSLenTimestamp       26 */    /* length of SQL Timestamp data   */
                                    /*    type                        */


/* Library Services sample anchor structure --------------------------*/

                                    /* LS Sample anchor structure     */
                                    /*   (allocated by LIB_init and   */
                                    /*   passed to all other Library  */
                                    /*   Services - just an example   */
                                    /*   for such an area w/o any     */
struct LSData {                     /*   data to be passed)           */
                                    /*   eyecatcher  "LSDATA  "       */
   UCHAR LSDataEyecatcher[LSLenEyeCatcher];
   UCHAR date[8];
   FILE* fh;
   UCHAR fname[_MAX_PATH];
   };                               /*                                */


/* Database Manager definitions --------------------------------------*/

#define LSDatabase  "IBMSM2"        /* Default database name          */


#endif                              /* end of include switch          */
