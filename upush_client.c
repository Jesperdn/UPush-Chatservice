#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include "linked_list.h"
#include "upush_client.h"
#include "send_packet.h"
#include "utils.h"

/*
U_PUSH client
Candidate: 15735

Handles chatting with other clients, server communication for lookup and registration.
Uses stop-and-wait to block incoming packets that do not match what it is expecting.
Allows for blocking nicks.
Sends a heartbeat to the server every 10 seconds.

A lot of the functions use the same variables, and a few of them branch into one another.
Because of this I have chosen quite a few global variables, to avoid issues with passing
and returning values.

Please type `QUIT` to avoid memleaks when closing :)

See upush_client.h for descriptions of each function.
*/

// --- GLOBAL VARIABLES ---
u_client *clients = NULL;
blocked_client *blocked = NULL;
client_msg_ctr *msg_ctrs = NULL;

fd_set main_fds;
fd_set wait_fds;
int timeout;
int socket_fd;
int seq_nr;
int server_seq_nr;
int recieved_seq_nr;

struct timeval tv;
struct timeval heartbeat;
struct sockaddr_in server_addr, other_addr;
socklen_t addr_len = sizeof(struct sockaddr_in);

char buf_in[BUFSIZE];
char buf_out[BUFSIZE];
char buf_ip[LEN_IP_ADDR];
char message[BUFSIZE];
char my_nick[MAX_NICK_LEN];
char other_nick[MAX_NICK_LEN];

