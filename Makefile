CFLAGS = -Wall -g -Werror -Wno-error=unused-variable

# Portul pe care asculta serverul
PORT = 12345

# Adresa IP a serverului
IP_SERVER = 127.0.0.1

# ID-ul clientului
ID_CLIENT = lel737

all: server subscriber

common.o: common.c

# Compileaza server.c
server: server.c common.o -lm

# Compileaza subscriber.c
subscriber: subscriber.c common.o -lm

.PHONY: clean run_server run_subscriber

# Ruleaza serverul
run_server:
	./server ${PORT}

# Ruleaza subscriberul 	
run_subscriber:
	./subscriber ${ID_CLIENT} ${IP_SERVER} ${PORT}

run_subscriber2:
	./subscriber ohoho ${IP_SERVER} ${PORT}

clean:
	rm -rf server subscriber *.o *.dSYM
