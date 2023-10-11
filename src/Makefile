# Basic make file for Aspic.

# The normal targets in this Makefile are not called directly, but should be
# called from the top-level Makefile.

# Compile step for the modules

.SUFFIXES:     .o .c
.c.o:;         @echo "$(CC) $*.c"
	       $(FE)$(CC) -c $(CFLAGS) $*.c


# Object module lists for the program

ASPICOBJ =     aspic.o rditem.o rdsubs.o read.o tables.o tree.o \
               write.o wrps.o wrsv.o


# Link step for the program

aspic:         $(ASPICOBJ)
	       @echo "$(CC) -o aspic"
	       $(FE)$(CC) $(CFLAGS) $(LFLAGS) -o aspic $(ASPICOBJ) -lm
	       @echo ">>> aspic command built"; echo ""


# Dependencies

aspic.o:       Makefile ../Makefile aspic.c   aspic.h mytypes.h
rditem.o:      Makefile ../Makefile rditem.c  aspic.h mytypes.h
rdsubs.o:      Makefile ../Makefile rdsubs.c  aspic.h mytypes.h
read.o:        Makefile ../Makefile read.c    aspic.h mytypes.h
tables.o:      Makefile ../Makefile tables.c  aspic.h mytypes.h
tree.o:        Makefile ../Makefile tree.c    aspic.h mytypes.h
write.o:       Makefile ../Makefile write.c   aspic.h mytypes.h
wrps.o:        Makefile ../Makefile wrps.c    aspic.h mytypes.h
wrsv.o:        Makefile ../Makefile wrsv.c    aspic.h mytypes.h
            
# Clean up etc

clean:;        /bin/rm -f *.o aspic 

# End
