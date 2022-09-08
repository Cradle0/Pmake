pmake : pmake.o helpers.o parse.o run_make.o
	gcc -Wall -g -o pmake pmake.o helpers.o parse.o run_make.o

%.o : %.c
	gcc -Wall -g -c $< 

clean :
	rm pmake *.o
