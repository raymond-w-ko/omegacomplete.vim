all:
	rm -rf build
	if hash python2 2>/dev/null; then python2 make.py build; else python make.py build; fi
	cp build/lib.*/core.* ../omegacomplete

mac:
	rm -rf build
	CFLAGS=-Wunused-command-line-argument-hard-error-in-future python make.py build
	cp build/lib.*/core.* ../omegacomplete
