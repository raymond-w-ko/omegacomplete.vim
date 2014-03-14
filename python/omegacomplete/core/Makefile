all:
	rm -rf build
	if hash python2 2>/dev/null; then python2 setup.py build; else python setup.py build; fi
	cp build/lib.*/core.* ../omegacomplete

mac:
	rm -rf build
	CFLAGS=-Wunused-command-line-argument-hard-error-in-future python setup.py build
	cp build/lib.*/core.* ../omegacomplete
