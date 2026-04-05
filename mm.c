#include <stdio.h>
#include <stdint.h>
#include <unistd.h>
#include <sys/mman.h>


typedef struct{
    void* pagePointer;
    int64_t size;
}Page;

Page pages[100];
int pageNumber = 0;
void* heapBase = NULL;
void* heapEnd = NULL;
char memInitialized = 0;


//=======UTILS FUNCTIONS=============

void* getBlockFooter(void* headerPtr){
    return (char*)headerPtr + ((*(int64_t*)headerPtr) & ~0b111) - 8;
}

void* getNextBlock(void* blockPtr){
    return (char*)blockPtr + ((*(int64_t*)blockPtr) & ~0b111);
}

int64_t getBlockFlags(void* headerPtr){
    return (*(int64_t*)headerPtr) & 0b111;
}

int64_t getBlockSize(void* headerPtr){
    return (*(int64_t*)headerPtr) & ~0b111;
}

void modifyBlockMetaData(void* blockPtr, int64_t header){
    *(int64_t*)getBlockFooter(blockPtr) = header;
    *(int64_t*)blockPtr = header;
}

void modifyBlockFooter(void* blockPtr, int64_t footer){
    *(int64_t*)getBlockFooter(blockPtr) = footer;
}

void modifyMetadatas(void* blockPtr, int64_t header){
    *(int64_t*)blockPtr = header;
}

void modifyBlockHeader(void* blockPtr, int64_t header){
    *(int64_t*)blockPtr = header;
}

void* getPreviousBlock(void* blockPtr){
    return (char*)blockPtr-(*(int64_t*)((char*)blockPtr-8) & ~0b111);
}

void* createNewPage(int64_t size){
        void* newPage = mmap(NULL, size, PROT_READ|PROT_WRITE, MAP_PRIVATE|MAP_ANONYMOUS, -1, 0);
        if(newPage == MAP_FAILED) return NULL;
        pages[pageNumber].pagePointer = newPage;
        pages[pageNumber].size = size;
        pageNumber++;
        return newPage;
}

int notInHeap(void* blockPtr){
    if((char*)blockPtr >= (char*)heapEnd || (char*)blockPtr < (char*)heapBase){
        return 1;
    }
    return 0;
}

int getPagePlace(void* pagePtr){
    for(int i=0;i<pageNumber;i++){
        if(pagePtr == pages[i].pagePointer){
            return i;
        }
    }
    return -1;
}

int expandHeap(int sizeInPages){
    int64_t size = sizeInPages * getpagesize();
    void* newPage = mmap(heapEnd, size, PROT_READ|PROT_WRITE, MAP_PRIVATE|MAP_ANONYMOUS|MAP_FIXED_NOREPLACE, -1, 0);

    if(newPage == MAP_FAILED){
        return 0;
    }

    if(newPage != heapEnd){
        munmap(newPage, size);
        return 0;
    }

    void* lastBlock = getPreviousBlock(heapEnd);

    if(!getBlockFlags(lastBlock)){
        int64_t newSize = getBlockSize(lastBlock) + size;
        modifyBlockHeader(lastBlock, newSize);
        modifyBlockFooter(lastBlock, newSize);
    } else {
        modifyBlockHeader(heapEnd, size);
        modifyBlockFooter(heapEnd, size);
    }

    heapEnd = (char*)heapEnd + size;
    return 1;
}
//================CORE FUNCTIONS===========

void minit(int sizeOfHeapByPages){ // inits a heap of size (sizeOfHeapByPages*OSPageSize)
    int heapSize = getpagesize() * sizeOfHeapByPages;
    heapBase = mmap(NULL, heapSize, PROT_READ|PROT_WRITE, MAP_PRIVATE|MAP_ANONYMOUS, -1, 0);
    int64_t header = heapSize;
    *(int64_t*)heapBase = header;
    *(int64_t*)getBlockFooter(heapBase) = header;
    heapEnd = getBlockFooter(heapBase)+8;
    memInitialized = 1;
}

