/* @(#) pf_text.c 98/01/26 1.3 */
/***************************************************************
** Text Strings for Error Messages
** Various Text tools.
**
** For PForth based on 'C'
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
** 19970702 PLB Fixed ConvertNumberToText for unsigned numbers.
** 19980522 PLB Expand PAD for ConvertNumberToText so "-1 binary .s" doesn't crash.
***************************************************************/
 
#include "pf_all.h"
 
#define PF_ENGLISH
 
/*
** Define array of error messages.
** These are defined in one place to make it easier to translate them.
*/
#ifdef PF_ENGLISH
/***************************************************************/
void pfReportError( const char *FunctionName, Err ErrCode )
{
 const char *s;
 
 MSG("Error in ");
 MSG(FunctionName);
 MSG(" - ");
 
 switch(ErrCode & 0xFF)
 {
 case PF_ERR_NO_MEM & 0xFF:
  s = "insufficient memory"; break;
 case PF_ERR_BAD_ADDR & 0xFF:
  s = "address misaligned"; break;
 case PF_ERR_TOO_BIG & 0xFF:
  s = "data chunk too large"; break;
 case PF_ERR_NUM_PARAMS & 0xFF:
  s = "incorrect number of parameters"; break;
 case PF_ERR_OPEN_FILE & 0xFF:
  s = "could not open file"; break;
 case PF_ERR_WRONG_FILE & 0xFF:
  s = "wrong type of file format"; break;
 case PF_ERR_BAD_FILE & 0xFF:
  s = "badly formatted file"; break;
 case PF_ERR_READ_FILE & 0xFF:
  s = "file read failed"; break;
 case PF_ERR_WRITE_FILE & 0xFF:
  s = "file write failed"; break;
 case PF_ERR_CORRUPT_DIC & 0xFF:
  s = "corrupted dictionary"; break;
 case PF_ERR_NOT_SUPPORTED & 0xFF:
  s = "not supported in this version"; break;
 case PF_ERR_VERSION_FUTURE & 0xFF:
  s = "version from future"; break;
 case PF_ERR_VERSION_PAST & 0xFF:
  s = "version is obsolete. Rebuild new one."; break;
 case PF_ERR_COLON_STACK & 0xFF:
  s = "stack depth changed between : and ; . Probably unbalanced conditional"; break;
 case PF_ERR_HEADER_ROOM & 0xFF:
  s = "no room left in header space"; break;
 case PF_ERR_CODE_ROOM & 0xFF:
  s = "no room left in code space"; break;
 case PF_ERR_NO_SHELL & 0xFF:
  s = "attempt to use names in forth compiled with PF_NO_SHELL"; break;
 case PF_ERR_NO_NAMES & 0xFF:
  s = "dictionary has no names";  break;
 case PF_ERR_OUT_OF_RANGE & 0xFF:
  s = "parameter out of range";  break;
 case PF_ERR_ENDIAN_CONFLICT & 0xFF:
  s = "endian-ness of dictionary does not match code";  break;
 case PF_ERR_FLOAT_CONFLICT & 0xFF:
  s = "float support mismatch between .dic file and code";  break;
 default:
  s = "unrecognized error code!"; break;
 }
 MSG(s);
 EMIT_CR;
}
#endif
 
/**************************************************************
** Copy a Forth String to a 'C' string.
*/
 
char *ForthStringToC( char *dst, const char *FString )
{
 int32 Len;
 
 Len = (int32) *FString;
 pfCopyMemory( dst, FString+1, Len );
 dst[Len] = '\0';
 
 return dst;
}
 
/**************************************************************
** Copy a NUL terminated string to a Forth counted string.
*/
char *CStringToForth( char *dst, const char *CString )
{
 char *s;
 int32 i;
 
 s = dst+1;
 for( i=0; *CString; i++ )
 {
  *s++ = *CString++;
 }
 *dst = (char ) i;
 return dst;
}
 
/**************************************************************
** Compare two test strings, case sensitive.
** Return TRUE if they match.
*/
int32 ffCompareText( const char *s1, const char *s2, int32 len )
{
 int32 i, Result;
 
 Result = TRUE;
 for( i=0; i<len; i++ )
 {
DBUGX(("ffCompareText: *s1 = 0x%x, *s2 = 0x%x\n", *s1, *s2 ));
  if( *s1++ != *s2++ )
  {
   Result = FALSE;
   break;
  }
 }
DBUGX(("ffCompareText: return 0x%x\n", Result ));
 return Result;
}
 
