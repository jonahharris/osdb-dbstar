/***************************************************************************
 *                                                                         *
 * db.linux                                                                *
 * open source platform support package for Linux (tm)                     *
 *                                                                         *
 * Copyright (c) 2000 Centura Software Corporation. All rights reseved.    *
 *                                                                         *
 * Use of this software, whether in source code format, or in executable,  *
 * binary object code form, is governed by the CENTURA OPEN SOURCE LICENSE *
 * which is fully described in the LICENSE.TXT file, included within this  *
 * distribution of source code files.                                      *
 *                                                                         *
 **************************************************************************/


#if defined(UNIX)
    #include <sys/wait.h>
#endif

#if defined(QNX)
    #include <tchar.h>
#endif /* QNX */

#if defined(LINUX)
    #define __USE_GNU
    #define _GNU_SOURCE
    #include <sys/time.h>
#endif /* LINUX */

#if defined(BSDI)
    #include <stdio.h>
    #include <time.h>
    #include <rune.h>
#endif /* BSDI */

#if defined(WIN32)
    #include <sys\timeb.h>
#endif /* WIN32 */

#include <wchar.h>

#if defined(SOLARIS) || defined(ISOLARIS)
    #include <widec.h>
#endif /* SOLARIS or ISOLARIS */

#if defined(LINUX)
    #include <wctype.h>
#endif

#if defined(UNIX) && !defined(QNX)
    #include <values.h>
    #define NEED_STRUPR
#endif /* UNIX but not QNX */

#if defined(VXWORKS)
    #define NO_UNICODE
#endif /* VXWORKS */

#if defined(UNIXWARE) || defined(BSDI) || defined(AIX) || defined(LINUX)
    #define NEED_WCSICMP
#endif /* UNIXWARE or BSDI or AIX */

#if defined(_UNICODE)
    #define UNICODE
#endif

#define DB_BYTE    unsigned char
#define DB_USHORT  unsigned short
#define DB_UINT    unsigned int
#define DB_ULONG   unsigned long
#define DB_BOOLEAN int

#if defined(BITSPERBYTE)
    #define BITS_PER_BYTE BITSPERBYTE
#else
    #define BITS_PER_BYTE 8
#endif

#if defined(UNICODE)

    #if !defined(WIN32)
        #error UNICODE should only be defined on Windows platforms
    #endif

    #define DB_TCHAR      wchar_t
    #define DB_TINT       wint_t
    #define DB_TEXT(s)    L ## s
    #define DB_TEOF       WEOF
    #define DB_STRFMT     L"%S"
    #define MAIN          wmain

    #define atot(a, t, l) atow(a, t, l)
    #define gettstring    getwstring
    #define ttoa(t, a, l) wtoa(t, a, l)
    #define ttow(t, w, l) wcsncpy(w, t, l)
    #define vistalnum     iswalnum
    #define vistalpha     iswalpha
    #define vistdigit     iswdigit
    #define vistlower     iswlower
    #define vistupper     iswupper
    #define vistspace     iswspace
    #define vfputtc       fputwc
    #define vfputts       fputws
    #define vftprintf     fwprintf
    #define vgettc        getwc
    #define vgettchar     getwchar
    #define vputtc        putwc
    #define vstprintf     swprintf
    #define vstscanf      swscanf
    #define vtctime       _wctime
    #define vtfopen       _wfopen
    #define vtotlower     towlower
    #define vtotupper     towupper
    #define vtprintf      wprintf
    #define vtremove      _wremove
    #define vtrename      _wrename
    #define vtscanf       wscanf
    #define vtstat        _wstat
    #define vtstrcat      wcscat
    #define vtstrchr      wcschr
    #define vtstrcmp      wcscmp
    #define vtstrcpy      wcscpy
    #define vtstricmp     _wcsicmp
    #define vtstrlen      wcslen
    #define vtstrlwr      _wcslwr
    #define vtstrncat     wcsncat
    #define vtstrncmp     wcsncmp
    #define vtstrncpy     wcsncpy
    #define vtstrnicmp    _wcsnicmp
    #define vtstrrchr     wcsrchr
    #define vtstrtol      wcstol
    #define vtstrupr      _wcsupr
    #define vttoi         _wtoi
    #define vttol         _wtol
    #define vungettc      ungetwc
    #define vvftprintf    vfwprintf
    #define vvstprintf    vswprintf

#else /* UNICODE */

    #define DB_TCHAR      char
    #define DB_TINT       int
    #define DB_TEXT(s)    s
    #define DB_TEOF       EOF
    #define DB_STRFMT     "%s"
    #if defined(VXWORKS)
        #define MAIN          MOD ## main
    #else
        #define MAIN          main
    #endif

    #define atot(a, t, l) strncpy(t, a, l)
    #define gettstring    getcstring
    #define ttoa(t, a, l) strncpy(a, t, l)
    #define ttow(t, w, l) atow(t, w, l)
    #define vistalnum     isalnum
    #define vistalpha     isalpha
    #define vistdigit     isdigit
    #define vistlower     islower
    #define vistupper     isupper
    #define vistspace     isspace
    #define vfputtc       fputc
    #define vfputts       fputs
    #define vftprintf     fprintf
    #define vgettc        getc
    #define vgettchar     getchar
    #define vputtc        putc
    #define vstprintf     sprintf
    #define vstscanf      sscanf
    #define vtctime       ctime
    #define vtfopen       fopen
    #define vtotlower     tolower
    #define vtotupper     toupper
    #define vtprintf      printf
    #define vtremove      remove
    #define vtrename      rename
    #define vtscanf       scanf
    #define vtstat        stat
    #define vtstrcat      strcat
    #define vtstrchr      strchr
    #define vtstrcmp      strcmp
    #define vtstrcpy      strcpy
    #define vtstricmp     stricmp
    #define vtstrlen      strlen
    #define vtstrlwr      strlwr
    #define vtstrncat     strncat
    #define vtstrncmp     strncmp
    #define vtstrncpy     strncpy
    #define vtstrnicmp    strnicmp
    #define vtstrrchr     strrchr
    #define vtstrtol      strtol
    #define vtstrupr      strupr
    #define vttoi         atoi
    #define vttol         atol
    #define vungettc      ungetc
    #define vvftprintf    vfprintf
    #define vvstprintf    vsprintf

