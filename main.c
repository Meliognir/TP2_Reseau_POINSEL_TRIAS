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
    HEADER* new_bloc = (HEADER *) sbrk(size+sizeof(HEADER)+sizeof(long));
    new_bloc->bloc_size=size;
    new_bloc->ptr_next=NULL;
    new_bloc->magic_number=MAGIC_NUMBER;
    return &(new_bloc->bloc_size);
}

int check_3is(void *block) {
    HEADER * block2 = block-1;

    if(block2->magic_number!=(block2->ptr_next)->magic_number){
        printf("Erreur : corruption mémoire détectée \n)");
        return -1;
    }
    return 0;
}



void free_3is(HEADER *block) {
    if (block==NULL) {
        return;
    }
    int osef = check_3is(block);
    block->ptr_next=freeBlocksList;
    freeBlocksList=block;
}



int main(void) {
    void* stringTest = malloc_3is(3200);
    sprintf(stringTest, "Hello world !\0");


    printf("%s",stringTest);
    free_3is(stringTest);

    return 0;
}
