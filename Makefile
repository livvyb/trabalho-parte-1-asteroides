# Compilador e Flags obrigatórias
CC = gcc
CFLAGS = -Wall -Wextra -pedantic -g -O2

# Executável final
TARGET = trabalho

# Todos os arquivos objeto
OBJS = cli.o estrutura.o item.o query.o reader.o reader_nasa.o reader_patrimonio.o \
       lista_seq.o lista_enc.o lista_cabeca.o lista_ord.o \
       lista_gen.o lista_cruz.o pilha.o fila.o main.o

# Todos os headers: se qualquer .h mudar, os .o são recompilados
HEADERS = $(wildcard *.h)

# Regra principal
all: $(TARGET)

# Regra de linkagem (Gera o executável 'trabalho')
$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $(TARGET) $(OBJS) -lm

# Regras de compilação genéricas para transformar .c em .o
%.o: %.c $(HEADERS)
	$(CC) $(CFLAGS) -c $< -o $@

# Regra para limpar os compilados
clean:
	rm -f $(OBJS) $(TARGET)

.PHONY: all clean