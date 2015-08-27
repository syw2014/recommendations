# Copyright (c) 2014 Cesanta Software
# All rights reserved


PUB_DIR:=/home/lscm/codebase
ILP_LIB:=$(PUB_DIR)/ilplib/include/
IZENE_LIB:=$(PUB_DIR)/izenelib/include/
JSON_INC:=../lib/3third-lib/jsoncpp/include/
JSON_LIB:=../lib/3third-lib/jsoncpp/lib/
SRC_INC:= ../include/

PROG = q_recommend
CFLAGS = -W -Wall -I../.. -pthread -g -O2 -DMONGOOSE_ENABLE_THREADS $(CFLAGS_EXTRA)
SOURCES = $(PROG).cc mongoose.c indexEngine.cpp recommendEngine.cpp $(PUB_DIR)/ilplib/source/knlp/normalize.cpp 

CC=g++
all: $(PROG) output

LDFLAGS=-lboost_system -lglog -ljson -lboost_filesystem
$(PROG): $(SOURCES) 
	$(CC) -o $(PROG) $(SOURCES) $(CFLAGS) $(LDFLAGS) -I $(SRC_INC) -I $(ILP_LIB) -I $(IZENE_LIB) -I $(JSON_INC) -L $(JSON_LIB)


output: $(PROG)
	@echo; echo "Copy executable file(s)..."
	@mv -v $(PROG) ../bin

clean:
	rm -rf ../bin/$(PROG) *.exe *.dSYM *.obj *.exp .*o *.lib
