all: myftpd myftpc

mydhcpd: myftpd.o
		gcc -o myftpd myftpd.o

mydhcpc: myftpc.o
		gcc -o myftpc myftpc.o


mydhcpd.o: myftpd.c
		gcc -c myftpd.c

mydhcpc.o: myftpc.c
		gcc -c myftpc.c


