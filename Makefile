
CC = gcc
CFLAGS = -Wall -Wextra -pedantic -std=c11
OBJETOS = main.o cli.o item.o query.o reader.o reader_nasa.o estrutura.o lista_seq.o
EXECUTAVEL = trabalho

all: $(EXECUTAVEL)

$(EXECUTAVEL): $(OBJETOS)
	$(CC) $(CFLAGS) -o $(EXECUTAVEL) $(OBJETOS) -lm

main.o: main.c cli.h item.h query.h reader.h
	$(CC) $(CFLAGS) -c main.c -o main.o

cli.o: cli.c cli.h
	$(CC) $(CFLAGS) -c cli.c -o cli.o

item.o: item.c item.h
	$(CC) $(CFLAGS) -c item.c -o item.o

query.o: query.c query.h item.h
	$(CC) $(CFLAGS) -c query.c -o query.o

reader.o: reader.c reader.h reader_nasa.h item.h
	$(CC) $(CFLAGS) -c reader.c -o reader.o

reader_nasa.o: reader_nasa.c reader_nasa.h reader.h item.h
	$(CC) $(CFLAGS) -c reader_nasa.c -o reader_nasa.o

estrutura.o: estrutura.c estrutura.h lista_seq.h item.h query.h
	$(CC) $(CFLAGS) -c estrutura.c -o estrutura.o

lista_seq.o: lista_seq.c lista_seq.h item.h
	$(CC) $(CFLAGS) -c lista_seq.c -o lista_seq.o

run: $(EXECUTAVEL)
	./$(EXECUTAVEL) $(ARGS)

clean:
	rm -f $(OBJETOS) $(EXECUTAVEL)

.PHONY: all run clean
