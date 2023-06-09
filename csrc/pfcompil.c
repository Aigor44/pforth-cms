/* @(#) pfcompil.c 98/01/26 1.5 */
/***************************************************************
** Compiler for PForth based on 'C'
**
** These routines could be left out of an execute only version.
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
** 941004 PLB Extracted IO calls from pforth_main.c
** 950320 RDG Added underflow checking for FP stack
***************************************************************/
 
#include "pf_all.h"
#include "pfcompil.h"
 
#ifdef __SYSC__
#    include <limits.h>
#    include <machine/atoe.h>
#endif
 
#define ABORT_RETURN_CODE   (10)
 
/***************************************************************/
/************** GLOBAL DATA ************************************/
/***************************************************************/
/* data for INCLUDE that allows multiple nested files. */
static IncludeFrame gIncludeStack[MAX_INCLUDE_DEPTH];
static int32 gIncludeIndex;
 
static ExecToken gNumberQ_XT;         /* XT of NUMBER? */
static ExecToken gQuitP_XT;           /* XT of (QUIT) */
 
/***************************************************************/
/************** Static Prototypes ******************************/
/***************************************************************/
 
static void ffStringColon( const ForthStringPtr FName );
static int32 CheckRedefinition( const ForthStringPtr FName );
static void ReportIncludeState( void );
static void ffUnSmudge( void );
static void FindAndCompile( const char * theWord );
static int32 ffCheckDicRoom( void );
static void ffCleanIncludeStack( void );
 
#ifndef PF_NO_INIT
 static void CreateDeferredC( ExecToken DefaultXT, const char *CName );
#endif
 
int32 NotCompiled( const char *FunctionName )
{
 MSG("Function ");
 MSG(FunctionName);
 MSG(" not compiled in this version of PForth.\n");
 return -1;
}
 
#ifndef PF_NO_SHELL
/***************************************************************
** Create an entry in the Dictionary for the given ExecutionToken.
** FName is name in Forth format.
*/
void CreateDicEntry( ExecToken XT, const ForthStringPtr FName, uint32 Flags )
{
    cfNameLinks * cfnl = (cfNameLinks *) gCurrentDictionary->dic_HeaderPtr.Byte;
 
    /* Set link to previous header, if any. */
    if ( gVarContext ) {
        WRITE_LONG_DIC( &cfnl->cfnl_PreviousName, ABS_TO_NAMEREL( gVarContext ) );
    } else {
        cfnl->cfnl_PreviousName = 0;
    } /*i*/
 
    /* Put Execution token in header. */
    WRITE_LONG_DIC( &cfnl->cfnl_ExecToken, XT );
 
    /* Advance Header Dictionary Pointer */
    gCurrentDictionary->dic_HeaderPtr.Byte += sizeof(cfNameLinks);
 
    /* Laydown name. */
    gVarContext = (char *) gCurrentDictionary->dic_HeaderPtr.Byte;
    pfCopyMemory( gCurrentDictionary->dic_HeaderPtr.Byte, FName, (*FName)+1 );
#ifdef __SYSC__
    size_t NameLen = (size_t) * FName;
    for ( int i = 0 ; i < NameLen ; i++ )
        gCurrentDictionary->dic_HeaderPtr.Byte[i+1] = __etoa( FName[i+1] ); /* EBCDIC to ASCII conversion */
#elif __CMS__
    size_t NameLen = (size_t) * FName;
    for ( int i = 0 ; i < NameLen ; i++ )
        gCurrentDictionary->dic_HeaderPtr.Byte[i+1] = __etoa( FName[i+1] ); /* EBCDIC to ASCII conversion */
#endif
    gCurrentDictionary->dic_HeaderPtr.Byte += (*FName)+1;
 
    /* Set flags. */
    * gVarContext |= (char) Flags;
 
    /* Align to quad byte boundaries with zeroes. */
    while ( ((uint32) gCurrentDictionary->dic_HeaderPtr.Byte) & 3 )
        * gCurrentDictionary->dic_HeaderPtr.Byte++ = 0;
}
 
/***************************************************************
** Convert name then create dictionary entry.
*/
void CreateDicEntryC( ExecToken XT, const char *CName, uint32 Flags )
{
    ForthString FName[40];
    CStringToForth( FName, CName );
    CreateDicEntry( XT, FName, Flags );
}
 
/***************************************************************
** Convert absolute namefield address to previous absolute name
** field address or NULL.
*/
const ForthString *NameToPrevious( const ForthString *NFA )
{
 cell RelNamePtr;
 const cfNameLinks *cfnl;
 
/* DBUG(("\nNameToPrevious: NFA = 0x%x\n", (int32) NFA)); */
 cfnl = (const cfNameLinks *) ( ((const char *) NFA) - sizeof(cfNameLinks) );
 
 RelNamePtr = READ_LONG_DIC((const cell *) (&cfnl->cfnl_PreviousName));
/* DBUG(("\nNameToPrevious: RelNamePtr = 0x%x\n", (int32) RelNamePtr )); */
 if( RelNamePtr )
 {
  return ( NAMEREL_TO_ABS( RelNamePtr ) );
 }
 else
 {
  return NULL;
 }
}
/***************************************************************
** Convert NFA to ExecToken.
*/
ExecToken NameToToken( const ForthString *NFA )
{
 const cfNameLinks *cfnl;
 
/* Convert absolute namefield address to absolute link field address. */
 cfnl = (const cfNameLinks *) ( ((const char *) NFA) - sizeof(cfNameLinks) );
 
 return READ_LONG_DIC((const cell *) (&cfnl->cfnl_ExecToken));
}
 
