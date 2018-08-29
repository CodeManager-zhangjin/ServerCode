CC=g++
CFLAGS= -g
CFLAGS= -g -DCACHE_DEVICE_STATUS
#CFLAGS= -g -D_TEST_STORAGE_

OBJECTS = Device.o DeviceInterface.o

#-I$(LINUX_INC) -I$(CAPPATH)
CFLAGS += -I. -I../include -I../../include

# Name of the executable
EXENAME = ../lib/libDevice.a

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


