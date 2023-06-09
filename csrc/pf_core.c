/* @(#) pf_core.c 98/01/28 1.5 */
/***************************************************************
** Forth based on 'C'
**
** This file has the main entry points to the pForth library.
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
****************************************************************
** 940502 PLB Creation.
** 940505 PLB More macros.
** 940509 PLB Moved all stack handling into inner interpreter.
**        Added Create, Colon, Semicolon, HNumberQ, etc.
** 940510 PLB Got inner interpreter working with secondaries.
**        Added (LITERAL).   Compiles colon definitions.
** 940511 PLB Added conditionals, LITERAL, CREATE DOES>
** 940512 PLB Added DO LOOP DEFER, fixed R>
** 940520 PLB Added INCLUDE
** 940521 PLB Added NUMBER?
** 940930 PLB Outer Interpreter now uses deferred NUMBER?
** 941005 PLB Added ANSI locals, LEAVE, modularised
** 950320 RDG Added underflow checking for FP stack
** 970702 PLB Added STACK_SAFETY to FP stack size.
***************************************************************/
 
#include "pf_all.h"
 
/***************************************************************
** Global Data
***************************************************************/
 
cfTaskData   *gCurrentTask;
cfDictionary *gCurrentDictionary;
int32         gNumPrimitives;
char          gScratch[TIB_SIZE];
ExecToken     gLocalCompiler_XT;   /* custom compiler for local variables */
 
/* Depth of data stack when colon called. */
int32         gDepthAtColon;
 
/* Global Forth variables. */
char *gVarContext;      /* Points to last name field. */
cell  gVarState;        /* 1 if compiling. */
cell  gVarBase;         /* Numeric Base. */
cell  gVarEcho;            /* Echo input. */
cell  gVarTraceLevel;   /* Trace Level for Inner Interpreter. */
cell  gVarTraceStack;   /* Dump Stack each time if true. */
cell  gVarTraceFlags;   /* Enable various internal debug messages. */
cell  gVarQuiet;        /* Suppress unnecessary messages, OK, etc. */
cell  gVarReturnCode;   /* Returned to caller of Forth, eg. UNIX shell. */
 
#define DEFAULT_RETURN_DEPTH (512)
#define DEFAULT_USER_DEPTH (512)
#define DEFAULT_HEADER_SIZE (120000)
#define DEFAULT_CODE_SIZE (300000)
 
/* Initialize non-zero globals in a function to simplify loading on
 * embedded systems which may only support uninitialized data segments.
 */
void pfInitGlobals( void )
{
    gVarBase = 10;       
    gVarTraceStack = 1;  
    gDepthAtColon = DEPTH_AT_COLON_INVALID;
}
 
/***************************************************************
** Task Management
***************************************************************/
 
void pfDeleteTask( cfTaskData *cftd )
{
    FREE_VAR( cftd->td_ReturnLimit );
    FREE_VAR( cftd->td_StackLimit );
    pfFreeMem( cftd );
}
/* Allocate some extra cells to protect against mild stack underflows. */
#define STACK_SAFETY  (8)
cfTaskData *pfCreateTask( int32 UserStackDepth, int32 ReturnStackDepth )
{
    cfTaskData *cftd;
 
    cftd = ( cfTaskData * ) pfAllocMem( sizeof( cfTaskData ) );
    if( !cftd ) goto nomem;
    pfSetMemory( cftd, 0, sizeof( cfTaskData ));
 
/* Allocate User Stack */
    cftd->td_StackLimit = (cell *) pfAllocMem((uint32)(sizeof(int32) *
                (UserStackDepth + STACK_SAFETY)));
    if( !cftd->td_StackLimit ) goto nomem;
    cftd->td_StackBase = cftd->td_StackLimit + UserStackDepth;
    cftd->td_StackPtr = cftd->td_StackBase;
 
/* Allocate Return Stack */
    cftd->td_ReturnLimit = (cell *) pfAllocMem((uint32)(sizeof(int32) * ReturnStackDepth) );
    if( !cftd->td_ReturnLimit ) goto nomem;
    cftd->td_ReturnBase = cftd->td_ReturnLimit + ReturnStackDepth;
    cftd->td_ReturnPtr = cftd->td_ReturnBase;
 
/* Allocate Float Stack */
#ifdef PF_SUPPORT_FP
/* Allocate room for as many Floats as we do regular data. */
    cftd->td_FloatStackLimit = (PF_FLOAT *) pfAllocMem((uint32)(sizeof(PF_FLOAT) *
                (UserStackDepth + STACK_SAFETY)));
    if( !cftd->td_FloatStackLimit ) goto nomem;
    cftd->td_FloatStackBase = cftd->td_FloatStackLimit + UserStackDepth;
    cftd->td_FloatStackPtr = cftd->td_FloatStackBase;
#endif
 
    cftd->td_InputStream = PF_STDIN;
 
    cftd->td_SourcePtr = &cftd->td_TIB[0];
    cftd->td_SourceNum = 0;
    
    return cftd;
 
nomem:
    ERR("CreateTaskContext: insufficient memory.\n");
    if( cftd ) pfDeleteTask( cftd );
    return NULL;
}
 
