#ifndef UPUSH_SERVER_H
#define UPUSH_SERVER_H

#include <stdio.h>
#include <stdlib.h>
#include <limits.h>
#include <signal.h>
#include "linked_list.h"
#include "utils.h"

#define MAX_SERVER_BUFFER 100 // server response buffer max length
#define MAX_INSTR_LEN 10 // the max length of an instruction
#define HEARTBEAT_LIMIT 30 // the limit on the number of seconds before a client is considered "offline"


/* Parses instruction parameters from the request and stores them
 * in the appropriate variables.
 */
int parse_instr(char* req, int* nr, char* instr ,char* nick);


/* Event handler for lookup from client.
 * Checks for existing clients with given nick, replies to client with information
 * if found.
 */
int lookup_event(int seq_nr, char* nick, char* s_message);


/* Event handler for registry from client.
 * Checks for existing clients with given nick, updates or creates new client
 * based on findings.
 */
int register_event(int seq_nr, char* nick, char* s_message);


/* Closes socket and frees list of registered clients.
 * Is never called by the user, only for error handling
 */
void server_shutdown(int status);


/* Catches interrupts made by the user of the server program.
 * Terminates and frees memory from server on ctrl-c from user
 */
void signal_handler(int signum);



#endif