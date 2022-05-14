#ifndef SEND_PACKET_H
#define SEND_PACKET_H

#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <netdb.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <sys/time.h>
#include <arpa/inet.h>

/*
 * @set_loss_probability: sets the probability of losing a packet to a value between 0 and 1
 * @send_packet: lossy alternative to the builtin sendto-function
 */
void set_loss_probability(float x);
ssize_t send_packet(int sock, void* buffer, size_t size, int flags, struct sockaddr* addr, socklen_t addrlen);

#endif