/***************************************************************
** Find XTs needed by compiler.
*/
int32 FindSpecialXTs( void )
{
    if( ffFindC( "(QUIT)", &gQuitP_XT ) == 0) goto nofind;
    if( ffFindC( "NUMBER?", &gNumberQ_XT ) == 0) goto nofind;
    DBUG(("gNumberQ_XT = 0x%x\n", gNumberQ_XT ));
    return 0;
nofind:
    ERR("FindSpecialXTs failed!\n");
    return -1;
}
 
/***************************************************************
** Build a dictionary from scratch.
*/
#ifndef PF_NO_INIT
cfDictionary *pfBuildDictionary( int32 HeaderSize, int32 CodeSize )
{
 cfDictionary *dic;
 
 dic = pfCreateDictionary( HeaderSize, CodeSize );
 if( !dic ) goto nomem;
 
 gCurrentDictionary = dic;
 gNumPrimitives = NUM_PRIMITIVES;
 
 CreateDicEntryC( ID_EXIT, "EXIT", 0 );
 CreateDicEntryC( ID_1MINUS, "1-", 0 );
 CreateDicEntryC( ID_1PLUS, "1+", 0 );
 CreateDicEntryC( ID_2_R_FETCH, "2R@", 0 );
 CreateDicEntryC( ID_2_R_FROM, "2R>", 0 );
 CreateDicEntryC( ID_2_TO_R, "2>R", 0 );
 CreateDicEntryC( ID_2DUP, "2DUP", 0 );
 CreateDicEntryC( ID_2LITERAL, "2LITERAL", FLAG_IMMEDIATE );
 CreateDicEntryC( ID_2LITERAL_P, "(2LITERAL)", 0 );
 CreateDicEntryC( ID_2MINUS, "2-", 0 );
 CreateDicEntryC( ID_2PLUS, "2+", 0 );
 CreateDicEntryC( ID_2OVER, "2OVER", 0 );
 CreateDicEntryC( ID_2SWAP, "2SWAP", 0 );
 CreateDicEntryC( ID_ACCEPT, "ACCEPT", 0 );
 CreateDicEntryC( ID_ALITERAL, "ALITERAL", FLAG_IMMEDIATE );
 CreateDicEntryC( ID_ALITERAL_P, "(ALITERAL)", 0 );
 CreateDicEntryC( ID_ALLOCATE, "ALLOCATE", 0 );
 CreateDicEntryC( ID_ARSHIFT, "ARSHIFT", 0 );
 CreateDicEntryC( ID_AND, "AND", 0 );
 CreateDicEntryC( ID_BAIL, "BAIL", 0 );
 CreateDicEntryC( ID_BL, "BL", 0 );
 CreateDicEntryC( ID_BRANCH, "BRANCH", 0 );
 CreateDicEntryC( ID_BODY_OFFSET, "BODY_OFFSET", 0 );
 CreateDicEntryC( ID_BYE, "BYE", 0 );
 CreateDicEntryC( ID_CFETCH, "C@", 0 );
 CreateDicEntryC( ID_CMOVE, "CMOVE", 0 );
 CreateDicEntryC( ID_CMOVE_UP, "CMOVE>", 0 );
 CreateDicEntryC( ID_COLON, ":", 0 );
 CreateDicEntryC( ID_COLON_P, "(:)", 0 );
 CreateDicEntryC( ID_COMPARE, "COMPARE", 0 );
 CreateDicEntryC( ID_COMP_EQUAL, "=", 0 );
 CreateDicEntryC( ID_COMP_NOT_EQUAL, "<>", 0 );
 CreateDicEntryC( ID_COMP_GREATERTHAN, ">", 0 );
 CreateDicEntryC( ID_COMP_U_GREATERTHAN, "U>", 0 );
 CreateDicEntryC( ID_COMP_LESSTHAN, "<", 0 );
 CreateDicEntryC( ID_COMP_U_LESSTHAN, "U<", 0 );
 CreateDicEntryC( ID_COMP_ZERO_EQUAL, "0=", 0 );
 CreateDicEntryC( ID_COMP_ZERO_NOT_EQUAL, "0<>", 0 );
 CreateDicEntryC( ID_COMP_ZERO_GREATERTHAN, "0>", 0 );
 CreateDicEntryC( ID_COMP_ZERO_LESSTHAN, "0<", 0 );
 CreateDicEntryC( ID_CR, "CR", 0 );
 CreateDicEntryC( ID_CREATE, "CREATE", 0 );
 CreateDicEntryC( ID_CREATE_P, "(CREATE)", 0 );
 CreateDicEntryC( ID_D_PLUS, "D+", 0 );
 CreateDicEntryC( ID_D_MINUS, "D-", 0 );
 CreateDicEntryC( ID_D_UMSMOD, "UM/MOD", 0 );
 CreateDicEntryC( ID_D_MUSMOD, "MU/MOD", 0 );
 CreateDicEntryC( ID_D_MTIMES, "M*", 0 );
 CreateDicEntryC( ID_D_UMTIMES, "UM*", 0 );
 CreateDicEntryC( ID_DEFER, "DEFER", 0 );
 CreateDicEntryC( ID_CSTORE, "C!", 0 );
 CreateDicEntryC( ID_DEPTH, "DEPTH",  0 );
 CreateDicEntryC( ID_DIVIDE, "/", 0 );
 CreateDicEntryC( ID_DOT, ".",  0 );
 CreateDicEntryC( ID_DOTS, ".S",  0 );
 CreateDicEntryC( ID_DO_P, "(DO)", 0 );
 CreateDicEntryC( ID_DROP, "DROP", 0 );
 CreateDicEntryC( ID_DUMP, "DUMP", 0 );
 CreateDicEntryC( ID_DUP, "DUP",  0 );
 CreateDicEntryC( ID_EMIT_P, "(EMIT)",  0 );
 CreateDeferredC( ID_EMIT_P, "EMIT");
 CreateDicEntryC( ID_EOL, "EOL",  0 );
 CreateDicEntryC( ID_ERRORQ_P, "(?ERROR)",  0 );
 CreateDicEntryC( ID_ERRORQ_P, "?ERROR",  0 );
 CreateDicEntryC( ID_EXECUTE, "EXECUTE",  0 );
 CreateDicEntryC( ID_FETCH, "@",  0 );
 CreateDicEntryC( ID_FILL, "FILL", 0 );
 CreateDicEntryC( ID_FIND, "FIND",  0 );
 CreateDicEntryC( ID_FILE_CREATE, "CREATE-FILE",  0 );
 CreateDicEntryC( ID_FILE_OPEN, "OPEN-FILE",  0 );
 CreateDicEntryC( ID_FILE_CLOSE, "CLOSE-FILE",  0 );
 CreateDicEntryC( ID_FILE_READ, "READ-FILE",  0 );
 CreateDicEntryC( ID_FILE_SIZE, "FILE-SIZE",  0 );
 CreateDicEntryC( ID_FILE_WRITE, "WRITE-FILE",  0 );
 CreateDicEntryC( ID_FILE_POSITION, "FILE-POSITION",  0 );
 CreateDicEntryC( ID_FILE_REPOSITION, "REPOSITION-FILE",  0 );
 CreateDicEntryC( ID_FILE_RO, "R/O",  0 );
 CreateDicEntryC( ID_FILE_RW, "R/W",  0 );
 CreateDicEntryC( ID_FINDNFA, "FINDNFA",  0 );
 CreateDicEntryC( ID_FLUSHEMIT, "FLUSHEMIT",  0 );
 CreateDicEntryC( ID_FREE, "FREE",  0 );
#include "pfcompfp.h"
 CreateDicEntryC( ID_HERE, "HERE",  0 );
 CreateDicEntryC( ID_NUMBERQ_P, "(SNUMBER?)",  0 );
 CreateDicEntryC( ID_I, "I",  0 );
 CreateDicEntryC( ID_J, "J",  0 );
 CreateDicEntryC( ID_INCLUDE_FILE, "INCLUDE-FILE",  0 );
 CreateDicEntryC( ID_KEY, "KEY",  0 );
 CreateDicEntryC( ID_LEAVE_P, "(LEAVE)", 0 );
 CreateDicEntryC( ID_LITERAL, "LITERAL", FLAG_IMMEDIATE );
 CreateDicEntryC( ID_LITERAL_P, "(LITERAL)", 0 );
 CreateDicEntryC( ID_LOADSYS, "LOADSYS", 0 );
 CreateDicEntryC( ID_LOCAL_COMPILER, "LOCAL-COMPILER", 0 );
 CreateDicEntryC( ID_LOCAL_ENTRY, "(LOCAL.ENTRY)", 0 );
 CreateDicEntryC( ID_LOCAL_EXIT, "(LOCAL.EXIT)", 0 );
 CreateDicEntryC( ID_LOCAL_FETCH, "(LOCAL@)", 0 );
 CreateDicEntryC( ID_LOCAL_FETCH_1, "(1_LOCAL@)", 0 );
 CreateDicEntryC( ID_LOCAL_FETCH_2, "(2_LOCAL@)", 0 );
 CreateDicEntryC( ID_LOCAL_FETCH_3, "(3_LOCAL@)", 0 );
 CreateDicEntryC( ID_LOCAL_FETCH_4, "(4_LOCAL@)", 0 );
 CreateDicEntryC( ID_LOCAL_FETCH_5, "(5_LOCAL@)", 0 );
 CreateDicEntryC( ID_LOCAL_FETCH_6, "(6_LOCAL@)", 0 );
 CreateDicEntryC( ID_LOCAL_FETCH_7, "(7_LOCAL@)", 0 );
 CreateDicEntryC( ID_LOCAL_FETCH_8, "(8_LOCAL@)", 0 );
 CreateDicEntryC( ID_LOCAL_STORE, "(LOCAL!)", 0 );
 CreateDicEntryC( ID_LOCAL_STORE_1, "(1_LOCAL!)", 0 );
 CreateDicEntryC( ID_LOCAL_STORE_2, "(2_LOCAL!)", 0 );
 CreateDicEntryC( ID_LOCAL_STORE_3, "(3_LOCAL!)", 0 );
 CreateDicEntryC( ID_LOCAL_STORE_4, "(4_LOCAL!)", 0 );
 CreateDicEntryC( ID_LOCAL_STORE_5, "(5_LOCAL!)", 0 );
 CreateDicEntryC( ID_LOCAL_STORE_6, "(6_LOCAL!)", 0 );
 CreateDicEntryC( ID_LOCAL_STORE_7, "(7_LOCAL!)", 0 );
 CreateDicEntryC( ID_LOCAL_STORE_8, "(8_LOCAL!)", 0 );
 CreateDicEntryC( ID_LOCAL_PLUSSTORE, "(LOCAL+!)", 0 );
 CreateDicEntryC( ID_LOOP_P, "(LOOP)", 0 );
 CreateDicEntryC( ID_LSHIFT, "LSHIFT", 0 );
 CreateDicEntryC( ID_MAX, "MAX", 0 );
 CreateDicEntryC( ID_MIN, "MIN", 0 );
 CreateDicEntryC( ID_MINUS, "-", 0 );
 CreateDicEntryC( ID_NAME_TO_TOKEN, "NAME>", 0 );
 CreateDicEntryC( ID_NAME_TO_PREVIOUS, "PREVNAME", 0 );
 CreateDicEntryC( ID_NOOP, "NOOP", 0 );
 CreateDeferredC( ID_NUMBERQ_P, "NUMBER?" );
 CreateDicEntryC( ID_OR, "OR", 0 );
 CreateDicEntryC( ID_OVER, "OVER", 0 );
 CreateDicEntryC( ID_PICK, "PICK",  0 );
 CreateDicEntryC( ID_PLUS, "+",  0 );
 CreateDicEntryC( ID_PLUSLOOP_P, "(+LOOP)", 0 );
 CreateDicEntryC( ID_PLUS_STORE, "+!",  0 );
 CreateDicEntryC( ID_QUIT_P, "(QUIT)",  0 );
 CreateDeferredC( ID_QUIT_P, "QUIT" );
 CreateDicEntryC( ID_QDO_P, "(?DO)", 0 );
 CreateDicEntryC( ID_QDUP, "?DUP",  0 );
 CreateDicEntryC( ID_QTERMINAL, "?TERMINAL",  0 );
 CreateDicEntryC( ID_QTERMINAL, "KEY?",  0 );
 CreateDicEntryC( ID_REFILL, "REFILL",  0 );
 CreateDicEntryC( ID_RESIZE, "RESIZE",  0 );
 CreateDicEntryC( ID_ROLL, "ROLL",  0 );
 CreateDicEntryC( ID_ROT, "ROT",  0 );
 CreateDicEntryC( ID_RSHIFT, "RSHIFT",  0 );
 CreateDicEntryC( ID_R_DROP, "RDROP",  0 );
 CreateDicEntryC( ID_R_FETCH, "R@",  0 );
 CreateDicEntryC( ID_R_FROM, "R>",  0 );
 CreateDicEntryC( ID_RP_FETCH, "RP@",  0 );
 CreateDicEntryC( ID_RP_STORE, "RP!",  0 );
 CreateDicEntryC( ID_SEMICOLON, ";",  FLAG_IMMEDIATE );
 CreateDicEntryC( ID_SP_FETCH, "SP@",  0 );
 CreateDicEntryC( ID_SP_STORE, "SP!",  0 );
 CreateDicEntryC( ID_STORE, "!",  0 );
 CreateDicEntryC( ID_SAVE_FORTH_P, "(SAVE-FORTH)",  0 );
 CreateDicEntryC( ID_SCAN, "SCAN",  0 );
 CreateDicEntryC( ID_SKIP, "SKIP",  0 );
 CreateDicEntryC( ID_SOURCE, "SOURCE",  0 );
 CreateDicEntryC( ID_SOURCE_SET, "SET-SOURCE",  0 );
 CreateDicEntryC( ID_SOURCE_ID, "SOURCE-ID",  0 );
 CreateDicEntryC( ID_SOURCE_ID_PUSH, "PUSH-SOURCE-ID",  0 );
 CreateDicEntryC( ID_SOURCE_ID_POP, "POP-SOURCE-ID",  0 );
 CreateDicEntryC( ID_SWAP, "SWAP",  0 );
 CreateDicEntryC( ID_TEST1, "TEST1",  0 );
 CreateDicEntryC( ID_TICK, "'", 0 );
 CreateDicEntryC( ID_TIMES, "*", 0 );
 CreateDicEntryC( ID_TO_R, ">R", 0 );
 CreateDicEntryC( ID_TYPE, "TYPE", 0 );
 CreateDicEntryC( ID_VAR_BASE, "BASE", 0 );
 CreateDicEntryC( ID_VAR_CODE_BASE, "CODE-BASE", 0 );
 CreateDicEntryC( ID_VAR_CODE_LIMIT, "CODE-LIMIT", 0 );
 CreateDicEntryC( ID_VAR_CONTEXT, "CONTEXT", 0 );
 CreateDicEntryC( ID_VAR_DP, "DP", 0 );
 CreateDicEntryC( ID_VAR_ECHO, "ECHO", 0 );
 CreateDicEntryC( ID_VAR_HEADERS_PTR, "HEADERS-PTR", 0 );
 CreateDicEntryC( ID_VAR_HEADERS_BASE, "HEADERS-BASE", 0 );
 CreateDicEntryC( ID_VAR_HEADERS_LIMIT, "HEADERS-LIMIT", 0 );
 CreateDicEntryC( ID_VAR_NUM_TIB, "#TIB", 0 );
 CreateDicEntryC( ID_VAR_RETURN_CODE, "RETURN-CODE", 0 );
 CreateDicEntryC( ID_VAR_TRACE_FLAGS, "TRACE-FLAGS", 0 );
 CreateDicEntryC( ID_VAR_TRACE_LEVEL, "TRACE-LEVEL", 0 );
 CreateDicEntryC( ID_VAR_TRACE_STACK, "TRACE-STACK", 0 );
 CreateDicEntryC( ID_VAR_OUT, "OUT", 0 );
 CreateDicEntryC( ID_VAR_STATE, "STATE", 0 );
 CreateDicEntryC( ID_VAR_TO_IN, ">IN", 0 );
 CreateDicEntryC( ID_VLIST, "VLIST", 0 );
 CreateDicEntryC( ID_WORD, "WORD", 0 );
 CreateDicEntryC( ID_WORD_FETCH, "W@", 0 );
 CreateDicEntryC( ID_WORD_STORE, "W!", 0 );
 CreateDicEntryC( ID_XOR, "XOR", 0 );
 CreateDicEntryC( ID_ZERO_BRANCH, "0BRANCH", 0 );
 
 if( FindSpecialXTs() < 0 ) goto error;
 
 if( CompileCustomFunctions() < 0 ) goto error; /* Call custom 'C' call builder. */
 
#ifdef PF_DEBUG
 DumpMemory( dic->dic_HeaderBase, 256 );
 DumpMemory( dic->dic_CodeBase, 256 );
#endif
 
 return dic;
 
error:
 pfDeleteDictionary( dic );
 return NULL;
 
nomem:
 return NULL;
}
#endif /* !PF_NO_INIT */
 
