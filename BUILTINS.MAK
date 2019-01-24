# AVRIL Makefile (using Borland C 3.1)
#
# Written by Bernie Roehl, January 1995
#

#DEBUGGER = 1

!ifdef DEBUGGER
CDEBUG = -v
LDEBUG = -v
NOVFLAGS = -v-
!endif

CC = bcc -c -ml
LINK = bcc -ml
LIBRARIAN = tlib
OBJ = .obj
LIB = .lib
EXE = .exe

CFLAGS = -3 -Ox $(CDEBUG)
CDRFLAGS = -3 $(CDEBUG)
CAUTIOUSFLAGS =
NO386 = -2
LFLAGS = -M $(LDEBUG)

# -c means "just compile, don't link"
# -ml is large model
# -v means include symbol information for debugging
# -3 means use 386 instructions
# -Ox means optimize for speed
# -M means generate a memory map (.map file).
# -N means check for stack overflows
#
# The -3 flag cannot be safely used in modules containing interrupt routines.
