#include "utils.h"

int check_nick(char* nick)
{   
    if (strlen(nick) < 1 || strlen(nick) > MAX_NICK_LEN)
        return -1;

    while(*nick)
    {
        if (isspace((int) *nick) || !(*nick >= 33) && (*nick <= 126))
        {
            return -1;
        }
        nick++;
    }
    return 0;
} 


void get_input(char* buf_in)
{
    char c;
    fgets(buf_in, BUFSIZE, stdin);
    if (buf_in[strlen(buf_in)-1] == '\n')
        buf_in[strlen(buf_in)-1] = 0;
}