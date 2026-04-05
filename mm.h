#include <stdint.h>
void* getBlockFooter(void* headerPtr); // returns the address of the block's footer
void* getNextBlock(void* blockPtr); // returns the address of the next block header
int64_t getBlockFlags(void* headerPtr); // returns the block flags
int64_t getBlockSize(void* headerPtr); // returns the size of the block
void modifyBlockMetaData(void* blockPtr, int64_t header); // modify the header and the footer of the block THE SIZE MUST NOT CHANGE 
void modifyBlockFooter(void* blockPtr, int64_t footer); // modify the footer of the block
void modifyMetadatas(void* blockPtr, int64_t header); // modify the value in the address (mostly made to avoid writing the casts by hand)
void modifyBlockHeader(void* blockPtr, int64_t header);  // modify the block header
void* getPreviousBlock(void* blockPtr); // returns the address of the previous block header DOESN'T VERIFY IF IT'S OUT OF THE BOUNDS
void* createNewPage(int64_t size); // asks to the kernel a new page, set it in the pages array and returns it's address
int notInHeap(void* blockPtr); // returns if a pointer's address is in the heap
int getPagePlace(void* pagePtr); // takes a page pointer and returns it's place in the pages array (-1 if it is't a page pointer)
int expandHeap(int sizeInPages); //tries to expand the heap, returns 1 if it succeded, 0 else
void* mgift(int64_t length); // allocates you memory
void minit(int sizeOfHeapByPages); // inits the stack (call it by yourself BEFORE allocating any memory to specify the heap's size otherwise it will be set at 1) - equivalent of malloc()
void mthrow(void* blockPtr); // frees the memory area associated with the pointer
int printHeap(); // prints heap and return the number of blocks in the heap

