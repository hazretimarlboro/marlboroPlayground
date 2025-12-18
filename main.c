#include <stdio.h>
#include <string.h>
#include <stdlib.h>


typedef enum {
    _OK,
    _COMMAND_NOT_FOUND,
    _INVALID_ARGUMENTS,
    _NOT_A_DIRECTORY,
    _NOT_FOUND,
    _PERMISSION_DENIED,
    _OBJECT_ALREADY_EXISTS,
    _ILLEGAL_CHARACTER
} error_t;

//set up the file system
typedef enum {
    _DIR,
    _FILE
} node_types;


typedef struct node{
    char name[32];

    node_types type;
    struct node* sibling;
    struct node* parent;
    struct node* children;

    size_t size;
    __uint8_t* data;
} node;


node* _root;
node* _curr_dir;

size_t _get_size(node* nd)
{
    if(!nd) return 0;
    size_t NODESIZE = sizeof(node);
    node* cur = nd->children;
    
    if(nd->data) NODESIZE += nd->size;

    while(cur)
    {
        NODESIZE += _get_size(cur);
        cur=cur->sibling;
    }

    return NODESIZE;
}

node* _create_node(char* name, node_types type)
{
    node *cur = malloc(sizeof(node));
    strcpy(cur->name,name);
    cur->type = type;
    cur->parent = NULL;
    cur->children = NULL;
    cur->sibling = NULL;
    cur->size = 0;
    cur->data = NULL;
    return cur;
}

void init()
{
    _root = _create_node("/",_DIR);
}

void _add_child(node* parent, node* child)
{
    child->parent = parent;
    child->sibling = parent->children;
    parent->children = child;
}

node* _find_child(char* name, node* parent)
{
    node* cur = parent->children;
    while(cur)
    {
        if(strcmp(cur->name, name)==0)
            return cur;
        cur= cur->sibling;
    }
    return NULL;
}

node* _node_from_path(char* path)
{
    if (path == NULL || path[0] == '\0')
        return NULL;

    node* cur;

    if(path[0] == '/')
        cur = _root;
    else
        return NULL;
    
    char* path_copy = malloc(strlen(path)+1);
    if(!path_copy) return NULL;
    strcpy(path_copy,path);

    char* del = "/";

    char* token = strtok(path_copy,del);
    while(token)
    {
        cur = _find_child(token,cur);
        if(!cur)
        {
            free(path_copy);
            return NULL;
        }
        token = strtok(NULL,del);
    }
    
    free(path_copy);
    return cur; 
}

void _free_node(node* n)
{
    if (!n) return;

    // Free all children recursively
    node* child = n->children;
    while (child)
    {
        node* next_sibling = child->sibling;
        _free_node(child);
        child = next_sibling;
    }

    // Free node's data if used (you have __uint8_t* data in struct)
    if (n->data) free(n->data);

    // Free the node itself
    free(n);
}


char* _get_absolute_path(node* cwd)
{
    if (!cwd) return NULL;


    const int MAX_DEPTH = 512;
    const node* stack[MAX_DEPTH];
    int depth = 0;


    const node* cur = cwd;
    while (cur && depth < MAX_DEPTH) {
        stack[depth++] = cur;
        cur = cur->parent;
    }

    if (depth >= MAX_DEPTH) return NULL;

    
    int total_len = 1; 
    for (int i = depth - 1; i >= 0; i--) {
        total_len += strlen(stack[i]->name) + 1; 
    }

    char* path = malloc(total_len + 1);
    if (!path) return NULL;

    path[0] = '\0';

    
    for (int i = depth - 1; i >= 0; i--) {
        strcat(path, "/");
        
        if (i != depth - 1 || strcmp(stack[i]->name, "/") != 0)
            strcat(path, stack[i]->name);
    }

    
    if (strcmp(path, "") == 0) strcpy(path, "/");

    return path;
}

//commands


int _ls(node* cwd)
{
    node* cur = cwd->children;
    while(cur)
    {
        if(cur->type == _DIR)
        {
            printf(">%s\n",cur->name);
        }
        else
        {
            printf("-%s\n",cur->name);
        }
        cur = cur->sibling;
    }
    return _OK;
}

