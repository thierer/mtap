README.TXT for mtap 0.36 (09 June 2002)

mtap is Copyright (C) 1998-2002 Markus Brenner <markus@brenner.de>
homepage: http://arnold.c64.org/~minstrel/


========================================
= Introduction                         =
========================================

   mtap produces .TAP files from Commodore 64, VIC-20 and C16 tapes for use
   with true tape emulation on the CCS64, VICE and YAPE emulators.

   ************************************************************
   ***  WARNING: NEVER RUN THIS PROGRAM UNDER WINDOWS !!!   ***
   ************************************************************

   REQUIREMENTS:

   - Commodore Datassette recorder or 100% compatible

   - C64S X1541/tape interface adapter or compatible (see links for information
     where to order this)
     with +5 V power supply for adapter (may be obtained from joystick port,
     ideally use a C64's tape interface to obtain the correct voltage)

     OR

     C64 computer and X1541 or XE1541 cable

   - Microsoft DOS and cwsdpmi.exe software


========================================
= Usage                                =
========================================


   1) connect Datassette to parallel port using C64S adapter
   2) from plain DOS start mtap with:
      mtap tapname.tap (if using the C64S style tape interface)
   3) when prompted insert tape in Datassette and press <PLAY>


   OR

   1) transfer 'tapserv' to your C64.
   2) connect Datassette to C64
   3) connect C64 to parallel PC port using X1541 or XE1541 cable.
   4) LOAD "TAPSERV" and RUN on C64
   5) Boot PC to plain DOS and start mtap with

      mtap -x tapname.tap  (if recording through X1541 cable)
      OR
      mtap -xe tapname.tap (if recording through XE1541 cable)
      OR
      mtap -xa tapname.tap (if recording through XA1541 cable)

   6) if the border flashes _without_ <PLAY> pressed, remove all
      peripherals from C64, until border stops flashing
   7) when prompted insert tape in Datassette and press <PLAY>


   The program will record all pulses between pressing <PLAY> on tape
   and <STOP>ping it.


   If you are recording VIC-20 or C16 tapes you need to use an additional
   commandline switch:

   mtap -xe -vicpal tapname.tap    (for PAL (European) VIC-20 tapes)
   mtap -xe -vicntsc tapname.tap   (for NTSC (US) VIC-20 tapes)
   mtap -xe -c16pal tapname.tap    (for PAL (European) C16 tapes)
   mtap -xe -c16ntsc tapname.tap   (for NTSC (US) C16 tapes)


========================================
= Common Problems                      =
========================================

   Buffer Size
   ===========

   mtap uses a default buffer size of 4 MB. This is enough to record
   most commercial tapes. But if you want to record exceedingly long
   tapes or multi-game collections the buffer will eventually be too
   small and mtap will crash with a nasty error message.

   Please use the -b <buffersize> switch in mtap to set a higher
   buffer size.

   For example:

   mtap -b 10 -xe summer1.tap

   will use 10 MB of buffer to record the first side of 'Summer Games',
   recording through an XE1541 cable.
   

   Batch Files
   ===========

   If you are going to use mtap for recording a lot of tapes, you
   might want to configure mtap to your needs. You can do this easily
   by using DOS BATCH files:

   For example, create the file "MYTAP.BAT" including the following line:

   mtap -b 10 -xe %1

   This batch file will invoke mtap with a buffer size of 10 MB and tells
   mtap to record through an XE1541 cable. The output file will be given
   by the first command-line parameter, like:

   mytap matrix.tap


========================================
= Additional Information on TAPSERV    =
========================================

   TAPSERV is a C64-native program that allows you to convert your C64
   into a TAP relay station. This allows you to record and play back
   TAP files with a PC connected to C64 by a standard X(E)1541 cable
   and a C2N Datasette.

   To use TAPSERV you need transfer TAPSERV.PRG from PC to your C64, either
   to tape or disk.

   Disk: Connect your 1541/1571 disk drive to PC, using the X(E)1541 cable.
         Use Star Commander to copy "TAPSERV.PRG" to disk. On the C64, store
         TAPSERV to tape for better access when transfering TAPS:
         SAVE "TAPSERV"

   Tape: Connect your C64 to PC, using the X(E)1541 cable.
         Use Torsten Paul's VC1541 program to transfer "TAPSERV.PRG"
         to C64. Save the program to tape:
         SAVE "TAPSERV"

   Before recording or playing back TAP files with mtap or ptap, load
   TAPSERV from tape:
   LOAD "TAPSERV":RUN


   Star Commander:  http://sta.c64.org/
   VC1541 software: http://os.inf.tu-dresden.de/~paul/VC1541/


