/* @(#) pf_all.h 98/01/26 1.2 */
 
#ifndef _pf_all_h
#define _pf_all_h
 
/***************************************************************
** Include all files needed for PForth
**
** Author: Phil Burk
** Copyright 1994 3DO, Phil Burk, Larry Polansky, Devid Rosenboom
**
** The pForth software code is dedicated to the public domain,
** and any third party may reproduce, distribute and modify
** the pForth software code or any derivative works thereof
** without any compensation or license.  The pForth software
** code is provided on an "as is" basis without any warranty
** of any kind, including, without limitation, the implied
** warranties of merchantability and fitness for a particular
** purpose and their equivalents under the laws of any jurisdiction.
**
** 940521 PLB Creation.
**
***************************************************************/
 
/* I don't see any way to pass compiler flags to the Mac Code Warrior compiler! */
#ifdef __MWERKS__
 #define PF_USER_INC1     "pf_mac.h"
 #define PF_SUPPORT_FP    (1)
#endif
 
#ifdef __CMS__
    #define pfCharToUpper              pfC2U
    #define pfCharToLower              pfC2T
    #define gCurrentTask               gCTask
    #define gCurrentDictionary         gCDic
    #define gVarTraceLevel             gVTL
    #define gVarTraceStack             gVTS
    #define gVarTraceFlags             gVTF
    #define pfCreateTask               pfCTask
    #define pfCreateDictionary         pfCreDic
    #define pfDeleteDictionary         pfDelDic
    #define pfDeleteTask               pfDelTask
    #define ffCompareTextCaseN         ffCmpTextCaseN
    #define ffCompare                  ffCmp
    #define CreateDicEntry             CrDicEntry
    #define CreateDicEntryC            cCrDicEntry
    #define ffCreateSecondaryHeader    ffCrSecondaryHeader
    #define ffStringColon              ffStringColon
    #define ffStringCreate             ffStrCreate
    #define ffConvertStreamToSourceID  ff1ConvertStreamToSourceID
    #define ffConvertSourceIDToStream  ff2ConvertSourceIDToStream
    #define ReadLongBigEndian          Read1LongBigEndian
    #define ReadLongLittleEndian       Read2LongLittleEndian
    #define ReadShortBigEndian         Read1ShortBigEndian
    #define ReadShortLittleEndian      Read2ShortLittleEndian
    #define WriteFloatBigEndian        Write1FloatBigEndian
    #define WriteFloatLittleEndian     Write2FloatLittleEndian
    #define ReadFloatBigEndian         Read1FloatBigEndian
    #define ReadFloatLittleEndian      Read2FloatLittleEndian
    #define WriteLongBigEndian         Write1LongBigEndian
    #define WriteLongLittleEndian      Write2LongLittleEndian
    #define WriteShortBigEndian        Write1ShortBigEndian
    #define WriteShortLittleEndian     Write2ShortLittleEndian
 
    extern unsigned int __atoe(unsigned char c);
    extern unsigned int __etoa(unsigned char c);
#endif    
 
 
 
#ifdef WIN32
 #define PF_USER_INC2     "pf_win32.h"
#endif
 
 
#if defined(PF_USER_INC1)
 #include PF_USER_INC1
#else
/* Default to UNIX if no host speciied. */
 #include "pf_unix.h"
#endif
 
#include "pf_types.h"
#include "pf_io.h"
#include "pf_guts.h"
#include "pf_text.h"
#include "pfcompil.h"
#include "pf_clib.h"
#include "pf_words.h"
#include "pf_save.h"
#include "pf_mem.h"
#include "pf_cglue.h"
#include "pf_core.h"
 
#ifdef PF_USER_INC2
/* This could be used to undef and redefine macros. */
 #include PF_USER_INC2
#endif
 
#endif /* _pf_all_h */
 
