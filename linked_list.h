#ifndef LINKED_LIST_H
#define LINKED_LIST_H

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
#include <netinet/in.h>

#include "utils.h"



/*
U_PUSH linked lists
Candidate: 15735

This file contains three different linked list structures that are used within the program.
Each struct has their own set of functions. These functions are all very similar to one another.
 - u_client: holds all client information
 - blocked_client: holds the nick of a blocked client
 - client_msg_ctr: holds a nick and the latest sequence number for a given conversation
*/




/* Struct for storing client information.
 * Works as a node in a linked list.
 * Server: Stores all clients registered
 * Client: Stores all clients it has recieved as reply from LOOKUP request. 
 */
typedef struct u_client
{   
    char nick[MAX_NICK_LEN];
    struct u_client* next;
    struct u_client* prev;
    struct sockaddr_in ip;
    long last_reg;
    int tries_counter; 
} u_client;


/* Struct for storing blocked client nicks
 * Works as a node in a linked list
 */
typedef struct blocked_client
{
    char nick[MAX_NICK_LEN];
    struct blocked_client* next;
    struct blocked_client* prev;
} blocked_client;


/* Struct for storing a nick and sequence number.
 * Used for detecting duplicates
 * Works as a node in a linked list
 */
typedef struct client_msg_ctr
{
    char nick[MAX_NICK_LEN];
    int last_seq_nr;
    struct client_msg_ctr* next;
} client_msg_ctr;






// --- u_client functions ---

/* Creates a u_client struct and returns pointer to it
 */
u_client* create_client(char* nick, struct sockaddr_in ip);

/* Adds a u_client struct to the start of the linked list and returns
 * a pointer to the new head (same u_client)
 */
void add_client(u_client** head, u_client* new_client);

/* Removes and returns a pointer to the client to be removed.
 * If the client is not in the list of clients, returns NULL instead
 */
int remove_client(u_client** head, char* nick);

/* Loops through the linked list and returns a pointer to the first 
 * client it can find with the given nick.
 */
u_client* find_client(u_client* head, char* nick);

/* Frees up the linked list from memory.
 */
void free_list(u_client* head);



// --- blocked_client functions ---

/* Takes in a nick and adds it to the list of blocked clients
 */
int block_nick(blocked_client** head, char* nick);

/* Finds the first blocked nick to match the given one and returns a pointer to it.
 */
blocked_client* find_blocked(blocked_client* head, char* nick);

/* Takes in a nick and removes it from the list of blocked clients
 * Returns the unblocked client.
 */
int unblock_nick(blocked_client** head, char* nick);

/* Frees up the linked list of blocked clients from memory
 */
void free_blocked(blocked_client* head);



// --- client_msg_ctr functions --- 



/* Finds or creates a new client_msg_ctr struct.
 * Returns a pointer to the struct it finds or creates
 */
client_msg_ctr* find_or_create_ctr(client_msg_ctr** start, char* nick);


/* Recursive function for finding and returning a client_msg_ctr struct
 */
client_msg_ctr* find_ctr(client_msg_ctr* start, char* nick);


/* Recursive function for clearing memory storing client_msg_ctr structs
 */
void free_msg_ctrs(client_msg_ctr* start);



 

















/* TESTING FUNCTIONS
 * Prints a list of all structs in the linked list.
 */
void print_clients(u_client* head);

void print_blocked(blocked_client* head);


#endif