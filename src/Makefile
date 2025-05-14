#
# @(#) Makefile - unlu
#
CFLAGS	+= -g -I.
LDFLAGS	+= -g

OBJ	= errors.o unlu.o crc16.o

PROG	= unlu

$(PROG):	$(OBJ)

clean:
	rm -f $(OBJ)

distclean:	clean
	rm -f $(PROG)

unlu.o: unlu.c dostime.h dirent.h errors.h crc16.h
crc16.o: crc16.c
errors.o: errors.c errors.h
