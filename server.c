#include "server.h"

client *cache;
int fd;

int main(int argc, char const **argv)
{
	validate_args(argc, argv, 1);
	set_loss_probability(atof(argv[2]) / 100);

	struct sockaddr_in server_addr, client_addr;
	struct timeval registration_time;

	cache = initiate_client_register();

	// Network specs
	fd = socket(AF_INET, SOCK_DGRAM, 0);
	check_perror(fd, "socket");

	server_addr.sin_family = AF_INET;
	server_addr.sin_port = htons(atoi(argv[1]));
	server_addr.sin_addr.s_addr = INADDR_ANY;

	int rc = bind(fd, (struct sockaddr *) &server_addr, sizeof(struct sockaddr_in));
	check_perror(rc, "Socket reserved");
	printf("> Server running on port %s...\n", argv[1]);
	for (;;)
	{
        char **request = receive_message(fd, &client_addr);

        int current_client_port = ntohs(client_addr.sin_port);
        char current_client_address[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &client_addr.sin_addr.s_addr, current_client_address, INET_ADDRSTRLEN);

        // Registers new client
        if (!strcmp(request[2], "REG"))
        {
            client *existing_client = find_client_by_nick(cache, request[3]);
            gettimeofday(&registration_time, NULL);
            int current_time = registration_time.tv_sec;
            char heartbeat = '0';

            if (existing_client)
            {
                if (!strcmp(existing_client->address, current_client_address) && (existing_client->port == current_client_port))
                heartbeat = '1';
                update_client(existing_client, current_client_address, current_client_port, current_time);
            }
            else register_client(cache, request[3], current_client_address, current_client_port, current_time);
            if (heartbeat == '0') send_ack_ok(fd, client_addr, request[1][0]);
        }

        // Looks up existing client
        else if (!strcmp(request[2], "LOOKUP"))
        {
            if (find_client_by_address(cache, current_client_address, current_client_port))
            {
                client *existing_client = find_client_by_nick(cache, request[3]);

                gettimeofday(&registration_time, NULL);
                int current_time = registration_time.tv_sec;
                if (existing_client && (current_time - existing_client->last_registered > 30))
                {
                    remove_client(cache, existing_client->nick);
                    existing_client = NULL;
                }

                if (!existing_client) send_ack_notfound(fd, client_addr, request[1][0]);
                else send_client_info(fd, client_addr, request[1][0], existing_client);
            }
        }
        free_request(request);
	}

	close(fd);
	free_clients(cache);
	return EXIT_SUCCESS;
}


client *initiate_client_register()
{
	client *head = malloc(sizeof(client));
	head->nick = NULL;
	head->address = NULL;
	head->next = NULL;
	return head;
}


void register_client(client *head, char *nick, char *address, uint16_t port, long registered)
{
	client *new_client = malloc(sizeof(client));
	new_client->nick = strdup(nick);
	new_client->address = strdup(address);
	new_client->port = port;
	new_client->last_registered = registered;
	new_client->sock_addr.sin_family = AF_INET;
	new_client->sock_addr.sin_port = htons(port);
	int wc = inet_pton(AF_INET, address, &new_client->sock_addr.sin_addr.s_addr);
	check_perror(wc, "inet_pton");
	check_error(wc, "inet_pton", __LINE__, __FUNCTION__);

	client *current = head;
	while (current->next)
    	current = current->next;
	current->next = new_client;
	new_client->next = NULL;
}


void update_client(client *current, char *address, uint16_t port, long registered)
{
	free(current->address);
	current->address = strdup(address);
	current->port = port;
	current->last_registered = registered;
	current->sock_addr.sin_family = AF_INET;
	current->sock_addr.sin_port = htons(port);
	int rc = inet_pton(AF_INET, address, &current->sock_addr.sin_addr.s_addr);
	check_perror(rc, "inet_pton");
	check_error(rc, "inet_pton", __LINE__, __FUNCTION__);
}


