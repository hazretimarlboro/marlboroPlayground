#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>

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
    _NOT_A_FILE,
    _WRONG_PASSWORD
} error_t;

//set up the file system
typedef enum 
{
    _DIR,
    _FILE
} node_types;

typedef enum 
{
    _CASUAL,
    _SUPERUSER
} users;

typedef struct node
{
    char name[32];
    struct node* sibling;
    struct node* parent;
    struct node* children;

    node_types type;
    size_t     size;
    uint8_t*   data;
    users      creator;
} node;

node* _root;
node* _curr_dir;
users _curr_usr = _CASUAL;
char _curr_pass[128] = "helloworld";

bool _is_allowed(node* nodeToAccess, users user)
{
    if(!nodeToAccess) return false;
    if(nodeToAccess->creator == _CASUAL) return true;
    if(nodeToAccess->creator == _SUPERUSER && user == _CASUAL) return false;
    return true;
}

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
    cur->creator = _CASUAL;
    return cur;
}

void init()
{
    _root = _create_node("/",_DIR);
    _root->creator = _CASUAL;
    _curr_dir      = _root;
}

void _add_child(node* parent, node* child)
{
    child->parent = parent;
    child->sibling = parent->children;
    parent->children = child;
}

node* _find_child(char* name, node* parent)
{
    if(!parent) return NULL;
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
        if(strcmp(stack[i]->name,"/") == 0) continue;
        
        strcat(path, "/");
    
        if (i != depth - 1 || strcmp(stack[i]->name, "/") != 0)
            strcat(path, stack[i]->name);
    }

    if (strcmp(path, "") == 0) strcpy(path, "/");

    return path;
}

char* _parse_command_for_insert(char* command)
{
    int   hashtag = strcspn(command,"#");
    char* content = &command[hashtag+1];
    content[strlen(content)] = '\0';
    return content;
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
    char* mode;

    while(cur)
    {
        if(cur->creator==_CASUAL)
        {
            mode = "Casual";
        }
        else
        {
            mode = "Superuser";
        }

        size_t size = _get_size(cur);

        if(cur->type == _DIR)
        {
            printf(">%s    (Size:%zu) Mode:%s\n",cur->name,size,mode);
        }
        else
        {
            printf("-%s    (Size:%zu) Mode:%s\n",cur->name,size,mode);
        }
        cur = cur->sibling;
    }
    return _OK;
}

int _touch(node* cwd, char* name,users currentuser)
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
               c == '_' || c == '-'  || c == '.'))
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
    if(_is_allowed(cwd,_curr_usr) == false) 
    {
        _free_node(new_file);
        return _PERMISSION_DENIED;
        
    }
    new_file->creator = currentuser;
    _add_child(cwd,new_file);
    return _OK;
}

int _move(char* source, char* destination, node* cwd)
{
    if(!source || !destination) return _INVALID_ARGUMENTS;

    node* SourceNode = strchr(source,'/') ? _node_from_path(source) : _find_child(source,cwd);
    node* TargetDestination = strchr(destination,'/') ? _node_from_path(destination) : _find_child(destination,cwd);

    if(!SourceNode) return _NOT_FOUND;
    if(!TargetDestination) return _NOT_FOUND;
    if(TargetDestination->type != _DIR) return _NOT_A_DIRECTORY;

    node* oldParent = SourceNode->parent;
    if(!oldParent) return _INVALID_ARGUMENTS; 

    if(oldParent->children == SourceNode) 
    {
        oldParent->children = SourceNode->sibling;
    }
    else 
    {
        node* cur = oldParent->children;
        while(cur && cur->sibling != SourceNode)
        {
            cur = cur->sibling;
        }
        if(!cur) return _NOT_FOUND;
        cur->sibling = SourceNode->sibling;
    }

    node* childNewParent = TargetDestination->children;
    while(childNewParent)
    {
        if(strcmp(childNewParent->name,SourceNode->name)==0)
        {
            printf("There is a file there with the same name!\n");
            return _OBJECT_ALREADY_EXISTS;
        }
        
        childNewParent = childNewParent->sibling;
    }

    SourceNode->parent = TargetDestination;
    SourceNode->sibling = TargetDestination->children;
    TargetDestination->children = SourceNode;

    return _OK;
}