/***************************************************************
** Dictionary Management
***************************************************************/
 
void pfExecByName( const char *CString )
{
    if( NAME_BASE != NULL)
    {
        ExecToken  autoInitXT;
        if( ffFindC( CString, &autoInitXT ) )
        {
            pfExecuteToken( autoInitXT );
        }
    }
}
 
/***************************************************************
** Delete a dictionary created by pfCreateDictionary()
*/
void pfDeleteDictionary( cfDictionary *dic )
{
    if( !dic ) return;
    
    if( dic->dic_Flags & PF_DICF_ALLOCATED_SEGMENTS )
    {
        FREE_VAR( dic->dic_HeaderBaseUnaligned );
        FREE_VAR( dic->dic_CodeBaseUnaligned );
    }
    pfFreeMem( dic );
}
 
/***************************************************************
** Create a complete dictionary.
** The dictionary consists of two parts, the header with the names,
** and the code portion.
** Delete using pfDeleteDictionary().
** Return pointer to dictionary management structure.
*/
cfDictionary *pfCreateDictionary( uint32 HeaderSize, uint32 CodeSize )
{
/* Allocate memory for initial dictionary. */
    cfDictionary *dic;
 
    dic = ( cfDictionary * ) pfAllocMem( sizeof( cfDictionary ) );
    if( !dic ) goto nomem;
    pfSetMemory( dic, 0, sizeof( cfDictionary ));
 
    dic->dic_Flags |= PF_DICF_ALLOCATED_SEGMENTS;
 
/* Align dictionary segments to preserve alignment of floats across hosts. */
#define DIC_ALIGNMENT_SIZE  (0x10)
#define DIC_ALIGN(addr)  ((uint8 *)((((uint32)(addr)) + DIC_ALIGNMENT_SIZE - 1) & ~(DIC_ALIGNMENT_SIZE - 1)))
 
/* Allocate memory for header. */
    if( HeaderSize > 0 )
    {
        dic->dic_HeaderBaseUnaligned = ( uint8 * ) pfAllocMem( (uint32) HeaderSize + DIC_ALIGNMENT_SIZE );
        if( !dic->dic_HeaderBaseUnaligned ) goto nomem;
/* Align header base. */
        dic->dic_HeaderBase = DIC_ALIGN(dic->dic_HeaderBaseUnaligned);
        pfSetMemory( dic->dic_HeaderBase, 0xA5, (uint32) HeaderSize);
        dic->dic_HeaderLimit = dic->dic_HeaderBase + HeaderSize;
        dic->dic_HeaderPtr.Byte = dic->dic_HeaderBase;
    }
    else
    {
        dic->dic_HeaderBase = NULL;
    }
 
/* Allocate memory for code. */
    dic->dic_CodeBaseUnaligned = ( uint8 * ) pfAllocMem( (uint32) CodeSize + DIC_ALIGNMENT_SIZE );
    if( !dic->dic_CodeBaseUnaligned ) goto nomem;
    dic->dic_CodeBase = DIC_ALIGN(dic->dic_CodeBaseUnaligned);
    pfSetMemory( dic->dic_CodeBase, 0x5A, (uint32) CodeSize);
 
    dic->dic_CodeLimit = dic->dic_CodeBase + CodeSize;
    dic->dic_CodePtr.Byte = dic->dic_CodeBase + QUADUP(NUM_PRIMITIVES); 
    
    return dic;
nomem:
    pfDeleteDictionary( dic );
    return NULL;
}
 
/***************************************************************
** Used by Quit and other routines to restore system.
***************************************************************/
 
void ResetForthTask( void )
{
/* Go back to terminal input. */
    gCurrentTask->td_InputStream = PF_STDIN;
    
/* Reset stacks. */
    gCurrentTask->td_StackPtr = gCurrentTask->td_StackBase;
    gCurrentTask->td_ReturnPtr = gCurrentTask->td_ReturnBase;
#ifdef PF_SUPPORT_FP  /* Reset Floating Point stack too! */
    gCurrentTask->td_FloatStackPtr = gCurrentTask->td_FloatStackBase;
#endif
 
/* Advance >IN to end of input. */
    gCurrentTask->td_IN = gCurrentTask->td_SourceNum;
    gVarState = 0;
}
 
/***************************************************************
** Set current task context.
***************************************************************/
 
