#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include "send_packet.h"
#include "upush_server.h"
#include "linked_list.h"



/*
U_PUSH server
Candidate: 15735

Handles communication with clients, provides registration and lookup services.

The server has no exposed exit-procedure to the user. Because of this, there would always be one memory leak per 
client registered. In our chatservice the server is always online, and it cleans up after each client 
becomes silent (no heartbeat), so it is not an issue. But in the practical sense of this exam I catch
interrupt-signals (SIGINT with ctrl-c) and close the server properly. Although it is not a part of the official task
I found it annoying to have Valgrind complain at me when testing.

The code contains some print-statements used during testing, these are commented out.

See upush_server.h for descriptions of each function
*/


struct timeval now;
struct sockaddr_in server_addr, client_addr;
socklen_t len_addr = sizeof(struct sockaddr_in);
int socket_fd;
u_client* clients = NULL;


int main(int argc, char *argv[])
{
    srand48(time(0));
    signal(SIGINT, signal_handler);

    int server_port;


    if (argc != 3)
    {
        printf("Usage: %s <port> <probability>", argv[0]);
        server_shutdown(EXIT_FAILURE);
    }
    else  
    {   
        server_port = (int) strtol(argv[1],NULL, 10);
        if (server_port > USHRT_MAX) 
        {
            printf("Port number too high. Upper limit 65 535\n");
            server_shutdown(EXIT_FAILURE);
        }
        float percentage = strtof(argv[2],NULL) / 100;
        set_loss_probability(percentage);
    }


    // Create UDP socket
    socket_fd = socket(AF_INET, SOCK_DGRAM, 0);
    if (socket_fd == -1)
    {
        fprintf(stderr, "Error on socket creation!\n");
        server_shutdown(EXIT_FAILURE);
    }

    // Set port and IP for server
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(server_port);
    server_addr.sin_addr.s_addr = INADDR_ANY;   

    // Bind to socket
    if (bind(socket_fd, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0)
    {
        perror("Could not bind");
        server_shutdown(EXIT_FAILURE);
    }
    
    // --- MAIN SERVER EVENT LOOP ---
    printf("[READY]\n");
    while(BLOCKING) {
        char buf_out[MAX_SERVER_BUFFER];
        char buf_in[BUFSIZE];
        char instr[MAX_INSTR_LEN];
        char nick[MAX_NICK_LEN];
        int seq_nr;


        //print_clients(clients); 


        if (recvfrom(socket_fd, buf_in, BUFSIZE, 0, (struct sockaddr*)&client_addr, &len_addr) < 0)
        {
            perror("Could not recieve");
            server_shutdown(EXIT_FAILURE);
        }
        printf("<- %s\n", buf_in);
        // --- Parse incoming instruction --- 
        if (!parse_instr(buf_in, &seq_nr, instr, nick)) {
            continue;
        }

        // --- REGISTER ---
        if (strcmp(instr, "REG") == 0)
        {
            if (!register_event(seq_nr, nick, buf_out))
                continue;
        }

        // --- LOOKUP ---
        if (strcmp(instr, "LOOKUP") == 0)
        {
            if (!lookup_event(seq_nr, nick, buf_out))
                continue;
        }
    }

    server_shutdown(EXIT_SUCCESS);
}



int parse_instr(char* buf_in, int* nr, char* instr ,char* nick)
{   

    char format[50]; //max size
    sprintf(format, "PKT %%d %%%ds %%%ds", MAX_INSTR_LEN, MAX_NICK_LEN);

    if (sscanf(buf_in, format, nr, instr, nick) != 3)
        return 0;
    return 1;
}


int lookup_event(int seq_nr, char* nick, char* buf_out)
{
    u_client* found = find_client(clients, nick);
    
    if (gettimeofday(&now, NULL) == -1)
        return 0;

    if (found == NULL)
    {
        sprintf(buf_out, "ACK %d NOT FOUND", seq_nr);
    }
    else {
        
        if ((now.tv_sec - found->last_reg) > HEARTBEAT_LIMIT)
        {
            sprintf(buf_out, "ACK %d NOT FOUND", seq_nr);
            if (remove_client(&clients, nick))
                return 0;
        }
        else 
        {
            sprintf(buf_out, "ACK %d NICK %s IP %s PORT %i", seq_nr, found->nick, inet_ntoa(found->ip.sin_addr), ntohs(found->ip.sin_port));
        }
    }
    //printf("-> %s\n", buf_out);
    if (send_packet(socket_fd, buf_out, strlen(buf_out) + 1, 0, (struct sockaddr*)&client_addr, len_addr) == -1)
        return 0;
    return 1;
}

int register_event(int seq_nr, char* nick, char* buf_out)
{
    u_client* existing = find_client(clients, nick);
    
    if (gettimeofday(&now, NULL) == -1)
        return 0;

    // --- New or existing client ---
    if (existing == NULL) { 
        u_client* new_client = create_client(nick, client_addr);
        new_client->last_reg = now.tv_sec;  
        add_client(&clients, new_client);
    }
    else { 
        existing->ip = client_addr;
        existing->last_reg = now.tv_sec;
    }
    sprintf(buf_out, "ACK %d OK", seq_nr);
    printf("-> %s\n", buf_out);
    if (send_packet(socket_fd, buf_out, strlen(buf_out) + 1, 0, (struct sockaddr*)&client_addr, len_addr) < 0)
        return 0;
    return 1;
}


void server_shutdown(int status)
{
    close(socket_fd);
    free_list(clients);
    exit(status);
}


void signal_handler(int signum) {
    server_shutdown(signum);
}