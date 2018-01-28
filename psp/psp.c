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

/* psp.c - Contains generic routines of the PSP */

#include "psp.h"
#include "pspint.h"

int psp_inited = 0;
int psp_terminating = 0;

/* ===========================================================================
   Initialize the PSP subsystem
*/
int psp_init(
    void)
{
    if (psp_inited++ == 0) {
        if (psp_syncInit() != PSP_OKAY) {
            psp_inited = 0;
            return PSP_FAILED;
        }

        if (psp_memInit() != PSP_OKAY) {
            psp_syncTerm();
            psp_inited = 0;
            return PSP_FAILED;
        }

        if (psp_fileInit() != PSP_OKAY) {
            psp_memTerm();
            psp_syncTerm();
            psp_inited = 0;
            return PSP_FAILED;
        }

        if (psp_osInit() != PSP_OKAY) {
            psp_fileTerm();
            psp_memTerm();
            psp_syncTerm();
            psp_inited = 0;
            return PSP_FAILED;
        }
    }

    return PSP_OKAY;
}

/* ===========================================================================
   Shut down the PSP subsystem
*/
void psp_term(
    void)
{
    if (psp_inited == 1) {
        psp_terminating = 1;
        psp_osTerm();
        psp_fileTerm();
        psp_syncShutdown();
        psp_memTerm();
        psp_syncTerm();
        psp_inited = 0;
    }
	else
        psp_inited--;
}

#if defined(NEED_STRICMP)
/* ===========================================================================
   Case insensitive comparison
*/
int stricmp(
    const char *s1,
    const char *s2)
{
    int result;
    int c1;
    int c2;

    if (!s1)
        return s2 ? -1 : 0;

    if (!s2)
        return 1;

    do {
        c1 = (int) ((unsigned char) (islower(*s1) ? toupper(*s1) : *s1));
        c2 = (int) ((unsigned char) (islower(*s2) ? toupper(*s2) : *s2));
        result = c1 - c2;   /* arithmetic over-/under-flow not possible */
        s1++;
        s2++;
    } while (!result && c1 && c2);

    return result;
}

/* ===========================================================================
   Case insensitive comparison up to n bytes
*/
int strnicmp(
    const char *s1,
    const char *s2,
    size_t      len)
{
    int result;
    int c1;
    int c2;

    if (!s1)
        return s2 ? -1 : 0;

    if (!s2)
        return 1;

    for (result = 0; len > 0 && !result; s1++, s2++, len--) {
        c1 = (int) ((unsigned char) (islower(*s1) ? toupper(*s1) : *s1));
        c2 = (int) ((unsigned char) (islower(*s2) ? toupper(*s2) : *s2));
        result = c1 - c2;  /* arithmetic over-/under-flow not possible */
    }

    return result;
}
#endif

#if defined(NEED_STRUPR)
/* ===========================================================================
   Convert the string to upper case
*/
char *strupr(
    char *str)
{
    register char *p;

    if (!str)
        return NULL;

    for (p = str; *p; p++) {
        if (islower(*p))
            *p = (char) toupper(*p);
    }

    return str;
}

/* ===========================================================================
   Convert the string to lower case
*/
char *strlwr(
    char *str)
{
    register char *p;

    if (!str)
        return NULL;

    for (p = str; *p; p++) {
        if (isupper(*p))
            *p = (char) tolower(*p);
    }

    return str;
}
#endif

#if defined(NEED_WCSICMP)
int wcsicmp(
    const wchar_t *s1,
    const wchar_t *s2)
{
    int result = 0;

    do {
        result = (int) towupper(*s1) - (int) towupper(*s2);
    } while (*s1++ && *s2++ && !result);

    return result;
}

int wcsnicmp(
   const wchar_t *s1,
   const wchar_t *s2,
   size_t         len)
{
   int result = 0;

   for ( ; len > 0 && !result; s1++, s2++, len--) {
       result = (int) towupper(*s1) - (int) towupper(*s2);
       if (!*s1 || !*s2)
           break;
   }

   return result;
}
#endif
