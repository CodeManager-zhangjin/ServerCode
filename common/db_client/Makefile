CC=g++
CFLAGS= -g

OBJECTS = CGetUserInfoMgr.o \
	        CGetUserGroupMgr.o \
	        CGetUserRoomMgr.o \
	        CGetUserDeviceMgr.o \
	        DBClient.o \
	        DBClientHandle.o \
	        MainDBClient.o \
	        DataBaseA.o \
	        StorageDBClient.o \
	        DataBaseB.o \
	        DataBaseInterface.o

#-I$(LINUX_INC) -I$(CAPPATH)
CFLAGS += -I. -I../../include

# Name of the executable
EXENAME = ../../lib/libdbclient.a

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
