# Dorm Elevator

This project involves adding custom system calls to a linux kernel and developing two kernel modules: my_timer and elevator. My_timer module functions as a timer that retrieves and stores an elapsed time. Elevator module functions as an elevator that allows passengers to be generated on a building's floors and be serviced by an elevator. It utilizes various states for handling the scheduling of the elevator along with managing shared resource access.

## Group Members
- **Patric Nurczyk**: pjn20@fsu.edu
- **Nathan Wallen**: ndw21b@fsu.edu
- **Chris Canty**: cmc20k@fsu.edu
## Division of Labor

### Part 1: System Call Tracing
- **Responsibilities**: Need to Make 2 files (empty.c and part1.c). part1 will contain 4 system calls. Need to make and trace the 2 files to demonstrate system calls.
- **Assigned to**: Patric Nurczyk

### Part 2: Timer Kernel Module
- **Responsibilities**: Need to develop a my_timer module (my_timer.ko), will write the current time + time elapsed to a proc file.
- **Assigned to**: Chris Canty

### Part 3a: Adding System Calls
- **Responsibilities**: Adding start_elevator (548), issue_request (549), and stop_elevator (550)
- **Assigned to**: Patric Nurczyk

### Part 3b: Kernel Compilation
- **Responsibilities**: Compile the recent kernel (6.5.8), include the additional system calls.
- **Assigned to**: Patric Nurczyk, Chris Canty

### Part 3c: Threads
- **Responsibilities**: Implement kernel thread inside elevator for handling elevator movement.
- **Assigned to**: Chris Canty, Nathan Wallen

### Part 3d: Linked List
- **Responsibilities**: Implement linked lists for storing newly generated passengers on floors.
- **Assigned to**: Chris Canty

### Part 3e: Mutexes
- **Responsibilities**: Implement mutexes for safe handling for shared data resources between elevator and floors.  
- **Assigned to**: Patric Nurczyk

### Part 3f: Scheduling Algorithm
- **Responsibilities**: Design and implement a scheduling algorithm for loading and unloading passengers between elevator and floors. Handle requirements for FIFO boarding and weight/number of passengers limitations.
- **Assigned to**: Nathan Wallen, Chris Canty

## File Listing
```
elevator/
├── part1/
│   ├── empty.c
│   ├── empty.trace
│   ├── part1.c
│   ├── part1.trace
│   └── Makefile
├── part2/
│   ├── my_timer.c
│   └── Makefile
├── part3/
│   ├── src/
|   |   ├── elevator.c
|   |   └── Makefile
│   ├── tests/
|   |   ├── elevator-test/  
|   |   |   ├── consumer.c
|   |   |   ├── Makefile
|   |   |   ├── producer.c
|   |   |   ├── README.md
|   |   |   └── wrappers.h
|   |   └── system-calls-test/
|   |   |   ├── Makefile
|   |   |   ├── README.md
|   |   |   ├── syscheck.c
|   |   |   ├── test-syscalls.c
|   |   |   └── test-syscalls.h
│   └── syscalls.c
├── Makefile
└── README.md

```
# How to Compile & Execute

### Requirements
- **Compiler**: `gcc`

## Part 1

### Compilation
For empty.c and part1.c inside elevator/part1/:
```bash
make
```
This will build the executables in elevator/part1/

### Execution
To start empty program:
```bash
./empty
```
To start part1 program:
```bash
./part1
```

## Part 2

### Compilation
For my_timer.c in elevator/part2/:
```bash
make
```

To install the kernel module:
```bash
sudo insmod my_timer.ko
```

### Execution 

To view my_timer proc file:
```bash
cat /proc/my_timer
```

## Part 3

### Compilation
For elevator.c in elevator/part3/src/:
```bash
make
```

To install the kernel module:
```bash
sudo insmod elevator.ko
```

For producer.c + consumer.c in elevator/part3/tests/elevator-test/:
```bash
make
```
This will build the executables in elevator/part3/tests/elevator-test/

### Execution
To view proc file:
```bash
watch -n 1 cat /proc/elevator
```
To generate passengers:
```bash
./producer <Amount to generate>
```
To start the elevator module:
```bash
./consumer --start
```
To stop the elevator module:
```bash
./consumer --stop
```

## Bugs
- **N/A**

## Considerations
Passengers inside of elevator are handled by a static array rather than a linked list.
Linked lists are still used for handling passengers on floors.
