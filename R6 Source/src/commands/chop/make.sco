#
# makefile
#
# Copyright (C) 1993 Be Inc.  All Rights Reserved
#

TOOL1	= chop
TOOL2	= unchop
INC 	= 

CC 	= cc
CFLAGS	= $(INC)
AS	= cc
ASFLAGS	= 
LD	= cc
LDFLAGS	= 

OBJS1 = chop.o
OBJS2 = unchop.o

all:	clean $(TOOL1) $(TOOL2)

$(TOOL1): $(OBJS1) makefile
	$(LD) -o $(TOOL1) $(OBJS1) $(LDFLAGS)

$(TOOL2): $(OBJS2) makefile
	$(LD) -o $(TOOL2) $(OBJS2) $(LDFLAGS)

clear clean shrink:
	-rm -f $(OBJS1) $(OBJS2) $(TOOL1) $(TOOL2)
	-rm -f [a-z] *.exe

