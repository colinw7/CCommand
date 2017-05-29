all:
	cd src; make

clean:
	cd src; make clean
	rm -f lib/libCCommand.a
	rm -f bin/CCommandTest
