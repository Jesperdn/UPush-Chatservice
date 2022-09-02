#include "linked_list.h"


/*
    See linked_list.h for descriptions
*/


client_msg_ctr* find_or_create_ctr(client_msg_ctr** start, char* nick)
{   
    client_msg_ctr* ctr = find_ctr(*start, nick);
    if (ctr == NULL)
    {
        ctr = malloc(sizeof(client_msg_ctr));
        if (ctr == NULL)
            return NULL;
        strcpy(ctr->nick, nick);
        ctr->last_seq_nr = -1;
        ctr->next = *start;
        *start = ctr;
    }
    return ctr;
}

client_msg_ctr* find_ctr(client_msg_ctr* start, char* nick)
{
    if (start == NULL)
        return NULL;
    if (strcmp(start->nick, nick) == 0)
        return start;
    return find_ctr(start->next, nick);
}


u_client* create_client(char* nick, struct sockaddr_in ip) 
{   
    u_client* new_client = malloc(sizeof(u_client));
    if (new_client == NULL)
    {
        free(new_client);
        perror("client malloc");
        return NULL;
    }

    strcpy(new_client->nick, nick);
    new_client->ip = ip;
    new_client->next = NULL;
    new_client->prev = NULL;
    return new_client;
}


void add_client(u_client** head, u_client* new_client)
{   
    new_client->next = *head;
    if (*head != NULL)
        (*head)->prev = new_client;
    *head = new_client;
}


u_client* find_client(u_client* start, char* nick)
{
    if (start == NULL)
        return NULL;

    if (strcmp(start->nick, nick) == 0)
        return start;
    
    return find_client(start->next, nick);
}



int remove_client(u_client** start, char* nick)
{
    u_client* registered = find_client(*start, nick);
    if (registered == NULL)
        return 0;

    if (registered == *start)
        *start = registered->next;

    if (registered->prev != NULL)
        registered->prev->next = registered->next;
    
    if (registered->next != NULL)
        registered->next->prev = registered->prev;

    free(registered);
    return 1;
}




int block_nick(blocked_client** head, char* nick)
{
    if (find_blocked(*head, nick) != NULL)
        return 0;

    blocked_client* blocked = malloc(sizeof(blocked_client));
    if (blocked == NULL)
    {
        return 0;
    }

    strcpy(blocked->nick, nick);
    blocked->next = NULL;
    blocked->prev = NULL;

    blocked->next = *head;
    if (*head != NULL)
        (*head)->prev = blocked;

    *head = blocked;
    return 1;
}

blocked_client* find_blocked(blocked_client* head, char* nick)
{
    if (head == NULL)
        return NULL; 

    if (strcmp(head->nick, nick) == 0)
        return head;
    
    return find_blocked(head->next, nick);

}

int unblock_nick(blocked_client** head, char* other_nick)
{
    blocked_client* found = find_blocked(*head, other_nick);
    if (found == NULL)
        return 0;

    if (found == *head)
        *head = found->next;

    if (found->prev != NULL)
        found->prev->next = found->next;
    
    if (found->next != NULL)
        found->next->prev = found->prev;

    free(found);
    return 1;
}




void free_blocked(blocked_client* head)
{
    if (head != NULL)
        free_blocked(head->next); 
    free(head);
}

void free_list(u_client* head)
{
    if (head != NULL)
        free_list(head->next);
    free(head);
}

void free_msg_ctrs(client_msg_ctr* start)
{
    if (start != NULL)
        free_msg_ctrs(start->next);
    free(start);
}


// TEST
void print_clients(u_client* head)
{
    printf("CLIENTS: ");
    u_client* tmp = head;
    while (tmp != NULL)
    {
        printf(" (%s , %d) ", tmp->nick, tmp->ip.sin_port); 
        tmp = tmp->next;
    }
    printf("\n");
    return;
}


void print_blocked(blocked_client* head)
{
    printf("BLOCKED CLIENTS: ");
    blocked_client* tmp = head;
    while (tmp != NULL)
    {
        printf(" (%s) ", tmp->nick); 
        tmp = tmp->next;
    }
    printf("\n");
    return;
}
