````markdown
# MarlboroPlayground

Welcome to **MarlboroPlayground** — a playful in-memory virtual filesystem where you can create, explore, and manipulate directories and files, all in your terminal!  

Think of it as a tiny sandboxed operating system for experimenting with file management.

---

## 🚀 Features

- **Directory Management**
  - `mkdir <dirName>` — Create a new directory.
  - `ls` — List directory contents.
  - `cd <dirName>` — Navigate into a directory.
  - `cd ..` — Go up to the parent directory.

- **File Management**
  - `touch <fileName>` — Create an empty file.
  - `insert >/>> <fileName> "<content>"` — Overwrite(>) or append(>>) file content.
  - `print! <fileName>` — View the content of a file.

- **Deletion**
  - `rm [-f] <file/dirName>` — Delete a file or directory (with confirmation).
  - Works with **absolute** (`/path/to/file`) or **relative** paths.
  - Cannot delete the **current working directory** or **root**.
  - You can skip confirmation if you add -f before directory name

- **Extras**
  - `clear` — Clear the terminal.
  - `exit` — Exit the program and free all memory.

- **Filesystem Mechanics**
  - Tracks directory sizes recursively.
  - Validates file/directory names (no illegal characters, max 31 chars).
  - Supports a maximum path depth of 512 directories.

---

## 🖥 Playground Example

```text
/$ mkdir documents
/$ cd documents
/documents$ touch notes.txt
/documents$ insert > notes.txt "This is my first note"
/documents$ print! notes.txt
This is my first note
/documents$ ls
-notes.txt    (Size:18)
/documents$ cd ..
/$ ls
>documents    (Size:18)
/$ rm documents
Are you sure you want to delete documents? (yes/no): yes
/$ ls
(empty)
````

---

## 📂 Filesystem Diagram Example

```
/
└── documents
    ├── notes.txt (Size:18)
    └── images
        └── pic.png (Size:102)
```

* `>` indicates a directory.
* `-` indicates a file with its size in bytes.
* You can navigate, add, or remove nodes freely in this virtual playground.

---

## ⚡ Quick Start

### Requirements

* GCC (or any C compiler)
* Linux/macOS terminal (tested on Debian-based systems)

### Compile

```bash
gcc -o marlboroplayground main.c
```

### Run

```bash
./marlboroplayground
```

Start exploring! The prompt shows your current directory:

```text
/$
```

---

## 📝 Notes & Limitations

* **In-memory only**: All data is lost when you exit.
* Maximum file name: **31 characters**
* Maximum file content: **1023 characters**
* Maximum path depth: **512 directories**
* Supports basic ASCII-friendly characters: `a-z`, `A-Z`, `0-9`, `_`, `-`, `.`

---

## 🔧 Code Structure

* **Nodes** represent directories and files with:

  * `name`, `type`, `size`, `parent`, `children`, `sibling`, and `data` (for files)
* **Commands** are handled by `_exec()` with robust error checks.
* **Memory Management**: `_free_node()` recursively frees all allocated memory.
* **Paths** support both absolute and relative references.

---

## 🎉 Have Fun!

MarlboroPlayground is your personal sandbox—experiment with file operations, learn about filesystem structures, and practice CLI commands without touching your actual disk.

---

## 📜 License

Open-source and free to use for educational and experimental purposes.
