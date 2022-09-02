#ifndef UTILS_H
#define UTILS_H

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <ctype.h> 
#include <time.h>
#include "utils.h"


/*
U_PUSH utils
Candidate: 15735

Contains definitions used by both server and client, as well as a couple of
functions for the client
*/


// Constants used throughout the server and client
#define TRUE 1
#define FALSE 0
#define BLOCKING 1
#define BUFSIZE 1500    // instruction + message (1400)
#define LEN_IP_ADDR 21  // max length + 0
#define MAX_NICK_LEN 21 // max length + 0


/* Checks wether the nickname given is in the right format (as stated by the exam)
 * Returns 0 for success, -1 otherwise
 */
int check_nick(char* nick);


/* Reads the input from STDIN and stores in buffer
 * Removes newline if that is present
 */
void get_input(char* buf_in);


#endif