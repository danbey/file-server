CC=gcc
CXX=g++
RM=rm -f
CPPFLAGS=-g
#$(shell root-config --cflags)
# -Wall
LDFLAGS=-g
#$(shell root-config --ldflags)
LDLIBS=-ljsoncpp
#$(shell root-config --libs)
INCLUDEJSON=-I./json/include/

$(DBY DEBUG AAAAAA)

SRCS=servercore.cpp serverconnection.cpp fileoperator.cpp ftpserver.cpp
CLIENT_SRCS=client.cpp

OBJS=$(subst .cpp,.o,$(SRCS))
CLIENT_OBJS=$(subst .cpp,.o,$(CLIENT_SRCS))

all: ftpserver ftpclient

ftpserver: $(OBJS)
	$(CXX) $(LDFLAGS) $(INCLUDEJSON) -o ftpserver $(OBJS) $(LDLIBS)

ftpclient: $(CLIENT_OBJS)
	$(CXX) $(LDFLAGS) $(INCLUDEJSON) -o ftpclient $(CLIENT_OBJS) $(LDLIBS)


depend: .depend

.depend: $(SRCS)
	$(RM) ./.depend
	$(CXX) $(CPPFLAGS) $(INCLUDEJSON) -MM $^>>./.depend;

clean: 
	$(RM) $(OBJS) $(CLIENT_OBJS) ftpserver ftpclient

distclean: clean
	$(RM) *~ .depend

include .depend
