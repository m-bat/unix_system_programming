all: mydhcpd mydhcpc

mydhcpd: mydhcpd.o
		gcc -o mydhcpd mydhcpd.o

mydhcpc: mydhcpc.o
		gcc -o mydhcpc mydhcpc.o


mydhcpd.o: mydhcpd.c
		gcc -c mydhcpd.c

mydhcpc.o: mydhcpc.c
		gcc -c mydhcpc.c


