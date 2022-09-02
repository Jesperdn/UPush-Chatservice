CFLAGS = --track-origins=yes --malloc-fill=0x40 --free-fill=0x23 --leak-check=full --show-leak-kinds=all 
SERVER_DEPS = linked_list.c send_packet.c upush_server.c utils.c
CLIENT_DEPS = linked_list.c send_packet.c upush_client.c utils.c

SERVER_IP = 127.0.0.1
SERVER_PORT = 8000
TIMEOUT = 5
LOSS = 0
NICKNAME = batman



all: server client
	
server: ${SERVER_DEPS}
	gcc ${SERVER_DEPS} -o upush_server

client: ${CLIENT_DEPS}
	gcc ${CLIENT_DEPS} -o upush_client

valgrind_server:
	gcc ${SERVER_DEPS} -o upush_server
	valgrind ${CFLAGS} ./upush_server ${SERVER_PORT} ${LOSS}

valgrind_client:
	gcc ${CLIENT_DEPS} -o upush_client
	valgrind ${CFLAGS} ./upush_client ${NICKNAME} ${SERVER_IP} ${SERVER_PORT} ${TIMEOUT} ${LOSS}

clean:
	rm upush_client
	rm upush_server

	
