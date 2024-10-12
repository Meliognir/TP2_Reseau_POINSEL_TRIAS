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



HEADER * divideBlock_3is(HEADER* block, size_t size){

    char* start_data = (char *) (block + 1);
    long* end_data = (long *) (start_data + size);

    HEADER* block2 = (void *) (end_data + 1);
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

    char* start_data = (char*) (block + 1);
    long* end_data = (long *) (start_data + block->block_size);

    int startSegFault = (block->magic_number != MAGIC_NUMBER);
    int endSegFault = (*end_data != MAGIC_NUMBER);

    if (startSegFault | endSegFault){
        if (startSegFault){
            printf("Erreur : corruption mémoire détectée (DEBUT)\n");
        }
        if (endSegFault){
            printf("Erreur : corruption mémoire détectée (FIN)\n");
        }
        return -1;
    }
    printf("Le bloc a été libéré avec succès !\n");
    return 0;
}



HEADER * fuseBlocks_3is(HEADER* block, HEADER* block2) { //  a function called to fuse two adjacent blocks in memory
    if (block2 == NULL){
        return block;
    }
    block->block_size = block->block_size + sizeof(long) + sizeof(HEADER) + block2->block_size;
    block->ptr_next = block2->ptr_next;
    return block;
}



int free_3is(void *ptr) {
    HEADER * block = (HEADER *) ptr -1;
    if (block==NULL) {
        printf("Aucun bloc n'a été libéré par l'appel à free_3is");
        return 0;
    }

    /* NOTE : We need to make sure the freeBlockList is sorted by block adress
    because we need to know if blocks are adjacent in memory in order to decide
    if we must fuse two blocks. */

    /* Insert the block in the list according to its adress */

    if (freeBlocksList == NULL){ // Case of an empty list
        /* Initialise freeBlocksList with "block" */
        block->ptr_next=NULL;
        freeBlocksList=block;
    }
    else if (block < freeBlocksList){ // Case where "block" comes first in memory before freeBlocksList
        /* "block" becomes the new head of the list */
        block->ptr_next=freeBlocksList;
        freeBlocksList=block;
        if (block->ptr_next == (HEADER*)((long*)((char*) (block + 1) + block->block_size)+ 1)) { //check if "block" and "block->ptr_next" are adjacent
            fuseBlocks_3is(block, block->ptr_next);
        }
    }
    else {
        /* Find where to insert the block */

        /* NOTE : Inserting the block requiring keeping track of the "prevblock"
        (after which "block" is inserted) to check if fusions of blocks are possible*/
        HEADER* prevBlock = freeBlocksList;

        while ((prevBlock->ptr_next != NULL)){
            if (block < prevBlock->ptr_next){
                break;
            }
            prevBlock = prevBlock->ptr_next;
        }

        /* Insert the block between previous and current blocks */
        block->ptr_next = prevBlock->ptr_next;
        prevBlock->ptr_next = block;
        if (block->ptr_next == (HEADER*)((long*)((char*) (block + 1) + block->block_size)+ 1)) { //check if "block" and "block->ptr_next" are adjacent
            fuseBlocks_3is(block, block->ptr_next);
        }
        if (block == (HEADER*)((long*)((char*) (prevBlock + 1) + prevBlock->block_size)+ 1)) { //check if "prevBlock" and "block" are adjacent
            fuseBlocks_3is(prevBlock, block);
        }
    }

    int testSegFaults = check_3is(block);
    if (testSegFaults){
        /* fix the magic numbers for future allocation */
        block->magic_number = MAGIC_NUMBER;
        void* start_data = (void *) (block + 1);
        long* end_data = (long *) (start_data + block->block_size);
        *end_data = MAGIC_NUMBER;
        return 1;
    }

    return 0;
}



void printList(/* HEADER* blockList */){
    if (/* blockList */freeBlocksList == NULL) printf("La liste de blocs est vide.\n\n");
    else if (freeBlocksList->ptr_next == NULL){
        printf("La liste contient un élément :\n");
        printf("(adresse : %p, espace : %lu o)\n\n", /* blockList */freeBlocksList, /* blockList */freeBlocksList->block_size);
    }
    else{
        printf("La liste contient les éléments suivants :\n");
        HEADER* element = /* blockList */freeBlocksList;
        printf("(adresse : %p, espace : %lu o)", element, element->block_size);
        element = element->ptr_next;
        while (element != NULL){
            printf("\n-> (adresse : %p, espace : %lu o)", element, element->block_size);
            element = element->ptr_next;
        }
        printf("\n\n");
    }
}



