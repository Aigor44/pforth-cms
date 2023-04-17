/* @(#) pf_io.c 96/12/23 1.12 */
/***************************************************************
** I/O subsystem for PForth based on 'C'
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
***************************************************************/
 
#include "pf_all.h"
 
#ifdef __SYSC__
#    include <machine/atoe.h>
#endif

#ifdef __CMS__
/*
 * Bijective EBCDIC (character set IBM-1047) to US-ASCII table: This table is
 * bijective - there are no ambiguous or duplicate characters.
 */
const unsigned char ebcdic2ascii[256] = {
    0x00, 0x01, 0x02, 0x03, 0x85, 0x09, 0x86, 0x7f, /* 00-0f: */
    0x87, 0x8d, 0x8e, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f, /* ................ */
    0x10, 0x11, 0x12, 0x13, 0x8f, 0x0a, 0x08, 0x97, /* 10-1f: */
    0x18, 0x19, 0x9c, 0x9d, 0x1c, 0x1d, 0x1e, 0x1f, /* ................ */
    0x80, 0x81, 0x82, 0x83, 0x84, 0x92, 0x17, 0x1b, /* 20-2f: */
    0x88, 0x89, 0x8a, 0x8b, 0x8c, 0x05, 0x06, 0x07, /* ................ */
    0x90, 0x91, 0x16, 0x93, 0x94, 0x95, 0x96, 0x04, /* 30-3f: */
    0x98, 0x99, 0x9a, 0x9b, 0x14, 0x15, 0x9e, 0x1a, /* ................ */
    0x20, 0xa0, 0xe2, 0xe4, 0xe0, 0xe1, 0xe3, 0xe5, /* 40-4f: */
    0xe7, 0xf1, 0xa2, 0x2e, 0x3c, 0x28, 0x2b, 0x7c, /* ...........<(+| */
    0x26, 0xe9, 0xea, 0xeb, 0xe8, 0xed, 0xee, 0xef, /* 50-5f: */
    0xec, 0xdf, 0x21, 0x24, 0x2a, 0x29, 0x3b, 0x5e, /* &.........!$*);^ */
    0x2d, 0x2f, 0xc2, 0xc4, 0xc0, 0xc1, 0xc3, 0xc5, /* 60-6f: */
    0xc7, 0xd1, 0xa6, 0x2c, 0x25, 0x5f, 0x3e, 0x3f, /* -/.........,%_>? */
    0xf8, 0xc9, 0xca, 0xcb, 0xc8, 0xcd, 0xce, 0xcf, /* 70-7f: */
    0xcc, 0x60, 0x3a, 0x23, 0x40, 0x27, 0x3d, 0x22, /* .........`:#@'=" */
    0xd8, 0x61, 0x62, 0x63, 0x64, 0x65, 0x66, 0x67, /* 80-8f: */
    0x68, 0x69, 0xab, 0xbb, 0xf0, 0xfd, 0xfe, 0xb1, /* .abcdefghi...... */
    0xb0, 0x6a, 0x6b, 0x6c, 0x6d, 0x6e, 0x6f, 0x70, /* 90-9f: */
    0x71, 0x72, 0xaa, 0xba, 0xe6, 0xb8, 0xc6, 0xa4, /* .jklmnopqr...... */
    0xb5, 0x7e, 0x73, 0x74, 0x75, 0x76, 0x77, 0x78, /* a0-af: */
    0x79, 0x7a, 0xa1, 0xbf, 0xd0, 0x5b, 0xde, 0xae, /* .~stuvwxyz...[.. */
    0xac, 0xa3, 0xa5, 0xb7, 0xa9, 0xa7, 0xb6, 0xbc, /* b0-bf: */
    0xbd, 0xbe, 0xdd, 0xa8, 0xaf, 0x5d, 0xb4, 0xd7, /* .............].. */
    0x7b, 0x41, 0x42, 0x43, 0x44, 0x45, 0x46, 0x47, /* c0-cf: */
    0x48, 0x49, 0xad, 0xf4, 0xf6, 0xf2, 0xf3, 0xf5, /* {ABCDEFGHI...... */
    0x7d, 0x4a, 0x4b, 0x4c, 0x4d, 0x4e, 0x4f, 0x50, /* d0-df: */
    0x51, 0x52, 0xb9, 0xfb, 0xfc, 0xf9, 0xfa, 0xff, /* }JKLMNOPQR...... */
    0x5c, 0xf7, 0x53, 0x54, 0x55, 0x56, 0x57, 0x58, /* e0-ef: */
    0x59, 0x5a, 0xb2, 0xd4, 0xd6, 0xd2, 0xd3, 0xd5, /* \.STUVWXYZ...... */
    0x30, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37, /* f0-ff: */
    0x38, 0x39, 0xb3, 0xdb, 0xdc, 0xd9, 0xda, 0x9f /* 0123456789...... */
};

