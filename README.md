# Heap Memory Management using Fibonacci Buddy System

This C program simulates a heap memory management system using the Fibonacci Buddy System. It allows dynamic memory allocation and deallocation by name and efficiently manages fragmentation through splitting and merging of memory blocks based on Fibonacci numbers.

## Features

- Initializes a simulated heap with blocks sized according to the Fibonacci sequence.
- Allocates memory using the closest fitting Fibonacci-sized block.
- Splits larger blocks into smaller Fibonacci-sized blocks when needed.
- Merges adjacent free blocks if they are consecutive Fibonacci numbers.
- Tracks memory usage by associating allocated blocks with variable names.
- Provides a command-line interface for interaction.

## How It Works

### Heap Structure

The heap is represented as a linked list of blocks, where each block contains:

- `size`: Size of the block (a Fibonacci number)
- `isFree`: A flag indicating whether the block is free or allocated
- `name`: The variable name assigned to the block (if allocated)
- `allocated_size`: The actual size requested by the user
- `next`: Pointer to the next block in the list

### Allocation

- The allocator searches for the best-fit block using the Fibonacci buddy system.
- If no suitable block is found, adjacent free blocks are merged and the search is retried.
- Larger blocks are split recursively into smaller Fibonacci blocks to closely match the requested size.

### Deallocation

- A memory block can be freed using the variable name.
- After deallocation, the system attempts to merge adjacent free blocks that are consecutive Fibonacci numbers.

## Menu Options

1. Allocate Memory
2. Free Memory
3. Display Heap Layout
0. Quit