int _insert(char* content, char* name, node* cwd,char* option,users current)
{
    if(!name)    return _INVALID_ARGUMENTS;
    if(!content) return _INVALID_ARGUMENTS;
    if(!option)  return _INVALID_ARGUMENTS;

    node* file = _find_child(name,cwd);
    if(!file)    return _NOT_FOUND;

    if(!_is_allowed(file,current))
    {
        return _PERMISSION_DENIED;
    }

    size_t old_size = file->data ? _get_size(file) : 0;

    if(strlen(content) > 1023) return _TOO_LONG;
    if(file->type != _FILE) return _NOT_A_FILE;
    
    //overwrite
    if(strcmp(option,">")==0)
    {
        free(file->data);
        file->data = malloc(strlen(content) + 1);
        strcpy((char*) file->data,content);
        file->size = strlen(content);
        size_t delta = _get_size(file) - old_size;

        node* cur = file->parent;
        while(cur)
        {
            cur->size += delta;
            cur = cur->parent;
        }
        
    }
    //append
    else if(strcmp(option, ">>") == 0)
    {
        size_t content_len = strlen(content);
        size_t new_size = old_size + content_len;

        char* new_data = malloc(new_size + 1);
        if (!new_data) return _INVALID_ARGUMENTS;

        if (file->data)
            memcpy(new_data, file->data, old_size);

        memcpy(new_data + old_size, content, content_len);
        new_data[new_size] = '\0';

        free(file->data);
        file->data = (uint8_t*)new_data;
        file->size = new_size;

        size_t delta = new_size - old_size;

        node* cur = file->parent;
        while (cur)
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

int _print(node* cwd, char* name, users current)
{
    if(!name) return _INVALID_ARGUMENTS;
    node* file = _find_child(name,cwd);
    if(!file) return _NOT_FOUND;

    if(!_is_allowed(file,current))
    {
        return _PERMISSION_DENIED;
    }

    if(file->type != _FILE) return _NOT_A_FILE;

    if(file->data)
        printf("%s", file->data);
    return _OK;
}

int _help()
{
    printf("ls     - List folders and files\n");
    printf("move   - Move folders/files around\n");
    printf("mkdir  - Create a folder\n");
    printf("cd     - Change current folder\n");
    printf("change - Change superuser password\n");
    printf("rm     - Remove item");
    printf("uprint - Print the current user status (Superuser/Casual)\n");
    printf("switch - Switch between casual user and superuser\n");
    printf("touch  - Create a file\n");
    printf("clear  - Clear the screen\n");
    printf("exit   - Exit the program\n");
    printf("insert - Insert data into a file\n");
    printf("print! - Print the contents of a file\n");
    printf("help   - Show this menu\n");

    return _OK;
}

int _mkdir(node* cwd, char* name, users current)
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
               c == '_' || c == '-'  || c == '.'))
            {
                return _ILLEGAL_CHARACTER;
            }
    }
    
    node* new_dir = _create_node(name, _DIR);
    if(_is_allowed(cwd,_curr_usr) == false) 
    {
        _free_node(new_dir);
        return _PERMISSION_DENIED;
    }
    if(cwd->creator == _SUPERUSER) new_dir->creator = _SUPERUSER;
    
    new_dir->creator = current;
    _add_child(cwd,new_dir);
    

    return _OK;
}

int _rm_by_path(char* path,users current)
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

            if(!_is_allowed(cur,current))
            {
                free(parent_path);
                return _PERMISSION_DENIED;
            }
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
    
            size_t delta = _get_size(cur);
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

int _rm(node* cwd, char* name,int conf,users current)
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
        return _rm_by_path(name,current);
    }

    
    node* prev = NULL;
    node* cur = cwd->children;
    while (cur) 
    {
        if (strcmp(cur->name, name) == 0) 
        {
            if(!_is_allowed(cur,current))
            {
                return _PERMISSION_DENIED;
            }
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
        if(target->type != _DIR) return _NOT_A_DIRECTORY;
        
        if(!_is_allowed(target,_curr_usr)) return _PERMISSION_DENIED;
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
            if(cur->type != _DIR)
            {
                return _NOT_A_DIRECTORY;
            }
            if(!_is_allowed(cur,_curr_usr)) return _PERMISSION_DENIED;
            _curr_dir = cur;
            return _OK;
        }
        cur=cur->sibling;
    }

    return _NOT_FOUND;
}

int _print_user()
{
    (_curr_usr == _CASUAL) ? printf("Casual\n") : printf("Superuser\n");
    return _OK;
}

