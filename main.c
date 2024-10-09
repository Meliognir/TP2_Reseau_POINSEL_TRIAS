#include <stdio.h>
#include <unistd.h>
#define MAGIC_NUMBER 0x0123456789ABCDEFL

typedef struct HEADER_TAG {
    struct HEADER_TAG * ptr_next; /* pointe sur le prochain bloc libre */
    size_t bloc_size; /* taille du memory bloc en octets*/
    long magic_number; /* 0x0123456789ABCDEFL */
} HEADER;

void * malloc_3is(size_t size) {
    HEADER* new_bloc = (HEADER *) sbrk(size + sizeof(HEADER));
    if (new_bloc== (void *)-1) {
        return NULL;
    }
    new_bloc->bloc_size=size;
    new_bloc->ptr_next=NULL;
    new_bloc->magic_number=MAGIC_NUMBER;
}



int main(void) {

    return 0;
}
