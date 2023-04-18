/*  @(#) pf_unix.h 98/01/28 1.4 */
#ifndef _pf_unix_h
#define _pf_unix_h
 
/***************************************************************
** UNIX dependant include file for PForth, a Forth based on 'C'
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
 
#include <ctype.h>
 
#ifndef PF_NO_CLIB
	#include <string.h>    /* Needed for strlen(), memcpy(), and memset(). */
	#include <stdlib.h>    /* Needed for exit(). */
#endif
 
#include <stdio.h>         /* Needed for FILE and getc(). */
 
#ifdef PF_SUPPORT_FP
	#include <math.h>
 
	#ifndef PF_USER_FP
		#include "pf_float.h"
	#else
		#include PF_USER_FP
	#endif
#endif
 
#endif /* _pf_unix_h */