/*
** ( xt -- nfa 1 , x 0 , find NFA in dictionary from XT )
** 1 for IMMEDIATE values
*/
cell ffTokenToName( ExecToken XT, const ForthString **NFAPtr )
{
 const ForthString *NameField;
 int32 Searching = TRUE;
 cell Result = 0;
 ExecToken TempXT;
 
 NameField = gVarContext;
DBUGX(("\ffCodeToName: gVarContext = 0x%x\n", gVarContext));
 
 do
 {
  TempXT = NameToToken( NameField );
  
  if( TempXT == XT )
  {
DBUGX(("ffCodeToName: NFA = 0x%x\n", NameField));
   *NFAPtr = NameField ;
   Result = 1;
   Searching = FALSE;
  }
  else
  {
   NameField = NameToPrevious( NameField );
   if( NameField == NULL )
   {
    *NFAPtr = 0;
    Searching = FALSE;
   }
  }
 } while ( Searching);
 
 return Result;
}
 
/*
** ( $name -- $addr 0 | nfa -1 | nfa 1 , find NFA in dictionary )
** 1 for IMMEDIATE values
*/
cell ffFindNFA( const ForthString * WordName, const ForthString ** NFAPtr )
{
    uint8               WordLen   = (uint8) ( (uint32) * WordName & 0x1F );
    const ForthString * WordChar  = WordName + 1;
    const char *        NameField = gVarContext;
    int32               Searching = TRUE;
    cell                Result    = 0;
/*
//LMfprintf( stderr, "ffFindNFA: gVarContext = 0x%x, WordLen = %d, SMUDGE=%c, WordName = ", gVarContext, WordLen, ( * NameField & FLAG_SMUDGE ) ? 'S' : 'V' );
//LMfor ( int i = 0 ; i < WordLen ; i++ )
//LM    fputc( WordName[i+1], stderr );
//LMfprintf( stderr, "\n" );
*/
    do {
        int8 NameLen  = (uint8) ( (uint32) (*NameField) & MASK_NAME_SIZE );
#ifdef __SYSC__
        char NameChar[40];
        for ( int i = 0 ; i < NameLen ; i++ )
            NameChar[i] = __atoe( NameField[i+1] ); /* ASCII to EBCDIC conversion */
#elif __CMS__
        char NameChar[40];
        for ( int i = 0 ; i < NameLen ; i++ )
            NameChar[i] = __atoe( NameField[i+1] ); /* ASCII to EBCDIC conversion */
#else
        const char * NameChar = NameField + 1;
#endif
        if ( ( ( * NameField & FLAG_SMUDGE) == 0 ) &&
             ( NameLen == WordLen )                &&
             ffCompareTextCaseN( NameChar, WordChar, WordLen ) /* FIXME - slow */
           )
        {
/*LM        fprintf( stderr, "ffFindNFA: found it at NFA = 0x%x\n", NameField ); */
            * NFAPtr  = NameField ;
            Result    = ( ( * NameField ) & FLAG_IMMEDIATE ) ? 1 : -1;
            Searching = FALSE;
        } else {
            NameField = NameToPrevious( NameField );
            if ( NameField == NULL ) {
                * NFAPtr = WordName;
                Searching = FALSE;
            } /*i*/
        } /*i*/
    } while ( Searching );
/*LMfprintf( stderr, "ffFindNFA: returns 0x%x\n", Result );*/
    return Result;
}
 
 
/***************************************************************
** ( $name -- $name 0 | xt -1 | xt 1 )
** 1 for IMMEDIATE values
*/
cell ffFind( const ForthString * WordName, ExecToken * pXT )
{
    const ForthString * NFA;
    int32 Result = ffFindNFA( WordName, & NFA );
    DBUG(("ffFind: %8s at 0x%x\n", WordName+1, NFA)); /* WARNING, not NUL terminated. %Q */
    *pXT = ( ( Result ) ? NameToToken( NFA ) : ( (ExecToken) WordName ) );
    return Result;
}
 
