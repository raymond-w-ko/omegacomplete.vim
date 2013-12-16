all:
	rm -rf build
	if hash python2 2>/dev/null; then python2 setup.py build; else python setup.py build; fi
	cp build/lib.*/core.* ../omegacomplete
