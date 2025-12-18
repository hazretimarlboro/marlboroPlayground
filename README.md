````markdown
# marlboroPlayground

**marlboroPlayground** is a lightweight, in-memory file system simulator written in C. It allows you to create directories and files, navigate the file system, and manage content safely, all without touching your real file system. Perfect for learning file system concepts or testing basic shell commands in a controlled environment.

---

## Features

- Navigate directories using `cd` (relative and absolute paths supported)  
- List files and directories with `ls`  
- Create directories with `mkdir`  
- Create empty files with `touch`  
- Remove files or directories with `rm` (with confirmation prompt)  
- Prevent deletion of the root or current working directory  
- Calculate memory usage of directories and files recursively  

---

## Commands

| Command      | Description |
| ------------ | ----------- |
| `ls`         | List all files and directories in the current directory. Directories are prefixed with `>`, files with `-`. |
| `mkdir NAME` | Create a new directory with the given `NAME`. |
| `touch NAME` | Create a new empty file with the given `NAME`. |
| `cd NAME`    | Change the current directory to `NAME`. Use `..` to go up one level. Absolute paths starting with `/` are supported. |
| `rm NAME`    | Remove the file or directory named `NAME`. Prompts for confirmation (`yes` or `no`). Cannot remove the current working directory or root. |

---

## Usage

1. Compile the program:

```bash
gcc -o marlboroPlayground main.c
````

2. Run the shell:

```bash
./marlboroPlayground
```

3. Interact with your in-memory file system:

```bash
$ mkdir projects
$ cd projects
$ touch readme.txt
$ ls
-readme.txt
$ cd ..
$ rm projects
Are you sure you want to delete projects? (yes/no): yes
```

---

## Notes

* Illegal characters are not allowed in file or directory names. Allowed characters: `A-Z a-z 0-9 _ - .`
* Memory is managed entirely in-memory, and all changes are lost when the program exits.
* Attempting to delete the root or the current working directory will result in an error.
* Recursive deletion is handled for directories.

---

## License

This project is open-source and free to use under the MIT License.

```
```