int main(int argc, char *argv[])
{
    srand48(time(0));

    char buf_nick[MAX_NICK_LEN];
    int port_num;
    float percent;

    if (argc != 6)
    {
        fprintf(stderr, "Usage: %s <nick> <adresse> <port> <timeout> <tapssannsynlighet>\n", argv[0]);
        client_shutdown(1);
    }

    if (check_nick(argv[1]) == -1)
    {
        fprintf(stderr, "Nickname not accepted\n");
        client_shutdown(1);
    }
    strncpy(my_nick, argv[1], MAX_NICK_LEN);

    strncpy(buf_ip, argv[2], LEN_IP_ADDR);
    port_num = (int)strtol(argv[3], NULL, 10);
    timeout = (int)strtol(argv[4], NULL, 10);

    if (port_num > USHRT_MAX)
    {
        fprintf(stderr, "Port number not accepted. Upper limit 65 535\n");
        client_shutdown(1);
    }

    // --- Set loss probability ---
    percent = strtof(argv[5], NULL) / 100;
    if (percent < 0.0 || percent > 1.0)
    {
        fprintf(stderr, "Percentage not accepted. Confine to 0 <= p <= 100\n");
        client_shutdown(1);
    }
    set_loss_probability(percent);

    // --- Create socket ---
    socket_fd = socket(AF_INET, SOCK_DGRAM, 0);
    if (socket_fd == -1)
    {
        fprintf(stderr, "Socket creation");
        client_shutdown(1);
    }

    // --- Setup server address ---
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port_num);
    if (inet_pton(AF_INET, buf_ip, &server_addr.sin_addr.s_addr) == 0)
    {
        fprintf(stderr, "Not a valid IP: %s", buf_ip);
        client_shutdown(1);
    }

    // --- Send initial register msg ---
    if (!reg_event(server_addr, my_nick))
    {
        fprintf(stderr, "TIMEOUT: No ACK from server within %d seconds. Shutting down\n", timeout);
        client_shutdown(1);
    }

    // --- MAIN EVENT LOOP ---
    printf("[CLIENT READY]\n");
    while (BLOCKING)
    {
        init_fds(socket_fd, &main_fds);
        FD_SET(STDIN_FILENO, &main_fds);

        heartbeat.tv_sec = HEARTBEAT_INTERVAL;
        heartbeat.tv_usec = 0;

        if (select(FD_SETSIZE, &main_fds, NULL, NULL, &heartbeat) == -1)
        {
            fprintf(stderr, "select() error in main loop\n");
            client_shutdown(1);
        }

        if (FD_ISSET(socket_fd, &main_fds) != 0)
        {
            if (recvfrom(socket_fd, buf_in, BUFSIZE - 1, 0, (struct sockaddr *)&other_addr, &addr_len) < 0)
            {
                fprintf(stderr, "recvfrom() error in main loop");
                client_shutdown(1);
            }

            if (sscanf(buf_in, "PKT %d FROM %s TO %s MSG %[^\n]", &recieved_seq_nr, other_nick, buf_nick, message) == 4)
            {
                client_msg_ctr *msg_ctr = find_or_create_ctr(&msg_ctrs, other_nick);
                if (msg_ctr == NULL)
                    client_shutdown(1);

                if (strncmp(my_nick, buf_nick, MAX_NICK_LEN) != 0)
                    ack_response(recieved_seq_nr, other_addr, recieved_seq_nr, buf_out, "WRONG NAME");

                else
                {

                    if ((find_blocked(blocked, other_nick) == NULL) && (msg_ctr->last_seq_nr != recieved_seq_nr))
                    {
                        printf("                                  [%s]: %s\n", other_nick, message);
                        msg_ctr->last_seq_nr = recieved_seq_nr;
                    }
                    ack_response(socket_fd, other_addr, recieved_seq_nr, buf_out, "OK");
                }
                continue;
            }
            ack_response(socket_fd, other_addr, recieved_seq_nr, buf_out, "WRONG FORMAT");
        }

        if (FD_ISSET(STDIN_FILENO, &main_fds) != 0)
        {
            get_input(buf_in);

            if (!strcmp(buf_in, "QUIT"))
                client_shutdown(0);

            else if (sscanf(buf_in, "@%s %[^\n]", other_nick, message) == 2)
            {
                if (find_blocked(blocked, other_nick) != NULL)
                {
                    fprintf(stderr, "You cannot chat to a blocked client!\n");
                    continue;
                }

                if (strncmp(other_nick, my_nick, MAX_NICK_LEN) == 0)
                {
                    fprintf(stderr, "You cannot chat to yourself!\n");
                    continue;
                }

                if (!chat_event(my_nick, other_nick, message))
                    continue;
                seq_nr++;
            }

            else if (sscanf(buf_in, "UNBLOCK %s", other_nick) == 1)
            {
                if (!unblock_nick(&blocked, other_nick))
                {
                    printf("Cannot unblock %s: not blocked\n", other_nick);
                }
                else
                    printf("%s is no longer blocked.\n", other_nick);

                continue;
            }

            else if (sscanf(buf_in, "BLOCK %s", other_nick) == 1)
            {
                if (!block_nick(&blocked, other_nick))
                {
                    printf("%s is already blocked.\n", other_nick);
                }
                else
                    printf("%s is now blocked.\n", other_nick);
                continue;
            }
            else
                fprintf(stderr, "Usage: @<to_nick> <message>\n");
        }

        // --- HEARTBEAT ---
        reg_event(server_addr, my_nick);
    }
    return EXIT_SUCCESS;
}

void init_fds(int socket_fd, fd_set *fds)
{
    FD_ZERO(fds);
    FD_SET(socket_fd, fds);
}

int send_reg_pkt(struct sockaddr_in server_addr, char *nick)
{

    sprintf(buf_out, "PKT %d REG %s", server_seq_nr, nick);
    if (send_packet(socket_fd, buf_out, strlen(buf_out) + 1, 0, (struct sockaddr *)&server_addr, sizeof(server_addr)) == -1)
    {
        fprintf(stderr, "send_packet() error in send_reg_pkt\n");
        return 0;
    }
    return 1;
}

