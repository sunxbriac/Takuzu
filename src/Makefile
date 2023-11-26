CFLAGS = -std=c11 -Wall -Wextra -pedantic -O2 -ggdb3
CPPFLAGS = -I../include -DEBUG
LDFLAGS = 
EXE = takuzu

all : takuzu 

rebuild : clean takuzu

debug: takuzu.o
	$(CC) $(CFLAGS) -g3 $(CPPFLAGS) -o $(EXE) $^ $(LDFLAGS)

takuzu : takuzu.o grid.o
	$(CC) $(CFLAGS) $(CPPFLAGS) -o  $@ $^ $(LDFLAGS)

takuzu.o : takuzu.c ../include/takuzu.h 
	$(CC) $(CFLAGS) $(CPPFLAGS) -c $^

grid.o : grid.c ../include/grid.h 
	$(CC) $(CFLAGS) $(CPPFLAGS) -c $^

clean : 
	@rm -rf *.o $(EXE)

help : 
	echo "Usage : "
	@echo "  make [all]\t\tBuild the software"
	@echo "  make clean\t\tRemove all files and outdated software"
	@echo "  make help\t\tDisplay this help"
	
.PHONY : all clean help