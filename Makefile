CCPP=g++
SRCFOLDER=src
BUILDFOLDER=build

all: clean release

release: clean
	${CCPP} ./${SRCFOLDER}/server.cpp ./${SRCFOLDER}/command-line.cpp ./${SRCFOLDER}/helper.cpp ./${SRCFOLDER}/hash/sha1.cpp ./${SRCFOLDER}/chord.cpp -o ./${BUILDFOLDER}/server.bin -lpthread

runsv:
	./${BUILDFOLDER}/server.bin 3500

clean:
	rm -f ${BUILDFOLDER}/server.bin
