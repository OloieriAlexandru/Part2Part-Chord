CCPP=g++
SRCFOLDER=src
BUILDFOLDER=build

all: clean release

release: clean
	${CCPP} ./${SRCFOLDER}/server.cpp ./${SRCFOLDER}/command-line.cpp -o ./${BUILDFOLDER}/server.bin -lpthread
	${CCPP} ./${SRCFOLDER}/client.cpp -o ./${BUILDFOLDER}/client.bin -lpthread

runcl: 
	./${BUILDFOLDER}/client.bin

runsv:
	./${BUILDFOLDER}/server.bin

clean:
	rm -f ${BUILDFOLDER}/server.bin
	rm -f ${BUILDFOLDER}/client.bin