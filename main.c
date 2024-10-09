#include <stdio.h>
#include <unistd.h>
#define MAGIC_NUMBER 0x0123456789ABCDEFL

typedef struct HEADER_TAG {
    struct HEADER_TAG * ptr_next; /* pointe sur le prochain bloc libre */
    size_t block_size; /* taille du memory bloc en octets*/
    long magic_number; /* 0x0123456789ABCDEFL */
} HEADER;

HEADER *freeBlocksList= NULL;

void * malloc_3is(size_t size) {
    HEADER *prev = NULL;
    HEADER *current = freeBlocksList;

    while (current != NULL) {
        if (current->block_size >= size) {
            if (prev != NULL) {
                prev->ptr_next = current->ptr_next;
            } else {
                freeBlocksList = current->ptr_next;
            }
            current->ptr_next = NULL;
            return (void*)(current + 1);
        }
        prev = current;
        current = current->ptr_next;
    }

    HEADER* new_block = (HEADER *) sbrk(size+sizeof(HEADER)+2*sizeof(long));
    new_block->block_size=size;
    new_block->ptr_next=NULL;
    new_block->magic_number=MAGIC_NUMBER;
    void* start_data = (void *) (new_block + 1);
    long* end_data = (long *) (start_data + size);
    *end_data = MAGIC_NUMBER; // initialise a second magic number at the end of data
    return (HEADER*) new_block + 1; // return the address of the future data
}

int check_3is(HEADER *block) {
    //if((block->ptr_next != NULL) && (block->magic_number != (block->ptr_next)->magic_number)){

    void* start_data = (void *) (block + 1);
    long* end_data = (long *) (start_data + block->block_size);

    int startSegFault = (block->magic_number != MAGIC_NUMBER);
    int endSegFault = (*end_data != MAGIC_NUMBER);

    if (startSegFault | endSegFault){
        if (startSegFault){
            printf("Erreur : corruption mémoire détectée (DEBUT)\n");
        }
        if (endSegFault){
            printf("Erreur : corruption mémoire détectée (FIN) \n");
        }
        return -1;
    }

    return 0;
}



void free_3is(void *ptr) {
    HEADER * block = (HEADER *) ptr -1;
    if (block==NULL) {
        return;
    }
    int test = check_3is(block);
    block->ptr_next=freeBlocksList;
    freeBlocksList=block;
}



int main(void) {
    void* stringTest = malloc_3is(20);
    HEADER * debugBlock = (HEADER *) stringTest -1;
    sprintf((char *) stringTest, "Hello world !\n\0");
    //sprintf((char *) stringTest-1, "Hello world !\n\0"); //Testing "start" seg faults


    printf("%s", stringTest);
    free_3is(stringTest);

    return 0;
}