int _switch_users(users currentUser,char* password)
{
    if(currentUser == _CASUAL)
    {
        if(!password) return _INVALID_ARGUMENTS;
        if(strcmp(password,_curr_pass)==0)
        {
            _curr_usr = _SUPERUSER;
            return _OK;
        }
        else
        {
            return _WRONG_PASSWORD;
        }
    }
    else
    {
        _curr_usr = _CASUAL;
        return _OK;
    }
}

int _change_pass(users currentUser, char* newPassword)
{
    if(currentUser == _CASUAL) return _PERMISSION_DENIED;

    if(strlen(newPassword) >= 127) return _TOO_LONG;

    strcpy(_curr_pass,newPassword);
    printf("The password has been changed successfully!\n");
    return _OK;
}

//command executer
int _exec(char** splt,int i, node* cwd, char* command)
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
    else if(strcmp(splt[0], "move")==0)
    {
        if(i != 3) 
        {
            printf("Bad Usage! The right way is: move <nodePathorNameToBeMoved> <DestinationPath>\n");
            return _INVALID_ARGUMENTS;
        }

        char* source = splt[1];
        char* target = splt[2];

        if(!source || !target || source[0] == '\0' || target[0] == '\0')
        {
            printf("Source or destination cannot be empty!\n");
            return _INVALID_ARGUMENTS;
        }

        int status = _move(source,target,cwd);

        if(status == _NOT_FOUND)
            printf("File or directory couldn't be found!\n");
        else if(status == _NOT_A_DIRECTORY)
            printf("Destination is not a directory!\n");

        return status;
    }
    else if(strcmp(splt[0],"mkdir")==0)
    {
        if(i != 2)
        {
            printf("Bad Usage! The right way is: mkdir dirName\n");
            return _INVALID_ARGUMENTS;
        }
        return _mkdir(cwd,splt[1],_curr_usr);
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
    else if(strcmp(splt[0],"change")==0)
    {
        if(i != 2) return _INVALID_ARGUMENTS;
        char* newpass = splt[1];

        return _change_pass(_curr_usr,newpass);
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

        return _rm(cwd, target_name, m,_curr_usr);
    }
    else if(strcmp(splt[0],"uprint")==0)
    {
        if(i!=1) return _INVALID_ARGUMENTS;

        return _print_user();
    }
    else if(strcmp(splt[0],"switch")==0)
    {
        if (_curr_usr == _CASUAL)
        {
            if (i != 2) return _INVALID_ARGUMENTS;
            return _switch_users(_curr_usr, splt[1]);
        }
        else
        {
            return _switch_users(_curr_usr, NULL);
        }
    }
    else if(strcmp(splt[0],"touch")==0)
    {
        if(i != 2)
        {
            printf("Bad Usage! The right way is: touch fileName\n");
            return _INVALID_ARGUMENTS;
        }
        return _touch(cwd,splt[1],_curr_usr);
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
    else if(strcmp(splt[0], "help")==0)
    {
        if(i != 1) return _INVALID_ARGUMENTS;
        return _help();
    }
    else if(strcmp(splt[0], "insert")==0)
    {
        if(i < 4) 
        {
            printf("Bad Usage! The right way is: insert >/>> <fileName> #<content>\n");
            return _INVALID_ARGUMENTS;
        }
        if(!splt[3]) return _INVALID_ARGUMENTS;
        char hash = '#';

        if(splt[3][0] != hash)
        {
            printf("Bad Usage! The right way is: insert >/>> <fileName> #<content>\n");
            return _INVALID_ARGUMENTS;
        }
        char* option = splt[1];
        char*  content;
        char* name = splt[2];
        command[strlen(command) -1] = '\0';

        content = _parse_command_for_insert(command);

        return _insert(content,name,cwd,option,_curr_usr);
    }
    else if(strcmp(splt[0], "print!") == 0)
    {
        if(i != 2) return _INVALID_ARGUMENTS;
        char* name = splt[1];
        return _print(cwd,name,_curr_usr);
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

    while(true)
    {
        char* abspath = _get_absolute_path(_curr_dir);
        printf("%s$", abspath);
        free(abspath);
        fgets(command,2048,stdin);
        char q[2048];
        strcpy(q,command);
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
        int STATUS = _exec(splitted_code,argc,_curr_dir, q);
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
            case _PERMISSION_DENIED:
                printf("You don't have the permission to make this action!\n");
                break;
            case _WRONG_PASSWORD:
                printf("Wrong password!\n");
                break;
        }

        splitted_code[i] = NULL;
        
    }
    _free_node(_root);
    return 0;
}
