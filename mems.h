/*
All the main functions with respect to the MeMS are inplemented here
read the function discription for more details

NOTE: DO NOT CHANGE THE NAME OR SIGNATURE OF FUNCTIONS ALREADY PROVIDED
you are only allowed to implement the functions 
you can also make additional helper functions a you wish

REFER DOCUMENTATION FOR MORE DETAILS ON FUNSTIONS AND THEIR FUNCTIONALITY
*/
// add other headers as required
#include<stdio.h>
#include<stdlib.h>
#include<sys/mman.h>
#include<ctype.h>


/*
Use this macro where ever you need PAGE_SIZE.
As PAGESIZE can differ system to system we should have flexibility to modify this 
macro to make the output of all system same and conduct a fair evaluation. 
*/
#define PAGE_SIZE 4096

struct subChain{

    int holeOrProcessFlag;
    size_t size;
    int virtualAddress;
    struct subChain *next;
    struct subChain *prev;
};

struct mainChain{
    struct mainChain *next;
    struct subChain *subchain;
    int startingVirtualAddress;
    int endingVirtualAddress;
};
/*
Initializes all the required parameters for the MeMS system. The main parameters to be initialized are:
1. the head of the free list i.e. the pointer that points to the head of the free list
2. the starting MeMS virtual address from which the heap in our MeMS virtual address space will start.
3. any other global variable that you want for the MeMS implementation can be initialized here.
Input Parameter: Nothing
Returns: Nothing
*/


struct mainChain *head;
int baseVA;
void mems_init(){
    head=NULL;
    baseVA=1000; //VA=virtual address
}


/*
This function will be called at the end of the MeMS system and its main job is to unmap the 
allocated memory using the munmap system call.
Input Parameter: Nothing
Returns: Nothing
*/
void mems_finish(){

    struct mainChain* currentMain=head;
    while(currentMain!=NULL){
        //munmap(currentMain->subchain,(size_t)(currentMain->endingVirtualAddress-currentMain->startingVirtualAddress));
        if (munmap(currentMain->subchain,(size_t)(currentMain->endingVirtualAddress-currentMain->startingVirtualAddress)) == -1) {
            perror("munmap");
            exit(1);
        }
        struct mainChain* temp=currentMain;
        currentMain=currentMain->next;
        //munmap(temp,(size_t)(sizeof(struct mainChain)));

        if (munmap(temp,(size_t)(sizeof(struct mainChain)))== -1) {
            perror("munmap");
            exit(1);
        }
        
    }
    
    head=NULL;
    
}


