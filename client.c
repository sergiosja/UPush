#include "client.h"

static correspondence *cache;
static blocked *block;
static char this_nick[20];
static int fd;

int main(int argc, char const *argv[])
{
  validate_args(argc, argv, 0);
  set_loss_probability(atof(argv[5]) / 100);
    strncpy(this_nick, argv[1], 19);
    validate_nick(this_nick, strlen(this_nick));
  int client_timeout = atoi(argv[4]);

  cache = initiate_correspondence();
  block = initiate_blocklist();

  char buffer[BUFSIZE];
  struct sockaddr_in client_addr, server_addr;
  struct timeval timeout, register_time;
  struct in_addr ip_addr;
  socklen_t addr_len;
  fd_set fds, original_fds;

  char server_number = '0';

  // Initialise socket
  fd = socket(AF_INET, SOCK_DGRAM, 0);
  check_perror(fd, "socket");

  int wc = inet_pton(AF_INET, argv[2], &ip_addr.s_addr);
  check_perror(wc, "inet_pton");
  check_error(wc, "inet_pton", __LINE__, __FUNCTION__);

  server_addr.sin_family = AF_INET;
  server_addr.sin_port = htons(atoi(argv[3]));
  server_addr.sin_addr = ip_addr;

  // Connect to server
    initiate_registration(fd, server_addr, server_number);

  FD_ZERO(&original_fds);
  FD_SET(fd, &original_fds);
  FD_SET(STDIN_FILENO, &original_fds);

    timeout.tv_sec = client_timeout;
    timeout.tv_usec = 0;
    
  fds = original_fds;
  int rc = select(FD_SETSIZE, &fds, NULL, NULL, &timeout);
  check_perror(rc, "select");


    if (FD_ISSET(fd, &fds)) {
        addr_len = sizeof(struct sockaddr_in);
        rc = recvfrom(fd, buffer, BUFSIZE-1, 0, (struct sockaddr *) &server_addr, &addr_len);
        check_perror(rc, "recv");

        printf("> %s successfully registered\n", this_nick);
    }
    else
    {
        fprintf(stderr, "> Registration failed, shutting down client ...\n");
        quit();
        return EXIT_SUCCESS;
    }

  rc = gettimeofday(&register_time, NULL);
    check_perror(rc, "gettimeofday");
  int previous_registration_time = register_time.tv_sec;

  for (;;)
  {
        timeout.tv_sec = client_timeout;
        timeout.tv_usec = 0;
    fds = original_fds;
    rc = select(FD_SETSIZE, &fds, NULL, NULL, &timeout);
    check_perror(rc, "select");

    // Received a message from another client
    if (FD_ISSET(fd, &fds))
    {
            if (heartbeat(&previous_registration_time))
            {
                server_number = server_number == '0' ? '1' : '0';
                initiate_registration(fd, server_addr, server_number);
            }
            stop_and_wait(cache, fd, server_addr, original_fds, server_number, client_timeout);

      rc = recvfrom(fd, buffer, BUFSIZE-1, 0, (struct sockaddr *) &client_addr, &addr_len);
      check_perror(rc, "read");
      buffer[rc] = 0;

      int current_client_port = ntohs(client_addr.sin_port);
      char current_client_address[INET_ADDRSTRLEN];
      inet_ntop(AF_INET, &client_addr.sin_addr.s_addr, current_client_address, INET_ADDRSTRLEN);

            correspondence *existing_correspondence = find_correspondence(cache, current_client_address, current_client_port);

            // If we received ack
      char buffer_copy[strlen(buffer)+1];
      strcpy(buffer_copy, buffer);
            char possible_ack = check_ack(buffer_copy);
            if (possible_ack != 2)
            {
                if (possible_ack == 'X')
                {
                    fprintf(stderr, "> Received a bad ack, shutting down client ...");
                    quit();
                    return EXIT_SUCCESS;
                }

                if (existing_correspondence && !find_blocked(block, existing_correspondence->nick))
                {
                    remove_delivered_packet_from_queue(existing_correspondence->queue);
                    existing_correspondence->status = '0';
                }
                continue;
            }

            if (current_client_port == atoi(argv[3]))
            {
                printf("> Server error, try again\n");
                continue;
            }

      // If we received something else
      strcpy(buffer_copy, buffer);
      int bad_message_check = verify_message(buffer_copy, block);
      if (bad_message_check)
      {
                if ((bad_message_check == 1) || (bad_message_check == 2))
                    send_ack_error(fd, client_addr, bad_message_check);
        continue;
      }

      strcpy(buffer_copy, buffer);
      packet *received_packet = deserealise_packet(buffer_copy);
            if (!existing_correspondence)
            {
                char failed_lookup = '0';
                server_number = server_number == '0' ? '1' : '0';
                lookup_client(fd, server_addr, server_number, received_packet->from_nick);
                timeout.tv_sec = client_timeout;
                timeout.tv_usec = 0;
                fds = original_fds;
                rc = select(FD_SETSIZE, &fds, NULL, NULL, &timeout);
                check_perror(rc, "select");

                if (FD_ISSET(fd, &fds))
                {
                    rc = recvfrom(fd, buffer, BUFSIZE - 1, 0, (struct sockaddr *) &server_addr, &addr_len);
                    check_perror(rc, "recv");
                    buffer[rc] = 0;
                }
                else failed_lookup = '1';
                if ((failed_lookup == '1') || !strstr(buffer, "NICK"))
                {
                    free_packets(received_packet);
                    continue;
                }

                existing_correspondence = find_correspondence_by_nick(cache, received_packet->from_nick);
                if (existing_correspondence) remove_correspondence(cache, existing_correspondence->nick);
                existing_correspondence = register_correspondence(cache, received_packet->from_nick, '0', buffer, NULL);
            }

            if (received_packet->number == existing_correspondence->last_received_number)
            {
                send_ack_ok(fd, client_addr, received_packet->number);
                free_packets(received_packet);
                continue;
            }
            existing_correspondence->last_received_number = received_packet->number;

            printf("%s: %s\n", received_packet->from_nick, received_packet->message);
            send_ack_ok(fd, client_addr, received_packet->number);

      free_packets(received_packet);
    }


    // Sending message
    else if (FD_ISSET(STDIN_FILENO, &fds))
    {
            if (heartbeat(&previous_registration_time))
            {
                server_number = server_number == '0' ? '1' : '0';
                initiate_registration(fd, server_addr, server_number);
            }
            stop_and_wait(cache, fd, server_addr, original_fds, server_number, client_timeout);
      read_stdin(buffer, BUFSIZE);

      if (!strcmp(buffer, "QUIT")) break;

      if (buffer[0] == '@')
      {
        char *message = strchr(buffer, ' ');
                if (!message)
                {
                    fprintf(stderr, "> warning: message cannot be empty\n");
                    continue;
                }

        int to_nick_length = strlen(buffer) - strlen(message);
        char to_nick[to_nick_length];
        strncpy(to_nick, &buffer[1], to_nick_length-1);
        to_nick[to_nick_length-1] = 0;

        if (find_blocked(block, to_nick)) continue;

        int message_length = strlen(message) >= MAX_MESSAGE_LENGTH ? MAX_MESSAGE_LENGTH - 1 : strlen(message);
        char prepared_message[message_length];
        strncpy(prepared_message, &message[1], message_length-1);
        prepared_message[message_length-1] = 0;

                if (!validate_message(prepared_message, message_length))
                {
                    fprintf(stderr, "> warning: message cannot contain æøå!\n");
                    continue;
                }

        // Looking up receiver
                correspondence *current = find_correspondence_by_nick(cache, to_nick);
        if (!current)
        {
                    server_number = server_number == '0' ? '1' : '0';
                    lookup_client(fd, server_addr, server_number, to_nick);
          int number_of_lookups = 1;
          for (;;)
          {
                        timeout.tv_sec = client_timeout;
                        timeout.tv_usec = 0;
            fds = original_fds;
            rc = select(FD_SETSIZE, &fds, NULL, NULL, &timeout);
            check_perror(rc, "select");

            if (FD_ISSET(fd, &fds))
                        {
              rc = recvfrom(fd, buffer, BUFSIZE - 1, 0, (struct sockaddr *) &server_addr, &addr_len);
              check_perror(rc, "recv");
              buffer[rc] = 0;
                            break;
            }
                        else if (number_of_lookups == 3)
                        {
              fprintf(stderr, "> Lookup failed, shutting down client ...\n");
              quit();
              return EXIT_SUCCESS;
                        }

                        lookup_client(fd, server_addr, server_number, to_nick);
                        number_of_lookups++;
          }

          if (strstr(buffer, "NOT FOUND"))
          {
            fprintf(stderr, "NICK %s NOT REGISTERED\n", to_nick);
            continue;
          }

                    if (strstr(buffer, "OK"))
                    {
                        printf("> Server error, try again\n");
                        continue;
                    }

                    packet *new_packet = register_new_packet('0', to_nick, prepared_message);
                    current = register_correspondence(cache, to_nick, '1', buffer, new_packet);
                    send_message_to_client('0', current->nick, strlen(current->nick),
                                            prepared_message, strlen(prepared_message), fd, current->sock_addr);
                    continue;
        }

                current->last_sent_number = current->last_sent_number == '0' ? '1' : '0';
                packet *new_packet = register_new_packet(current->last_sent_number, current->nick, prepared_message);
                add_packet_to_queue(current->queue, new_packet);

                if (current->status == '0' && !current->queue->next->next)
                {
                    current->status = '1';
                    struct timeval new_timeout;
                    rc = gettimeofday(&new_timeout, NULL);
                    check_perror(rc, "gettimeofday");
                    current->timeout = new_timeout.tv_sec;
                    send_message_to_client(current->queue->next->number, current->queue->next->to_nick,
                                            strlen(current->queue->next->to_nick), current->queue->next->message,
                                            strlen(current->queue->next->message), fd, current->sock_addr);
                }
      }
      else if (strstr(buffer, "UNBLOCK"))
      {
        strtok(buffer, " ");
        unblock_client(block, strtok(NULL, " "));
      }
      else if (strstr(buffer, "BLOCK"))
      {
        strtok(buffer, " ");
                char *token = strtok(NULL, " ");
        block_client(block, find_correspondence_by_nick(cache, token), token);
      }
    }
        else
        {
            server_number = server_number == '0' ? '1' : '0';
            initiate_registration(fd, server_addr, server_number);
            stop_and_wait(cache, fd, server_addr, original_fds, server_number, client_timeout);
        }
  }

  quit();
  return EXIT_SUCCESS;
}


