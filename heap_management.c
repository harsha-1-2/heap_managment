#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include<string.h>

// Structure for a heap block
typedef struct Node {
    int size;
    bool isFree;
    char name[20];  // Store the variable name
    struct Node* next;
    int allocated_size;
} Node;


// Function to generate the largest Fibonacci number ≤ totalMemory
void generateFibonacciList(int totalMemory, int* fibArr, int* count) {
    int a = 1, b = 1, c = 2;  
    *count = 0;

    while (c <= totalMemory) {
        fibArr[(*count)++] = c;  
        a = b;
        b = c;
        c = a + b;
    }
}


// Function to initialize heap with Fibonacci-sized blocks
Node* initializeHeap(int totalMemory) {
    int fibArr[100]; 
    int count = 0;

    generateFibonacciList(totalMemory, fibArr, &count); 

    Node* head = NULL;
    Node* prev = NULL;

   
    for (int i = 0; i < count; i++) {
        Node* newNode = (Node*)malloc(sizeof(Node));
        newNode->size = fibArr[i];
        newNode->isFree = true;
        newNode->next = NULL;
        newNode->allocated_size=0;

        if (head == NULL) {
            head = newNode;
        } else {
            prev->next = newNode;
        }
        prev = newNode;
    }

    return head; 
}

// Prints the current state of the heap
void traverseHeap(Node* head) {
    Node* current = head;
    printf("Heap State:\n");

    while (current != NULL) {
        printf("[Size: %d | Free: %s | Name: %s | allocated size: %d] ->\n", 
            current->size, 
            current->isFree ? "Yes" : "No", 
            current->isFree ? "None" : current->name,
            current->allocated_size
        );
        current = current->next;
    }
    printf("NULL\n");
}


// Returns the previous Fibonacci number before the given size
int getPreviousFibonacci(int size) {
    if (size <= 1) return 0;  

    int a = 1, b = 1, c = a + b;  

    while (c < size) {  
        a = b;  
        b = c;  
        c = a + b;  
    }

    return b;  
}

// Checks if two numbers are consecutive Fibonacci numbers
bool isFibonacciPair(int a, int b) {
    int x = 1, y = 1, z = x + y;

    while (z < a || z < b) {  
        x = y;  
        y = z;  
        z = x + y;  
    }

    return (z == a && y == b) || (z == b && y == a);  
}


// Merges adjacent free Fibonacci-sized blocks recursively
void mergeBlock(Node* head) {
    if (head == NULL) return;

    Node* current = head;

    while (current != NULL && current->next != NULL) {
        if (current->isFree && current->next->isFree && 
            isFibonacciPair(current->size, current->next->size)) {
            
            current->size += current->next->size;  
            Node* temp = current->next;  
            current->next = current->next->next;  
            free(temp);  

            memset(current->name, 0, sizeof(current->name));  

            // Restart merge after any change
            current = head;
            continue;
        }
        current = current->next;
    }
}


// Splits a larger Fibonacci block into Fibonacci-sized buddy blocks
void splitBlock(Node* node, int requiredSize) {
    if (node == NULL || node->size <= requiredSize) return;

    while (node->size > requiredSize) {  
        int newSize = getPreviousFibonacci(node->size);  

        Node* newNode = (Node*)malloc(sizeof(Node));  
        newNode->size = newSize;  
        newNode->isFree = true;  
        newNode->next = node->next;  

        node->next = newNode;  
        node->size -= newSize;  
    }
}



// Returns the smallest Fibonacci number that is >= requested size
int getClosestFibonacci(int size) {
    if (size <= 1) return 1;  

    int a = 1, b = 1, c = a + b;  

    while (c < size) {  
        a = b;  
        b = c;  
        c = a + b;  
    }

    return c;  
}


// Finds the smallest free block that fits the requested size using the buddy system
Node* findBestFit_by_buddy_system(Node* head, int size) {
    int closestFibSize = getClosestFibonacci(size);  
    Node* bestFit = NULL;  
    Node* current = head;  

    while (current != NULL) {  
        if (current->isFree && current->size >= closestFibSize) {  
            if (bestFit == NULL || current->size < bestFit->size) {  
                bestFit = current;  
            }
        }
        current = current->next;  
    }

    return bestFit;  
}


// Allocates memory and assigns a name to the block
void* allocate_memory(Node* head, char* name, int size) {
    if (strlen(name) > 19) {
        printf("Error: Name too long.\n");
        return NULL;
    }

    // Check for duplicate names
    Node* temp = head;
    while (temp != NULL) {
        if (!temp->isFree && strcmp(temp->name, name) == 0) {
            printf("Error: Duplicate name '%s'.\n", name);
            return NULL;
        }
        temp = temp->next;
    }

    Node* bestFit = findBestFit_by_buddy_system(head, size);

    if (bestFit == NULL) {  
        printf("No suitable block found. Attempting to merge free blocks...\n");
        mergeBlock(head);
        bestFit = findBestFit_by_buddy_system(head, size);

        if (bestFit == NULL) {  
            printf("Memory allocation failed after merging.\n");  
            return NULL;  
        }
    }

    int closestFibSize = getClosestFibonacci(size);

    while (bestFit->size > closestFibSize) {
        splitBlock(bestFit, getPreviousFibonacci(bestFit->size));
    }

    bestFit->isFree = false;
    strncpy(bestFit->name, name, 19);
    bestFit->name[19] = '\0';
    bestFit->allocated_size=size;

    printf("Memory allocated: %s -> Size %d\n", bestFit->name, bestFit->size);
    return (void*)bestFit;
}


// Frees a memory block by its name and merges adjacent free blocks
void free_memory(Node* head, char* name) {
    if (name == NULL) {
        printf("Error: Cannot free an unnamed block.\n");
        return;
    }

    Node* current = head;

    while (current != NULL) {
        if (!current->isFree && strcmp(current->name, name) == 0) {
            current->isFree = true;
            printf("Block '%s' of size %d freed.\n", current->name, current->size);
            memset(current->name, 0, sizeof(current->name));

            mergeBlock(head);
            return;
        }
        current = current->next;
    }

    printf("Error: No allocated block found with name '%s'.\n", name);
}

int main() {
    int totalMemory = 16000;  // Adjust as needed
    Node* heap = initializeHeap(totalMemory);
    int choice;
    size_t size;
    char name[20];

    while (1) {
        printf("\n===== Heap Manager Menu =====\n");
        printf("1. Allocate Memory\n");
        printf("2. Free Memory\n");
        printf("3. Display Heap Layout\n");
        printf("0. Quit\n");
        printf("Enter your choice: ");
        
        if (scanf("%d", &choice) != 1) {
            fprintf(stderr, "Invalid input. Exiting.\n");
            break;
        }

        switch (choice) {
            case 1:
                printf("Enter variable name to allocate: ");
                scanf("%s", name);
                printf("Enter size to allocate: ");
                scanf("%zu", &size);
                allocate_memory(heap, name, size);
                break;
            case 2:
                traverseHeap(heap);
                printf("Enter the variable name to free: ");
                scanf("%s", name);
                free_memory(heap, name);
                break;
            case 3:
                traverseHeap(heap);
                break;
            case 0:
                printf("Exiting.\n");
                return 0;
            default:
                printf("Invalid choice. Please try again.\n");
        }
    }
    return 0;
}








    

