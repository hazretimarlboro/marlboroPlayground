#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>

typedef enum 
{
    _OK,
    _COMMAND_NOT_FOUND,
    _INVALID_ARGUMENTS,
    _NOT_A_DIRECTORY,
    _NOT_FOUND,
    _PERMISSION_DENIED,
    _OBJECT_ALREADY_EXISTS,
    _ILLEGAL_CHARACTER,
    _TOO_LONG,
    _NOT_A_FILE
} error_t;

//set up the file system
typedef enum 
{
    _DIR,
    _FILE
} node_types;

typedef struct node
{
    char name[32];
    struct node* sibling;
    struct node* parent;
    struct node* children;

    node_types type;
    size_t size;
    uint8_t* data;
} node;

node* _root;
node* _curr_dir;

size_t _get_size(node* nd)
{
    if(!nd) return 0;
    size_t NODESIZE = 0;
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

    if (strcmp(path,"/")==0) return _root;

    char *save_pointer;
    node* cur;

    if(path[0] == '/')
        cur = _root;
    else
        return NULL;
    
    char* path_copy = malloc(strlen(path)+1);
    if(!path_copy) return NULL;
    strcpy(path_copy,path);

    char* del = "/";

    char* token = strtok_r(path_copy,del,&save_pointer);
    while(token)
    {
        cur = _find_child(token,cur);
        if(!cur)
        {
            free(path_copy);
            return NULL;
        }
        token = strtok_r(NULL,del,&save_pointer);
    }
    
    free(path_copy);
    return cur; 
}

void _free_node(node* n)
{
    if (!n) return;

    node* child = n->children;
    while (child)
    {
        node* next_sibling = child->sibling;
        _free_node(child);
        child = next_sibling;
    }

    if (n->data) free(n->data);

    free(n);
}


char* _get_absolute_path(node* cwd)
{
    if (!cwd) return NULL;

    const int MAX_DEPTH = 512;
    const node* stack[MAX_DEPTH];
    int depth = 0;

    const node* cur = cwd;
    while (cur && depth < MAX_DEPTH) 
    {
        stack[depth++] = cur;
        cur = cur->parent;
    }

    if (depth >= MAX_DEPTH) return NULL;

    int total_len = 1; 
    for (int i = depth - 1; i >= 0; i--) 
    {
        total_len += strlen(stack[i]->name) + 1; 
    }

    char* path = malloc(total_len + 1);
    if (!path) return NULL;

    path[0] = '\0';

    for (int i = depth - 1; i >= 0; i--) 
    {
        strcat(path, "/");
    
        if (i != depth - 1 || strcmp(stack[i]->name, "/") != 0)
            strcat(path, stack[i]->name);
    }

    if (strcmp(path, "") == 0) strcpy(path, "/");

    return path;
}

void _remove_first_last(char* str) {
    size_t len = strlen(str);
    if (len <= 2) {
        str[0] = '\0';
        return;
    }

    for (size_t i = 0; i < len - 1; i++) {
        str[i] = str[i + 1];
    }

    str[len - 2] = '\0';
}

//commands
int _clear()
{
    printf("\x1b[2J\x1b[H");
    return _OK;
}

int _ls(node* cwd)
{
    node* cur = cwd->children;
    while(cur)
    {
        if(cur->type == _DIR)
        {
            printf(">%s    (Size:%zu)\n",cur->name,cur->size);
        }
        else
        {
            printf("-%s    (Size:%zu)\n",cur->name,cur->size);
        }
        cur = cur->sibling;
    }
    return _OK;
}