packet *packet_head()
{
    packet *p = malloc(sizeof(packet));
    p->from_nick = NULL;
    p->to_nick = NULL;
    p->message = NULL;
    p->next = NULL;
    return p;
}


packet *register_new_packet(char number, char *to, char *message)
{
    packet *p = malloc(sizeof(packet));
    p->number = number;
    p->from_nick = strdup(this_nick);
    p->to_nick = strdup(to);
    p->message = strdup(message);
    p->next = NULL;    
    return p;
}


void add_packet_to_queue(packet *head, packet *current)
{
    if (!current) return;

    packet *new = head;
    while (new->next)
        new = new->next;

    new->next = current;
    current->next = NULL;
}


void remove_delivered_packet_from_queue(packet *head)
{
    packet *current = head->next;
    head->next = head->next->next;
    free(current->from_nick);
    free(current->to_nick);
    free(current->message);
    free(current);
}


packet *deserealise_packet(char *buffer)
{
  packet *current = malloc(sizeof(packet));

  strtok(buffer, " ");
  current->number = strtok(NULL, " ")[0];

  strtok(NULL, " ");
  current->from_nick = malloc(MAX_NICK_LENGTH);
  strncpy(current->from_nick, strtok(NULL, " "), MAX_NICK_LENGTH-1);

  strtok(NULL, " ");
  current->to_nick = malloc(MAX_NICK_LENGTH);
  strncpy(current->to_nick, strtok(NULL, " "), MAX_NICK_LENGTH-1);

  current->message = malloc(MAX_MESSAGE_LENGTH);
  memset(current->message, 0, MAX_MESSAGE_LENGTH);

  strtok(NULL, " ");
  char *token = strtok(NULL, " ");
  while (token)
  {
    strcat(current->message, token);
    strcat(current->message, " ");
    token = strtok(NULL, " ");
  }

    current->next = NULL;
  return current;
}


