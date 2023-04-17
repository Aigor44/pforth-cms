/* @(#) pforth.h 98/01/26 1.2 */
#ifndef _pforth_h
#define _pforth_h
 
/***************************************************************
** Include file for pForth, a portable Forth based on 'C'
**
** This file is included in any application that uses pForth as a tool.
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
**
***************************************************************/
 
/* Define stubs for data types so we can pass pointers but not touch inside. */
typedef struct cfTaskData   cfTaskData;
typedef struct cfDictionary cfDictionary;
 
typedef unsigned long ExecToken;              /* Execution Token */
 
#ifndef int32
	typedef long int32;
#endif
 
#ifdef __cplusplus
extern "C" {
#endif
 
/* Main entry point to pForth. */
int32 pfDoForth( const char *DicName, const char *SourceName, int32 IfInit );
 
/* Turn off messages. */
void  pfSetQuiet( int32 IfQuiet );
 
/* Query message status. */
int32  pfQueryQuiet( void );
 
/* Send a message using low level I/O of pForth */
void  pfMessage( const char *CString );
 
/* Create a task used to maintain context of execution. */
cfTaskData *pfCreateTask( int32 UserStackDepth, int32 ReturnStackDepth );
 
/* Establish this task as the current task. */
void  pfSetCurrentTask( cfTaskData *cftd );
 
/* Delete task created by pfCreateTask */
void  pfDeleteTask( cfTaskData *cftd );
 
/* Build a dictionary with all the basic kernel words. */
cfDictionary *pfBuildDictionary( int32 HeaderSize, int32 CodeSize );
 
/* Create an empty dictionary. */
cfDictionary *pfCreateDictionary( int32 HeaderSize, int32 CodeSize );
 
/* Load dictionary from a file. */
cfDictionary *pfLoadDictionary( const char *FileName, ExecToken *EntryPointPtr );
 
/* Delete dictionary data. */
void  pfDeleteDictionary( cfDictionary *dic );
 
/* Execute the pForth interpreter. */
int32   pfRunForth( void );
 
/* Execute a single execution token in the current task. */
void pfExecuteToken( ExecToken XT );
 
/* Include the given pForth source code file. */
int32   pfIncludeFile( const char *FileName );
 
/* Execute a Forth word by name. */
void   pfExecByName( const char *CString );
 
#ifdef __cplusplus
}   
#endif
 
#endif  /* _pforth_h */
