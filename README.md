# Information about the 'heapdump' utility

## Description
`heapdump` is made to dump/crack/analyze the heap of

 - Linux applications, using GLIBC library to manage their heap
   
   Full source code for gcc/g++ is included in this package!

 - Win32 applications using MSVCRT.dll (Release build) or
   MSVCRTD.dll (Debug build ) to manage their heap

   Full source code for Visual C++ 6.0 is included in this package!

All important functionality is provided by 2 source files:

`heapdump.h`

`heapdump.cpp`

There is the *complete* sources for generically finding the `&main_arena` of
the glibc, as well as the *complete* sources for detecting the heap and
stack limits. Finding `&main_arena` is a tricky thing and can be very useful!

If you don't want to use the sources to put the hands on your application's
heap, then you may just want to use the pre-built tool:

### INTERACTIVE MODE:

`heapdump [-v] [-alloc_mb <size/MB>]`

### DUMP THE HEAP FOOTPRINT:

`heapdump [-v] [-alloc_mb <size/MB>] -footprint`

### DEBUG DUMP OF THE HEAP:

`heapdump [-v] [-alloc_mb <size/MB>] -debug`

### HEX DUMP OR RAW DATA OUTPUT OF THE HEAP:

`heapdump [-v] [-alloc_mb <size/MB>] [-hex|-raw] [-max_kb <size/KB>]`

```
Parameters:

   -?                   Print this screen
   -v                   Verbose output
   -alloc_mb <size/MB>  Allocate <size> MB using malloc()
                        REMARK: <size> as integer *or* floating point
   -max_kb <size/KB>    Limit the output to <size> KB
                        REMARK: <size> as integer
```

I wish you a lot of success using my work,

Peter

## Author
PTHOEM LINUX UTILS (c) 2007-2018 Peter Thoemmes

Weinbergstrasse 3a, D-54441 Ockfen/Germany


## License
GPLv3, see license file

