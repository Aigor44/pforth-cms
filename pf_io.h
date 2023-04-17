/* @(#) pf_io.h 98/01/26 1.2 */
#ifndef _pf_io_h
#define _pf_io_h
 
/***************************************************************
** Include file for PForth IO
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
 
#ifdef PF_NO_CHARIO
 int  sdTerminalOut( char c );
 int  sdTerminalFlush( void );
 int  sdTerminalIn( void );
 int  sdQueryTerminal( void );
#else   /* PF_NO_CHARIO */
 #ifdef PF_USER_CHARIO
/* Get user prototypes or macros from include file.
** API must match that defined above for the stubs.
*/
/* If your sdTerminalIn echos, define PF_KEY_ECHOS. */
  #include PF_USER_CHARIO
 #else
  #define sdTerminalOut(c)  putchar(c)
  #define sdTerminalIn      getchar
/* Since getchar() echos, define PF_KEY_ECHOS. */
  #define PF_KEY_ECHOS
/*
 * If you know a way to implement ?TERMINAL in STANDARD ANSI 'C',
 * please let me know.  ?TERMINAL ( -- charAvailable? )
 */
  #define sdQueryTerminal()   (0)
  #ifdef PF_NO_FILEIO
   #define sdTerminalFlush() /* fflush(PF_STDOUT) */
  #else
   #define sdTerminalFlush() fflush(PF_STDOUT)
  #endif
 #endif
#endif   /* PF_NO_CHARIO */
 
 
/* Define file access modes. */
/* User can #undef and re#define using PF_USER_FILEIO if needed. */
#define PF_FAM_READ_ONLY (0)
#define PF_FAM_READ_WRITE (1)
 
#define PF_FAM_CREATE  ("w+")
#define PF_FAM_OPEN_RO  ("r")
#define PF_FAM_OPEN_RW  ("r+")
 
#ifdef PF_NO_FILEIO
 
 typedef void FileStream;
 
 extern FileStream *PF_STDIN;
 extern FileStream *PF_STDOUT;
 
 #ifdef __cplusplus
 extern "C" {
 #endif
 
 /* Prototypes for stubs. */
 FileStream *sdOpenFile( const char *FileName, const char *Mode );
 int32 sdFlushFile( FileStream * Stream  );
 int32 sdReadFile( void *ptr, int32 Size, int32 nItems, FileStream * Stream  );
 int32 sdWriteFile( void *ptr, int32 Size, int32 nItems, FileStream * Stream  );
 int32 sdSeekFile( FileStream * Stream, int32 Position, int32 Mode );
 int32 sdTellFile( FileStream * Stream );
 int32 sdCloseFile( FileStream * Stream );
 int32 sdInputChar( FileStream *stream );
 
 #ifdef __cplusplus
 }
 #endif
 
 #define  PF_SEEK_SET   (0)
 #define  PF_SEEK_CUR   (1)
 #define  PF_SEEK_END   (2)
 /*
 ** printf() is only used for debugging purposes.
 ** It is not required for normal operation.
 */
 #define PRT(x) /* No printf(). */
 
#else
 
 #ifdef PF_USER_FILEIO
/* Get user prototypes or macros from include file.
** API must match that defined above for the stubs.
*/
  #include PF_USER_FILEIO
  
 #else
  typedef FILE FileStream;
 
  #define sdOpenFile      fopen
  #define sdFlushFile     fflush
  #define sdReadFile      fread
  #define sdWriteFile     fwrite
  #define sdSeekFile      fseek
  #define sdTellFile      ftell
  #define sdCloseFile     fclose
  #define sdInputChar     fgetc
  
  #define PF_STDIN  ((FileStream *) stdin)
  #define PF_STDOUT ((FileStream *) stdout)
  
  #define  PF_SEEK_SET   (0)
  #define  PF_SEEK_CUR   (1)
  #define  PF_SEEK_END   (2)
  
  /*
  ** printf() is only used for debugging purposes.
  ** It is not required for normal operation.
  */
  #define PRT(x) { printf x; sdFlushFile(PF_STDOUT); }
 #endif
 
#endif  /* PF_NO_FILEIO */
 
 
#ifdef __cplusplus
extern "C" {
#endif
 
cell ioAccept( char *Target, cell n1, FileStream *Stream );
cell ioKey( void);
void ioEmit( char c );
void ioType( const char *s, int32 n);
void ioTypeA2E( const char *s, int32 n);
 
#ifdef __cplusplus
}
#endif
 
#endif /* _pf_io_h */
