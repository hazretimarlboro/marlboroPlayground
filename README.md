````markdown
# 🗂️ In-Memory Virtual File System (C)

This project implements a **Unix-like virtual file system entirely in memory**, written in **C**, with its own command interpreter, directory tree, permission model, and file content handling.

It is **not backed by the real OS filesystem** — everything exists only during program execution.

---

## ✨ Features

- Hierarchical **directory tree** (`/` root, unlimited depth)
- **Files and directories** as nodes
- **Casual vs Superuser** permission model
- File content insertion (overwrite & append)
- Recursive directory size calculation
- Absolute and relative path handling
- Interactive shell with familiar commands
- Safe memory cleanup (recursive free)

---

## 🧠 Core Concepts

### Node-Based Filesystem
Every file or directory is represented by a `node`:

```c
typedef struct node
{
    char name[32];
    struct node* sibling;
    struct node* parent;
    struct node* children;

    node_types type;   // _DIR or _FILE
    size_t     size;   // cumulative size
    uint8_t*   data;   // file content
    users      creator;
} node;
````

* **Directories** store children
* **Files** store data
* `sibling` enables a linked-list per directory

---

## 👥 User Model

Two user types exist:

```c
typedef enum { _CASUAL, _SUPERUSER } users;
```

### Rules

* Casual users **cannot modify Superuser-owned nodes**
* Superuser can access everything
* Password-protected elevation (`switch` command)

---

## 🔐 Permission Logic

```c
bool _is_allowed(node* nodeToAccess, users user);
```

| Node Owner | Current User | Access |
| ---------- | ------------ | ------ |
| Casual     | Casual       | ✅      |
| Casual     | Superuser    | ✅      |
| Superuser  | Casual       | ❌      |
| Superuser  | Superuser    | ✅      |

---

## 📦 Directory Size Calculation

Directory size is computed **recursively**:

```c
size_t _get_size(node* nd);
```

* File size = content length
* Directory size = sum of all descendant sizes

---

## 🧭 Path Handling

* Absolute paths (`/dir/subdir/file`)
* Relative names (`file`, `dir`)
* Root path `/` supported

```c
node* _node_from_path(char* path);
char* _get_absolute_path(node* cwd);
```

---

## 🖥️ Interactive Shell

The program runs an interactive loop:

```bash
/$ mkdir test
/$ cd test
/test$ touch hello.txt
/test$ insert > hello.txt #Hello World
/test$ print! hello.txt
Hello World
```

Prompt always shows the **current absolute path**.

---

## 📜 Supported Commands

### Navigation

| Command    | Description             |
| ---------- | ----------------------- |
| `ls`       | List directory contents |
| `cd <dir>` | Change directory        |
| `cd ..`    | Go up                   |
| `pwd`      | (Implicit via prompt)   |

### Files & Directories

| Command             | Description              |
| ------------------- | ------------------------ |
| `mkdir <name>`      | Create directory         |
| `touch <name>`      | Create file              |
| `rm <name>`         | Remove file or directory |
| `rm -f <name>`      | Force remove             |
| `move <src> <dest>` | Move node                |

### File Content

| Command                | Description        |
| ---------------------- | ------------------ |
| `insert > file #text`  | Overwrite file     |
| `insert >> file #text` | Append             |
| `print! <file>`        | Print file content |

### User Management

| Command             | Description               |
| ------------------- | ------------------------- |
| `uprint`            | Print current user        |
| `switch <password>` | Become superuser          |
| `switch`            | Return to casual          |
| `change <newpass>`  | Change superuser password |

### Utility

| Command | Description       |
| ------- | ----------------- |
| `clear` | Clear screen      |
| `help`  | Show command list |
| `exit`  | Exit program      |

---

## ⚠️ Error Handling

All operations return an `error_t` enum:

```c
typedef enum {
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
```

Errors are centrally handled in `main()` with clear user feedback.

---

## 🧹 Memory Management

* All nodes allocated via `malloc`
* Recursive cleanup using:

```c
void _free_node(node* n);
```

* Ensures no memory leaks on exit

---

## 🚀 Build & Run

```bash
gcc -Wall -Wextra -O2 filesystem.c -o vfs
./vfs
```

(No external dependencies)

---

## 🧪 Limitations

* In-memory only (data lost on exit)
* No symbolic links
* Single-threaded
* No disk persistence (by design)

---

## 🧩 Possible Extensions

* Persistent storage (serialize tree)
* File permissions (r/w/x flags)
* Journaling
* Custom allocator integration
* Command history

---

## 📌 Summary

This project demonstrates:

* Tree-based data structures
* Recursive algorithms
* Manual memory management
* Command parsing
* Permission systems
* Unix-style filesystem logic

A **solid systems-programming exercise** that mirrors real OS concepts without OS complexity.

```


```
