#ifndef SERVER_UTILS
#define SERVER_UTILS

#include "utils.h"

#define ACK_NOT_FOUND_SIZE 16
#define CLIENT_INFO_SIZE 22

// The servers client-cache
typedef struct client
{
    char *nick;
    char *address;
    uint16_t port;
    long last_registered;

    struct sockaddr_in sock_addr;
    struct client *next;
} client;

/*
 * @initiate_client_register: creates and returns a dummy for accessing registered clients
 * @register_client: registers a new client in a local client register
 * @update_client: updates client info if a nick tries to register with a new address and port
 * @find_client_by_nick: traverses the register and returns a client if its nick matches. Returns NULL if no matches are found
 * @find_client_by_address: same as above, but matches against address and port
 * @remove_client: frees and removes a client from the register.
 * @free_clients: frees ALL clients in register. Used for cleaning up
 */
client *initiate_client_register();
void register_client(client *head, char *nick, char *address, uint16_t port, long registered);
void update_client(client *current, char *address, uint16_t port, long registered);
client *find_client_by_nick(client *head, char *nick);
client *find_client_by_address(client *head, char *address, uint16_t port);
void remove_client(client *head, char *nick);
void free_clients(client *head);

/*
 * @deserealise_message: deserealises message
 * @receive_message: intercepts message received from client and returns result from applying @deserealise_message
 * @free_request: frees messages received from client
 */
char **deserealise_message(char *message);
char **receive_message(int fd, struct sockaddr_in *address);
void free_request(char **request);

/*
 * @get_port_length: returns number of digits in port
 * @send_ack_ok: sends a message on the format "ACK number OK" to given address
 * @send_ack_notfound: sends "ACK number NOT FOUND" to client if requested nick not in local client register
 * @send_client_info: sends information about requested client if nick exists in local client register
 * @check_error: if result is 0, we free cache and socket and exit with EXIT_FAILURE
 * @check_perror: If result is -1, we free cache and socket and exit with EXIT_FAILURE
 */
int get_port_length(int port);
void send_ack_ok(int fd, struct sockaddr_in address, char number);
void send_ack_notfound(int fd, struct sockaddr_in address, char number);
void send_client_info(int fd, struct sockaddr_in address, char number, client *current);
void check_error(int result, char *msg, int line, const char *function);
void check_perror(int result, char *msg);

#endif