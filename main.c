#include <stdio.h>
#include <unistd.h>
#include <string.h>
#define MAGIC_NUMBER 0x0123456789ABCDEFL
#define MAX_MESSAGE_SIZE 128
#define ALLOW_REUSE 1
#define FORBID_REUSE 0

typedef struct HEADER_TAG {
    struct HEADER_TAG * ptr_next; /* pointe sur le prochain bloc libre */
    size_t block_size; /* taille du memory bloc en octets*/
    long magic_number; /* 0x0123456789ABCDEFL */
} HEADER;


HEADER *freeBlocksList= NULL;

/* Find the length of the list */

int listLength(HEADER* head) {
    int length = 0;
    while (head != NULL) {
        length++;
        head = head->ptr_next;
    }
    return length;
}


HEADER * blocksFusion_3is(HEADER* block, HEADER* block2) { //  a function called to fuse two adjacent blocks in memory
    if (block2 == NULL){
        return block;
    }
    block->block_size = block->block_size + sizeof(long) + sizeof(HEADER) + block2->block_size;
    block->ptr_next = block2->ptr_next;
    return block;
}



HEADER * divideBlock_3is(HEADER* block, size_t size){

    void* start_data = (void *) (block + 1);
    long* end_data = (long *) (block + size);

    HEADER* block2 = (void *) (end_data + sizeof(long));
    block2->ptr_next = block->ptr_next;

    block2->block_size = block->block_size - size - sizeof(HEADER) - sizeof(long);
    block2->magic_number = MAGIC_NUMBER;

    block->ptr_next = block2;
    block->block_size = size;
    *end_data = MAGIC_NUMBER;

    return block;
}