void* mgift(int64_t length){ // allocates you memory : on the heap if there is space for or on a new page.
    if(!memInitialized){
        minit(1);
    }
    length = (length+7)/8*8+16; // calculates the real amount of space needed

    void* currentBlock = heapBase;
    while(currentBlock<heapEnd){
        if(!getBlockFlags(currentBlock) && getBlockSize(currentBlock) >= length){
            break;
        }
        currentBlock = getNextBlock(currentBlock);
    }

    if(currentBlock==heapEnd){
        if(!expandHeap(1)){
            printf("Not enough space : create a new page of length %ld\n", length-16);
            void* newPage = createNewPage(length);
            return newPage;
        }
    }
    int64_t size = getBlockSize(currentBlock);
    
    //Create new block
    if(size-length >= 16){
        //printf("New block creation\n");
        void* newBlockPtr = (char*)currentBlock + length;
        int64_t newBlockSize = size-length;
        modifyMetadatas(newBlockPtr, newBlockSize); // creates header for the new block
        modifyBlockFooter(newBlockPtr, newBlockSize); // creates footer for the new block
        size = length;
    }
    int64_t flags = 0b001;
    modifyBlockHeader(currentBlock, size|flags);
    modifyBlockFooter(currentBlock, size|flags);
    return (char*)currentBlock+8;
}

void mthrow(void* blockPtr){ // frees the memory area represented by the address of the pointer. If it is in the heap, it will consolidate the memory block with the adjacent ones,
                             // and if it is not in the heap, it will free the entire page.
    if(!memInitialized){
        minit(1);
    }
    if(notInHeap(blockPtr)){
        int pagePlace = getPagePlace(blockPtr);
        if(pagePlace == -1){ // page not found
            printf("Trying to free a unindentified memory zone\n");
            return;
        }
        int idx = getPagePlace(blockPtr);
        int64_t size = pages[idx].size;
        //printf("Freeing page of size %d\n", pages[getPagePlace(blockPtr)].size);
        while(pagePlace<pageNumber){
            pages[pagePlace] = pages[pagePlace+1];
            pagePlace++;
        }
        pageNumber--;
        munmap(blockPtr, size); // if not in heap, free the entire memory area
        return;
    }
    blockPtr = (char*)blockPtr - 8; // goes to the header



    if(!getBlockFlags(blockPtr)){
        printf("Trying to free an already freed block !\n");
        return;
    }
    
    modifyBlockMetaData(blockPtr, getBlockSize(blockPtr));


    //tries to consolidate the current and next block
    if(getNextBlock(blockPtr)<heapEnd && !getBlockFlags(getNextBlock(blockPtr))){
        int64_t newBlockSize = getBlockSize(blockPtr) + getBlockSize(getNextBlock(blockPtr));
        modifyBlockHeader(blockPtr, newBlockSize);
        modifyBlockFooter(blockPtr, newBlockSize);
    }
    
    //tries to consolidate the current and previous block
    if(blockPtr > heapBase && !getBlockFlags(getPreviousBlock(blockPtr))){
        int64_t newBlockSize = getBlockSize(blockPtr) + getBlockSize(getPreviousBlock(blockPtr));
        modifyBlockHeader(getPreviousBlock(blockPtr), newBlockSize);
        modifyBlockFooter(getPreviousBlock(blockPtr), newBlockSize);
    }
    return;
}
    




int printHeap(){ // prints heap and the other pages and return the number of blocks in the heap
    if(!memInitialized){
        minit(1);
    }
    void* currentBlock = heapBase;
    int64_t size = 0;
    int64_t flags = 0;
    int i = 0;
    printf("=======HEAP STATE========\n");
    while(currentBlock < heapEnd){
        flags = getBlockFlags(currentBlock);
        size = getBlockSize(currentBlock);
        printf("Block n°%d : Address=%p - Size=%ld - Flags=%ld (%s)\n", i, currentBlock, size, flags, (flags ? "taken" : "free"));
        i++;
        currentBlock = getNextBlock(currentBlock);
    }
    printf("=========================\n\n");
    int numberOfBlocksInHeap = i;
    printf("=====STANDALONE PAGES====\n");
    for(i=0;i<pageNumber;i++){
        printf("Standalone page at address=%p and size %ld\n", pages[i].pagePointer, pages[i].size);
    }
    printf("=========================\n\n");
    return numberOfBlocksInHeap;
}


        