/*
Allocates memory of the specified size by reusing a segment from the free list if 
a sufficiently large segment is available. 

Else, uses the mmap system call to allocate more memory on the heap and updates 
the free list accordingly.

Note that while mapping using mmap do not forget to reuse the unused space from mapping
by adding it to the free list.
Parameter: The size of the memory the user program wants
Returns: MeMS Virtual address (that is created by MeMS)
*/ 
void* mems_malloc(size_t size){

    //Initially the main chain also does not have any node, so taking care of that
    if(head==NULL){
        int sizeToAllocate=PAGE_SIZE;
        while(sizeToAllocate<size){
            sizeToAllocate+=PAGE_SIZE;
        }

        head=(struct mainChain*)(mmap(NULL,(sizeof(struct mainChain)), PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS , -1, 0));
        if (head == MAP_FAILED) {
            perror("mmap");
            exit(1);
        }
        //giving memory to head of the main chain using mmap
        void* allocatedMemory=(mmap(NULL,sizeToAllocate, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0));
        if (allocatedMemory == MAP_FAILED) {
            perror("mmap failed to allocate Memory\n");
            exit(1);
        }
        head->startingVirtualAddress=(baseVA);
        head->endingVirtualAddress=((baseVA)+sizeToAllocate);
        //printf("%d\n",head->endingVirtualAddress); //remove this line in the final code
        head->next=NULL;
        if(head->next==NULL){
            //printf("Fuck you\n");
        }
        struct subChain* node = (struct subChain*)((char*)allocatedMemory);

        //printf("head=%p\n",&head);
        
        node->prev=NULL;
        node->next=NULL;
        node->holeOrProcessFlag=1;
        
        //printf("Type of head is %s\n",type(head));
        node->size=size;
        node->virtualAddress=(baseVA);
        head->subchain=node;
        
        //printf("%d\n",(node->virtualAddress)+(int)size); //remove this line in the final code
        //printf("%d\n",((node->virtualAddress)+(int)size)+(head->endingVirtualAddress-((node->virtualAddress)+(int)size))); //remove this line in the final code
        struct subChain* holeNode=(struct subChain*)(((char*)(node)+size));
        
        head->subchain->next=holeNode;
        holeNode->next=NULL;
        holeNode->prev=head->subchain;
        holeNode->holeOrProcessFlag=0;
        holeNode->size=(head->endingVirtualAddress-((node->virtualAddress)+(int)size));
        //printf("%d\n",(int)holeNode->size); //remove this line in the final code
        holeNode->virtualAddress=(node->virtualAddress)+(int)size;
        //printf("%d\n",holeNode->virtualAddress); //remove this line in the final code
        //head->next=NULL;
        //holeNode->next=NULL;

        return (void*)(size_t)baseVA;
    }
    
    //adding the new process if the Main list is not NULL and if there is a hole in some subchain big enough to add the process of size
    struct mainChain* currentMain=head;
    
    //Iterating through the Main chain
    while(currentMain!=NULL){
        struct subChain* currentSub=currentMain->subchain;
        //Iterating through the subchain of this *currentMain node of the Main Chain
        while(currentSub!=NULL){
            if(currentSub->holeOrProcessFlag==0){     //checking if there is a hole in the subChain
                if(currentSub->size>=size){           //Checking if there is enough size in this hole
                    struct subChain* newNode=currentSub;
                    struct subChain* newHole=(struct subChain*)((char*)currentSub + size);
                    newHole->size=(currentSub->size)-size;
                    newHole->holeOrProcessFlag=0;
                    newHole->virtualAddress=((currentSub->virtualAddress)+(int)size);
                    if(currentSub->next==NULL){
                        //printf("Kill me\n");
                    }
                    newHole->next=currentSub->next;
                    newHole->prev=newNode;

                    newNode->size=size;
                    newNode->next=newHole;
                    newNode->prev=currentSub->prev;
                    newNode->holeOrProcessFlag=1;
                    newNode->virtualAddress=currentSub->virtualAddress;
                    if(currentSub->prev!=NULL){
                        currentSub->prev->next=newNode;
                    }
                    

                    return (void*)(size_t)(newNode->virtualAddress);
                }
            }
            //printf("%d\n",(int)currentSub->size);
            currentSub=currentSub->next;
        }
        currentMain=currentMain->next;
        if(currentMain==NULL){
            //printf("Fuck you bitch\n");
        }
        //printf("YEADHHDH\n");
    }
    //printf("YEADHHDH");
    //By this point we know that there is not sufficient space in any subchain to accomodate the process of size
    //So now we are going to add a new main node with enough space
    //printf("YEADHHDH");
    currentMain=head;
    int sizeToAllocate=PAGE_SIZE;
    while(sizeToAllocate<size){
        sizeToAllocate+=PAGE_SIZE;
    }
    while(currentMain->next!=NULL){
        currentMain=currentMain->next;
    }
    
    //Now we will traverse through the subChain of the currentMain node to get the last Virtual Address as the virtual addresses are contigous

    struct subChain* currentSub=currentMain->subchain;
    while(currentSub->next!=NULL){
        currentSub=currentSub->next;
    }

    int newNodeVirtualAddress=currentSub->virtualAddress+(int)(currentSub->size);
    //printf("%d\n",newNodeVirtualAddress);

    //giving memory to head of the main chain using mmap
    struct mainChain* newMain=(struct mainChain*)(mmap(NULL,sizeof(struct mainChain), PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0));
    if (newMain == MAP_FAILED) {
        perror("mmap");
        exit(1);
    }

    void* newAllocatedMemory=(mmap(NULL,sizeToAllocate, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0));
    if (newAllocatedMemory == MAP_FAILED) {
        perror("mmap");
        exit(1);
    }
    newMain->startingVirtualAddress=(newNodeVirtualAddress);
    newMain->endingVirtualAddress=((newNodeVirtualAddress)+sizeToAllocate);
    //printf("%d\n",newMain->endingVirtualAddress); //remove this line in the final code
    newMain->next=NULL;
    struct subChain* node = (struct subChain*)((char*)newAllocatedMemory);
    node->prev=NULL;
    node->next=NULL;
    node->holeOrProcessFlag=1;
    node->size=size;
    node->virtualAddress=(newNodeVirtualAddress);
    newMain->subchain=node;
    //printf("%d\n",(node->virtualAddress)+(int)size); //remove this line in the final code
    //printf("%d\n",((node->virtualAddress)+(int)size)+(newMain->endingVirtualAddress-((node->virtualAddress)+(int)size))); //remove this line in the final code
    struct subChain* holeNode=(struct subChain*)(((char*)(node)+size));
    newMain->subchain->next=holeNode;  //basically node->next
    holeNode->next=NULL;
    holeNode->prev=newMain->subchain;
    holeNode->holeOrProcessFlag=0;
    holeNode->size=(newMain->endingVirtualAddress-((node->virtualAddress)+(int)size));
    //printf("%d\n",(int)holeNode->size); //remove this line in the final code
    holeNode->virtualAddress=(node->virtualAddress)+(int)size;
    //printf("%d\n",holeNode->virtualAddress); //remove this line in the final code

    currentMain->next=newMain;
    return (void*)(size_t)newNodeVirtualAddress;

}


