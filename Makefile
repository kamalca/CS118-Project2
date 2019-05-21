.SILENT:

default: clientsource.c serversource.c
	gcc -g -o client clientsource.c
	gcc -g -o server serversource.c

dist: clientsource.c serversource.c Makefile README report.pdf
	tar -zcf project2_504988167_604939143.tar

clean:
	rm -f client server *.tar
