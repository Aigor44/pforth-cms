/* @(#) pfcompil.h 96/12/18 1.11 */
 
#ifndef _pforth_compile_h
#define _pforth_compile_h
 
/***************************************************************
** Include file for PForth Compiler
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
***************************************************************/
 
#ifdef __cplusplus
extern "C" {
#endif
 
Err   ffPushInputStream( FileStream *InputFile );
ExecToken NameToToken( const ForthString *NFA );
FileStream * ffConvertSourceIDToStream( cell id );
FileStream *ffPopInputStream( void );
cell  ffConvertStreamToSourceID( FileStream *Stream );
cell  ffFind( const ForthString *WordName, ExecToken *pXT );
cell  ffFindC( const char *WordName, ExecToken *pXT );
cell  ffFindNFA( const ForthString *WordName, const ForthString **NFAPtr );
cell  ffIncludeFile( FileStream *InputFile );
cell  ffNumberQ( const char *FWord, cell *Num );
cell  ffRefill( void );
cell  ffTokenToName( ExecToken XT, const ForthString **NFAPtr );
cell *NameToCode( ForthString *NFA );
cfDictionary *pfBuildDictionary( int32 HeaderSize, int32 CodeSize );
char *ffWord( char c );
const ForthString *NameToPrevious( const ForthString *NFA );
int32 FindSpecialCFAs( void );
int32 FindSpecialXTs( void );
int32 NotCompiled( const char *FunctionName );
int32 ffInterpret( void );
void  CreateDicEntry( ExecToken XT, const ForthStringPtr FName, uint32 Flags );
void  CreateDicEntryC( ExecToken XT, const char *CName, uint32 Flags );
void  ff2Literal( cell dHi, cell dLo );
void  ffALiteral( cell Num );
void  ffAbort( void );
void  ffColon( void );
void  ffCreate( void );
void  ffCreateSecondaryHeader( const ForthStringPtr FName);
void  ffDefer( void );
void  ffFinishSecondary( void );
void  ffLiteral( cell Num );
void  ffOK( void );
void  ffQuit( void );
void  ffSemiColon( void );
void  ffStringCreate( ForthStringPtr FName);
void  ffStringDefer( const ForthStringPtr FName, ExecToken DefaultXT );
 
#ifdef PF_SUPPORT_FP
void ffFPLiteral( PF_FLOAT fnum );
#endif
 
#ifdef __cplusplus
}   
#endif
 
#endif /* _pforth_compile_h */