/****************************************************************
** Find name when passed 'C' string.
*/
cell ffFindC( const char *WordName, ExecToken *pXT )
{
    DBUG(("ffFindC: %s\n", WordName ));
    CStringToForth( gScratch, WordName );
    return ffFind( gScratch, pXT );
}
 
 
/***********************************************************/
/********* Compiling New Words *****************************/
/***********************************************************/
#define DIC_SAFETY_MARGIN  (400)
 
/*************************************************************
**  Check for dictionary overflow.
*/
static int32 ffCheckDicRoom( void )
{
 int32 RoomLeft;
 RoomLeft = gCurrentDictionary->dic_HeaderLimit -
            gCurrentDictionary->dic_HeaderPtr.Byte;
 if( RoomLeft < DIC_SAFETY_MARGIN )
 {
  pfReportError("ffCheckDicRoom", PF_ERR_HEADER_ROOM);
  return PF_ERR_HEADER_ROOM;
 }
 
 RoomLeft = gCurrentDictionary->dic_CodeLimit -
            gCurrentDictionary->dic_CodePtr.Byte;
 if( RoomLeft < DIC_SAFETY_MARGIN )
 {
  pfReportError("ffCheckDicRoom", PF_ERR_CODE_ROOM);
  return PF_ERR_CODE_ROOM;
 }
 return 0;
}
 