void free_packets(packet *p)
{
    packet *current;
    while (p)
    {
        current = p;
        p = p->next;
        free(current->from_nick);
        free(current->to_nick);
        free(current->message);
        free(current);
    }
}


correspondence *initiate_correspondence()
{
    correspondence *head = malloc(sizeof(correspondence));
    head->nick = NULL;
    head->address = NULL;
    head->queue = packet_head();
    head->next = NULL;
    return head;
}

correspondence *register_correspondence(correspondence *head, char *nick, char status, char *buffer, packet *p)
{
    correspondence *new_correspondence = malloc(sizeof(correspondence));

    char *token = strtok(buffer, " ");
    for (int i = 0; i < 5; i++) token = strtok(NULL, " ");
    char *address = token;
    for (int i = 0; i < 2; i++) token = strtok(NULL, " ");
    uint16_t port = atoi(token);

    new_correspondence->nick = strdup(nick);
    new_correspondence->address = strdup(address);
    new_correspondence->port = port;
    new_correspondence->sock_addr.sin_family = AF_INET;
  new_correspondence->sock_addr.sin_port = htons(port);
  int wc = inet_pton(AF_INET, address, &new_correspondence->sock_addr.sin_addr.s_addr);
  check_perror(wc, "inet_pton");
  check_error(wc, "inet_pton", __LINE__, __FUNCTION__);

    new_correspondence->queue = packet_head();
    add_packet_to_queue(new_correspondence->queue, p);
    new_correspondence->status = status;
    new_correspondence->last_received_number = 'x';
    new_correspondence->last_sent_number = '0';

    struct timeval timer;
    int rc = gettimeofday(&timer, NULL);
    check_perror(rc, "gettimeofday");
    new_correspondence->timeout = timer.tv_sec;

    correspondence *current = head;
    while (current->next)
        current = current->next;

    current->next = new_correspondence;
    new_correspondence->next = NULL;

    return new_correspondence;
}