/*
 * The US-ASCII to EBCDIC (character set IBM-1047) table: This table is
 * bijective (no ambiguous or duplicate characters)
 */
const unsigned char ascii2ebcdic[256] = {
    0x00, 0x01, 0x02, 0x03, 0x37, 0x2d, 0x2e, 0x2f, /* 00-0f: */
    0x16, 0x05, 0x15, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f, /* ................ */
    0x10, 0x11, 0x12, 0x13, 0x3c, 0x3d, 0x32, 0x26, /* 10-1f: */
    0x18, 0x19, 0x3f, 0x27, 0x1c, 0x1d, 0x1e, 0x1f, /* ................ */
    0x40, 0x5a, 0x7f, 0x7b, 0x5b, 0x6c, 0x50, 0x7d, /* 20-2f: */
    0x4d, 0x5d, 0x5c, 0x4e, 0x6b, 0x60, 0x4b, 0x61, /* !"#$%&'()*+,-./ */
    0xf0, 0xf1, 0xf2, 0xf3, 0xf4, 0xf5, 0xf6, 0xf7, /* 30-3f: */
    0xf8, 0xf9, 0x7a, 0x5e, 0x4c, 0x7e, 0x6e, 0x6f, /* 0123456789:;<=>? */
    0x7c, 0xc1, 0xc2, 0xc3, 0xc4, 0xc5, 0xc6, 0xc7, /* 40-4f: */
    0xc8, 0xc9, 0xd1, 0xd2, 0xd3, 0xd4, 0xd5, 0xd6, /* @ABCDEFGHIJKLMNO */
    0xd7, 0xd8, 0xd9, 0xe2, 0xe3, 0xe4, 0xe5, 0xe6, /* 50-5f: */
    0xe7, 0xe8, 0xe9, 0xad, 0xe0, 0xbd, 0x5f, 0x6d, /* PQRSTUVWXYZ[\]^_ */
    0x79, 0x81, 0x82, 0x83, 0x84, 0x85, 0x86, 0x87, /* 60-6f: */
    0x88, 0x89, 0x91, 0x92, 0x93, 0x94, 0x95, 0x96, /* `abcdefghijklmno */
    0x97, 0x98, 0x99, 0xa2, 0xa3, 0xa4, 0xa5, 0xa6, /* 70-7f: */
    0xa7, 0xa8, 0xa9, 0xc0, 0x4f, 0xd0, 0xa1, 0x07, /* pqrstuvwxyz{|}~. */
    0x20, 0x21, 0x22, 0x23, 0x24, 0x04, 0x06, 0x08, /* 80-8f: */
    0x28, 0x29, 0x2a, 0x2b, 0x2c, 0x09, 0x0a, 0x14, /* ................ */
    0x30, 0x31, 0x25, 0x33, 0x34, 0x35, 0x36, 0x17, /* 90-9f: */
    0x38, 0x39, 0x3a, 0x3b, 0x1a, 0x1b, 0x3e, 0xff, /* ................ */
    0x41, 0xaa, 0x4a, 0xb1, 0x9f, 0xb2, 0x6a, 0xb5, /* a0-af: */
    0xbb, 0xb4, 0x9a, 0x8a, 0xb0, 0xca, 0xaf, 0xbc, /* ................ */
    0x90, 0x8f, 0xea, 0xfa, 0xbe, 0xa0, 0xb6, 0xb3, /* b0-bf: */
    0x9d, 0xda, 0x9b, 0x8b, 0xb7, 0xb8, 0xb9, 0xab, /* ................ */
    0x64, 0x65, 0x62, 0x66, 0x63, 0x67, 0x9e, 0x68, /* c0-cf: */
    0x74, 0x71, 0x72, 0x73, 0x78, 0x75, 0x76, 0x77, /* ................ */
    0xac, 0x69, 0xed, 0xee, 0xeb, 0xef, 0xec, 0xbf, /* d0-df: */
    0x80, 0xfd, 0xfe, 0xfb, 0xfc, 0xba, 0xae, 0x59, /* ................ */
    0x44, 0x45, 0x42, 0x46, 0x43, 0x47, 0x9c, 0x48, /* e0-ef: */
    0x54, 0x51, 0x52, 0x53, 0x58, 0x55, 0x56, 0x57, /* ................ */
    0x8c, 0x49, 0xcd, 0xce, 0xcb, 0xcf, 0xcc, 0xe1, /* f0-ff: */
    0x70, 0xdd, 0xde, 0xdb, 0xdc, 0x8d, 0x8e, 0xdf /* ................ */
};

unsigned int __atoe(unsigned char c) { return ( c < 256 ) ? ascii2ebcdic[c] : 0; };
unsigned int __etoa(unsigned char c) { return ( c < 256 ) ? ebcdic2ascii[c] : 0; };
#endif
 
