CC=g++
CFLAGS= -g

OBJECTS = Dispatch.o DispatchInterface.o

#-I$(LINUX_INC) -I$(CAPPATH)
CFLAGS += -I. -I../include -I../../include

# Name of the executable
EXENAME = ../lib/libDispatch.a

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