void * malloc_3is(size_t size, int canReuse) {
    HEADER *prev = NULL;
    HEADER *current = freeBlocksList;

    if (canReuse){
        while (current != NULL) {

            if (current->block_size >= size) {
                
                if (current->block_size > size + sizeof(HEADER) + sizeof(long)){
                    current = divideBlock_3is(current, size);
                }

                if (prev != NULL) {
                    prev->ptr_next = current->ptr_next;
                } else {
                    freeBlocksList = current->ptr_next;
                }
                current->ptr_next = NULL;
                return (HEADER*)(current + 1);
            }
            prev = current;
            current = current->ptr_next;
        }
    }

    /* Case where all the blocks of the blocklist were not enough to match size : */

    HEADER* new_block = (HEADER *) sbrk(size +sizeof(HEADER)+sizeof(long));
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



int free_3is(void *ptr) {
    HEADER * block = (HEADER *) ptr -1;
    if (block==NULL) {
        printf("Aucun bloc n'a été libéré par l'appel à free_3is");
        return 0;
    }

    int testSegFault = check_3is(block);
    block->ptr_next=freeBlocksList;
    freeBlocksList=block;

    if (testSegFault){
        /* fix the magic numbers for future allocation */
        block->magic_number = MAGIC_NUMBER;
        void* start_data = (void *) (block + 1);
        long* end_data = (long *) (start_data + block->block_size);
        *end_data = MAGIC_NUMBER;
        return 1;
    }

    return 0;
}



int main(void) {

    /*-------------------------------------------------------------------*/
    printf("--------   Memory Allocation with our malloc_3is   ---------");
    /*-------------------------------------------------------------------*/
    printf("\n\n");


    char* message = "Hello world !\nUn bloc de mémoire est alloué pour ce message.\n";

    void* block1 = malloc_3is(strlen(message)+1, ALLOW_REUSE); // allocate length+1 to include the additional '\0' end character
    HEADER * debugBlock1 = (HEADER *) block1 -1;
    strcpy((char *) block1, message);

    printf("%s", (char *) block1);

    printf("\n\n");


    /*-------------------------------------------------------------------*/
    printf("-------------------   Freeing a block   --------------------");
    /*-------------------------------------------------------------------*/
    printf("\n\n");


    printf("Libération du block1...\n");
    int testSegFault = free_3is(block1); //free the block and check for seg faults
    if (!testSegFault){
        printf("Le bloc a été libéré avec succès !\n");
    }

    printf("\n\n");


    /*-------------------------------------------------------------------*/
    printf("--------------   Segmentation Fault tests   ----------------");
    /*-------------------------------------------------------------------*/
    printf("\n\n");


    /* NOTE :
    In this section, we cannot reuse old blocks when allocating memory
    because we need to create blocks matching the exact size of our data.
    If we don't know the exact size of the allocated block, it will be
    complicated to corrupt the magic number located AFTER the data.
    
    This is why we added a parameter "int canReuse" in malloc_3is(...) */


    /* Testing "start" seg faults : */
    message = " Corruption du code précédant la donnée...\n";

    void* segFault1 = malloc_3is(strlen(message)+1, FORBID_REUSE);
    strcpy((char *) segFault1-1, message);
    printf("%s", (char *) segFault1);
    free_3is(segFault1); //free the block and check for seg faults

    printf("\n\n");

    /* Testing "end" seg faults : */
    message = "Corruption du code suivant la donnée...\n";

    void* segFault2 = malloc_3is(strlen(message), FORBID_REUSE);
    strcpy((char *) segFault2, message);
    printf("%s", (char *) segFault2);
    free_3is(segFault2); //free the block and check for seg faults

    printf("\n\n");

    /* Testing both seg faults : */
    message = " Corruption du code précédant et suivant la donnée...\n";

    void* segFault3 = malloc_3is(strlen(message)-1, FORBID_REUSE);
    strcpy((char *) segFault3-1, message);
    printf("%s", (char *) segFault3);
    free_3is(segFault3); //free the block and check for seg faults

    printf("\n\n");


    /*-------------------------------------------------------------------*/
    printf("----------------   Reusing free blocks   -------------------");
    /*-------------------------------------------------------------------*/
    printf("\n\n");


    message = "Allocation d'un bloc de 1000 octets, puis libération de ce bloc...\n";

    void* block2 = malloc_3is(1000, ALLOW_REUSE); // allocate length+1 to include the additional '\0' end character
    HEADER * debugBlock2 = (HEADER *) block2 -1;
    strcpy((char *) block2, message);
    printf("%s", (char *) block2);
    free_3is(block2);

    /* Display the number of free blocks before allocating memory for a new block of 1000 bytes too : */
    printf("Il y a maintenant %i blocs différents dans la freeBlocksList.\n", listLength(freeBlocksList));

    /* Build a new block smaller than 1000 bytes in order to divide and use one of the free blocks available : */
    message = "Création d'un second bloc de taille 1000 octets afin\nde tester la réutilisation du grand block2 disponible dans la liste...\n";

    void* block3 = malloc_3is(1000, ALLOW_REUSE); // allocate length+1 to include the additional '\0' end character
    HEADER * debugBlock3 = (HEADER *) block3 -1;
    strcpy((char *) block3, message);
    printf("%s", (char *) block3);

    /* Display again the number of free blocks after allocating memory for block3 : */
    printf("Il y a maintenant %i blocs différents dans la freeBlocksList,\njuste après la création du block3 de taille 1000 octets.\n", listLength(freeBlocksList));
    printf("Ce nombre a normalement diminué d'un car le block2\nde 1000 octets a été réutilisé entièrement pour créer le block3.\n\n", listLength(freeBlocksList));

    printf("Libération du block3...\n");
    free_3is(block3); 

    printf("\n\n");


    /*-------------------------------------------------------------------*/
    printf("---------------   Subdividing free blocks   ----------------");
    /*-------------------------------------------------------------------*/
    printf("\n\n");


    /* Display the number of free blocks before allocating memory for a new SMALLER block : */
    printf("Il y a maintenant %i blocs différents dans la freeBlocksList.\n", listLength(freeBlocksList));

    /* Build a new block smaller than 1000 bytes in order to divide and use one of the free blocks available : */
    message = "Création d'un second bloc de taille 800 octets afin\nde tester la division du grand block2 disponible dans la liste...\n";

    void* block4 = malloc_3is(1000, ALLOW_REUSE); // allocate length+1 to include the additional '\0' end character
    HEADER * debugBlock4 = (HEADER *) block4 -1;
    strcpy((char *) block4, message);
    printf("%s", (char *) block4);

    /* Display again the number of free blocks after creating the SMALLER block : */
    printf("Il y a maintenant %i blocs différents dans la freeBlocksList,\njuste après la création du block4 de taille 800 octets.\n", listLength(freeBlocksList));
    printf("Ce nombre n'a normalement pas changé ici car un morceau de 200\noctets du block3 de 1000 est resté libre donc dans la liste.\n\n", listLength(freeBlocksList));

    printf("Libération du block4...\n");
    free_3is(block4); 

    printf("Il y a finalement %i blocs libres dans la liste à l'issue de l'exécution\n", listLength(freeBlocksList));

    printf("\n\n");


    return 0;
}