/*************************************************************
**  Create a dictionary entry given a string name.
*/
void ffCreateSecondaryHeader( const ForthStringPtr FName)
{
/* Check for dictionary overflow. */
 if( ffCheckDicRoom() ) return;
 
 CheckRedefinition( FName );
/* Align CODE_HERE */
 CODE_HERE = (cell *)( (((uint32)CODE_HERE) + 3) & ~3);
 CreateDicEntry( (ExecToken) ABS_TO_CODEREL(CODE_HERE), FName, FLAG_SMUDGE );
DBUG(("ffCreateSecondaryHeader, XT = 0x%x, Name = %8s\n"));
}
 
/*************************************************************
** Begin compiling a secondary word.
*/
static void ffStringColon( const ForthStringPtr FName)
{
 ffCreateSecondaryHeader( FName );
 gVarState = 1;
}
 
/*************************************************************
** Read the next ExecToken from the Source and create a word.
*/
void ffColon( void )
{
 char *FName;
 
 gDepthAtColon = DATA_STACK_DEPTH;
 
 FName = ffWord( BLANK );
 if( *FName > 0 )
 {
  ffStringColon( FName );
 }
}
 
/*************************************************************
** Check to see if name is already in dictionary.
*/
static int32 CheckRedefinition( const ForthStringPtr FName )
{
 int32 Flag;
 ExecToken XT;
 
 Flag = ffFind( FName, &XT);
 if( Flag )
 {
  ioType( FName+1, (int32) *FName );
  MSG( " already defined.\n" );
 }
 return Flag;
}
 