/**************************************************************
** Compare two test strings, case INsensitive.
** Return TRUE if they match.
*/
int32 ffCompareTextCaseN( const char * s1, const char * s2, int32 len )
{
/*LMfprintf( stderr, "ffCompareTextCaseN( %s, %s, %d ) return ", s1, s2, len );*/
    int32 Result = TRUE;
    int32 i;
    for ( i = 0; i < len; i++ ) {
        if ( pfCharToLower(*s1++) != pfCharToLower(*s2++) ) {
            Result = FALSE;
            break;
        } /*i*/
    } /*f*/
/*LMfprintf( stderr, "0x%x\n", Result );*/
    return Result;
}
 
/**************************************************************
** Compare two strings, case sensitive.
** Return zero if they match, -1 if s1<s2, +1 is s1>s2;
*/
int32 ffCompare( const char *s1, int32 len1, const char *s2, int32 len2 )
{
 int32 i, result, n, diff;
 
 result = 0;
 n = MIN(len1,len2);
 for( i=0; i<n; i++ )
 {
  if( (diff = (*s2++ - *s1++)) != 0 )
  {
   result = (diff > 0) ? -1 : 1 ;
   break;
  }
 }
 if( result == 0 )  /* Match up to MIN(len1,len2) */
 {
  if( len1 < len2 )
  {
   result = -1;
  }
  else if ( len1 > len2 )
  {
   result = 1;
  }
 }
 return result;
}
 
/***************************************************************
** Convert number to text.
*/
#define CNTT_PAD_SIZE ((sizeof(int32)*8)+2)  /* PLB 19980522 - Expand PAD so "-1 binary .s" doesn't crash. */
static char cnttPad[CNTT_PAD_SIZE];
 
char *ConvertNumberToText( int32 Num, int32 Base, int32 IfSigned, int32 MinChars )
{
 int32 IfNegative = 0;
 char *p,c;
 uint32 NewNum, Rem, uNum;
 int32 i = 0;
 
 uNum = Num;
 if( IfSigned )
 {
/* Convert to positive and keep sign. */
  if( Num < 0 )
  {
   IfNegative = TRUE;
   uNum = -Num;
  }
 }
 
/* Point past end of Pad */
 p = cnttPad + CNTT_PAD_SIZE;
 *(--p) = (char) 0; /* NUL terminate */
 
 while( (i++<MinChars) || (uNum != 0) )
 {
  NewNum = uNum / Base;
  Rem = uNum - (NewNum * Base);
  c = (char) (( Rem < 10 ) ? (Rem + '0') : (Rem - 10 + 'A'));
  *(--p) = c;
  uNum = NewNum;
 }
 
 if( IfSigned )
 {
  if( IfNegative ) *(--p) = '-';
 }
 return p;
}
 
/***************************************************************
** Diagnostic routine that prints memory in table format.
*/
void DumpMemory( void *addr, int32 cnt)
{
 int32 ln, cn, nlines;
 unsigned char *ptr, *cptr, c;
 
 nlines = (cnt + 15) / 16;
 
 ptr = (unsigned char *) addr;
 
 EMIT_CR;
 
 for (ln=0; ln<nlines; ln++)
 {
  MSG( ConvertNumberToText( (int32) ptr, 16, FALSE, 8 ) );
  MSG(": ");
  cptr = ptr;
  for (cn=0; cn<16; cn++)
  {
   MSG( ConvertNumberToText( (int32) *cptr++, 16, FALSE, 2 ) );
   EMIT(' ');
  }
  EMIT(' ');
  for (cn=0; cn<16; cn++)
  {
   c = *ptr++;
   if ((c < ' ') || (c > '}')) c = '.';
   EMIT(c);
  }
  EMIT_CR;
 }
}
 
 
/* Print name, mask off any dictionary bits. */
void TypeName( const char *Name )
{
 const char *FirstChar;
 int32 Len;
 
 FirstChar = Name+1;
 Len = *Name & 0x1F;
 
 ioType( FirstChar, Len );
}
 