void update_correspondence(correspondence *current, char *buffer, long timeout)
{
    char *token = strtok(buffer, " ");
    for (int i = 0; i < 5; i++) token = strtok(NULL, " ");
    free(current->address);
    current->address = strdup(token);
    for (int i = 0; i < 2; i++) token = strtok(NULL, " ");
    current->port = atoi(token);
    current->timeout = timeout;
}


correspondence *find_correspondence_by_nick(correspondence *head, char *nick)
{
    correspondence *current = head->next;
    while (current)
    {
        if (!strcmp(current->nick, nick))
            return current;
        current = current->next;
    }
    return NULL;
}


correspondence *find_correspondence(correspondence *head, char *address, uint16_t port)
{
    correspondence *current = head->next;
    while (current)
    {
        if (!strcmp(current->address, address) && current->port == port)
            return current;
        current = current->next;
    }
    return NULL;
}


void remove_correspondence(correspondence *head, char *nick)
{
  correspondence *current = head;
    while (current->next)
    {
        if (!strcmp(current->next->nick, nick))
        {
            correspondence *this = current->next;
            current->next = this->next;

            free(this->address);
            free(this->nick);
            free_packets(this->queue);
            free(this);
            break;
        }

        current = current->next;
    }
}


void free_correspondences(correspondence *head)
{
  correspondence *current;
  while (head)
  {
    current = head;
    head = head->next;
    free(current->nick);
    free(current->address);
        free_packets(current->queue);
    free(current);
  }
}


blocked *initiate_blocklist()
{
    blocked *block = malloc(sizeof(blocked));
    block->nick = NULL;
    block->next = NULL;
    return block;
}


void block_client(blocked *head, correspondence *corr, char *nick)
{
  blocked *new_blocked = malloc(sizeof(blocked));
  new_blocked->nick = malloc(MAX_NICK_LENGTH);
  strncpy(new_blocked->nick, nick, MAX_NICK_LENGTH-1);

  blocked *current = head;
  while (current->next)
    current = current->next;

  current->next = new_blocked;
  new_blocked->next = NULL;

    if (corr)
        corr->last_received_number = 'x';
}


void unblock_client(blocked *head, char *nick)
{
  blocked *current = head;
  while (current->next)
  {
    if (!strcmp(current->next->nick, nick))
    {
      blocked *tmp = current->next;
      current->next = current->next->next;
      free(tmp->nick);
      free(tmp);
      break;
    }
    current = current->next;
  }
}