========================================
= History                              =
========================================

   0.10  First version to recorded a working .TAP
   0.12  .TAP header corrections:
         - fill 'future use' field with: 00 00 00 00
         - set filelength entry to correct data length value
         - honor <STOP> event on datassette
   0.13  Improved use of '00' bytes - one 00 byte represents 2550 ticks
   0.14  Again improved '00' bytes timing.
         '00' now represents 1/50 s, this is 2463 ticks
         Print program name and version at startup.
         Set output name extension to "TAP".
         Removed bug which saved huge files when in fact no pulse was recorded.
   0.15  Added a Borderflasher during tape reading. (thanks Richard Storer!)
         fixed first pulse after longpulses
   0.16  Choice of LPT port
         delay play key
   0.17  According to Andreas Boose fixed 'ZERO' to 2500 ticks.
         Added a command line switch to increase buffer size.
   0.18  Minor change in 'Usage'
   0.19  Added support for X(E)1541 cables
   0.20  Bugfixes
   0.21  added TAP Version 1 support
   0.22  fix first pulse after pause
   0.23  made Version 1 the default
   0.24  fixed Version 1 pauses
   0.25  fixed Version 1 pauses (Ben's 8x bug)
   0.26  added -vicpal  switch for recording VIC-20 (PAL)  tapes
   0.27  added -vicntsc switch for recording VIC-20 (NTSC) tapes
   0.28   changed to new TAPSERV standard
   0.29   added LPT detection, improved program structure
   0.30   C16,C116,PLUS/4 support
   0.31   experimental halfwave support
   0.32   fixed bug in argcmp()
   0.33   fixed severe overflow bug in calculation of pauses
   0.34   improved conversion by using long long integers
   0.35   warn user if output file exists, improved rounding
   0.36   added XA1541 support

   to do:
         - optional starting and stopping with 'esc' key
         - tape adjustment tool


========================================
= Links and additional Information     =
========================================

  The latest version of this program is available on
  http://arnold.ml.org/~minstrel/

   - H†kan Sundell's .TAP specification
     http://www.computerbrains.com/tapformat.html

   - Circuit-diagrams and order form for the adaptor and cables
     http://www.phs-edv.de/c64s/doc/lpt64.htm  (C64S adaptor diagram)
     https://ssl.phs-edv.de/order.htm          (C64S adaptor order form)
     http://arnold.c64.org/~minstrel/adapter/  (tips for the power supply)
     http://sta.c64.org/cables.html            (diagrams and shop for X-cables)

   - Tomaz Kac's excellent C64 emulation utilites (voc2tap etc.)
     http://warez.sd.uni-mb.si/zx/Utilities/64utils.zip

   - Richard Storer's Tapload, another TAP generation tool
     http://members.tripod.com/rstorer/c64

   - CCS64 homepage
     http://www.computerbrains.com/ccs64/

   - VICE homepage
     http://www.cs.cmu.edu/~dsladic/vice/vice.html

   - YAPE, the first Plus/4 emulator to support the TAP format
     http://yape.plus4.net/



   "Thank you!" to all people who helped me out with information:

   - Andreas Boose         <boose@linux.rz.fh-hannover.de>
   - Ben Castricum         <B.Castricum@gns.getronics.nl>
   - Tim Denning           <tim@timsplace.screaming.net>
   - Joe Forster           <sta@c64.org>
   - Attila Grosz          <grosza@hotmail.com>
   - Martijn van der Heide <mheide@bns.getronics.nl>
   - Tomaz Kac             <tomaz.kac@uni-mb.si>
   - Chris Link            <Chris.Link@StudServ.Stud.Uni-Hannover.DE>
   - Martin Pugh           <martin@pugh.prestel.co.uk>
   - Tom Roger Skauen      <tomsk@powertech.no>
   - Richard Storer        <rstorer@cyberspace.org>
   - H†kan Sundell         <Hakan.Sundell@xpress.se>
   - Nicolas Welte         <welte@chemie.uni-konstanz.de>