/*
this function print the stats of the MeMS system like
1. How many pages are utilised by using the mems_malloc
2. how much memory is unused i.e. the memory that is in freelist and is not used.
3. It also prints details about each node in the main chain and each segment (PROCESS or HOLE) in the sub-chain.
Parameter: Nothing
Returns: Nothing but should print the necessary information on STDOUT
*/
void mems_print_stats(){

    //First printing the whole Main list and their subchains

    int pagesUsed=0;
    int spaceUnused=0;
    int mainChainLength=0;

    struct mainChain* currentMain=head;
    while(currentMain!=NULL){
        mainChainLength++;
        pagesUsed+=((currentMain->endingVirtualAddress-currentMain->startingVirtualAddress)/PAGE_SIZE);
        printf("Main[%d:%d]-> ",currentMain->startingVirtualAddress,currentMain->endingVirtualAddress-1);
        struct subChain* currentSub=currentMain->subchain;
        while(currentSub!=NULL){
            if(currentSub->holeOrProcessFlag==1){
                printf("P[%d:%d] <-> ",currentSub->virtualAddress,currentSub->virtualAddress+(((int)currentSub->size)-1));
            }
            else if(currentSub->holeOrProcessFlag==0){
                printf("H[%d:%d] <-> ",currentSub->virtualAddress,currentSub->virtualAddress+(((int)currentSub->size)-1));
                spaceUnused+=(int)currentSub->size;
            }
            currentSub=currentSub->next;
            
        }
        currentMain=currentMain->next;
        printf("NULL\n");
        printf(" |\n v\n");

    }
    printf("NULL\n");


    //Now printing the rest of the details except the sub-chain length array
    printf("\nPages Used:%d\n",pagesUsed);
    printf("Space unused:%d\n",spaceUnused);
    printf("Main Chain Length:%d\n",mainChainLength);

    //Now calculating the length of each subchain and storing the lengths in an array called arr

    int arr[mainChainLength];
    int i=0;
    currentMain=head;
    while(currentMain!=NULL){
        struct subChain* currentSub=currentMain->subchain;
        int count=0;
        while(currentSub!=NULL){
            currentSub=currentSub->next;
            count++;
        }

        arr[i]=count;
        i++;
        currentMain=currentMain->next;
    }

    //Now printing the sub-Chain length array
    printf("Array with sub-Chain lengths of each Main node:[");
    for(int j=0;j<mainChainLength-1;j++){
        printf("%d, ",arr[j]);
    }
    printf("%d]\n",arr[mainChainLength-1]);

}


/*
Returns the MeMS physical address mapped to ptr ( ptr is MeMS virtual address).
Parameter: MeMS Virtual address (that is created by MeMS)
Returns: MeMS physical address mapped to the passed ptr (MeMS virtual address).
*/
void *mems_get(void*v_ptr){

    int ptr=(int)(size_t)v_ptr;

    struct mainChain* currentMain=head;
    while(currentMain!=NULL){
        struct subChain* currentSub=currentMain->subchain;
        while(currentSub!=NULL){
            if(ptr>=currentSub->virtualAddress && ptr<=(currentSub->virtualAddress+(int)currentSub->size)){
                return (void*)((char*)currentSub+(ptr-currentSub->virtualAddress));
            }
            currentSub=currentSub->next;
        }
        currentMain=currentMain->next;
    }
    return NULL;
    
}


/*
this function free up the memory pointed by our virtual_address and add it to the free list
Parameter: MeMS Virtual address (that is created by MeMS) 
Returns: nothing
*/
void mems_free(void *v_ptr){

    int ptr=(int)(size_t)v_ptr;
    struct mainChain* currentMain=head;
    while(currentMain!=NULL){
        struct subChain* currentSub=currentMain->subchain;
        while(currentSub!=NULL){
            if(currentSub->virtualAddress==ptr){
                currentSub->holeOrProcessFlag=0;  //marking as hole here

                //combining with an adjacent previous hole
                if(currentSub->prev!=NULL ){
                    if(currentSub->prev->holeOrProcessFlag==0){
                        currentSub->prev->size+=currentSub->size;
                        currentSub->prev->next=currentSub->next;
                        return;
                    }
                    
                }

                //combining with an adjacent next hole
                if(currentSub->next!=NULL){
                    if(currentSub->next->holeOrProcessFlag==0){
                        currentSub->size+=currentSub->next->size;
                        currentSub->next=currentSub->next->next;
                        return;
                    }
                }

                return;
            }
            currentSub=currentSub->next;
        }
        currentMain=currentMain->next;
    }
    
}