void pfSetCurrentTask( cfTaskData *cftd )
{    
    gCurrentTask = cftd;
}
 
/***************************************************************
** Set Quiet Flag.
***************************************************************/
 
void pfSetQuiet( int32 IfQuiet )
{    
    gVarQuiet = (cell) IfQuiet;
}
 
/***************************************************************
** Query message status.
***************************************************************/
 
int32  pfQueryQuiet( void )
{    
    return gVarQuiet;
}
 
/***************************************************************
** RunForth
***************************************************************/
 
int32 pfRunForth( void )
{
    ffQuit();
    return gVarReturnCode;
}
 
/***************************************************************
** Include file based on 'C' name.
***************************************************************/
 
int32 pfIncludeFile( const char *FileName )
{
    FileStream *fid;
    int32 Result;
    char  buffer[32];
    int32 numChars, len;
    
/* Open file. */
    fid = sdOpenFile( FileName, "r" );
    if( fid == NULL )
    {
        ERR("pfIncludeFile could not open ");
        ERR(FileName);
        EMIT_CR;
        return -1;
    }
    
/* Create a dictionary word named ::::FileName for FILE? */
    pfCopyMemory( &buffer[0], "::::", 4);
    len = pfCStringLength(FileName);
    numChars = ( len > (32-4-1) ) ? (32-4-1) : len;
    pfCopyMemory( &buffer[4], &FileName[len-numChars], numChars+1 );
    CreateDicEntryC( ID_NOOP, buffer, 0 );
    
    Result = ffIncludeFile( fid );
    
/* Create a dictionary word named ;;;; for FILE? */
    CreateDicEntryC( ID_NOOP, ";;;;", 0 );
    
    sdCloseFile(fid);
    return Result;
}
 
/***************************************************************
** Output 'C' string message.
** This is provided to help avoid the use of printf() and other I/O
** which may not be present on a small embedded system.
***************************************************************/
 
void pfMessage( const char *CString )
{
#ifdef __CMS__
    ioTypeE( CString, pfCStringLength(CString) );
#else
    ioType( CString, pfCStringLength(CString) );
#endif
}
 
/**************************************************************************
** Main entry point fo pForth
*/
int32 pfDoForth( const char *DicName, const char *SourceName, int32 IfInit )
{
    cfTaskData *cftd;
    cfDictionary *dic;
    int32 Result = 0;
    ExecToken  EntryPoint = 0;
    
#ifdef PF_USER_INIT
    Result = PF_USER_INIT;
    if( Result < 0 ) goto error;
#endif
 
    pfInitGlobals();
    
/* Allocate Task structure. */
    cftd = pfCreateTask( DEFAULT_USER_DEPTH, DEFAULT_RETURN_DEPTH );
 
    if( cftd )
    {
        pfSetCurrentTask( cftd );
        
        if( !pfQueryQuiet() )
        {
            MSG( "PForth V"PFORTH_VERSION"\n" );
        }
 
#if 0
/* Don't use MSG before task set. */
        if( IfInit ) MSG("Build dictionary from scratch.\n");
    
        if( DicName )
        {
            MSG("DicName = "); MSG(DicName); MSG("\n");
        }
        if( SourceName )
        {
            MSG("SourceName = "); MSG(SourceName); MSG("\n");
        }
#endif
 
 
#ifdef PF_NO_GLOBAL_INIT
        if( LoadCustomFunctionTable() < 0 ) goto error; /* Init custom 'C' call array. */
#endif
 
#if (!defined(PF_NO_INIT)) && (!defined(PF_NO_SHELL))
        if( IfInit )
        {
            dic = pfBuildDictionary( DEFAULT_HEADER_SIZE, DEFAULT_CODE_SIZE );
        }
        else
#else
    TOUCH(IfInit);
#endif /* !PF_NO_INIT && !PF_NO_SHELL*/
        {
            dic = pfLoadDictionary( DicName, &EntryPoint );
        }
        if( dic == NULL ) goto error;
        
        pfExecByName("AUTO.INIT");
 
        if( EntryPoint != 0 )
        {
            pfExecuteToken( EntryPoint );
        }
#ifndef PF_NO_SHELL
        else
        {
            if( SourceName == NULL )
            {
                Result = pfRunForth();
            }
            else
            {
                MSG("Including: ");
                MSG(SourceName);
                MSG("\n");
                Result = pfIncludeFile( SourceName );
            }
        }
#endif /* PF_NO_SHELL */
        pfExecByName("AUTO.TERM");
        pfDeleteDictionary( dic );
        pfDeleteTask( cftd );
    }
    
#ifdef PF_USER_TERM
    PF_USER_TERM;
#endif
 
    return Result;
    
error:
    MSG("pfDoForth: Error occured.\n");
    pfDeleteTask( cftd );
    return -1;
}