## Example output
```
+-----------------------------------------------------------------------------+
| HEAP layout:                                                                |
+-----------------------------------------------------------------------------+

  0xffffffffffff +--------------------------+ ADDRESS SPACE TOP (256 TB)
                 |                          |
                 |                          |
                 |      MAPPED KERNEL       |
                 |                          |
                 |                          |
  0x7fffffffffff +--------------------------+ USER SPACE TOP (128 TB)
                 |        Guard Page        |
  0x7fffffffefff +--------------------------+ STACK TOP
                 |          STACK           |
                 +---||------||-------||----+
                 |   \/      \/       \/    |
                 |        Free Space        |
                 |   /\      /\       /\    |
       0x243f000 +---||------||-------||----+ MAIN HEAP TOP
                 |                          |
                 |        MAIN HEAP         |
                 |          132 KB          |
                 |                          |
       0x241e000 +--------------------------+ MAIN HEAP BOTTOM
                 |                          |
                 |      STATIC MEMORY       |
                 |         4108 KB          |
                 |                          |
        0x40585c +--------------------------+ STATIC DATA BOTTOM
                 |                          |
                 |          CODE            |
                 |         (TEXT)           |
                 |                          |
                 +--------------------------+ CODE BOTTOM
                 |                          |
             0x0 +--------------------------+ ADDRESS SPACE BOTTOM

+-----------------------------------------------------------------------------+
| HEAP footprint:                                                             |
+-----------------------------------------------------------------------------+

--------- MAIN ARENA: ---------

      0xd16000  mem:       0xd16010          48 bytes USED
      0xd16030  mem:       0xd16040         288 bytes USED
      0xd16150  * !HEAP TOP CHUNK! *     134832 bytes FREE

--------- NEXT ARENA: ---------

0x7f8180000970  * !HEAP TOP CHUNK! *     132752 bytes FREE

                 +--------------------------+ STACK TOP
                 |          STACK           |
                 +---||------||-------||----+
                 |   \/      \/       \/    |
                 |        Free Space        |
                 |   /\      /\       /\    |
                 +---||------||-------||----+
                 |                          |
                 |          HEAP            |
                 |        261 KB size       |
                 |                          |
                 |        336 BY used       |
                 |        261 KB free       |
                 |                          |
        0xd16000 +--------------------------+ HEAP BOTTOM

+-----------------------------------------------------------------------------+
| HEAP DEBUG DUMP:                                                            |
+-----------------------------------------------------------------------------+

--------- MAIN ARENA at 0x7f8184bde720:

          HEAP at 0xd16000:

      0xd16000 +-----------------------------------
               | 00 00 00 00 00 00 00 00
               +-----------------------------------
               | chunk_size = 48 BYTES | AMP = 001
               +-----------------------------------
               | 00 00 00 00 00 00 00 00 | ........
               | 00 00 00 00 00 00 00 00 | ........
               | ...
               |
               | USED (32 BYTES)
               |
      0xd16030 +-----------------------------------
               | 00 00 00 00 00 00 00 00
               +-----------------------------------
               | chunk_size = 288 BYTES | AMP = 001
               +-----------------------------------
               | 0F 00 00 00 00 00 00 00 | ........
               | 00 00 00 00 00 00 00 00 | ........
               | ...
               |
               | USED (272 BYTES)
               |
      0xd16150 +-----------------------------------
   (TOP CHUNK) | 00 00 00 00 00 00 00 00
               +-----------------------------------
               | chunk_size = 131 KB | AMP = 001
               +-----------------------------------
               | next_free = (nil)
               +-----------------------------------
               | prev_free = (nil)
               +-----------------------------------
               |
               | FREE (131 KB)
               |
      0xd37000 +---------- TOP = sbrk(0) ----------

--------- ALLOCATED ARENA at 0x7f8180000020:

          HEAP at 0x7f8180000000:

0x7f8180000000 +========== HEAP INFO ==============
               | ar_ptr = 0x7f8180000020
               +-----------------------------------
               | prev = (nil)
               +-----------------------------------
               | size = 135168 -> top = 0x7f8180021000
               +-----------------------------------
               | ...
               +===================================
0x7f8180000970 +-----------------------------------
   (TOP CHUNK) | 00 00 00 00 00 00 00 00
               +-----------------------------------
               | chunk_size = 129 KB | AMP = 001
               +-----------------------------------
               | next_free = (nil)
               +-----------------------------------
               | prev_free = (nil)
               +-----------------------------------
               |
               | FREE (129 KB)
               |
0x7f8180021000 +-------------- TOP ----------------

+-----------------------------------------------------------------------------+
| HEAP HEX DUMP:                                                              |
+-----------------------------------------------------------------------------+

MAIN ARENA at 0x7f8184bde720 (MAIN HEAP at 0xd16000):

00 00 00 00   00 00 00 00 | .... ....       0xd16000 ...       0xd16007
31 00 00 00   00 00 00 00 | 1... ....       0xd16008 ...       0xd1600f
00 00 00 00   00 00 00 00 | .... ....       0xd16010 ...       0xd16017
00 00 00 00   00 00 00 00 | .... ....       0xd16018 ...       0xd1601f
00 00 00 00   00 00 00 00 | .... ....       0xd16020 ...       0xd16027
00 00 00 00   00 00 00 00 | .... ....       0xd16028 ...       0xd1602f
00 00 00 00   00 00 00 00 | .... ....       0xd16030 ...       0xd16037
21 01 00 00   00 00 00 00 | !... ....       0xd16038 ...       0xd1603f
0F 00 00 00   00 00 00 00 | .... ....       0xd16040 ...       0xd16047
00 00 00 00   00 00 00 00 | .... ....       0xd16048 ...       0xd1604f
01 00 00 00   00 00 00 00 | .... ....       0xd16050 ...       0xd16057
00 00 00 00   00 00 00 00 | .... ....       0xd16058 ...       0xd1605f
90 46 82 84   81 7F 00 00 | .F.. ...       0xd16060 ...       0xd16067
01 00 00 00   00 00 00 00 | .... ....       0xd16068 ...       0xd1606f
00 00 00 00   00 00 00 00 | .... ....       0xd16070 ...       0xd16077
00 00 00 00   00 00 00 00 | .... ....       0xd16078 ...       0xd1607f
00 00 00 00   00 00 00 00 | .... ....       0xd16080 ...       0xd16087
00 00 00 00   00 00 00 00 | .... ....       0xd16088 ...       0xd1608f
00 00 00 00   00 00 00 00 | .... ....       0xd16090 ...       0xd16097
00 00 00 00   00 00 00 00 | .... ....       0xd16098 ...       0xd1609f
00 00 00 00   00 00 00 00 | .... ....       0xd160a0 ...       0xd160a7
00 00 00 00   00 00 00 00 | .... ....       0xd160a8 ...       0xd160af
160 more zero words -> total zero block size: 232 bytes
00 00 00 00   00 00 00 00 | .... ....       0xd16150 ...       0xd16157
B1 0E 02 00   00 00 00 00 | .... ....       0xd16158 ...       0xd1615f
00 00 00 00   00 00 00 00 | .... ....       0xd16160 ...       0xd16167
00 00 00 00   00 00 00 00 | .... ....       0xd16168 ...       0xd1616f
00 00 00 00   00 00 00 00 | .... ....       0xd16170 ...       0xd16177
00 00 00 00   00 00 00 00 | .... ....       0xd16178 ...       0xd1617f
00 00 00 00   00 00 00 00 | .... ....       0xd16180 ...       0xd16187
00 00 00 00   00 00 00 00 | .... ....       0xd16188 ...       0xd1618f
00 00 00 00   00 00 00 00 | .... ....       0xd16190 ...       0xd16197
00 00 00 00   00 00 00 00 | .... ....       0xd16198 ...       0xd1619f
```
