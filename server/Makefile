CC = g++

#CFLAGS = -march=native -Ofast -flto -fuse-linker-plugin -fwhole-program
CFLAGS = -march=native -O3
#CFLAGS = -O0 -ggdb

BOOST_COMPILER=-mgw47
BOOST_THREADING=-mt
BOOST_DEBUG=
BOOST_VERSION=-1_49

BOOST_SUFFIX=
#BOOST_SUFFIX=$(BOOST_COMPILER)$(BOOST_THREADING)$(BOOST_DEBUG)$(BOOST_VERSION)

LDFLAGS =
LDFLAGS += -lboost_system$(BOOST_SUFFIX) -lboost_thread$(BOOST_SUFFIX)

SOURCES = EntryPoint.cpp Server.cpp Session.cpp Participant.cpp Room.cpp
SOURCES += Buffer.cpp TrieNode.cpp LookupTable.cpp CompletionSet.cpp
SOURCES += Tags.cpp TagsSet.cpp GlobalWordSet.cpp stdafx.cpp TestCases.cpp

HEADERS = $(SOURCES:.cpp=.hpp)
OBJECTS = $(SOURCES:.cpp=.o)
EXECUTABLE = omegacomplete_server

all: $(SOURCES) $(EXECUTABLE)

$(EXECUTABLE): $(OBJECTS)
	$(CC) $(CFLAGS) $(OBJECTS) -o $@ $(LDFLAGS) 

%.o: %.cpp $(HEADERS)
	$(CC) $(CFLAGS) -c $< -o $@

stdafx.hpp.gch: stdafx.hpp
	$(CC) $(CFLAGS) -Winvalid-pch -x c++-header -c stdafx.hpp
	
clean:
	rm -f *.gch *.o omegacomplete_server