int reg_event(struct sockaddr_in server_addr, char *nick)
{
    struct timeval reg_timeout;
    struct timeval currentTime;
    long time = 0;

    if (!send_reg_pkt(server_addr, nick))
        return 0;

    while (BLOCKING)
    {
        init_fds(socket_fd, &wait_fds);
        if (gettimeofday(&currentTime, NULL) == -1)
            return 0;

        time = currentTime.tv_sec - time;
        if (time > timeout)
        {
            reg_timeout.tv_sec = timeout;
            reg_timeout.tv_usec = 0;
        }
        else
        {
            reg_timeout.tv_sec = timeout - time;
            reg_timeout.tv_usec = 0;
        }

        if (select(FD_SETSIZE, &wait_fds, NULL, NULL, &reg_timeout) == -1)
            client_shutdown(1);

        if (FD_ISSET(socket_fd, &wait_fds) != 0)
        {
            if (recvfrom(socket_fd, buf_in, BUFSIZE - 1, 0, (struct sockaddr *)&other_addr, &addr_len) < 0)
                client_shutdown(1);

            if ((other_addr.sin_port != server_addr.sin_port) || (sscanf(buf_in, "ACK %d OK", &recieved_seq_nr) != 1))
                continue;

            if (server_seq_nr == recieved_seq_nr)
            {
                server_seq_nr++;
                return 1;
            }
        }
        return 0;
    }
}

int send_msg(u_client *client, char *other_nick, char *message, char *my_nick)
{
    sprintf(buf_out, "PKT %d FROM %s TO %s MSG %s", seq_nr, my_nick, other_nick, message);
    if (send_packet(socket_fd, buf_out, strlen(buf_out) + 1, 0, (struct sockaddr *)&client->ip, addr_len) < 0)
        return 0;
    return 1;
}

int chat_event(char *my_nick, char *to_nick, char *message)
{
    struct timeval chat_timeout;
    struct timeval currentTime;
    char ack_message[100];
    long time = 0;
    int tries = 0;

    u_client *to_client = find_client(clients, to_nick);
    if (to_client == NULL)
    {
        to_client = lookup_event(to_nick, 3);
        if (to_client == NULL)
            return 0;
    }

    while (tries < 4)
    {
        init_fds(socket_fd, &wait_fds);

        if (tries == 2)
        {
            to_client = lookup_event(to_nick, 1);
            if (to_client == NULL)
            {
                fprintf(stderr, "NICK %s NOT REGISTERED\n", to_nick);
                return 0;
            }
        }

        if (!send_msg(to_client, to_nick, message, my_nick))
            return 0;

        if (gettimeofday(&currentTime, NULL) == -1)
            return 0;

        time = currentTime.tv_sec - time;
        if (time > timeout || time == timeout)
        {
            chat_timeout.tv_sec = timeout;
            chat_timeout.tv_usec = 0;
        }
        else
        {
            chat_timeout.tv_sec = timeout - time;
            chat_timeout.tv_usec = 0;
        }

        if (select(FD_SETSIZE, &wait_fds, NULL, NULL, &chat_timeout) == -1)
            client_shutdown(1);

        if (FD_ISSET(socket_fd, &wait_fds) != 0)
        {

            if (recvfrom(socket_fd, buf_in, BUFSIZE - 1, 0, (struct sockaddr *)&other_addr, &addr_len) < 0)
                client_shutdown(1);

            if (sscanf(buf_in, "ACK %d %[^/n]", &recieved_seq_nr, ack_message) != 2)
                continue;

            if (recieved_seq_nr == seq_nr)
                return 1;

            if (strcmp(ack_message, "OK") == 0)
                return 1;

            else if (strcmp(ack_message, "WRONG NAME") == 0)
            {
                printf("Wrong name sent to client.\n");
                return 0;
            }

            else if (strcmp(ack_message, "WRONG FORMAT") == 0)
            {
                printf("Wrong format on message.");
                return 0;
            }
        }
        tries++;
    }
    fprintf(stderr, "NICK %s UNREACHABLE\n", to_nick);
    return 1;
}

