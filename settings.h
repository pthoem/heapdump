//*****************************************************************************
// File ..................: settings.h
// Description ...........: Settings for this Project
// Author ................: Peter Thoemmes
//-----------------------------------------------------------------------------
// Copyright (c) 2018 Peter Thoemmes, Weinbergstrasse 3a, 54441 Ockfen
//*****************************************************************************

//*****************************************************************************
// Include control (begin):
//*****************************************************************************

#ifndef SETTINGS_H_
#define SETTINGS_H_

//*****************************************************************************
// Standard headers:
//*****************************************************************************

#ifndef _MSC_VER //MSVC++
    #include <unistd.h>
    #include <stdint.h>
    #include <sys/time.h>
#endif
#include <sys/types.h>
#include <time.h>
#include <limits.h>
#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <stddef.h>
#include <string.h>
#include <ctype.h>
#include <errno.h>
#include <fcntl.h>
#include <assert.h>
#include <malloc.h>

//*****************************************************************************
// Other stuff:
//*****************************************************************************

//#include <string>
//using namespace std;

//-----------------------------------------------------------------------------
// Unused variables:
//-----------------------------------------------------------------------------

#ifdef _MSC_VER //MSVC++
    //No warning 'unreferenced inline function has been removed':
    #pragma warning(disable:4514)
    //No warning 'truncated identifier':
    #pragma warning(disable:4786)
    //No warning 'decorated name length exceeded...truncated':
    #pragma warning(disable:4503)
    //No warning 'conversion from ... to ...':
    #pragma warning(disable:4244)
#endif

#if defined(_WIN32) || defined(_WIN64)

    typedef __int16 int16;
    typedef __int32 int32;
    typedef __int64 int64;

    typedef unsigned __int16 uint16;
    typedef unsigned __int32 uint32;
    typedef unsigned __int64 uint64;

    #define NEW_LINE__ "\r\n"

#else

    #include <unistd.h> //sbrk()

    typedef int16_t int16;
    typedef int32_t int32;
    typedef int64_t int64;

    typedef uint16_t uint16;
    typedef uint32_t uint32;
    typedef uint64_t uint64;

    #define NEW_LINE__ "\n"

#endif

//*****************************************************************************
// Include control (end):
//*****************************************************************************

#endif