/***************************************************************
** Send single character to output stream.
*/
void ioEmit( char c )
{
#ifdef __SYSC_ASCIIOUT__
    int32 Result = sdTerminalOut( __atoe( c ) );  /* ASCII to EBCDIC conversion */
#else
    int32 Result = sdTerminalOut( c );
#endif
    if ( Result < 0 ) EXIT( 1 );
    if ( c == '\n' ) {
        gCurrentTask->td_OUT = 0;
        sdTerminalFlush();
    } else {
        gCurrentTask->td_OUT++;
    } /*i*/
}

void ioType( const char *s, int32 n )
{
    for ( int32 i = 0 ; i < n ; ++i )
        ioEmit(*s++);
}

void ioTypeA2E( const char *s, int32 n )
{
    for ( int32 i = 0 ; i < n ; ++i )
        ioEmit(__atoe(*s++));
}
 
/***************************************************************
** Return single character from input device, always keyboard.
*/
cell ioKey( void )
{
 return sdTerminalIn();
}
 
/**************************************************************
** Receive line from input stream.
** Return length, or -1 for EOF.
*/
#define BACKSPACE  (8)
cell ioAccept( char *Target, cell MaxLen, FileStream *stream )
{
 int32 c;
 int32 Len;
 char *p;
 
DBUGX(("ioAccept(0x%x, 0x%x, 0x%x)\n", Target, Len, stream ));
 p = Target;
 Len = MaxLen;
 while(Len > 0)
 {
  if( stream == PF_STDIN )
  {
   c = ioKey();
/* If KEY does not echo, then echo here. If using getchar(), KEY will echo. */
#ifndef PF_KEY_ECHOS
   ioEmit( c );
   if( c == '\r') ioEmit('\n'); /* Send LF after CR */
#endif
  }
  else
  {
   c = sdInputChar(stream);
  }
  switch(c)
  {
   case EOF:
    DBUG(("EOF\n"));
    return -1;
    break;
    
   case '\r':
   case '\n':
    *p++ = (char) c;
    DBUGX(("EOL\n"));
    goto gotline;
    break;
    
   case BACKSPACE:
    if( Len < MaxLen )  /* Don't go beyond beginning of line. */
    {
     EMIT(' ');
     EMIT(BACKSPACE);
     p--;
     Len++;
    }
    break;
    
   default:
    *p++ = (char) c;
    Len--;
    break;
  }
  
 }
gotline:
 *p = '\0';
  
 return pfCStringLength( Target );
}
 
#define UNIMPLEMENTED(name) { MSG(name); MSG("is unimplemented!\n"); }
 
#ifdef PF_NO_CHARIO
int  sdTerminalOut( char c )
{
 TOUCH(c);
 return 0;
}
int  sdTerminalIn( void )
{
 return -1;
}
int  sdTerminalFlush( void )
{
 return -1;
}
#endif
 
/***********************************************************************************/
#ifdef PF_NO_FILEIO
 
/* Provide stubs for standard file I/O */
 
FileStream *PF_STDIN;
FileStream *PF_STDOUT;
 
int32  sdInputChar( FileStream *stream )
{
 UNIMPLEMENTED("sdInputChar");
 TOUCH(stream);
 return -1;
}
 
FileStream *sdOpenFile( const char *FileName, const char *Mode )
{
 UNIMPLEMENTED("sdOpenFile");
 TOUCH(FileName);
 TOUCH(Mode);
 return NULL;
}
int32 sdFlushFile( FileStream * Stream  )
{
 TOUCH(Stream);
 return 0;
}
int32 sdReadFile( void *ptr, int32 Size, int32 nItems, FileStream * Stream  )
{
 UNIMPLEMENTED("sdReadFile");
 TOUCH(ptr);
 TOUCH(Size);
 TOUCH(nItems);
 TOUCH(Stream);
 return 0;
}
int32 sdWriteFile( void *ptr, int32 Size, int32 nItems, FileStream * Stream  )
{
 UNIMPLEMENTED("sdWriteFile");
 TOUCH(ptr);
 TOUCH(Size);
 TOUCH(nItems);
 TOUCH(Stream);
 return 0;
}
int32 sdSeekFile( FileStream * Stream, int32 Position, int32 Mode )
{
 UNIMPLEMENTED("sdSeekFile");
 TOUCH(Stream);
 TOUCH(Position);
 TOUCH(Mode);
 return 0;
}
int32 sdTellFile( FileStream * Stream )
{
 UNIMPLEMENTED("sdTellFile");
 TOUCH(Stream);
 return 0;
}
int32 sdCloseFile( FileStream * Stream )
{
 UNIMPLEMENTED("sdCloseFile");
 TOUCH(Stream);
 return 0;
}
#endif
 
