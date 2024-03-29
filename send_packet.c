#include "send_packet.h"

static float loss_probability = 0.0f;

void set_loss_probability(float x)
{
    loss_probability = x;
    srand48(time(NULL));
}

ssize_t send_packet(int sock, void *buffer, size_t size, int flags, struct sockaddr *addr, socklen_t addrlen)
{
    float rnd = drand48();
    if (rnd < loss_probability)
        return size;

    return sendto(sock, buffer, size, flags, addr, addrlen);
}