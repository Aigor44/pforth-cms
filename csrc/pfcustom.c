/* @(#) pfcustom.c 98/01/26 1.3 */
 
#ifndef PF_USER_CUSTOM
 
/***************************************************************
** Call Custom Functions for pForth
**
** Create a file similar to this and compile it into pForth
** by setting -DPF_USER_CUSTOM="mycustom.c"
**
** Using this, you could, for example, call X11 from Forth.
** See "pf_cglue.c" for more information.
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
 
 
#include "pf_all.h"
 
static int32 CTest0( int32 Val );
static void CTest1( int32 Val1, cell Val2 );
 
/****************************************************************
** Step 1: Put your own special glue routines here
**     or link them in from another file or library.
****************************************************************/
static int32 CTest0( int32 Val )
{
	MSG_NUM_D("CTest0: Val = ", Val);
	return Val+1;
}
 
static void CTest1( int32 Val1, cell Val2 )
{
 
	MSG("CTest1: Val1 = "); ffDot(Val1);
	MSG_NUM_D(", Val2 = ", Val2);
}
 
/****************************************************************
** Step 2: Create CustomFunctionTable.
**     Do not change the name of CustomFunctionTable!
**     It is used by the pForth kernel.
****************************************************************/
 
#ifdef PF_NO_GLOBAL_INIT
/******************
** If your loader does not support global initialization, then you
** must define PF_NO_GLOBAL_INIT and provide a function to fill
** the table. Some embedded system loaders require this!
** Do not change the name of LoadCustomFunctionTable()!
** It is called by the pForth kernel.
*/
#define NUM_CUSTOM_FUNCTIONS  (2)
void *CustomFunctionTable[NUM_CUSTOM_FUNCTIONS];
 
Err LoadCustomFunctionTable( void )
{
	CustomFunctionTable[0] = CTest0;
	CustomFunctionTable[1] = CTest1;
	return 0;
}
 
#else
/******************
** If your loader supports global initialization (most do.) then just
** create the table like this.
*/
 
void *CustomFunctionTable[] =
{
	(void *)CTest0,
	(void *)CTest1
};	
#endif
 
/****************************************************************
** Step 3: Add custom functions to the dictionary.
**     Do not change the name of CompileCustomFunctions!
**     It is called by the pForth kernel.
****************************************************************/
 
#if (!defined(PF_NO_INIT)) && (!defined(PF_NO_SHELL))
Err CompileCustomFunctions( void )
{
	Err err;
 
/* Compile Forth words that call your custom functions.
** Make sure order of functions matches that in LoadCustomFunctionTable().
** Parameters are: Name in UPPER CASE, Function, Index, Mode, NumParams
*/
	err = CreateGlueToC( "CTEST0", 0, C_RETURNS_VALUE, 1 );
	if( err < 0 ) return err;
	err = CreateGlueToC( "CTEST1", 1, C_RETURNS_VOID, 2 );
	if( err < 0 ) return err;
	
	return 0;
}
#else
Err CompileCustomFunctions( void ) { return 0; }
#endif
 
/****************************************************************
** Step 4: Recompile using compiler option PF_USER_CUSTOM
**         and link with your code.
**         Then rebuild the Forth using "pforth -i"
**         Test:   10 Ctest0 ( should print message then '11' )
****************************************************************/
 
#endif  /* PF_USER_CUSTOM */
 
