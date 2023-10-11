CFLAGS = -std=c11 -Wall -Wextra -pedantic -O2
CPPFLAGS = -I../include -DEBUG
LDFLAGS = 
EXE = takuzu

all : takuzu 

takuzu : takuzu.o 
	$(CC) $(CFLAGS) $(CPPFLAGS) -o  $@ $^ $(LDFLAGS)

takuzu.o : takuzu.c ../include/takuzu.h 
	$(CC) $(CFLAGS) $(CPPFLAGS) -c $^

clean : 
	@rm -rf *.o $(EXE)

help : 
	echo "Usage : "
	@echo "  make [all]\t\tBuild the software"
	@echo "  make clean\t\tRemove all files and outdated software"
	@echo "  make help\t\tDisplay this help"
	
.PHONY : all clean help