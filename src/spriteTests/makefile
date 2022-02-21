3DODEV	= C:/3DODev/
ARMDEV	= C:/ARM251/
COMMON_DIR	= ../common/
CD_DIR	= ../../CD/

# Project specific settings
NAME	= $(notdir $(CURDIR))
STACKSIZE = 4096

CC		= $(ARMDEV)bin/armcc
AS 		= $(ARMDEV)bin/armasm
LD		= $(ARMDEV)bin/armlink
RM		= $(3DODEV)bin/rm
MODBIN	= $(3DODEV)bin/modbin
CCFLAGS	= -O2 -Otime -bi -za1 -d DEBUG=0 -cpu ARM60
ASFLAGS	= -BI
INCPATH	= -I$(3DODEV)includes -I$(COMMON_DIR)
LDFLAGS	= -reloc -nodebug -remove -ro-base 0x80
LIBPATH	= $(3DODEV)libs/
STARTUP	= $(LIBPATH)cstartup.o
LIBS	=	$(LIBPATH)Lib3DO.lib \
			$(LIBPATH)operamath.lib \
			$(LIBPATH)graphics.lib \
			$(LIBPATH)audio.lib \
			$(LIBPATH)music.lib \
			$(LIBPATH)filesystem.lib \
			$(LIBPATH)input.lib \
			$(LIBPATH)swi.lib \
			$(LIBPATH)clib.lib

ARMLIB	= $(ARMDEV)lib
ARMINC	= $(ARMDEV)inc

SRC_S		= $(wildcard *.s) $(wildcard $(COMMON_DIR)*.s)
SRC_C		= $(wildcard *.c) $(wildcard $(COMMON_DIR)*.c)

OBJ	+= $(SRC_S:.s=.o)
OBJ	+= $(SRC_C:.c=.o) 


all: $(NAME)
	
$(NAME): $(OBJ)
	$(LD) -dupok -o $(NAME). $(LDFLAGS) $(STARTUP) $(LIBS) $(OBJ)
	$(MODBIN) $(STACKSIZE) $(NAME)

%.o: %.c
	$(CC) $(INCPATH) $(CCFLAGS) -c $< -o $@

%.o: %.s
	$(AS) $(INCPATH) $(ASFLAGS) $< -o $@

clean:
	RM -f $(OBJ)
	RM -f $(NAME)
	RM -f $(CD_DIR)LaunchMe
	RM -f $(CD_DIR)$(NAME)
	RM -f $(CD_DIR)../demo.iso

copy:
	CP $(NAME) $(CD_DIR)LaunchMe