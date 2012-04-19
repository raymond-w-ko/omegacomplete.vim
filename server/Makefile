CC = g++

CFLAGS = -std=c++11
CFLAGS += -O3 -march=native
CFLAGS += -flto -fwhole-program
#CFLAGS += -O0
CFLAGS += -I"C:\Boost"

BOOST_COMPILER=-mgw47
BOOST_THREADING=-mt
BOOST_DEBUG=
BOOST_VERSION=-1_49

BOOST_SUFFIX=$(BOOST_COMPILER)$(BOOST_THREADING)$(BOOST_DEBUG)$(BOOST_VERSION)

LDFLAGS =
LDFLAGS += -L"C:\Boost\stage\lib"
LDFLAGS += -lboost_system$(BOOST_SUFFIX)
LDFLAGS += -lboost_filesystem$(BOOST_SUFFIX)
LDFLAGS += -lboost_regex$(BOOST_SUFFIX)
LDFLAGS += -lboost_thread$(BOOST_SUFFIX)
LDFLAGS += -lwsock32 -lws2_32

SOURCES = EntryPoint.cpp Server.cpp Session.cpp Participant.cpp Room.cpp
SOURCES += Buffer.cpp KeywordTrie.cpp

HEADERS = $(SOURCES:.cpp=.hpp)
OBJECTS = $(SOURCES:.cpp=.o)
EXECUTABLE = omegacomplete_server.exe

all: stdafx.hpp.gch $(SOURCES) $(EXECUTABLE)

$(EXECUTABLE): $(OBJECTS)
	$(CC) $(CFLAGS) $(OBJECTS) -o $@ $(LDFLAGS) 
	strip -s $(EXECUTABLE)

%.o: %.cpp $(HEADERS)
	$(CC) $(CFLAGS) -c $< -o $@

stdafx.hpp.gch: stdafx.hpp
	$(CC) $(CFLAGS) -Winvalid-pch -x c++-header -c stdafx.hpp
	
clean:
	del *.gch
	del *.o
	del *.exe
