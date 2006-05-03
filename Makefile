# ----------------------------------------------------------------------------
#
# PuZ: (N^2 - 1)-Puzzle Solver
# Copyright (C) 2006 Lionel Imbs & Benjamin Gaillard
#
# ----------------------------------------------------------------------------
#
#        File: Makefile
# Description: Project Makefile
#
# ----------------------------------------------------------------------------
#
# This program is free software; you can redistribute it and/or modify it
# under the terms of the GNU General Public License as published by the Free
# Software Foundation; either version 2 of the License, or (at your option)
# any later version.
#
# This program is distributed in the hope that it will be useful, but WITHOUT
# ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
# FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
# more details.
#
# You should have received a copy of the GNU General Public License along
# with this program; if not, write to the Free Software Foundation, Inc., 59
# Temple Place - Suite 330, Boston, MA 02111-1307, USA.
#
# ----------------------------------------------------------------------------


##############################################################################
#
# Programs and Flags
#
#

#
# C
#

# GCC (GNU Compiler Collection)
#CC = gcc
#CFLAGS = -O2 -fomit-frame-pointer -pipe -std=c99 -pedantic -Wall -W
#CLD = gcc
#CLDFLAGS =

# ICC (Intel C++ Compiler)
CC = icc
CFLAGS = -c99 -tpp2 -O3 -ipo -Wall -wd981
CLD = icc
CLDFLAGS = -O3 -ipo

#
# UPC
#

# GCC-UPC
#UPCC = upc
#UPCFLAGS = -O2 -fomit-frame-pointer -pipe -std=c99 -Wall -W \
#	   -fupc-threads-$(UPCTHREADS)
#UPCLDFLAGS = -fupc-threads-$(UPCTHREADS)

# Berkeley UPC with ICC backend
UPCC = upcc
UPCFLAGS = -Wc,"-c99 -Wall -wd174,188,193,593,810,1418,1683" -T $(UPCTHREADS)
UPCLDFLAGS = -T $(UPCTHREADS)


#
# Common
#

# UPC thread number
UPCTHREADS = 4

# Preprocessor flags
CPPFLAGS = -DNDEBUG
#CPPFLAGS = -DDEBUG


##############################################################################
#
# Files
#
#

EXESUFFIX =
OBJSUFFIX = .o

COMMON_SRC = input.c main.c util.c
C_SRC = $(COMMON_SRC) ida-seq.c
UPC_SRC = $(COMMON_SRC) ida-prl.upc

C_TARGET = puz-c$(EXESUFFIX)
C_OBJ = $(C_SRC:.c=_c$(OBJSUFFIX))

UPC_TARGET = puz-upc$(EXESUFFIX)
UPC_OBJ_C = $(UPC_SRC:.c=_upc$(OBJSUFFIX))
UPC_OBJ = $(UPC_OBJ_C:.upc=_upc$(OBJSUFFIX))

#default: $(C_TARGET) $(UPC_TARGET)
default: $(UPC_TARGET)


##############################################################################
#
# Targets
#
#

#
# Special Targets
#

# Pseudo targets
.PHONY: default clean test results
.SUFFIXES:
.SUFFIXES: .c .upc _c$(OBJSUFFIX) _upc$(OBJSUFFIX)

# Implicit targets
.c_c$(OBJSUFFIX):
	$(CC) $(CFLAGS) $(CPPFLAGS) -c $< -o $@
.c_upc$(OBJSUFFIX) .upc_upc$(OBJSUFFIX):
	$(UPCC) $(UPCFLAGS) $(CPPFLAGS) -c $< -o $@


#
# Main Targets
#

$(C_TARGET): $(C_OBJ)
	$(CLD) $(CLDFLAGS) $^ -o $@
	strip $@

$(UPC_TARGET): $(UPC_OBJ)
	$(UPCC) $(UPCLDFLAGS) $^ -o $@
	strip $@

clean:
	rm -f $(C_OBJ) $(C_TARGET) $(UPC_OBJ) $(UPC_TARGET)

test: $(C_TARGET)
	test UULDDLUU = "`./$(C_TARGET) -q -i matrices/example.txt`"

matrices/results.txt: $(C_TARGET)
	rm -f $@
	for file in matrices/*.txt; do \
	    echo -n "`basename $$file .txt`: "; \
	    ./$(C_TARGET) -q -i $$file; \
	done | tee $@

results: matrices/results.txt


#
# Explicit dependencies
#

ida-seq_c$(OBJSUFFIX) ida-prl_upc$(OBJSUFFIX): common.h ida.h input.h util.h
input_c$(OBJSUFFIX) input_upc$(OBJSUFFIX): common.h input.h
main_c$(OBJSUFFIX) main_upc$(OBJSUFFIX): common.h ida.h input.h util.h

# End of File
