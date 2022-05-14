CFLAGS = -g -std=gnu11 -Wall -Wextra
VG = --track-origins=yes --malloc-fill=0x40 --free-fill=0x23 --leak-check=full --show-leak-kinds=all
BIN = upush_server upush_client

all: $(BIN)

upush_server: server.c utils.c send_packet.c server.h utils.h send_packet.h
	gcc $(CFLAGS) utils.c send_packet.c server.c -o upush_server

upush_client: client.c utils.c send_packet.c client.h utils.h send_packet.h
	gcc $(CFLAGS) utils.c send_packet.c client.c -o upush_client

clean:
	rm -f $(BIN)