int _touch(node* cwd, char* name)
{
    node* cur =cwd->children;
    if (name[0] == '\0' || strcmp(name,".")==0 || strcmp(name,"..")==0) 
    return _ILLEGAL_CHARACTER;
    for (int i = 0; name[i]; i++) {
        char c = name[i];
        if (!((c >= 'a' && c <= 'z') ||
              (c >= 'A' && c <= 'Z') ||
              (c >= '0' && c <= '9') ||
              c == '_' || c == '-' || c == '.'))
            {
                return _ILLEGAL_CHARACTER;
            }
    }
    while(cur)
    {
        if(strcmp(cur->name,name)==0) return _OBJECT_ALREADY_EXISTS;

        cur = cur->sibling;
    }

    node* new_file = _create_node(name, _FILE);
    _add_child(cwd,new_file);
    return _OK;
}

int _mkdir(node* cwd, char* name)
{
    node* cur = cwd->children;

    if (name[0] == '\0' || strcmp(name,".")==0 || strcmp(name,"..")==0) 
    return _ILLEGAL_CHARACTER;
    while(cur)
    {
        if(strcmp(cur->name,name)==0) return _OBJECT_ALREADY_EXISTS;

        cur = cur->sibling;
    }

    for (int i = 0; name[i]; i++) {
        char c = name[i];
        if (!((c >= 'a' && c <= 'z') ||
              (c >= 'A' && c <= 'Z') ||
              (c >= '0' && c <= '9') ||
              c == '_' || c == '-' || c == '.'))
            {
                return _ILLEGAL_CHARACTER;
            }
    }
    
    node* new_dir = _create_node(name, _DIR);
    _add_child(cwd,new_dir);
    

    return _OK;
}

int _rm_by_path(char* path)
{
    if (!path || strcmp(path, "/") == 0) {
        printf("Cannot delete root!\n");
        return _INVALID_ARGUMENTS;
    }

    char* parent_path = strdup(path);
    if (!parent_path) return _INVALID_ARGUMENTS;

    char* node_name = strrchr(parent_path, '/');
    if (!node_name) {
        free(parent_path);
        return _NOT_FOUND;
    }

    *node_name = '\0';
    node_name++;

    node* parent = (strlen(parent_path) == 0) ? _root : _node_from_path(parent_path);
    if (!parent) {
        free(parent_path);
        return _NOT_FOUND;
    }

    node* prev = NULL;
    node* cur = parent->children;
    while (cur) {
        if (strcmp(cur->name, node_name) == 0) {
            if (cur == _curr_dir) {
                printf("Cannot delete current working directory!\n");
                free(parent_path);
                return _INVALID_ARGUMENTS;
            }

            if (prev)
                prev->sibling = cur->sibling;
            else
                parent->children = cur->sibling;

            _free_node(cur);
            free(parent_path);
            return _OK;
        }
        prev = cur;
        cur = cur->sibling;
    }

    free(parent_path);
    return _NOT_FOUND;
}

int _rm(node* cwd, char* name)
{
    char choice[10];
    printf("Are you sure you want to delete %s? (yes/no): ", name);
    if (!fgets(choice, sizeof(choice), stdin)) return _INVALID_ARGUMENTS;

    
    choice[strcspn(choice, "\n")] = '\0';

    
    int c;
    while ((c = getchar()) != '\n' && c != EOF);

    if (strcmp(choice, "no") == 0) return _OK;
    if (strcmp(choice, "yes") != 0) return _INVALID_ARGUMENTS;

    
    if (strchr(name, '/')) {
        return _rm_by_path(name);
    }

    
    node* prev = NULL;
    node* cur = cwd->children;
    while (cur) {
        if (strcmp(cur->name, name) == 0) {
            if (cur == _curr_dir) {
                printf("Cannot delete current working directory!\n");
                return _INVALID_ARGUMENTS;
            }

            if (prev)
                prev->sibling = cur->sibling;
            else
                cwd->children = cur->sibling;

            _free_node(cur);
            return _OK;
        }
        prev = cur;
        cur = cur->sibling;
    }

    return _NOT_FOUND;
}


