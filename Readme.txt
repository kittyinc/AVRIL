
                                  AVRIL

This is release 2.0 of AVRIL, a library for creating virtual reality
applications.

Changes since 1.1 include:
    HMD support:
        CyberMaxx (VictorMaxx Technologies)
        i-glasses! (Virtual i/o)
        7th Sense (All-Pro)
    support for CyberWand from InWorld VR
    support for the FifthGlove from 5DT
    stereoscopic viewing
    Gouraud shading
    modular video driver
    modular display driver

These changes are detailed in Appendix A of the documentation file.

Unfortunately, there are some features that are not fully implemented.

    The Gouraud shading has a few flaws (little glitches here and there
    in the shading), and hasn't been exhaustively tested; it's the last
    feature I added before the release.  It's also very slow; using a
    little bit of assembly language here should speed it up considerably,
    but I'll save that for the next release.

    Not all the stereo modes are implemented yet.

    The horizon code was always broken, so I've temporarily
    disabled it; I hope to completely rewrite it for a future
    release, adding a "stepped" or "layered" horizon; if anyone
    has any code for doing this that they'd like to contribute,
    let me know!

The current version of the code is also relatively slow, since I've
converted everything to C (no assembly language at all) and haven't done
any optimization.  This is the final stage before porting to other
platforms; once the ports have been done, I'll be re-optimizing the PC
version and putting assembler into a few time-critical routines (thereby
getting the speed back up).

The latest version of this software can always be found on sunee.uwaterloo.ca,
in the pub/avril directory.

CONTENTS

The contents of this archive are as follows:

read.me         this file
av.exe          the world-viewing program
av.c            the source code for av.exe
avril.cfg       the configuration file for av.exe
avril.h         the AVRIL header file
avril.lib       the AVRIL library
avrilkey.h      definitions of keycodes
avrildrv.h      definitions for device driver functions
example1.c      example 1 from the AVRIL tutorial
example2.c      example 2 from the AVRIL tutorial
example3.c      example 3 from the AVRIL tutorial
example4.c      example 4 from the AVRIL tutorial
example5.c      example 5 from the AVRIL tutorial
example6.c      example 6 from the AVRIL tutorial
example6.cfg    the configuration file for example6
example7.c      example 7 from the AVRIL tutorial
shade32.pal     a palette used by example7
input.c         source code for some routines used by all the examples
system.c        source code for some of the system routines
cfg.c           source code for the configuration file parser
drvcyman.c      sample driver source code (for the Logitech Cyberman)
vidsamp.c       sample source code for low-level video driver (mode 0x13)
packet.c        source code for collecting packets of binary data
avriltut.ps     the AVRIL tutorial, in Postscript
avriltut.txt    the AVRIL tutorial, in plain ASCII
avrilref.ps     the AVRIL technical reference manual, in Postscript
avrilref.txt    the AVRIL technical reference manual, in plain ASCII
avrilapp.ps     the AVRIL tech ref appendices manual, in Postscript
avrilapp.txt    the AVRIL tech ref appendices manual, in plain ASCII
asteroid.plg    a PLG file used by example2
colorful.plg    a PLG file used by example3
sample.zip      the data for a sample world

USING AV

Unzip sample.zip, using the -d option to create a directory for the sample
world.  In other words, type "unzip -d sample".

To run AV, just type "av sample.wld".  You can also view individual PLG
files and FIG files using AV.

MOVING AROUND

Moving around the virtual environment is easy.  The following applies to AV
or any of the example programs (once they've been compiled).

         Key                                  Function

    up/down arrows                      move forward/backward
    left/right arrows                   turn left/right
    leftshift plus up/down arrows       move up/down
    leftshift plus left/right arrows    move left/right
    rightshift plus up/down arrows      look up/down
    rightshift plus left/right arrows   tilt head left/right

OTHER KEYS

The following keys perform various functions:

    ESC     quit
     f      toggle frame rate display
     c      toggle compass display
     p      toggle position display
     d      toggle display of debugging information
     _      (underscore) toggle horizon off/on
    +,-     zoom in, out
     =      reset zoom
    h,H     adjust hither clipping plane distance by current space step
     w      toggle wireframe mode
  (space)   toggle mouse mode between 6D input and on-screen selection

CONTACTING THE AUTHOR

My email address is broehl@sunee.uwaterloo.ca; mention AVRIL in the
subject line so I'll know what it's about.