int _touch(node* cwd, char* name)
{
    if(strlen(name) >= 32)
    {
        return _TOO_LONG;
    }

    node* cur =cwd->children;
    if (name[0] == '\0' || strcmp(name,".")==0 || strcmp(name,"..")==0) 
    return _ILLEGAL_CHARACTER;
    for (int i = 0; name[i]; i++) 
    {
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

int _insert(char* content, char* name, node* cwd,char* option)
{
    node* file = _find_child(name,cwd);
    size_t old_size = file->data ? file->size : 0;

    if(!name) return _INVALID_ARGUMENTS;
    if(!content) return _INVALID_ARGUMENTS;
    if(!file) return _NOT_FOUND;
    if(strlen(content) > 1023) return _TOO_LONG;
    if(file->type != _FILE) return _NOT_A_FILE;
    
    /* size_t old_size = file->size;
    free(file->data);
    file->data = malloc(strlen(content)+ 1);
    strcpy((char*) file->data,content);
    file->size = strlen(content);
    size_t delta = file->size - old_size;

    node* cur = file->parent;
    while(cur)
    {
        cur->size += delta;
        cur = cur->parent;
    } */


    //overwrite
    if(strcmp(option,">")==0)
    {
        free(file->data);
        file->data = malloc(strlen(content) + 1);
        strcpy((char*) file->data,content);
        file->size = strlen(content);
        size_t delta = file->size - old_size;

        node* cur = file->parent;
        while(cur)
        {
            cur->size += delta;
            cur = cur->parent;
        }
        
    }
    //appeend
    else if(strcmp(option,">>")==0)
    {
        char* old_data = file->data ? strdup((char*)file->data) : strdup("");       
        char* new_data = malloc(old_size + strlen(content) + 1);
        
        new_data[0]='\0';
        
        strncat(new_data, old_data, sizeof(new_data)-strlen(new_data)-1);
        strncat(new_data,content,sizeof(new_data)-strlen(new_data)-1);
        
        file->size = strlen(new_data);
        
        free(file->data);
        
        file->data = (uint8_t*)new_data;
        size_t delta = file->size - old_size;
        
        free(old_data);

        node* cur = file->parent;
        while(cur)
        {
            cur->size += delta;
            cur = cur->parent;
        }
    }
    else
    {
        return _INVALID_ARGUMENTS;
    }

    return _OK;

}

int _print(node* cwd, char* name)
{
    node* file = _find_child(name,cwd);

    if(!name) return _INVALID_ARGUMENTS;
    if(!file) return _NOT_FOUND;
    if(file->type != _FILE) return _NOT_A_FILE;

    if(file->data)
        printf("%s", file->data);
    return _OK;
}

int _mkdir(node* cwd, char* name)
{
    if(strlen(name) >= 32)
    {
        return _TOO_LONG;
    }

    node* cur = cwd->children;

    if (name[0] == '\0' || strcmp(name,".")==0 || strcmp(name,"..")==0) 
    return _ILLEGAL_CHARACTER;
    while(cur)
    {
        if(strcmp(cur->name,name)==0) return _OBJECT_ALREADY_EXISTS;

        cur = cur->sibling;
    }

    for (int i = 0; name[i]; i++) 
    {
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
    if (!path || strcmp(path, "/") == 0) 
    {
        printf("Cannot delete root!\n");
        return _INVALID_ARGUMENTS;
    }

    char* parent_path = strdup(path);
    if (!parent_path) return _INVALID_ARGUMENTS;

    char* node_name = strrchr(parent_path, '/');
    if (!node_name) 
    {
        free(parent_path);
        return _NOT_FOUND;
    }

    *node_name = '\0';
    node_name++;

    node* parent = (strlen(parent_path) == 0) ? _root : _node_from_path(parent_path);
    if (!parent) 
    {
        free(parent_path);
        return _NOT_FOUND;
    }

    node* prev = NULL;
    node* cur = parent->children;
    while (cur) 
    {
        if (strcmp(cur->name, node_name) == 0) 
        {
            if (cur == _curr_dir) 
            {
                printf("Cannot delete current working directory!\n");
                free(parent_path);
                return _INVALID_ARGUMENTS;
            }

            if (prev)
                prev->sibling = cur->sibling;
            else
                parent->children = cur->sibling;
    
            size_t delta = cur->size;
            node* par = parent;
            while(par)
            {
                par->size -= delta;
                par = par->parent;
            }
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

int _rm(node* cwd, char* name,int conf)
{
    if(!conf)
    {
        char choice[10];
        printf("Are you sure you want to delete %s? (yes/no): ", name);
        if (!fgets(choice, sizeof(choice), stdin)) return _INVALID_ARGUMENTS;

    
        choice[strcspn(choice, "\n")] = '\0';

    
        int c;
        while ((c = getchar()) != '\n' && c != EOF);

        if (strcmp(choice, "no") == 0) return _OK;
        if (strcmp(choice, "yes") != 0) return _INVALID_ARGUMENTS;
    }

    
    if (strchr(name, '/')) 
    {
        return _rm_by_path(name);
    }

    
    node* prev = NULL;
    node* cur = cwd->children;
    while (cur) 
    {
        if (strcmp(cur->name, name) == 0) 
        {
            if (cur == _curr_dir) 
            {
                printf("Cannot delete current working directory!\n");
                return _INVALID_ARGUMENTS;
            }

            if (prev)
                prev->sibling = cur->sibling;
            else
                cwd->children = cur->sibling;

            size_t delta = _get_size(cur);
            _free_node(cur);
            
            if(cwd == _root) return _OK;

            node* par = cwd;
            while(par)
            {
                par->size -= delta;
                par = par->parent;
            }
            
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
            return _OK;
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
        int m = 0;
        char* target_name = NULL;

        if (i == 3 && strcmp(splt[1], "-f") == 0)
        {
            m = 1;
            target_name = splt[2];
        } 
        else if (i == 2)
        {
            target_name = splt[1];
        }
        else
        {
            printf("Bad Usage! The right way is: rm [-f] dir/fileName\n");
            return _INVALID_ARGUMENTS;
        }

        return _rm(cwd, target_name, m);
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
    else if(strcmp(splt[0],"clear")==0)
    {
        if(i > 1) return _INVALID_ARGUMENTS;
        return _clear();
    }
    else if(strcmp(splt[0],"exit")==0)
    {
        if(i > 1) return _INVALID_ARGUMENTS;
        _free_node(_root);
        exit(_OK);
    }
    else if(strcmp(splt[0], "insert")==0)
    {
        if(i < 4) 
        {
            printf("Bad Usage! The right way is: insert >/>> <fileName> \"<content>\"\n");
            return _INVALID_ARGUMENTS;
        }
        if(splt[3][0]!='"'||splt[i-1][strlen(splt[i-1])-1]!='"')
            return _INVALID_ARGUMENTS;
        if(!splt[3]) return _INVALID_ARGUMENTS;
        
        char* name = splt[2];
        char buffer[1024];
        char* option = splt[1];
        buffer[0] = '\0';

        if(strlen(splt[3]) > 1022) return _TOO_LONG;
        if(strlen(splt[3]) < 2) return _INVALID_ARGUMENTS; 
        
        for(int v =3; v < i; ++v)
        {
            strncat(buffer,splt[v],sizeof(buffer)-strlen(buffer)-1);

            if(v < i-1)
            {
                strncat(buffer," ",sizeof(buffer)-strlen(buffer)-1);
            }
        }
        buffer[strlen(buffer)] = '\0';

        _remove_first_last(buffer);
        return _insert(buffer,name,cwd,option);
    }
    else if(strcmp(splt[0], "print!") == 0)
    {
        if(i != 2) return _INVALID_ARGUMENTS;
        char* name = splt[1];
        return _print(cwd,name);
    }
    else
    {
        return _COMMAND_NOT_FOUND;
    }
}

int main()
{
    init();

    char command[2048];
    char* del = " ";

    _curr_dir = _root;

    while(1)
    {
        char* abspath = _get_absolute_path(_curr_dir);
        printf("%s$", abspath);
        free(abspath);
        fgets(command,2048,stdin);
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
        
        switch (STATUS)
        {
            case _COMMAND_NOT_FOUND:
                printf("Sorry, your command is invalid!\n");
                break;
            case _INVALID_ARGUMENTS:
                printf("Sorry, your arguments are invalid!\n");
                break;
            case _ILLEGAL_CHARACTER:
                printf("Your statement includes illegal characters!\n");
                break;
            case _NOT_FOUND:
                printf("File/Directory could not be found!\n");
                break;
            case _OBJECT_ALREADY_EXISTS:
                printf("File/Directory already exists!\n");
                break;
            case _NOT_A_DIRECTORY:
                printf("Not a directory!\n");
                break;
            case _TOO_LONG:
                printf("The name/content is too long!\n");
                break;
            case _NOT_A_FILE:
                printf("Not a file!\n");
                break;
        }

        splitted_code[i] = NULL;
        
    }
    _free_node(_root);
    return 0;
}