#endif /* UNICODE else */

#if defined(NO_UNICODE)
    #define atow(a, w, l) strncpy(w, a, l)
    #define wtoa(w, a, l) strncpy(a, w, l)
    #define vgetwc        getc
    #define vwcslen       strlen
    #define vwcsncmp      strncmp
    #define vwcsncpy      strncpy
    #define vwcsncoll     strncmp
    #define vwcsnicoll    strnicmp
    #define DB_WEOF       EOF
#else /* NO_UNICODE */
    #if defined(WIN32)
        #define atow(a, w, l) MultiByteToWideChar(CP_ACP, 0, a, -1, w, l)
        #define wtoa(w, a, l) WideCharToMultiByte(CP_ACP, 0, w, -1, a, l, NULL, NULL)
    #else /* WIN32 */
        #define atow(a, w, l) mbstowcs(w, a, l)
        #define wtoa(w, a, l) wcstombs(a, w, l)
    #endif /* WIN32 else */
    #if defined(QNX)
        #define vgetwc        _ugetc
        #define vwcslen       _ustrlen
        #define vwcsncmp      _ustrncmp
        #define vwcsncpy      _ustrncpy
        #define vwcsncoll     _ustrncmp
        #define vwcsnicoll    _ustrnicmp
        #define DB_WEOF       EOF
    #else /* QNX */
        #define vgetwc        getwc
        #define vwcslen       wcslen
        #define vwcsncmp      wcsncmp
        #define vwcsncpy      wcsncpy
        #if defined(WIN32)
            #define vwcsncoll     _wcsncoll
            #define vwcsnicoll    _wcsnicoll
        #else /* WIN32 */
            #define vwcsncoll     wcsncmp
            #define vwcsnicoll    wcsnicmp
        #endif /* WIN32 else */
        #define DB_WEOF       WEOF
    #endif /* QNX else */
#endif /* NO_UNICODE else */

#if defined(BSDI)
    #define towupper toupper /* It appears from the man pages that toupper and */
    #define towlower tolower /* tolower handle wide characters */
#endif /* BSDI */

#if defined(WIN32) || defined(QNX)
    #include <mbstring.h>
    #define vmbsncmp      _mbsncmp
    #define vmbsnicmp     _mbsnicmp
#else /* WIN32 or QNX */
    #define vmbsncmp      strncmp
    #define vmbsnicmp     strnicmp
#endif /* WIN32 or QNX else */

#if defined(UNIX)
    #define stricmp  strcasecmp
    #define strnicmp strncasecmp

    #if !defined(NEED_WCSICMP)
        #if defined(SOLARIS) || defined(ISOLARIS)
             #define wcsicmp  wscasecmp
             #define wcsnicmp wsncasecmp
        #else /* SOLARIS or ISOLARIS */
             #define wcsicmp  wcscasecmp
             #define wcsnicmp wcsncasecmp
        #endif /* SOLARIS or ISOLARIS else */
    #endif /* NEED_WSICMP */

    #if !defined(UNIXWARE) && !defined(SOLARIS) && !defined(ISOLARIS)
        #define getwc    getw
    #endif

    #define fnupper(a) (a)
    #define DIRCHAR DB_TEXT('/')
#else /* UNIX */
    #define fnupper(a) vtotupper(a)
    #define DIRCHAR DB_TEXT('\\')
    #define NO_PREPRO
#endif /* UNIX else */

#if defined(VXWORKS)
    #define VXSTARTUP(name, dbg)\
    int parse_ ## MOD(char *a1, char *a2, char *a3, char *a4, char *a5, char *a6,\
                     char *a7, char *a8, char *a9, char *a10)\
    {\
        int   argc = 0;\
        char *argv[12] = { name, a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, NULL);\
    \
        while (argv[++argc])\
            ;\
    \
        return MOD##_main(argc, argv);\
    }\
    \
    int MOD(int a1, int a2, int a3, int a4, int a5, int a6, int a7, int a8, int a9,\
            int a10)\
    {\
        int debug = dbg;\
    \
        if (!debug) {\
            taskSpawn("t" name, 99, VX_STDIO, 0x20000, (FUNCPTR) parse_ ## MOD,\
                    a1, a2, a3,  a4, a5, a6, a7, a8, a9, a10);\
        }\
        else {\
            parse_ ## MOD((char *) a1, (char *) a2, (char *) a3, (char *) a4,\
                    (char *) a5, (char *) a6, (char *) a7, (char *) a8,\
                    (char *) a9, (char *) a10);\
        }\
    }
#else /* VXWORKS */
    #define VXSTARTUP(name, dbg)
#endif /* VXWORKS else */
