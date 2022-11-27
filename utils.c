#include "utils.h"

void validate_args(int argc, char const **argv, char server)
{
    if (server && (argc != 3))
    {
        fprintf(stderr, "usage: ./upush_server <port> <loss_probability>\n");
        exit(EXIT_FAILURE);
    }
    else if (!server && (argc != 6))
    {
        fprintf(stderr, "usage: ./upush_client <nick> <address> <port> <timeout> <loss_probability>\n");
        exit(EXIT_FAILURE);
    }

    int loss_prob = server ? 2 : 5;
    if (atoi(argv[loss_prob]) > 100 || atoi(argv[loss_prob]) < 0)
    {
        fprintf(stderr, "Error: loss probability should be between (inclusive) 0 and 100!\n");
        exit(EXIT_FAILURE);
    }
}