void ffStringCreate( char *FName)
{
 ffCreateSecondaryHeader( FName );
 
 CODE_COMMA( ID_CREATE_P );
 CODE_COMMA( ID_EXIT );
 ffFinishSecondary();
 
}
 
/* Read the next ExecToken from the Source and create a word. */
void ffCreate( void )
{
 char *FName;
 
 FName = ffWord( BLANK );
 if( *FName > 0 )
 {
  ffStringCreate( FName );
 }
}
 
void ffStringDefer( const ForthStringPtr FName, ExecToken DefaultXT )
{
 
 ffCreateSecondaryHeader( FName );
 
 CODE_COMMA( ID_DEFER_P );
 CODE_COMMA( DefaultXT );
 
 ffFinishSecondary();
 
}
#ifndef PF_NO_INIT
/* Convert name then create deferred dictionary entry. */
static void CreateDeferredC( ExecToken DefaultXT, const char *CName )
{
 char FName[40];
 CStringToForth( FName, CName );
 ffStringDefer( FName, DefaultXT );
}
#endif
 
/* Read the next token from the Source and create a word. */
void ffDefer( void )
{
 char *FName;
 
 FName = ffWord( BLANK );
 if( *FName > 0 )
 {
  ffStringDefer( FName, ID_QUIT_P );
 }
}
 
/* Unsmudge the word to make it visible. */
void ffUnSmudge( void )
{
 *gVarContext &= ~FLAG_SMUDGE;
}
 
/* Implement ; */
void ffSemiColon( void )
{
 gVarState = 0;
 
 if( (gDepthAtColon != DATA_STACK_DEPTH) &&
     (gDepthAtColon != DEPTH_AT_COLON_INVALID) ) /* Ignore if no ':' */
 {
  pfReportError("ffSemiColon", PF_ERR_COLON_STACK);
  ffAbort();
 }
 else
 {
  ffFinishSecondary();
 }
 gDepthAtColon = DEPTH_AT_COLON_INVALID;
}
 
/* Finish the definition of a Forth word. */
void ffFinishSecondary( void )
{
 CODE_COMMA( ID_EXIT );
 ffUnSmudge();
}
 
/**************************************************************/
/* Used to pull a number from the dictionary to the stack */
void ff2Literal( cell dHi, cell dLo )
{
 CODE_COMMA( ID_2LITERAL_P );
 CODE_COMMA( dHi );
 CODE_COMMA( dLo );
}
void ffALiteral( cell Num )
{
 CODE_COMMA( ID_ALITERAL_P );
 CODE_COMMA( Num );
}
void ffLiteral( cell Num )
{
 CODE_COMMA( ID_LITERAL_P );
 CODE_COMMA( Num );
}
 
#ifdef PF_SUPPORT_FP
void ffFPLiteral( PF_FLOAT fnum )
{
 /* Hack for Metrowerks complier which won't compile the
  * original expression.
  */
 PF_FLOAT  *temp;
 cell      *dicPtr;
 
/* Make sure that literal float data is float aligned. */
 dicPtr = CODE_HERE + 1;
 while( (((uint32) dicPtr++) & (sizeof(PF_FLOAT) - 1)) != 0)
 {
  DBUG((" comma NOOP to align FPLiteral\n"));
  CODE_COMMA( ID_NOOP );
 }
 CODE_COMMA( ID_FP_FLITERAL_P );
 
 temp = (PF_FLOAT *)CODE_HERE;
 WRITE_FLOAT_DIC(temp,fnum);   /* Write to dictionary. */
 temp++;
 CODE_HERE = (cell *) temp;
}
#endif /* PF_SUPPORT_FP */
 
