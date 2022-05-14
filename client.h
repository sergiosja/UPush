#ifndef CLIENT_UTILS
#define CLIENT_UTILS

#include "utils.h"

#define REGISTER_CLIENT_SIZE 11
#define LOOKUP_SIZE 14
#define ACK_ERROR_SIZE 19
#define CLIENT_MESSAGE_SIZE 21

// Queue of outgoing packets
typedef struct packet
{
    char number;
    char *from_nick;
    char *to_nick;
    char *message;
    struct packet *next;
} packet;


// The clients client-cache
typedef struct correspondence
{
    char *nick;
    char *address;
    uint16_t port;
    char status;
    packet *queue;
    char last_received_number;
    char last_sent_number;
    long timeout;

    struct sockaddr_in sock_addr;
    struct correspondence *next;
} correspondence;


// The clients list of blocked nicks
typedef struct blocked
{
    char *nick;
    struct blocked *next;
} blocked;

/*
 * @packet_head: creates and returns a dummy to be placed first in the queue of outgoing packets
 * @register_new_packet: creates a new packet in the correct format and returns it
 * @add_packet_to_queue: adds a new outgoing packet to the end of the packet queue
 * @remove_delivered_packet_from_queue: removes a packet from the queue of outgoing packets
 * @deserealise_packet: extracts the number, nicks and message from a packet and returns them
 * @free_packets: frees ALL packets in the queue. Used for cleaning up
 */
packet *packet_head();
packet *register_new_packet(char number, char *to, char *message);
void add_packet_to_queue(packet *head, packet *current);
void remove_delivered_packet_from_queue(packet *head);
packet *deserealise_packet(char *buffer);
void free_packets(packet *p);

/*
 * @initiate_correspondence: creates and returns a dummy for accessing correspondences
 * @register_correspondence: adds a new correspondence to the current list and returns it
 * @update_correspondence: updates address, port and timeout values of registered correspondence
 * @find_correspondence_by_nick: traverses the list of correspondences. Return correspondence if there is a nick match, or NULL if else
 * @find_correspondence: same as above, but with address and port
 * @remove_correspondence: frees and removes a given correspondence
 * @free_correspondences: frees ALL correspondences with this client. Used for cleaning up
 */
correspondence *initiate_correspondence();
correspondence *register_correspondence(correspondence *head, char *nick, char status, char *buffer, packet *p);
void update_correspondence(correspondence *current, char *buffer, long timeout);
correspondence *find_correspondence_by_nick(correspondence *head, char *nick);
correspondence *find_correspondence(correspondence *head, char *address, uint16_t port);
void remove_correspondence(correspondence *head, char *nick);
void free_correspondences(correspondence *head);

/*
 * @initiate_blocklist: creates and returns a dummy for accessing blocked nicks
 * @block_client: "blocks" a nick by putting it on a particular list
 * @unblock_client: "unblocks" a nick by removing it from the list
 * @find_blocked: traverses the list and tries to match the nick. If no matches are found, NULL is returned
 * @free_blocked_clients: frees ALL entries in list. Used for cleaning up
 */
blocked *initiate_blocklist();
void block_client(blocked *head, correspondence *corr, char *nick);
void unblock_client(blocked *head, char *nick);
blocked *find_blocked(blocked *head, char *nick);
void free_blocked_clients(blocked *head);

/*
 * @initiate_registration: sends a message to server for registration
 * @heartbeat: checks if 10 seconds have passed. Returns 1 if yes and 0 if no
 * @lookup_client: requests server to look up a certain nick
 * @send_message_to_client: sends a message to another client
 * @send_ack_ok: sends a message on the format "ACK number OK" to given address
 * @send_ack_error: sends an ack to a client, but with "WRONG [NAME|FORMAT]" rather than "OK".
 */
void initiate_registration(int fd, struct sockaddr_in server_address, char number);
int heartbeat(int *previous_registration_time);
void lookup_client(int fd, struct sockaddr_in address, char number, char *nick);
void send_message_to_client(char number, char *to, int to_length,
                            char *msg, int msg_length, int fd, struct sockaddr_in dest);
void send_ack_ok(int fd, struct sockaddr_in address, char number);
void send_ack_error(int fd, struct sockaddr_in address, int format);

/*
 * @check_ack: checks if we have received an ack. Returns ack number if yes, or 2 if no
 * @verify_message: verifies format of incoming packet.
 * @read_stdin: reads the string received from stdin
 */
char check_ack(char *buffer);
int verify_message(char *buffer, blocked *check);
void read_stdin(char buffer[], int size);

/*
 * @stop_and_wait: checks if we have timed out before receiving an ack from any correspondences.
                   If yes, proceeds to either resend message, lookup client again, or give up
 * @quit: frees everything in the running program
 * @check_error: if result is 0, we call quit() and exit with EXIT_FAILURE
 * @check_perror: If result is -1, we call quit() and exit with EXIT_FAILURE
 * @validate_nick: Makes sure nick is strictly ascii-symbols
 * @validate_message: Makes sure the outgoing message does not contain 'æ', 'ø' or 'å'
 */
void stop_and_wait(correspondence *head, int fd, struct sockaddr_in server_addr, fd_set fds, char outgoing_number, int client_timeout);
void quit();
void check_error(int result, char *msg, int line, const char *function);
void check_perror(int result, char *msg);
void validate_nick(char *nick, int size);
int validate_message(char *message, int size);

#endif