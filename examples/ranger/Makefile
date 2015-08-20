SUPPORT_FILE_DIRECTORY = /opt/ti-mspgcc/include

DEVICE  = msp430fr5728
CC      = msp430-elf-g++

CXXFLAGS = -I $(SUPPORT_FILE_DIRECTORY) -I ../../ -I ../../../../targets/micro_ekho/ -mmcu=$(DEVICE) -g -O0 -mhwmult=f5series -std=c++11
LFLAGS = -L $(SUPPORT_FILE_DIRECTORY)

OBJ = cc1101.o

%.o : ../../%.cpp
	$(CC) -c $(CXXFLAGS) $(LFLAGS) $< -o $@

rx: $(OBJ)
	$(CC) $(CXXFLAGS) $(LFLAGS) $< rx.cpp -o rx.elf

tx: $(OBJ)
	$(CC) $(CXXFLAGS) $(LFLAGS) $< tx.cpp -o tx.elf

all: rx tx

installrx: rx
	mspdebug rf2500 "prog rx.elf"

installtx: tx
	mspdebug rf2500 "prog tx.elf"

clean: 
	rm -f *.elf *.asm *.o
