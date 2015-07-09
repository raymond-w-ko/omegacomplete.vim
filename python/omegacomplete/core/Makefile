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
	gcc \
		-std=c++11 \
		-pthread -fno-strict-aliasing -march=x86-64 -mtune=generic -O2 -pipe -fstack-protector --param=ssp-buffer-size=4 -DNDEBUG -march=x86-64 -mtune=generic -O2 -pipe -fstack-protector --param=ssp-buffer-size=4 -fPIC -I/usr/include/python2.7 \
		-Wall -Wno-char-subscripts -mtune=native -march=native -fno-stack-protector -fstrict-aliasing -Ofast -flto \
		-I.. -I. \
		Algorithm.cpp  Buffer.cpp   LookupTable.cpp  Omegacomplete.cpp TagsCollection.cpp  Tags.cpp  TestCases.cpp  TrieNode.cpp  WordCollection.cpp Tests/SpeedTest.cpp \
		-o SpeedTest \
		-Wl,-O1,--sort-common,--as-needed,-z,relro -L/usr/lib -lboost_thread -lboost_system -lboost_chrono -lboost_filesystem -lpython2.7 -lstdc++
