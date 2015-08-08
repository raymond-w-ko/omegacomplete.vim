.PHONY: all mac speedtest trienodetest

all:
	rm -rf build
	rm -f all.cpp
	cat *.cpp > all.cpp
	if hash python2 2>/dev/null; then python2 make.py build; else python make.py build; fi
	cp build/lib.*/core.* ../omegacomplete

mac:
	rm -rf build
	rm -f all.cpp
	cat *.cpp > all.cpp
	CFLAGS=-Wunused-command-line-argument-hard-error-in-future python make.py build
	cp build/lib.*/core.* ../omegacomplete

speedtest:
	rm -f all.cpp
	cat *.cpp > all.cpp
	gcc \
		-std=c++11 \
		-pthread -fno-strict-aliasing -pipe -fstack-protector --param=ssp-buffer-size=4 -DNDEBUG -fPIC -I/usr/include/python2.7 \
		-Wall -Wno-char-subscripts -mtune=native -march=native -fno-stack-protector -fstrict-aliasing -Ofast -flto \
		-I.. -I. \
		all.cpp Tests/SpeedTest.cpp \
		-o speedtest \
		-L/usr/lib -lboost_thread -lboost_system -lboost_chrono -lboost_filesystem -lpython2.7 -lstdc++
	./speedtest

trienodetest:
	rm -f all.cpp
	cat *.cpp > all.cpp
	gcc \
		-std=c++11 \
		-pthread -fno-strict-aliasing -pipe -fstack-protector --param=ssp-buffer-size=4 -DNDEBUG -fPIC -I/usr/include/python2.7 \
		-Wall -Wno-char-subscripts -mtune=native -march=native -fno-stack-protector -fstrict-aliasing -Ofast -flto \
		-I.. -I. \
		all.cpp Tests/TrieNodeTest.cpp \
		-o trienodetest \
		-L/usr/lib -lboost_thread -lboost_system -lboost_chrono -lboost_filesystem -lpython2.7 -lstdc++
	(cd ./wordlists && ../trienodetest)
