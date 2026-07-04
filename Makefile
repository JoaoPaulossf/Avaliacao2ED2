#Bloco de determinações para auxilio dos comandos make
EXEC = sistema_rh #Nome do executavel final
CC = gcc #Compilador e flags de compilação
CFLAGS = -Wall -Wextra -g
SRC = main.c Bplus.c AuxRH.c #Arquivos de código-fonte e objetos gerados
DEPS = Bplus.h AuxRH.h #Arquivos de hearders
OBJ = $(SRC:.c=.o)

#Bloco de comandos make
all: $(EXEC) #Comando de compilação final
$(EXEC): $(OBJ)
	$(CC) $(CFLAGS) -o $@ $^

#Regra de compilação para transformar os .c em objetos .o
%.o: %.c $(DEPS)
	$(CC) $(CFLAGS) -c $< -o $@

#Comando de compilação e execução
run: all
	./$(EXEC)

#Regra 'make clean': remove arquivos objetos, executável e arquivos binários do disco
clean:
	rm -f *.o $(EXEC) *.bin

#Comando para teste de vazamento de memória
valgrind: sistema_rh
	valgrind --leak-check=full --show-leak-kinds=all --track-origins=yes ./$(EXEC)