blocked *find_blocked(blocked *head, char *nick)
{
  blocked *current = head->next;
  while (current)
  {
    if (!strcmp(current->nick, nick))
      return current;
    current = current->next;
  }

  return NULL;
}


void free_blocked_clients(blocked *head)
{
  blocked *current;
  while (head)
  {
    current = head;
    head = head->next;
    free(current->nick);
    free(current);
  }
}


void initiate_registration(int fd, struct sockaddr_in server_address, char number)
{
  char *message = malloc(strlen(this_nick) + REGISTER_CLIENT_SIZE);
  snprintf(message, strlen(this_nick) + REGISTER_CLIENT_SIZE, "PKT %c REG %s", number, this_nick);
    int wc = send_packet(fd, message, strlen(message), 0,
          (struct sockaddr *) &server_address, sizeof(struct sockaddr_in));
  check_perror(wc, "send_packet");
  free(message);
}


int heartbeat(int *previous_registration_time)
{
    struct timeval timer;
    int rc = gettimeofday(&timer, NULL);
    check_perror(rc, "gettimeofday");
    int now = timer.tv_sec;
    if (now - *previous_registration_time >= 10)
    {
        *previous_registration_time = now;
        return 1;
    }
    return 0;
}


void lookup_client(int fd, struct sockaddr_in address, char number, char *nick)
{
  char *message = malloc(LOOKUP_SIZE + strlen(nick));
  snprintf(message, LOOKUP_SIZE + strlen(nick), "PKT %c LOOKUP %s", number, nick);
    int wc = send_packet(fd, message, strlen(message), 0,
                    (struct sockaddr *) &address, sizeof(struct sockaddr_in));
    check_perror(wc, "send_packet");
    free(message);
}


