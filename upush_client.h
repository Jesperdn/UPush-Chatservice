#ifndef UPUSH_CLIENT_H
#define UPUSH_CLIENT_H

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <limits.h>
#include <netinet/in.h>
#include "error.h"
#include "utils.h"

#define HEARTBEAT_INTERVAL 10


/* Takes in a socket file descriptor and fd_set and zeroes and readies the fd_set.
 */
void init_fds(int socket_fd, fd_set* fds);


/* Takes in the server address and a nick, uses [send_packet] to send a registration 
 * to the server.
 * Returns 1 for success, 0 for fail.
 */
int send_reg_pkt(struct sockaddr_in server_addr, char* nick);

/* Takes in the server address and a nick, handles all logic related to 
 * stop-and-wait in registration
 * Returns 1 for success, 0 for fail.
 */
int reg_event(struct sockaddr_in server_addr, char* nick);


/* Takes in client nick, u_client* representing the other client, and the message
 * Handles all logic related to stop-and-wait in chatting
 * Returns 1 for success, 0 for fail.
 */
int chat_event(char* my_nick, char* to_nick, char* message);


/* Takes in a u_client* representing the other client, the message and the senders nick.
 * Uses [send_packet] to send a msg to the client
 * Returns 1 for success, 0 for fail.
 */
int send_msg(u_client* client, char* other_nick, char* message, char* my_nick);


/* Takes in a nick and number of tries
 * Handles all logic related to stop-and-wait in lookups
 * Returns a ptr to client found.
 */
u_client* lookup_event(char* nick, int tries);


/* Takes in nick, uses [send_packet] to send lookup request
 * Returns 1 for success, 0 for fail.
 */
int send_lookup(char* nick);


/* Takes in a nick, buffer for ip and a port number.
 * Creates or updates a client. Returns a ptr to client
 */
u_client* parse_lookup_info(char* nick, char* buf_ip, int port_num);


/* Sends an ack response with [send_packet] to the target given by the address struct
 * returns 1 for success, 0 for fail.
 */
int ack_response(int socket_fd, struct sockaddr_in address, int seq_nr, char* buf_out, char* msg);


/* Frees up all memory used by the client, closes the socket
 * performs exit()
 */
void client_shutdown(int status);

#endif