/**************************************************************/
void FindAndCompile( const char *theWord )
{
    ExecToken XT;
    cell Num;
 
    int32 Flag = ffFind( theWord, & XT );
    DBUG(("FindAndCompile: theWord = %8s, XT = 0x%x, Flag = %d\n", theWord, XT, Flag ));
 
    if ( Flag == -1 ) {         /* Is it a normal word ? */
        if ( gVarState ) {      /* compiling? */
            CODE_COMMA( XT );
        } else
            pfExecuteToken( XT );
    } else if ( Flag == 1 ) {   /* or is it IMMEDIATE ? */
        DBUG(("FindAndCompile: IMMEDIATE, theWord = 0x%x\n", theWord ));
        pfExecuteToken( XT );
    } else {                    /* try to interpret it as a number. */
        /* Call deferred NUMBER? */
        DBUG(("FindAndCompile: not found, try number?\n" ));
#ifdef __SYSC__
        /* converting the number digits from EBCDIC to ASCII */
        char asciiWord[UCHAR_MAX];
        asciiWord[0] = theWord[0];
        for ( size_t i = 0 ; i < (size_t) * theWord ; i++ )
            asciiWord[i+1] = __etoa( theWord[i+1] ); /* EBCDIC to ASCII conversion */
        PUSH_DATA_STACK( asciiWord );   /* Push text of number */
#elif __CMS__
        /* converting the number digits from EBCDIC to ASCII */
        char asciiWord[256];
        asciiWord[0] = theWord[0];
        size_t lim = (size_t) * theWord;
        for ( size_t i = 0 ; i < lim ; i++ )
            asciiWord[i+1] = __etoa( theWord[i+1] ); /* EBCDIC to ASCII conversion */
        PUSH_DATA_STACK( asciiWord );   /* Push text of number */
#else
        PUSH_DATA_STACK( theWord );     /* Push text of number */
#endif
        pfExecuteToken( gNumberQ_XT );
        DBUG(("FindAndCompile: after number?\n" ));
        int32 NumResult = POP_DATA_STACK;  /* Success? */
        switch ( NumResult ) {
            case NUM_TYPE_SINGLE:
                if( gVarState ) { /* compiling? */
                    Num = POP_DATA_STACK;
                    ffLiteral( Num );
                } /*i*/
                break;
            case NUM_TYPE_DOUBLE:
                if( gVarState ) { /* compiling? */
                    Num = POP_DATA_STACK;  /* get hi portion */
                    ff2Literal( Num, POP_DATA_STACK );
                } /*i*/
                break;
#ifdef PF_SUPPORT_FP
            case NUM_TYPE_FLOAT:
                if( gVarState )   /* compiling? */
                    ffFPLiteral( * gCurrentTask->td_FloatStackPtr++ );
                break;
#endif
        case NUM_TYPE_BAD:
            default:
                ioType( theWord + 1, * theWord );
                MSG( "  ? - unrecognized word!\n" );
                ffAbort( );
                break;
        } /*s*/
    } /*i*/
}
 
/**************************************************************
** Forth outer interpreter.  Parses words from Source.
** Executes them or compiles them based on STATE.
*/
int32 ffInterpret( void )
{
 int32 Flag;
 char *theWord;
 
/* Is there any text left in Source ? */
 while( (gCurrentTask->td_IN < (gCurrentTask->td_SourceNum-1) ) &&
  !CHECK_ABORT)
 {
DBUGX(("ffInterpret: IN=%d, SourceNum=%d\n", gCurrentTask->td_IN,
 gCurrentTask->td_SourceNum ) );
  theWord = ffWord( BLANK );
DBUGX(("ffInterpret: theWord = 0x%x, Len = %d\n", theWord, *theWord ));
  if( *theWord > 0 )
  {
   Flag = 0;
   if( gLocalCompiler_XT )
   {
    PUSH_DATA_STACK( theWord );   /* Push word. */
    pfExecuteToken( gLocalCompiler_XT );
    Flag = POP_DATA_STACK;  /* Compiled local? */
   }
   if( Flag == 0 )
   {
    FindAndCompile( theWord );
   }
  }
 }
 DBUG(("ffInterpret: CHECK_ABORT = %d\n", CHECK_ABORT));
 return( CHECK_ABORT ? -1 : 0 );
}
  
/**************************************************************/
void ffOK( void )
{
/* Check for stack underflow.   %Q what about overflows? */
 if( (gCurrentTask->td_StackBase - gCurrentTask->td_StackPtr) < 0 )
 {
  MSG("Stack underflow!\n");
  ResetForthTask( );
 }
#ifdef PF_SUPPORT_FP  /* Check floating point stack too! */
 else if((gCurrentTask->td_FloatStackBase - gCurrentTask->td_FloatStackPtr) < 0)
 {
  MSG("FP stack underflow!\n");
  ResetForthTask( );
 }
#endif
 else if( gCurrentTask->td_InputStream == PF_STDIN)
 {
  if( !gVarState )  /* executing? */
  {
   if( !gVarQuiet )
   {
    MSG( "   ok\n" );
    if(gVarTraceStack) ffDotS();
   }
   else
   {
    EMIT_CR;
   }
  }
 }
}
 
/***************************************************************
** Report state of include stack.
***************************************************************/
static void ReportIncludeState( void )
{
 int32 i;
/* If not INCLUDing, just return. */
 if( gIncludeIndex == 0 ) return;
 
/* Report line number and nesting level. */
 MSG_NUM_D("INCLUDE error on line #", gCurrentTask->td_LineNumber );
 MSG_NUM_D("INCLUDE nesting level = ", gIncludeIndex );
 
/* Dump line of error and show offset in line for >IN */
 MSG( gCurrentTask->td_SourcePtr );
 for( i=0; i<(gCurrentTask->td_IN - 1); i++ ) EMIT('^');
 EMIT_CR;
}
 
 
/***************************************************************
** Interpret input in a loop.
***************************************************************/
void ffQuit( void )
{
 gCurrentTask->td_Flags |= CFTD_FLAG_GO;
 
 while( gCurrentTask->td_Flags & CFTD_FLAG_GO )
 {
  if(!ffRefill())
  {
/*   gCurrentTask->td_Flags &= ~CFTD_FLAG_GO; */
   return;
  }
  ffInterpret();
  DBUG(("gCurrentTask->td_Flags = 0x%x\n",  gCurrentTask->td_Flags)); 
  if(CHECK_ABORT)
  {
   CLEAR_ABORT;
  }
  else
  {
   ffOK( );
  }
 }
}
 
