# mm — Custom Memory Manager

A lightweight memory allocator written in C, implementing `malloc`/`free`-like functionality on top of raw memory pages. It manages a heap structured as a doubly-linked list of blocks, each carrying a header and footer for coalescing and boundary checking.

---

## Overview

`mm` provides a simple API to allocate and free memory without relying on the standard library's `malloc`. It operates on a fixed-size heap composed of pages requested from the kernel at initialization. Blocks are tracked with metadata (size + flags) stored in headers and footers, enabling efficient coalescing of adjacent free blocks.

---

## API

### Initialization

```c
void minit(int sizeOfHeapByPages);
```

Initializes the heap. Must be called before any allocation. The argument specifies the heap size in pages. If omitted or called after allocation has already started, the heap defaults to 1 page.

---

### Allocation

```c
void* mgift(int64_t length);
```

Allocates a block of at least `length` bytes and returns a pointer to the usable memory region. Returns `NULL` if no suitable block is found and no new page can be obtained.

---

### Deallocation

```c
void mthrow(void* blockPtr);
```

Frees the memory block associated with the given pointer. Performs coalescing with adjacent free blocks where possible.

---

### Debugging

```c
int printHeap();
```

Prints a representation of the current heap state (all blocks, their sizes, and their allocation status). Returns the total number of blocks in the heap.

---

## Internal Utilities
These functions are exposed in `mm.h` and used internally by the allocator. They are not intended to be called directly in most use cases.

| Function | Description |
|---|---|
| `getBlockSize(headerPtr)` | Returns the size stored in a block's header |
| `getBlockFlags(headerPtr)` | Returns the flags (e.g. allocated bit) from a header |
| `getBlockFooter(headerPtr)` | Returns a pointer to the block's footer |
| `getNextBlock(blockPtr)` | Returns a pointer to the next block's header |
| `getPreviousBlock(blockPtr)` | Returns a pointer to the previous block's header |
| `modifyBlockHeader(blockPtr, header)` | Writes a new value to the block's header |
| `modifyBlockFooter(blockPtr, footer)` | Writes a new value to the block's footer |
| `modifyBlockMetaData(blockPtr, header)` | Updates both header and footer (size must not change) |
| `modifyMetadatas(blockPtr, header)` | Writes a value at the given address (avoids manual casting) |
| `createNewPage(size)` | Requests a new page from the kernel and registers it |
| `notInHeap(blockPtr)` | Returns non-zero if the pointer is outside the managed heap |
| `getPagePlace(pagePtr)` | Returns the index of a page in the pages array, or -1 if not found |

---

## Usage Example

```c
#include "mm.h"
#include <stdio.h>
#include <string.h>

int main() {
    minit(3); // Initialize heap with 3 pages

    char* p = mgift(80);
    strcpy(p, "Hello, world!");
    printf("%s\n", p);

    mthrow(p); // Free the block

    printHeap(); // Inspect heap state
    return 0;
}
```

---

## Building

No external dependencies are required beyond a standard C compiler.

```bash
gcc main.c mm.c -o mm_demo
./mm_demo
```

Adjust the compilation command to match the actual source file names in your project.

---

## Design Notes

- The heap is divided into contiguous blocks, each with an 8-byte header and an 8-byte footer storing the block size and allocation flag.
- `mgift` uses a first-fit strategy to find a free block large enough to satisfy the request.
- `mthrow` coalesces with neighboring free blocks (left, right, or both) to reduce fragmentation.
- If no free block is large enough, `createNewPage` extends the heap by requesting additional memory from the kernel.
- The `pages` array tracks all pages allocated from the kernel for bounds checking via `notInHeap` and `getPagePlace`.
