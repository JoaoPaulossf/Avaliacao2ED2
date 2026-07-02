# Compilador e flags
CC = gcc
CFLAGS = -Wall -Wextra -g

# Nome do executavel
EXEC = sistema_rh

# Comando padrão de compilação
all: $(EXEC)

# Regra para gerar o executável conectando os objetos
$(EXEC): main.o Bplus.o AuxRH.o
	$(CC) $(CFLAGS) main.o Bplus.o AuxRH.o -o $(EXEC)

# Regra para compilar o main.c
main.o: main.c Bplus.h AuxRH.h
	$(CC) $(CFLAGS) -c main.c

# Regra para compilar a arvore B+
Bplus.o: Bplus.c Bplus.h
	$(CC) $(CFLAGS) -c Bplus.c

# Regra para compilar as funcoes do RH
AuxRH.o: AuxRH.c AuxRH.h
	$(CC) $(CFLAGS) -c AuxRH.c

# Comando exigido para compilar e executar o sistema automaticamente
run: $(EXEC)
	./$(EXEC)

# Comando exigido para limpar objetos, executavel e o banco de dados binario
clean:
	rm -f *.o $(EXEC) *.bin