/***************************************************************
** Include a file
***************************************************************/
 
cell ffIncludeFile( FileStream *InputFile )
{
 cell Result;
 
/* Push file stream. */
 Result = ffPushInputStream( InputFile );
 if( Result < 0 ) return Result;
 
/* Run outer interpreter for stream. */
 ffQuit();
 
/* Pop file stream. */
 ffPopInputStream();
 
 return gVarReturnCode;
}
 
#endif /* !PF_NO_SHELL */
 
/***************************************************************
** Save current input stream on stack, use this new one.
***************************************************************/
Err ffPushInputStream( FileStream *InputFile )
{
 cell Result = 0;
 IncludeFrame *inf;
 
/* Push current input state onto special include stack. */
 if( gIncludeIndex < MAX_INCLUDE_DEPTH )
 {
  inf = &gIncludeStack[gIncludeIndex++];
  inf->inf_FileID = gCurrentTask->td_InputStream;
  inf->inf_IN = gCurrentTask->td_IN;
  inf->inf_LineNumber = gCurrentTask->td_LineNumber;
  inf->inf_SourceNum = gCurrentTask->td_SourceNum;
/* Copy TIB plus any NUL terminator into saved area. */
  if( (inf->inf_SourceNum > 0) && (inf->inf_SourceNum < (TIB_SIZE-1)) )
  {
   pfCopyMemory( inf->inf_SaveTIB, gCurrentTask->td_TIB, inf->inf_SourceNum+1 );
  }
 
/* Set new current input. */
  DBUG(( "ffPushInputStream: InputFile = 0x%x\n", InputFile ));
  gCurrentTask->td_InputStream = InputFile;
  gCurrentTask->td_LineNumber = 0;
 }
 else
 {
  ERR("ffPushInputStream: max depth exceeded.\n");
  return -1;
 }
 
 
 return Result;
}
 
/***************************************************************
** Go back to reading previous stream.
** Just return gCurrentTask->td_InputStream upon underflow.
***************************************************************/
FileStream *ffPopInputStream( void )
{
 IncludeFrame *inf;
 FileStream *Result;
 
DBUG(("ffPopInputStream: gIncludeIndex = %d\n", gIncludeIndex));
 Result = gCurrentTask->td_InputStream;
 
/* Restore input state. */
 if( gIncludeIndex > 0 )
 {
  inf = &gIncludeStack[--gIncludeIndex];
  gCurrentTask->td_InputStream = inf->inf_FileID;
  DBUG(("ffPopInputStream: stream = 0x%x\n", gCurrentTask->td_InputStream ));
  gCurrentTask->td_IN = inf->inf_IN;
  gCurrentTask->td_LineNumber = inf->inf_LineNumber;
  gCurrentTask->td_SourceNum = inf->inf_SourceNum;
/* Copy TIB plus any NUL terminator into saved area. */
  if( (inf->inf_SourceNum > 0) && (inf->inf_SourceNum < (TIB_SIZE-1)) )
  {
   pfCopyMemory( gCurrentTask->td_TIB, inf->inf_SaveTIB, inf->inf_SourceNum+1 );
  }
 
 }
DBUG(("ffPopInputStream: return = 0x%x\n", Result ));
 
 return Result;
}
 
/***************************************************************
** Convert file pointer to value consistent with SOURCE-ID.
***************************************************************/
cell ffConvertStreamToSourceID( FileStream *Stream )
{
 cell Result;
 if(Stream == PF_STDIN)
 {
  Result = 0;
 }
 else if(Stream == NULL)
 {
  Result = -1;
 }
 else
 {
  Result = (cell) Stream;
 }
 return Result;
}
 
/***************************************************************
** Convert file pointer to value consistent with SOURCE-ID.
***************************************************************/
FileStream * ffConvertSourceIDToStream( cell id )
{
 FileStream *stream;
 
 if( id == 0 )
 {
  stream = PF_STDIN;
 }
 else if( id == -1 )
 {
  stream = NULL;
 }
 else
 {
  stream = (FileStream *) id;
 }
 return stream;
}
 
/***************************************************************
** Cleanup Include stack by popping and closing files.
***************************************************************/
static void ffCleanIncludeStack( void )
{
 FileStream *cur;
 
 while( (cur = ffPopInputStream()) != PF_STDIN)
 {
  DBUG(("ffCleanIncludeStack: closing 0x%x\n", cur ));
  sdCloseFile(cur);
 }
}
 
/**************************************************************/
void ffAbort( void )
{
#ifndef PF_NO_SHELL
 ReportIncludeState();
#endif /* PF_NO_SHELL */
 ffCleanIncludeStack();
 ResetForthTask();
 SET_ABORT;
 if( gVarReturnCode == 0 ) gVarReturnCode = ABORT_RETURN_CODE;
}
 
/**************************************************************/
/* ( -- , fill Source from current stream ) */
/* Return FFALSE if no characters. */
cell ffRefill( void )
{
 cell Num, Result = FTRUE;
 
/* get line from current stream */
 Num = ioAccept( gCurrentTask->td_SourcePtr,
  TIB_SIZE, gCurrentTask->td_InputStream );
 if( Num < 0 )
 {
  Result = FFALSE;
  Num = 0;
 }
 
/* reset >IN for parser */
 gCurrentTask->td_IN = 0;
 gCurrentTask->td_SourceNum = Num;
 gCurrentTask->td_LineNumber++;  /* Bump for include. */
 
/* echo input if requested */
 if( gVarEcho && ( Num > 0))
 {
  MSG( gCurrentTask->td_SourcePtr );
 }
 
 return Result;
}
