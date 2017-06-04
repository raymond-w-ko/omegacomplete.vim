PYTHON := python3

.PHONY: all mac speedtest trienodetest

all: normal

########################################

clean:
	rm -rf build
	rm -f all.cpp

concat:
	cat *.cpp > all.cpp
	
########################################

normal: clean concat
	$(PYTHON) make.py build
	cp build/lib.*/core.* ../omegacomplete

fast-make: clean concat
	$(PYTHON) fast-make.py build
	cp build/lib.*/core.* ../omegacomplete

mac: clean concat
	CFLAGS=-Wunused-command-line-argument-hard-error-in-future $(PYTHON) make.py build
	cp build/lib.*/core.* ../omegacomplete

########################################

speedtest: clean concat
	gcc \
		-std=c++11 \
		-pthread -fno-strict-aliasing -pipe -fstack-protector --param=ssp-buffer-size=4 -DNDEBUG -fPIC -I/usr/include/python2.7 \
		-Wall -Wno-char-subscripts -mtune=native -march=native -fno-stack-protector -fstrict-aliasing -Ofast -flto \
		-I.. -I. \
		all.cpp Tests/SpeedTest.cpp \
		-o speedtest \
		-L/usr/lib -lboost_thread -lboost_system -lboost_chrono -lboost_filesystem -lpython2.7 -lstdc++
	./speedtest

trienodetest: clean concat
	gcc \
		-std=c++11 \
		-pthread -fno-strict-aliasing -pipe -fstack-protector --param=ssp-buffer-size=4 -DNDEBUG -fPIC -I/usr/include/python2.7 \
		-Wall -Wno-char-subscripts -mtune=native -march=native -fno-stack-protector -fstrict-aliasing -Ofast -flto \
		-I.. -I. \
		all.cpp Tests/TrieNodeTest.cpp \
		-o trienodetest \
		-L/usr/lib -lboost_thread -lboost_system -lboost_chrono -lboost_filesystem -lpython2.7 -lstdc++
	(cd ./wordlists && ../trienodetest)
