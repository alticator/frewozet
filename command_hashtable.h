typedef void (*function)(int argc, char **argv);
typedef struct Node {
    char * key;
    function value;
    char * help_text;
    struct Node * next;
} Node;

void register_command(const char *key, function value, const char * help_text);
function lookup_command(const char *key);
Node * lookup_command_node(const char *key);
Node ** get_full_table();
int get_table_size();