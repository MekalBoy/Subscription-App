# Protocoale de comunicatii
# Laborator 7 - TCP
# Echo Server
# Makefile

CFLAGS = -Wall -g -Werror -Wno-error=unused-variable

# Portul pe care asculta serverul
PORT = 12345

# Adresa IP a serverului
IP_SERVER = 127.0.0.1

# ID-ul clientului
ID_CLIENT = 1

all: server subscriber

common.o: common.c

# Compileaza server.c
server: server.c common.o

# Compileaza subscriber.c
subscriber: subscriber.c common.o

.PHONY: clean run_server run_subscriber

# Ruleaza serverul
run_server:
	./server ${PORT}

# Ruleaza subscriberul 	
run_subscriber:
	./subscriber ${ID_CLIENT} ${IP_SERVER} ${PORT}

clean:
	rm -rf server subscriber *.o *.dSYM
