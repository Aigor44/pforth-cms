/* @(#) pf_main.c 98/01/26 1.2 */
/***************************************************************
** Forth based on 'C'
**
** main() routine that demonstrates how to call PForth as
** a module from 'C' based application.
** Customize this as needed for your application.
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
 
#ifdef PF_NO_STDIO
	#define NULL  ((void *) 0)
	#define ERR(msg) /* { printf msg; } */
#else
	#include <stdio.h>
	#define ERR(msg) { printf msg; }
#endif
 
#include "pforth.h"
	
#ifdef __MWERKS__
	#include <console.h>
	#include <sioux.h>
#endif
 
#ifndef TRUE
#define TRUE (1)
#define FALSE (0)
#endif
 
int main( int argc, char **argv )
{
	const char *DicName = "pforth.dic";
	const char *SourceName = NULL;
	char IfInit = FALSE;
	char *s;
	int32 i;
	int Result;
 
/* For Metroworks on Mac */
	#ifdef __MWERKS__
	argc = ccommand(&argv);
	#endif
 
/* Parse command line. */
	for( i=1; i<argc; i++ )
	{
		s = argv[i];
 
		if( *s == '-' )
		{
			char c;
			s++; /* past '-' */
			c = *s++;
			switch(c)
			{
			case 'i':
			case 'I':
				IfInit = TRUE;
				DicName = NULL;
				break;
			case 'q':
			case 'Q':
				pfSetQuiet( TRUE );
				break;
			case 'd':
			case 'D':
				DicName = s;
				break;
			default:
				ERR(("Unrecognized option!\n"));
				ERR(("pforth [-i] [-q] [-dfilename.dic] [sourcefilename]\n"));
				Result = 1;
				goto on_error;
				break;
			}
		}
		else
		{
			SourceName = s;
		}
	}
/* Force Init */
#ifdef PF_INIT_MODE
	IfInit = TRUE;
	DicName = NULL;
#endif
 
	Result = pfDoForth( DicName, SourceName, IfInit);
 
on_error:
	return Result;
}
