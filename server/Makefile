CC = g++

CFLAGS =
CFLAGS += -O0

LDFLAGS =
LDFLAGS += -lboost_system -lboost_filesystem -lboost_regex -lwsock32 -lws2_32

SOURCES = EntryPoint.cpp Server.cpp Session.cpp Participant.cpp Room.cpp
SOURCES += Buffer.cpp
HEADERS = $(SOURCES:.cpp=.o)
OBJECTS = $(SOURCES:.cpp=.o)
EXECUTABLE = omegacomplete_server.exe

all: stdafx.hpp.gch $(SOURCES) $(EXECUTABLE)

$(EXECUTABLE): Makefile $(OBJECTS) 
	$(CC) $(CFLAGS) $(OBJECTS) -o $@ $(LDFLAGS) 

.cpp.o: $(HEADERS)
	$(CC) $(CFLAGS) -c $< -o $@

stdafx.hpp.gch: Makefile stdafx.hpp
	$(CC) $(CFLAGS) -Winvalid-pch -x c++-header -c stdafx.hpp
	
clean:
	del *.gch
	del *.o
