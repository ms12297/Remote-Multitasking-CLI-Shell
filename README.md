# MyShell - Remote Multitasking CLI Shell & Hybrid Scheduling Simulation

This project implements a custom shell application (`myshell`) with extended functionality across four phases of development. By Phase 4, the project simulates a **hybrid scheduling algorithm** (Round Robin + Shortest Job First) to manage multiple client connections to the shell server. Key features include advanced command parsing, multithreaded client-server interaction, and hybrid scheduling for demo and program commands.

## Features

### Phase 1: Basic Shell Implementation
- **Command Parsing**:
  - Supports up to 3 pipes.
  - Handles input/output redirection (`<`, `>`).
  - Commands are tokenized for efficient processing.
- **Supported Commands**: Common UNIX utilities like `ls`, `cat`, `grep`, `sort`, etc.
- **Execution**: Single and piped commands are executed using child processes.

### Phase 2: Remote Access
- **Client-Server Architecture**:
  - The server (`myshell`) processes commands sent by the client (`myshell_cli`) over socket communication.
  - Supports remote command execution.
- **Extended Functionality**:
  - Added support for custom executable files.

### Phase 3: Multithreading
- **Parallelism**:
  - The server now supports multiple clients using threads.
  - Each client receives command outputs directly.
- **Thread Management**:
  - Tracks client requests with unique client IDs.

### Phase 4: Hybrid Scheduling
- **Scheduling Algorithm**:
  - A combination of Round Robin and Shortest Job First.
  - Quantum initialized at 3 and adjusted dynamically.
  - Preemption occurs for shorter burst time jobs.
- **Command Execution**:
  - Shell commands execute immediately.
  - Demo and program commands are added to a **waiting queue** for scheduling.
- **Semaphore Synchronization**:
  - `mutexRunning`, `sem_running`, and `sem_waitingQueue` ensure proper thread synchronization and scheduling.

## How to Run

### Requirements
- GCC compiler with pthread support.
- All source files (`myshell.c`, `myshell_cli.c`, `demo.c`, etc.) in the same directory as the `Makefile`.

### Compilation
Run the following command to compile the project:
```bash
make
```

### Execution
Start the server in one terminal:
```bash
./myshell
```

Start the client in another terminal:
```bash
./myshell_cli
```

### Cleanup
Run the following command to clean up generated files:
```bash
make clean
```
