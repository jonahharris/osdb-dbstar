/***************************************************************************
 *                                                                         *
 * db.*                                                                    *
 * open source database, ida utility                                       *
 *                                                                         *
 * Copyright (c) 2000 Centura Software Corporation. All rights reserved.   *
 *                                                                         *
 * Use of this software, whether in source code format, or in executable,  *
 * binary object code form, is governed by the CENTURA OPEN SOURCE LICENSE *
 * which is fully described in the LICENSE.TXT file, included within this  *
 * distribution of source code files.                                      * 
 *                                                                         *
 **************************************************************************/

/*--------------------------------------------------------------------------
    Terminal I/O Control Module
--------------------------------------------------------------------------*/

#if defined(QNX) || defined(BSDI)

/*************************** QNX VERSION *****************************/

ioc_on()
{
}


ioc_off()
{

}

#else  /* QNX */



/************************* UNIX SYSTEM V VERSION **************************/
#include <stdio.h>
#include <termio.h>
#include <unistd.h>

/**************************** LOCAL VARIABLES *****************************/
static struct termio tio1, tio2;
static int iostat = 0;

/* ========================================================================
    Turn on program's terminal i/o control
*/
int ioc_on()
{
    if (!iostat)
    {
        ioctl(0, TCGETA, &tio1);
        ioctl(0, TCGETA, &tio2);
        tio2.c_lflag = tio2.c_lflag & ~ISIG;
        tio2.c_lflag = tio2.c_lflag & ~ICANON;
        tio2.c_lflag = tio2.c_lflag & ~ECHO;
        tio2.c_cc[VMIN] = '\1';
        tio2.c_cc[VTIME] = '\0';
        ioctl(0, TCSETA, &tio2);
        iostat = 1;
    }
    return 0;
}

/* ========================================================================
    Turn off program's terminal i/o control
*/
int ioc_off()
{
    if (iostat)
    {
        ioctl(0, TCSETA, &tio1);
        iostat = 0;
    }

    return 0;
}

#endif /* QNX */