int send_lookup(char *nick)
{

    if (sprintf(buf_out, "PKT %d LOOKUP %s", server_seq_nr, nick) < 0)
    {
        fprintf(stderr, "sprintf() error in send_lookup");
        return 0;
    }

    if (send_packet(socket_fd, buf_out, strlen(buf_out), 0, (struct sockaddr *)&server_addr, addr_len) < 0)
    {
        fprintf(stderr, "send_packet() error in send_lookup");
        return 0;
    }
    return 1;
}

u_client *lookup_event(char *nick, int tries)
{
    struct timeval lookup_timeout = {timeout, 0};
    struct timeval currentTime;
    long time = 0;
    int tries_done = 0;
    int other_port_num;

    while (tries_done < tries)
    {
        init_fds(socket_fd, &wait_fds);

        if (!send_lookup(nick))
            return NULL;

        if (gettimeofday(&currentTime, NULL) == -1)
            return 0;

        time = currentTime.tv_sec - time;
        if (time > timeout || time == timeout)
        {
            lookup_timeout.tv_sec = timeout;
            lookup_timeout.tv_usec = 0;
        }
        else
        {
            lookup_timeout.tv_sec = timeout - time;
            lookup_timeout.tv_usec = 0;
        }

        if (select(FD_SETSIZE, &wait_fds, NULL, NULL, &lookup_timeout) == -1)
        {
            fprintf(stderr, "select() error in lookup\n");
            client_shutdown(1);
        }

        if (FD_ISSET(socket_fd, &wait_fds) != 0)
        {
            if (recvfrom(socket_fd, buf_in, BUFSIZE - 1, 0, (struct sockaddr *)&server_addr, &addr_len) < 0)
                client_shutdown(1);

            if (sscanf(buf_in, "ACK %d NICK %s IP %s PORT %d", &recieved_seq_nr, other_nick, buf_ip, &other_port_num) == 4)
                return parse_lookup_info(other_nick, buf_ip, other_port_num);

            else if (sscanf(buf_in, "ACK %d NOT FOUND", &recieved_seq_nr) == 1)
            {
                if (server_seq_nr == recieved_seq_nr)
                    fprintf(stderr, "NICK %s NOT REGISTERED\n", other_nick);
                return NULL;
            }
        }
        tries_done++;
    }
    fprintf(stderr, "SERVER UNREACHABLE\n");
    client_shutdown(EXIT_FAILURE);
    return NULL;
}

u_client *parse_lookup_info(char *nick, char *buf_ip, int port_num)
{
    u_client *client = find_client(clients, nick);
    if (client != NULL)
    {
        client->ip.sin_port = htons(port_num);
        if (inet_pton(AF_INET, buf_ip, &client->ip.sin_addr) == -1)
        {
            fprintf(stderr, "inet_pton() error in handle_lookup_reply");
            return NULL;
        }
    }
    else
    {
        memset(&other_addr, 0, addr_len);
        other_addr.sin_family = AF_INET;
        other_addr.sin_port = htons(port_num);
        if (inet_pton(AF_INET, buf_ip, &other_addr.sin_addr) == -1)
        {
            fprintf(stderr, "inet_pton() 2 error in handle_lookup_reply");
            return NULL;
        }

        client = create_client(nick, other_addr);
        if (client == NULL)
            client_shutdown(1);

        add_client(&clients, client);
    }
    return client;
}

int ack_response(int socket_fd, struct sockaddr_in address, int seq_nr, char *buf_out, char *msg)
{
    sprintf(buf_out, "ACK %d %s", seq_nr, msg);
    if (send_packet(socket_fd, buf_out, strlen(buf_out) + 1, 0, (struct sockaddr *)&address, addr_len) < 0)
        return -1;
    return 0;
}

void client_shutdown(int status)
{
    close(socket_fd);
    free_list(clients);
    free_blocked(blocked);
    free_msg_ctrs(msg_ctrs);
    exit(status);
}
