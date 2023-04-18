/* @(#) pf_mac.h 98/01/26 1.2 */
#ifndef _pf_mac_h
#define _pf_mac_h
 
/***************************************************************
** Macintosh dependant include file for PForth, a Forth based on 'C'
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
 
#include <CType.h>
#include <String.h>
 
#include <StdLib.h>
#include <StdIO.h>
 
 
#ifdef PF_SUPPORT_FP
	#include <Math.h>
	
	#ifndef PF_USER_FP
		#include "pf_float.h"
	#else
		#include PF_USER_FP
	#endif
#endif
 
#endif /* _pf_mac_h */