void send_message_to_client(char number, char *to, int to_length, char *msg, int msg_length, int fd, struct sockaddr_in dest)
{
    int length = strlen(this_nick) + to_length + msg_length + CLIENT_MESSAGE_SIZE;
  char *message = malloc(length);
  snprintf(message, length, "PKT %c FROM %s TO %s MSG %s", number, this_nick, to, msg);

    int wc = send_packet(fd, message, strlen(message), 0,
            (struct sockaddr *) &dest, sizeof(struct sockaddr_in));
    check_perror(wc, "send_packet");
    free(message);
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


void send_ack_error(int fd, struct sockaddr_in address, int format)
{
  char *message = malloc(ACK_ERROR_SIZE);
    if (format == 1)
      snprintf(message, ACK_ERROR_SIZE, "ACK X WRONG FORMAT");
    else if (format == 2)
      snprintf(message, ACK_ERROR_SIZE, "ACK X WRONG NAME");

    int rc = send_packet(fd, message, strlen(message), 0,
                    (struct sockaddr *) &address, sizeof(struct sockaddr_in));
    check_perror(rc, "send_packet");
    free(message);
}


char check_ack(char *buffer)
{
  if (!strcmp(strtok(buffer, " "), "ACK"))
  {
    return strtok(NULL, " ")[0];
  }
  return 2;
}


int verify_message(char *buffer, blocked *check)
{
  if (strcmp("PKT", strtok(buffer, " ")))
    return 1;

  char number = strtok(NULL, " ")[0];
  if ((number != '0') && (number != '1'))
    return 1;

  if (strcmp("FROM", strtok(NULL, " ")))
    return 1;

    if (find_blocked(check, strtok(NULL, " "))) {
        return 3;
    }

  if (strcmp("TO", strtok(NULL, " ")))
    return 1;

  if (strcmp(this_nick, strtok(NULL, " ")))
    return 2;

  if (strcmp("MSG", strtok(NULL, " ")))
    return 1;

  return 0;
}


void read_stdin(char buffer[], int size)
{
  char c;
  fgets(buffer, size, stdin);
  if (buffer[strlen(buffer) - 1 == '\n'])
    buffer[strlen(buffer) - 1] = 0;
  else
    while((c = getchar()) != '\n' && c != EOF);
}


void stop_and_wait(correspondence *head, int fd, struct sockaddr_in server_addr, fd_set fds, char outgoing_number, int client_timeout)
{
    correspondence *corr = head->next;
    struct timeval check_timeout;
    while (corr)
    {
        int rc = gettimeofday(&check_timeout, NULL);
        check_perror(rc, "gettimeofday");
        if (check_timeout.tv_sec - corr->timeout >= client_timeout)
        {
            if (corr->status == '0' && corr->queue->next)
            {
                corr->status = '1';
                corr->timeout = check_timeout.tv_sec;
                send_message_to_client(corr->queue->next->number, corr->queue->next->to_nick,
                                        strlen(corr->queue->next->to_nick), corr->queue->next->message,
                                        strlen(corr->queue->next->message), fd, corr->sock_addr);
            }
            else if (corr->status == '1')
            {
                corr->status = '2';
                corr->timeout = check_timeout.tv_sec;
                send_message_to_client(corr->queue->next->number,
                
                corr->queue->next->to_nick,
                                        strlen(corr->queue->next->to_nick),
                                        corr->queue->next->message,
                                        strlen(corr->queue->next->message),
                                        fd,
                                        corr->sock_addr);
            }
            else if (corr->status == '2')
            {
                char buffer[BUFSIZE];
                char not_registered = '0';
                outgoing_number = outgoing_number == '0' ? '1' : '0';
                lookup_client(fd, server_addr, outgoing_number, corr->queue->next->to_nick);

                struct timeval timeout;
                timeout.tv_sec = client_timeout;
                timeout.tv_usec = 0;
                rc = select(FD_SETSIZE, &fds, NULL, NULL, &timeout);
                check_perror(rc, "select");

                if (FD_ISSET(fd, &fds))
                {
                    socklen_t addr_len = sizeof(struct sockaddr_in);
                    rc = recvfrom(fd, buffer, BUFSIZE - 1, 0, (struct sockaddr *) &server_addr, &addr_len);
                    check_perror(rc, "recv");
                    buffer[rc] = 0;
                }
                else not_registered = '1';
                if ((not_registered == '1') || !strstr(buffer, "PORT"))
                {
                    fprintf(stderr, "NICK %s NOT REGISTERED\n", corr->queue->next->to_nick);
                    correspondence *tmp = corr;
                    corr = corr->next;
                    remove_correspondence(head, tmp->nick);
                    continue;
                }

                update_correspondence(corr, buffer, check_timeout.tv_sec);
                corr->status = '3';
                send_message_to_client(corr->queue->next->number, corr->queue->next->to_nick,
                                        strlen(corr->queue->next->to_nick), corr->queue->next->message,
                                        strlen(corr->queue->next->message), fd, corr->sock_addr);
            }
            else if (corr->status == '3')
            {
                corr->timeout = check_timeout.tv_sec;
                corr->status = '4';
                send_message_to_client(corr->queue->next->number, corr->queue->next->to_nick,
                                        strlen(corr->queue->next->to_nick), corr->queue->next->message,
                                        strlen(corr->queue->next->message), fd, corr->sock_addr);
            }
            else if (corr->status == '4')
            {
                fprintf(stderr, "NICK %s UNREACHABLE\n", corr->queue->next->to_nick);
                correspondence *tmp = corr;
                corr = corr->next;
                remove_correspondence(head, tmp->nick);
                continue;
            }
        }
        corr = corr->next;
    }
}


void quit()
{ 
  free_blocked_clients(block);
    free_correspondences(cache);
  close(fd);
}


void check_perror(int result, char *msg)
{
  if (result == -1)
  {
    perror(msg);
        quit();
        exit(EXIT_FAILURE);
  }
}


void check_error(int result, char *msg, int line, const char *function)
{
  if (!result)
  {
    fprintf(stderr, "> Shutdown triggered by %s in %d on line %s\n", msg, line, function);
        quit();
        exit(EXIT_FAILURE);
  }
}


void validate_nick(char *nick, int size)
{
    for (int i = 0; i < size; i++)
    {
        if (!isalpha(nick[i]))
        {
            fprintf(stderr, "> Username should only be ASCII-characters, shutting down client ...\n");
            exit(EXIT_FAILURE);
        }
    }
}

int validate_message(char *message, int size)
{
    char *illegal_chars = "æøå";
    for (int i = 0; i < size; i++)
        for (int j = 0; j < 3; j++)
            if (message[i] == illegal_chars[j])
                return 0;
    return 1;
}