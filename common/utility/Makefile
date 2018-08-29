CC=g++
CFLAGS= -g

OBJECTS = 3des.o	\
					crc32.o \
					base64.o \
					MD5Sum.o \
					TimerManager.o \
					Http.o \
					vurl.o \
					DealMulPkt.o \
					UtilityInterface.o

#-I$(LINUX_INC) -I$(CAPPATH)
CFLAGS += -I. -I../../include

# Name of the executable
EXENAME = ../../lib/libUtility.a

EXTLIBS = -lstdc++  -lpthread
EXTLIBSPATH = 
 
.cpp.o:
	$(CC) -c  $(CFLAGS)  -D__cplusplus $< -o $@
.c.o:
	$(CC) -c  $(CFLAGS)   $< -o $@

# Builds all object files and executable
${EXENAME} : $(OBJECTS) 
	$(AR) rc $@ $(OBJECTS)
	
# Clean up
clean :
	rm -f $(OBJECTS) 
	rm -f $(EXENAME)


