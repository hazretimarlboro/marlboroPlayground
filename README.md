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