client *find_client_by_nick(client *head, char *nick)
{
	client *current = head->next;
	while (current)
	{
		if (!strcmp(current->nick, nick))
			return current;
		current = current->next;
	}
	return NULL;
}


client *find_client_by_address(client *head, char *address, uint16_t port)
{
	client *current = head->next;
	while (current)
	{
		if (!strcmp(current->address, address) && (current->port == port))
			return current;
		current = current->next;
	}
	return NULL;
}


void remove_client(client *head, char *nick)
{
	client *current = head;
	while (current->next)
	{
		if (!strcmp(current->next->nick, nick))
		{
			client *tmp = current->next;
			current->next = current->next->next;
			free(tmp->nick);
			free(tmp->address);
			free(tmp);
			break;
		}
		current = current->next;
	}
}

void free_clients(client *head)
{
	client *current;
	while (head)
	{
		current = head;
		head = head->next;
		free(current->nick);
		free(current->address);
		free(current);
	}
}


char** deserealise_message(char *message)
{
	char **messages = malloc(sizeof(char *) * 4);
	char *token = strtok(message, " ");

	for (int i = 0; i < 4; i++)
	{
		messages[i] = strdup(token);
		token = strtok(NULL, " ");
	}
	return messages;
}


char **receive_message(int fd, struct sockaddr_in *address) {
	char buffer[BUFSIZE];
	socklen_t addr_len = sizeof(struct sockaddr_in);
	int rc = recvfrom(fd, buffer, BUFSIZE-1, 0, (struct sockaddr *) address, &addr_len);
	check_perror(rc, "recvfrom");
	buffer[rc] = 0;
	return deserealise_message(buffer);
}


void free_request(char **request)
{
	for (int i = 0; i < 4; i++)
	free(request[i]);
}


int get_port_length(int port)
{
	int length = 0;
	while (port)
	{
		port /= 10;
		length++;
	}
	return length;
}


void send_ack_ok(int fd, struct sockaddr_in address, char number)
{
	char *message = malloc(ACK_OK_SIZE);
	snprintf(message, ACK_OK_SIZE, "ACK %c OK", number);
	int wc = send_packet(fd, message, strlen(message), 0,
						(struct sockaddr *) &address, sizeof(struct sockaddr_in));
	check_perror(wc, "send_packet");
	free(message);
}


void send_ack_notfound(int fd, struct sockaddr_in address, char number)
{
	char *message = malloc(ACK_NOT_FOUND_SIZE);
	snprintf(message, ACK_NOT_FOUND_SIZE, "ACK %c NOT FOUND", number);
	int wc = send_packet(fd, message, strlen(message), 0,
						(struct sockaddr *) &address, sizeof(struct sockaddr_in));
	check_perror(wc, "send_packet");
	free(message);
}


void send_client_info(int fd, struct sockaddr_in address, char number, client *current)
{
	int message_length = CLIENT_INFO_SIZE + strlen(current->nick) +
						 strlen(current->address) + get_port_length(current->port);
	char *message = malloc(message_length);
	snprintf(message, message_length, "ACK %c NICK %s IP %s PORT %d",
			 number, current->nick, current->address, current->port);

	int wc = send_packet(fd, message, strlen(message), 0,
						(struct sockaddr *) &address, sizeof(struct sockaddr_in));
	check_perror(wc, "send_packet");
	free(message);
}


void check_perror(int result, char *msg)
{
    if (result == -1)
    {
        perror(msg);
        close(fd);
        free_clients(cache);
        exit(EXIT_FAILURE);
    }
}


void check_error(int result, char *msg, int line, const char *function)
{
    if (!result)
    {
        fprintf(stderr, "> Shutdown triggered by %s in %d on line %s\n", msg, line, function);
        close(fd);
        free_clients(cache);
        exit(EXIT_FAILURE);
    }
}