int main(void) {

    /*-------------------------------------------------------------------*/
    printf("--------------   Preallocation of memory   -----------------");
    /*-------------------------------------------------------------------*/
    printf("\n\n");

    /* We want to limit our calls to sbrk() as much as possible. For this,
    we can initially allocate 1500 bytes of memory and free it, so we can easily
    reuse it afterward */

/*
    char* msg_prealloc = "Preallocation of 1500 bytes of memory...\n";
    void* preallocation = malloc_3is(1500, ALLOW_REUSE);
    strcpy((char *) preallocation, msg_prealloc);

    printf("%s", (char *) preallocation);
    free_3is(preallocation);
*/

    printf("Décommenter le code (lignes 224 à 231) pour effectuer la préallocation.\nCependant, les tests suivant n'auront pas de sens si de la mémoire a été préallouée.\n");
    printf("\n\n");


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
    free_3is(block1); //free the block and check for seg faults

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

    /* NOTE : For the next steps, we need to not let the blocks fuse after they have been freed */
    message = "Création d'un bloc de séparation pour empêcher la fusion\ndes prochains blocs...\n";
    void* separatorBlock = malloc_3is(100, FORBID_REUSE);
    HEADER * debugseparatorBlock = (HEADER *) separatorBlock -1;
    strcpy((char *) separatorBlock, message);
    printf("%s", (char *) separatorBlock);
    printf("(Ce séparateur sera libéré à la fin du programme.)\n\n");

    message = "Afin de le réutiliser plus tard, allocation d'un bloc\nde 1000 octets puis libération de ce même bloc...\n";
    void* block2 = malloc_3is(1000, ALLOW_REUSE);
    HEADER * debugBlock2 = (HEADER *) block2 -1;
    strcpy((char *) block2, message);
    printf("%s", (char *) block2);
    free_3is(block2);

    printList();

    /* Build a new block smaller than 1000 bytes in order to divide and use one of the free blocks available : */
    message = "Création d'un second bloc de taille 1000 octets afin\nde tester la réutilisation du grand block2 disponible dans la liste...\n";
    void* block3 = malloc_3is(1000, ALLOW_REUSE);
    HEADER * debugBlock3 = (HEADER *) block3 -1;
    strcpy((char *) block3, message);
    printf("%s", (char *) block3);

    /* Display again the number of free blocks after allocating memory for block3 : */
    printf("Le block2 de 1000 octets a été réutilisé entièrement pour créer le block3.\n\n");
    printList();

    printf("Libération du block3...\n");
    free_3is(block3); 

    printf("\n\n");


    /*-------------------------------------------------------------------*/
    printf("---------------   Subdividing free blocks   ----------------");
    /*-------------------------------------------------------------------*/
    printf("\n\n");


    /* Display the number of free blocks before allocating memory for a new SMALLER block : */
    printf("Voici l'état des blocs libres avant réutilisation du block3 :\n");
    printList();

    /* Build a new block smaller than 1000 bytes in order to divide and use one of the free blocks available : */
    message = "Création d'un bloc de taille 600 octets afin\nde tester la division du grand block2 disponible dans la liste...\n";
    void* block4 = malloc_3is(600, ALLOW_REUSE);
    HEADER * debugBlock4 = (HEADER *) block4 -1;
    strcpy((char *) block4, message);
    printf("%s", (char *) block4);

    /* Display again the number of free blocks after creating the SMALLER block : */
    printf("Un morceau de 400 octets du block3 est resté libre dans la liste.\n\n");
    printList();

    printf("Libération du block4...\n");
    free_3is(block4); 
    printList();
    printf("\n\n");

    printf("Libération du bloc séparateur qui empêchait les fusions...\n");
    free_3is(separatorBlock);
    printList();
    printf("Tous les blocs ont pu être fusionnés.\n");

    return 0;
}
