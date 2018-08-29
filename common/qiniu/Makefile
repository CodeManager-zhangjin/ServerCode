CC=gcc
CPP=g++
CFLAGS= -g

OBJECTS = urlsafe_b64.o cJSON.o conf.o base.o base_io.o http.o auth_mac.o rs.o qiniuInterface.o

#-I$(LINUX_INC) -I$(CAPPATH)
CFLAGS += -I. -I../../include

# Name of the executable
EXENAME = ../../lib/libQiniu.a

EXTLIBS = -lstdc++  -lpthread
EXTLIBSPATH = 
 
.cpp.o:
	$(CPP) -c  $(CFLAGS)  -D__cplusplus $< -o $@
.c.o:
	$(CC) -c  $(CFLAGS)   $< -o $@

# Builds all object files and executable
${EXENAME} : $(OBJECTS) 
	$(AR) rc $@ $(OBJECTS)
	
# Clean up
clean :
	rm -f $(OBJECTS) 
	rm -f $(EXENAME)


