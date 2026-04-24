#include "memory.h"
#include "string.h"
#include "command_hashtable.h"

#define TABLE_SIZE 100

typedef void (*function)(int argc, char **argv);

static Node *table[TABLE_SIZE];

unsigned int hash(const char *key){
    unsigned int h = 0;
    while(*key){
        h = h + 739061 * *key++;
    }
    return h;
}

void register_command(const char *key, function value, const char * help_text){
    unsigned int index = hash(key) % TABLE_SIZE;

    Node *e = kmalloc(sizeof(Node));
    e->key = strdup(key);
    e->value = value;
    e->help_text = help_text;
    e->next = table[index];

    table[index] = e;
}

Node ** get_full_table(){
    return table;
}

int get_table_size(){
    return TABLE_SIZE;
}

function lookup_command(const char *key){
    unsigned int index = hash(key) % TABLE_SIZE;
    Node *e = table[index];

    while (e) {
        if(strcmp(e->key, key) == 0) return e->value;
        e = e->next;
    }
    return NULL;
}

Node * lookup_command_node(const char *key){
    unsigned int index = hash(key) % TABLE_SIZE;
    Node *e = table[index];
    if (e) return e;
    return NULL;
}

