SUPPORT_FILE_DIRECTORY = /opt/ti-mspgcc/include

DEVICE  = msp430fr5739
CC      = msp430-elf-g++

CXXFLAGS = -I $(SUPPORT_FILE_DIRECTORY) -I ../../ -mmcu=$(DEVICE) -g -O0 -mhwmult=none
LFLAGS = -L $(SUPPORT_FILE_DIRECTORY)

OBJ = cc1101.o
ID = 0

%.o : ../../%.cpp
	$(CC) -c $(CXXFLAGS) $(LFLAGS) $< -o $@

node: $(OBJ)
	$(CC) $(CXXFLAGS) $(LFLAGS) -DDEVICEID=$(ID) $< node.cpp -o node.elf

all: node

install: node
	mspdebug rf2500 "prog node.elf"

clean: 
	rm -f *.elf *.asm *.o
