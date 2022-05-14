#ifndef UTILS
#define UTILS

#include <math.h>
#include <netdb.h>
#include <ctype.h>
#include "send_packet.h"

#define BUFSIZE 1500
#define ACK_OK_SIZE 9

#define MAX_NICK_LENGTH 20
#define MAX_MESSAGE_LENGTH 1400

/*
 * @validate_args: validates the command line arguments when attempting to run a client or a server
 */
void validate_args(int argc, char const **argv, char server);

#endif