README.TXT for ptap 0.37 (27. March 2015)

ptap is Copyright (C) 1998-2015 Markus Brenner <markus@brenner.de>
homepage: http://arnold.c64.org/~minstrel/


========================================
= Introduction                         =
========================================

   ptap plays back .TAP files to a real Commodore 64 tape recorder
   model C2N, or a C16 type (black) tape recorder (model 1531).

   ************************************************************
   ***  WARNING: NEVER RUN THIS PROGRAM UNDER WINDOWS !!!   ***
   ************************************************************

   REQUIREMENTS:

   - Commodore Datassette recorder or 100% compatible

   - C64S tape interface adaptor or compatible (see links for information
     where to order this)
     with +5 V power supply for adaptor (may be obtained from joystick port,
     ideally use a C64's tape interface to obtain the correct voltage)

     OR
     
     X1531 adaptor for connecting a C16-style black 1531 tape unit

     OR

     C64 computer and X1541/XA1541/XE1541 cable.

   - Microsoft DOS and cwsdpmi.exe software


========================================
= Usage                                =
========================================

   1) connect Datassette to parallel port using C64S or X1531 adaptor
   2) from plain DOS type:
      ptap tapname.tap (if using the C64S style tape interface)
   3) when prompted insert tape in Datassette and press <RECORD/PLAY>


   OR


   1) transfer 'tapserv' to your C64.
   2) connect Datassette to C64
   3) connect C64 to parallel PC port using X1541 or XE1541 cable.
   4) LOAD "TAPSERV" and RUN it on C64
   5) Boot PC to plain DOS and start ptap with

      ptap -x tapname.tap  (if playing back through X1541 cable)
      OR
      ptap -xe tapname.tap (if playing back through XE1541 cable)
      OR
      ptap -xa tapname.tap (if playing back trhough XA1541 cable)

   6) if the border flashes _without_ <RECORD/PLAY> pressed, remove all
      peripherals from C64, until border stops flashing
   7) when prompted insert tape in Datassette and press <RECORD/PLAY>


   The whole .TAP file will be played back to your tape. You're not able
   to interrupt a running playback process, so have some patience.
   After the whole TAP has been played back, press <STOP> to stop the tape.


========================================
= Additional Information on TAPSERV    =
========================================

   TAPSERV is a C64-native program that converts your C64 into a TAP
   relay station. This allows you to record and play back TAP files
   with a PC connected to C64 by a standard X(E)1541 cable and a C2N
   Datasette.

   To use TAPSERV you need to transfer TAPSERV.PRG from PC to your C64,
   either to tape or disk.

   Disk: Connect your 1541/1571 disk drive to PC, using the X(E)1541 cable.
         Use Star Commander to copy "TAPSERV.PRG" to disk. On the C64, store
         TAPSERV to tape for better access when transfering TAPS:
         SAVE "TAPSERV"

   Tape: Connect your C64 to PC, using the X(A/E)1541 cable.
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

   0.22  First public 'beta' release
   0.23  According to Andreas Boose fixed 'ZERO' to 2500 ticks.
         Choice of LPT port
   0.24  added delay after tape starts
   0.25  changed calibration delay to 10000
         and start recording 1s after hitting <Enter>
   0.26  added TAP Version 1 support
   0.27  bugfixes
   0.28  bugfixes in Version 1 support
   0.29  started work on X(E)1541 transfer
   0.30  nearly complete rewrite (timer, LPT detection, XE-cables)
   0.31  added VIC-20 PAL/NTSC switches
   0.32  C16,C116,PLUS/4 support
   0.33  Various improvements
   0.34  Usage message updated
   0.35  Fixes X(E)1541 support
   0.36  added XA1541 support
   0.37  fixed timer not initialized bug

========================================
= Links and additional Information     =
========================================

  The latest version of this program is available on
  http://arnold.ml.org/~minstrel/


   - Hakan Sundell's .TAP specification

     http://www.computerbrains.com/tapformat.html    (current specification)
     http://www.fatal-design.com/ccs64/              (CCS64 Homepage)
     http://www.dejanews.com/getdoc.xp?AN=372073838  (historical news posting)


   - Fabrizio Gennari's: Tape Info-Central (Info about tape transfers)

     http://www.geocities.com/SiliconValley/Platform/8224/c64tape/


   - Circuit-diagrams and order form for the adaptor and cables

     http://www.phs-edv.de/c64s/doc/lpt64.htm  (C64S adaptor diagram)
     http://www.phs-edv.de/mailbox/order.htm   (C64S adaptor order form)
     http://arnold.c64.org/~minstrel/adapter/  (tips for the power supply)
     http://sta.c64.org/cables.html            (diagrams and shop for X-cables)


   - CCS64 homepage

     http://www.computerbrains.com/ccs64/


   - Tomaz Kac's excellent C64 emulation utilites (voc2tap etc.)

     http://warez.sd.uni-mb.si/zx/Utilities/64utils.zip


   - TAP pages:

     http://cia.c64.org/
     http://tapes.c64.org/


   "Thank you!" to all people for helping me out with information
                and test driving:
    

   - Andreas Boose
   - Ben Castricum
   - Tim Denning
   - Joe Forster
   - Martijn van der Heide
   - Tomaz Kac
   - Chris Link
   - Martin Pugh
   - Tom Roger Skauen
   - Richard Storer
   - Hakan Sundell
   - Nicolas Welte
   - Endre Piret
   - Terje Abbedissen

