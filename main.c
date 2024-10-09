#include <stdio.h>
#include <unistd.h>
#define MAGIC_NUMBER 0x0123456789ABCDEFL

typedef struct HEADER_TAG {
    struct HEADER_TAG * ptr_next; /* pointe sur le prochain bloc libre */
    size_t bloc_size; /* taille du memory bloc en octets*/
    long magic_number; /* 0x0123456789ABCDEFL */
} HEADER;

HEADER *freeBlocksList= NULL;

void * malloc_3is(size_t size) {
    HEADER* new_bloc;
    new_bloc = sbrk(size + sizeof(HEADER) + sizeof(long));
    new_bloc->bloc_size=size;
    new_bloc->ptr_next=NULL;
    new_bloc->magic_number=MAGIC_NUMBER;
    long *end_magic = (long*)((void*)new_bloc + sizeof(HEADER) + size);
    *end_magic = MAGIC_NUMBER;
    return (HEADER *)new_bloc+1;
}

int check_3is(HEADER * block) {

    if(block->magic_number!=MAGIC_NUMBER){
        printf("Erreur : corruption mémoire détectée (début) \n");
        return -1;
    }
    long *end_magic = (long*)((void *)block + sizeof(HEADER) + block->bloc_size);
    if (*end_magic != MAGIC_NUMBER) {
        printf("Erreur : corruption mémoire détectée (fin) \n");
        return -1;
    }
    return 0;
}



void free_3is(void *ptr) {

    HEADER * block = (HEADER *)ptr-1;
    if (block==NULL) {
        return;
    }
    int test = check_3is(block);
    block->ptr_next=freeBlocksList;
    freeBlocksList=block;
}



int main(void) {
    void * stringTest = malloc_3is(1000000);
    sprintf((char*)stringTest, "Hello world ! \n\0");


    printf("%s",stringTest);
    free_3is(stringTest);

    return 0;
}