int _cd(node* cwd, char* name)
{

    node* cur = cwd->children;
    
    if(strchr(name,'/'))
    {
        node* target=_node_from_path(name);
        if(!target) return _NOT_FOUND;
        _curr_dir =target;
        return _OK;
    }

    if(strcmp(name,"..")==0)
    {
        if(strcmp(_curr_dir->name,"/")==0)
        {
            return _NOT_FOUND;
        }
        else
        {
            _curr_dir = _curr_dir->parent;
            return _OK;
        }
    }

    while(cur)
    {
        if(strcmp(cur->name,name)==0)
        {
            if(cur->type == _FILE)
            {
                return _NOT_A_DIRECTORY;
            }
            _curr_dir = cur;
            return _OK;
        }
        cur=cur->sibling;
    }

    return _NOT_FOUND;
}

//command executer
int _exec(char** splt,int i, node* cwd)
{
    if(strcmp(splt[0],"ls")==0)
    {
        if(i > 1)
        { 
            printf("Bad Usage! The right way is: ls\n");
            return _INVALID_ARGUMENTS;
        }
        return _ls(cwd);
    }
    else if(strcmp(splt[0],"mkdir")==0)
    {
        if(i != 2)
        {
            printf("Bad Usage! The right way is: mkdir dirName\n");
            return _INVALID_ARGUMENTS;
        }
        return _mkdir(cwd,splt[1]);
    }
    else if(strcmp(splt[0],"cd")==0)
    {
        if(i != 2)
        {
            printf("Bad Usage! The right way is: cd dirName\n");
            return _INVALID_ARGUMENTS;
        }
        return _cd(cwd,splt[1]);
    }
    else if(strcmp(splt[0],"rm")==0)
    {
        if(i != 2)
        {
            printf("Bad Usage! The right way is: rm dir/fileName\n");
            return _INVALID_ARGUMENTS;
        }
        return _rm(cwd,splt[1]);
    }
    else if(strcmp(splt[0],"touch")==0)
    {
        if(i != 2)
        {
            printf("Bad Usage! The right way is: touch fileName\n");
            return _INVALID_ARGUMENTS;
        }
        return _touch(cwd,splt[1]);
    }
    else
    {
        return _COMMAND_NOT_FOUND;
    }
}



int main()
{
    init();

    char command[128];
    char* del = " ";

    _curr_dir = _root;

    while(1)
    {
        char* abspath = _get_absolute_path(_curr_dir);
        printf("%s$", abspath);
        free(abspath);
        fgets(command,128,stdin);
        command[strcspn(command, "\n")] = '\0';
        char* token = strtok(command,del);
        char* splitted_code[32];
        int i = 0;
        while (token && i < 31)
        {
            splitted_code[i] = token;
            token = strtok(NULL,del);
            ++i;
        }
        if(i >= 31)
        {
            printf("Too many arguments!\n");
        }
        
        int argc = i;
        int STATUS = _exec(splitted_code,argc,_curr_dir);
        if(STATUS == _COMMAND_NOT_FOUND)
        {
            printf("Sorry, your command is invalid!\n");
        }
        else if(STATUS == _INVALID_ARGUMENTS)
        {
            printf("Sorry, your arguments are invalid!\n");
        }
        else if(STATUS == _ILLEGAL_CHARACTER)
        {
            printf("Your statement includes illegal characters!\n");
        }
        else if(STATUS == _NOT_FOUND)
        {
            printf("File/Directory could not be found!\n");
        }
        else if(STATUS == _OBJECT_ALREADY_EXISTS)
        {
            printf("File/Directory already exists!\n");
        }
        else if(STATUS == _NOT_A_DIRECTORY)
        {
            printf("Not a directory!\n");
        }

        splitted_code[i] = NULL;
        
    }
    _free_node(_root);
    return 0;
}