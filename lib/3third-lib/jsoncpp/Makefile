WORKROOT = ../..
INCLUDE = -I./include

LIB     = 

COMMON_DEFINES = -DLINUX -D_REENTERANT

ifeq "$(MAKECMDGOALS)" "release"
	DEFINES=$(COMMON_DEFINES) -DNDEBUG
	CPPFLAGS= -fPIC
	CFLAGS= -O2 -fPIC -Wall $(DEFINES) $(INCLUDE)  
	CXXFLAGS= -O2 -fPIC -Wall $(DEFINES) $(INCLUDE) 
else
	ifeq "$(MAKECMDGOALS)" "withpg"
		DEFINES=$(COMMON_DEFINES)
		CPPFLAGS=
		CFLAGS= -g -pg -fPIC -Wall $(DEFINES) $(INCLUDE)  
		CXXFLAGS= -g -pg -fPIC -Wall $(DEFINES) $(INCLUDE) 
	else
		DEFINES=$(COMMON_DEFINES)
		CPPFLAGS= 
		CFLAGS= -g -Wall -fPIC $(DEFINES) $(INCLUDE)  
		CXXFLAGS= -g -Wall -fPIC $(DEFINES) $(INCLUDE) 
	endif
endif

CC  = g++
CPP = g++
CXX = g++ 
AR  = ar

vpath %.cpp ./src/lib_json

LIBS         = libjson.a
RELEASE_LIBS =
EXECUTABLE   =
TEST_EXEC    =  
INSTALL_PATH = .
ULLIB_OBJS=json_reader.o json_value.o json_writer.o

all		: release
alls	: $(LIBS) $(EXECUTABLE) $(TEST_EXEC)

clean   :
	/bin/rm -f *.o
	/bin/rm -f $(EXECUTABLE) $(TEST_EXEC) $(LIBS)

rebuild : clean alls
	/bin/rm -rf  $(INSTALL_PATH)/lib
	mkdir -p $(INSTALL_PATH)/lib
	mv -f $(LIBS) $(INSTALL_PATH)/lib
	rm *.o

release : rebuild

withpg : rebuild


deps :
	g++ -MM -MG *.cpp >depends
cleanall : clean 
	rm -f *~

$(LIBS) : $(ULLIB_OBJS)
	$(AR) rcv $@ $^

-include depends



