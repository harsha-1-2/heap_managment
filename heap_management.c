#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <time.h>

// ANSI color codes for better visibility
#define COLOR_RESET   "\x1b[0m"
#define COLOR_RED     "\x1b[31m"
#define COLOR_GREEN   "\x1b[32m"
#define COLOR_YELLOW  "\x1b[33m"
#define COLOR_BLUE    "\x1b[34m"
#define COLOR_MAGENTA "\x1b[35m"
#define COLOR_CYAN    "\x1b[36m"
#define COLOR_BOLD    "\x1b[1m"

// Structure for a heap block
typedef struct Node {
    int size;
    bool isFree;
    char name[20];
    struct Node* next;
    int allocated_size;
    
    // GC-related fields
    bool marked;
    bool isRoot;
    char** references;
    int numReferences;
    int refCapacity;
} Node;

// Audit log entry
typedef struct AuditLog {
    char operation[100];
    char timestamp[26];
    struct AuditLog* next;
} AuditLog;

// Global audit log
AuditLog* auditHead = NULL;

// Global GC statistics
typedef struct {
    int totalCollections;
    int totalFreed;
    int lastFreedCount;
    int totalAllocations;
    int totalManualFrees;
} GCStats;

GCStats gcStats = {0, 0, 0, 0, 0};

// Function to get current timestamp
void getCurrentTimestamp(char* buffer, size_t size) {
    time_t now = time(NULL);
    struct tm* t = localtime(&now);
    strftime(buffer, size, "%Y-%m-%d %H:%M:%S", t);
}

// Add entry to audit log
void addAuditLog(const char* operation) {
    AuditLog* newLog = (AuditLog*)malloc(sizeof(AuditLog));
    strncpy(newLog->operation, operation, 99);
    newLog->operation[99] = '\0';
    getCurrentTimestamp(newLog->timestamp, sizeof(newLog->timestamp));
    newLog->next = auditHead;
    auditHead = newLog;
}

// Print audit log
void printAuditLog() {
    printf("\n");
    printf(COLOR_CYAN COLOR_BOLD);
    printf("╔════════════════════════════════════════════════════════════════════════╗\n");
    printf("║                          AUDIT LOG                                     ║\n");
    printf("╚════════════════════════════════════════════════════════════════════════╝\n");
    printf(COLOR_RESET);
    
    if (auditHead == NULL) {
        printf(COLOR_YELLOW "  No operations recorded yet.\n" COLOR_RESET);
        return;
    }
    
    AuditLog* current = auditHead;
    int count = 1;
    
    // Print in reverse order (most recent first)
    printf(COLOR_CYAN "  [Timestamp]          | [Operation]\n");
    printf("  ─────────────────────┼──────────────────────────────────────────────\n" COLOR_RESET);
    
    while (current != NULL && count <= 20) { // Show last 20 operations
        printf("  %s | %s\n", current->timestamp, current->operation);
        current = current->next;
        count++;
    }
    
    if (current != NULL) {
        printf(COLOR_YELLOW "  ... and more (showing last 20 entries)\n" COLOR_RESET);
    }
    printf("\n");
}

// Print a box around text
void printBox(const char* title, const char* content) {
    int len = strlen(title);
    printf(COLOR_BOLD COLOR_GREEN "\n╔");
    for (int i = 0; i < len + 4; i++) printf("═");
    printf("╗\n");
    printf("║  %s  ║\n", title);
    printf("╚");
    for (int i = 0; i < len + 4; i++) printf("═");
    printf("╝\n" COLOR_RESET);
    
    if (content != NULL && strlen(content) > 0) {
        printf("%s\n", content);
    }
}

// Function to generate Fibonacci numbers
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

// Initialize heap
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
        newNode->allocated_size = 0;
        newNode->marked = false;
        newNode->isRoot = false;
        newNode->references = NULL;
        newNode->numReferences = 0;
        newNode->refCapacity = 0;

        if (head == NULL) {
            head = newNode;
        } else {
            prev->next = newNode;
        }
        prev = newNode;
    }

    char logMsg[100];
    sprintf(logMsg, "Heap initialized with %d bytes in %d Fibonacci blocks", totalMemory, count);
    addAuditLog(logMsg);

    return head; 
}

// Print heap state with better formatting
void traverseHeap(Node* head) {
    Node* current = head;
    int allocatedCount = 0;
    int freeCount = 0;
    int totalAllocated = 0;
    int totalFree = 0;
    
    printf("\n");
    printf(COLOR_BOLD COLOR_MAGENTA);
    printf("╔════════════════════════════════════════════════════════════════════════╗\n");
    printf("║                          HEAP MEMORY MAP                               ║\n");
    printf("╚════════════════════════════════════════════════════════════════════════╝\n");
    printf(COLOR_RESET);
    
    printf(COLOR_CYAN "  ┌──────────────────────────────────────────────────────────────┐\n" COLOR_RESET);
    
    while (current != NULL) {
        if (!current->isFree) {
            allocatedCount++;
            totalAllocated += current->size;
            
            printf(COLOR_GREEN "  │ [ALLOCATED] " COLOR_RESET);
            printf("%-15s | Size: " COLOR_YELLOW "%-5d" COLOR_RESET, current->name, current->size);
            printf(" | Used: " COLOR_YELLOW "%-5d" COLOR_RESET, current->allocated_size);
            printf(" │\n");
            
            printf("  │             Root: " COLOR_CYAN "%-3s" COLOR_RESET, current->isRoot ? "YES" : "NO");
            printf(" | References: " COLOR_CYAN "%-2d" COLOR_RESET, current->numReferences);
            
            if (current->numReferences > 0) {
                printf(" [");
                for (int i = 0; i < current->numReferences; i++) {
                    printf("%s%s", current->references[i], 
                           i < current->numReferences - 1 ? ", " : "");
                }
                printf("]");
            }
            printf("     │\n");
            printf(COLOR_CYAN "  ├──────────────────────────────────────────────────────────────┤\n" COLOR_RESET);
        } else {
            freeCount++;
            totalFree += current->size;
            
            printf(COLOR_RED "  │ [FREE]      " COLOR_RESET);
            printf("%-15s | Size: " COLOR_YELLOW "%-5d" COLOR_RESET, "Available", current->size);
            printf("                      │\n");
            printf(COLOR_CYAN "  ├──────────────────────────────────────────────────────────────┤\n" COLOR_RESET);
        }
        current = current->next;
    }
    
    printf(COLOR_CYAN "  └──────────────────────────────────────────────────────────────┘\n" COLOR_RESET);
    
    printf("\n" COLOR_BOLD "  Summary:\n" COLOR_RESET);
    printf("  • Allocated Blocks: " COLOR_GREEN "%d" COLOR_RESET " (Total: " COLOR_YELLOW "%d bytes" COLOR_RESET ")\n", 
           allocatedCount, totalAllocated);
    printf("  • Free Blocks: " COLOR_RED "%d" COLOR_RESET " (Total: " COLOR_YELLOW "%d bytes" COLOR_RESET ")\n", 
           freeCount, totalFree);
    printf("  • Total Memory: " COLOR_CYAN "%d bytes" COLOR_RESET "\n\n", totalAllocated + totalFree);
}

// Fibonacci helper functions
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

bool isFibonacciPair(int a, int b) {
    int x = 1, y = 1, z = x + y;
    while (z < a || z < b) {
        x = y;
        y = z;
        z = x + y;
    }
    return (z == a && y == b) || (z == b && y == a);
}

// Merge blocks with detailed logging
void mergeBlock(Node* head) {
    if (head == NULL) return;

    Node* current = head;
    int mergeCount = 0;

    while (current != NULL && current->next != NULL) {
        if (current->isFree && current->next->isFree && 
            isFibonacciPair(current->size, current->next->size)) {
            
            int oldSize1 = current->size;
            int oldSize2 = current->next->size;
            
            current->size += current->next->size;
            Node* temp = current->next;
            current->next = current->next->next;
            free(temp);

            memset(current->name, 0, sizeof(current->name));
            
            printf(COLOR_YELLOW "  ⚡ MERGE: " COLOR_RESET "Combined blocks [%d + %d = " COLOR_GREEN "%d" COLOR_RESET "]\n", 
                   oldSize1, oldSize2, current->size);
            
            mergeCount++;
            current = head;
            continue;
        }
        current = current->next;
    }
    
    if (mergeCount > 0) {
        char logMsg[100];
        sprintf(logMsg, "Merged %d adjacent free blocks", mergeCount);
        addAuditLog(logMsg);
    }
}

// Split block with logging
void splitBlock(Node* node, int requiredSize) {
    if (node == NULL || node->size <= requiredSize) return;

    printf(COLOR_YELLOW "  ⚡ SPLIT: " COLOR_RESET "Block of size %d being split...\n", node->size);

    while (node->size > requiredSize) {
        int newSize = getPreviousFibonacci(node->size);

        Node* newNode = (Node*)malloc(sizeof(Node));
        newNode->size = newSize;
        newNode->isFree = true;
        newNode->next = node->next;
        newNode->marked = false;
        newNode->isRoot = false;
        newNode->references = NULL;
        newNode->numReferences = 0;
        newNode->refCapacity = 0;

        node->next = newNode;
        node->size -= newSize;
        
        printf(COLOR_YELLOW "    → " COLOR_RESET "Created free block of size " COLOR_GREEN "%d" COLOR_RESET "\n", newSize);
    }
}

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

Node* findNodeByName(Node* head, char* name) {
    Node* current = head;
    while (current != NULL) {
        if (!current->isFree && strcmp(current->name, name) == 0) {
            return current;
        }
        current = current->next;
    }
    return NULL;
}

// Add reference with logging
void addReference(Node* head, char* fromName, char* toName) {
    Node* fromNode = findNodeByName(head, fromName);
    Node* toNode = findNodeByName(head, toName);
    
    if (fromNode == NULL) {
        printf(COLOR_RED "  ✗ ERROR: " COLOR_RESET "Block '%s' not found.\n", fromName);
        return;
    }
    
    if (toNode == NULL) {
        printf(COLOR_RED "  ✗ ERROR: " COLOR_RESET "Block '%s' not found.\n", toName);
        return;
    }
    
    for (int i = 0; i < fromNode->numReferences; i++) {
        if (strcmp(fromNode->references[i], toName) == 0) {
            printf(COLOR_YELLOW "  ⚠ WARNING: " COLOR_RESET "Reference '%s → %s' already exists.\n", 
                   fromName, toName);
            return;
        }
    }
    
    if (fromNode->numReferences >= fromNode->refCapacity) {
        int newCapacity = fromNode->refCapacity == 0 ? 4 : fromNode->refCapacity * 2;
        char** newRefs = (char**)realloc(fromNode->references, newCapacity * sizeof(char*));
        if (newRefs == NULL) {
            printf(COLOR_RED "  ✗ ERROR: " COLOR_RESET "Memory allocation failed.\n");
            return;
        }
        fromNode->references = newRefs;
        fromNode->refCapacity = newCapacity;
    }
    
    fromNode->references[fromNode->numReferences] = (char*)malloc(20 * sizeof(char));
    strncpy(fromNode->references[fromNode->numReferences], toName, 19);
    fromNode->references[fromNode->numReferences][19] = '\0';
    fromNode->numReferences++;
    
    printf(COLOR_GREEN "  ✓ SUCCESS: " COLOR_RESET "Reference added: " COLOR_CYAN "'%s' → '%s'\n" COLOR_RESET, 
           fromName, toName);
    
    char logMsg[100];
    sprintf(logMsg, "Reference added: '%s' → '%s'", fromName, toName);
    addAuditLog(logMsg);
}

// Remove reference with logging
void removeReference(Node* head, char* fromName, char* toName) {
    Node* fromNode = findNodeByName(head, fromName);
    
    if (fromNode == NULL) {
        printf(COLOR_RED "  ✗ ERROR: " COLOR_RESET "Block '%s' not found.\n", fromName);
        return;
    }
    
    for (int i = 0; i < fromNode->numReferences; i++) {
        if (strcmp(fromNode->references[i], toName) == 0) {
            free(fromNode->references[i]);
            
            for (int j = i; j < fromNode->numReferences - 1; j++) {
                fromNode->references[j] = fromNode->references[j + 1];
            }
            fromNode->numReferences--;
            
            printf(COLOR_GREEN "  ✓ SUCCESS: " COLOR_RESET "Reference removed: " COLOR_CYAN "'%s' → '%s'\n" COLOR_RESET, 
                   fromName, toName);
            
            char logMsg[100];
            sprintf(logMsg, "Reference removed: '%s' → '%s'", fromName, toName);
            addAuditLog(logMsg);
            return;
        }
    }
    
    printf(COLOR_YELLOW "  ⚠ WARNING: " COLOR_RESET "Reference '%s → %s' not found.\n", fromName, toName);
}

// Set root status with logging
void setRoot(Node* head, char* name, bool isRoot) {
    Node* node = findNodeByName(head, name);
    if (node == NULL) {
        printf(COLOR_RED "  ✗ ERROR: " COLOR_RESET "Block '%s' not found.\n", name);
        return;
    }
    
    node->isRoot = isRoot;
    printf(COLOR_GREEN "  ✓ SUCCESS: " COLOR_RESET "Block " COLOR_CYAN "'%s'" COLOR_RESET " is now %s root.\n", 
           name, isRoot ? COLOR_GREEN "a" COLOR_RESET : COLOR_RED "NOT a" COLOR_RESET);
    
    char logMsg[100];
    sprintf(logMsg, "Block '%s' root status: %s", name, isRoot ? "SET" : "UNSET");
    addAuditLog(logMsg);
}

// Mark phase with logging
void markBlock(Node* head, Node* node, int depth) {
    if (node == NULL || node->isFree || node->marked) {
        return;
    }
    
    node->marked = true;
    
    for (int i = 0; i < depth; i++) printf("  ");
    printf(COLOR_GREEN "  ✓ MARKED: " COLOR_RESET "'%s' (size: %d)\n", node->name, node->size);
    
    for (int i = 0; i < node->numReferences; i++) {
        Node* referenced = findNodeByName(head, node->references[i]);
        if (referenced != NULL) {
            for (int j = 0; j < depth + 1; j++) printf("  ");
            printf(COLOR_CYAN "    → Following reference to '%s'\n" COLOR_RESET, node->references[i]);
            markBlock(head, referenced, depth + 1);
        }
    }
}

// Sweep phase with logging
int sweepBlocks(Node* head) {
    Node* current = head;
    int freedCount = 0;
    int totalFreedSize = 0;
    
    printf("\n" COLOR_YELLOW "  SWEEP PHASE:\n" COLOR_RESET);
    
    while (current != NULL) {
        if (!current->isFree && !current->marked) {
            printf(COLOR_RED "  ✗ FREEING: " COLOR_RESET "'%s' (size: %d, allocated: %d) " 
                   COLOR_RED "[UNREACHABLE]\n" COLOR_RESET,
                   current->name, current->size, current->allocated_size);
            
            totalFreedSize += current->size;
            
            for (int i = 0; i < current->numReferences; i++) {
                free(current->references[i]);
            }
            free(current->references);
            current->references = NULL;
            current->numReferences = 0;
            current->refCapacity = 0;
            
            current->isFree = true;
            memset(current->name, 0, sizeof(current->name));
            current->allocated_size = 0;
            current->isRoot = false;
            
            freedCount++;
        }
        
        current->marked = false;
        current = current->next;
    }
    
    if (freedCount == 0) {
        printf(COLOR_GREEN "  ✓ No unreachable blocks found.\n" COLOR_RESET);
    } else {
        printf(COLOR_YELLOW "\n  Total freed: %d blocks (%d bytes)\n" COLOR_RESET, 
               freedCount, totalFreedSize);
    }
    
    return freedCount;
}

// Garbage collection with detailed logging
void garbageCollect(Node* head) {
    printBox("GARBAGE COLLECTION STARTED", NULL);
    
    printf(COLOR_BOLD "\n  MARK PHASE:\n" COLOR_RESET);
    printf(COLOR_CYAN "  Finding all reachable blocks from roots...\n\n" COLOR_RESET);
    
    Node* current = head;
    int rootCount = 0;
    
    while (current != NULL) {
        if (!current->isFree && current->isRoot) {
            printf(COLOR_MAGENTA "  ROOT: " COLOR_RESET "'%s'\n", current->name);
            markBlock(head, current, 1);
            rootCount++;
        }
        current = current->next;
    }
    
    if (rootCount == 0) {
        printf(COLOR_RED "  ⚠ WARNING: No root blocks found! All non-root blocks will be freed.\n" COLOR_RESET);
    }
    
    int freedCount = sweepBlocks(head);
    
    if (freedCount > 0) {
        printf("\n" COLOR_YELLOW "  POST-SWEEP CLEANUP:\n" COLOR_RESET);
        mergeBlock(head);
    }
    
    gcStats.totalCollections++;
    gcStats.totalFreed += freedCount;
    gcStats.lastFreedCount = freedCount;
    
    printf("\n");
    printBox("GARBAGE COLLECTION COMPLETE", NULL);
    printf(COLOR_GREEN "  ✓ Freed: " COLOR_YELLOW "%d" COLOR_GREEN " block(s)\n" COLOR_RESET, freedCount);
    printf(COLOR_CYAN "  ℹ Total GC runs: %d | Total blocks freed: %d\n" COLOR_RESET, 
           gcStats.totalCollections, gcStats.totalFreed);
    printf("\n");
    
    char logMsg[100];
    sprintf(logMsg, "GC #%d completed - freed %d blocks", gcStats.totalCollections, freedCount);
    addAuditLog(logMsg);
}

// Allocate memory with detailed logging
void* allocate_memory(Node* head, char* name, int size, bool isRoot) {
    printf("\n" COLOR_BOLD "═══ ALLOCATION REQUEST ═══\n" COLOR_RESET);
    printf("  Name: " COLOR_CYAN "%s" COLOR_RESET " | Size: " COLOR_YELLOW "%d" COLOR_RESET " | Root: %s\n", 
           name, size, isRoot ? COLOR_GREEN "YES" COLOR_RESET : COLOR_RED "NO" COLOR_RESET);
    
    if (strlen(name) > 19) {
        printf(COLOR_RED "  ✗ ERROR: Name too long (max 19 characters).\n" COLOR_RESET);
        return NULL;
    }

    Node* temp = head;
    while (temp != NULL) {
        if (!temp->isFree && strcmp(temp->name, name) == 0) {
            printf(COLOR_RED "  ✗ ERROR: Duplicate name '%s'.\n" COLOR_RESET, name);
            return NULL;
        }
        temp = temp->next;
    }

    Node* bestFit = findBestFit_by_buddy_system(head, size);

    if (bestFit == NULL) {
        printf(COLOR_YELLOW "  ⚠ No suitable block found. Running GC...\n" COLOR_RESET);
        garbageCollect(head);
        bestFit = findBestFit_by_buddy_system(head, size);

        if (bestFit == NULL) {
            printf(COLOR_RED "  ✗ FAILED: Memory allocation failed after GC.\n" COLOR_RESET);
            return NULL;
        }
    }

    int closestFibSize = getClosestFibonacci(size);

    if (bestFit->size > closestFibSize) {
        splitBlock(bestFit, getPreviousFibonacci(bestFit->size));
    }

    bestFit->isFree = false;
    strncpy(bestFit->name, name, 19);
    bestFit->name[19] = '\0';
    bestFit->allocated_size = size;
    bestFit->isRoot = isRoot;
    bestFit->marked = false;

    gcStats.totalAllocations++;

    printf(COLOR_GREEN "  ✓ SUCCESS: " COLOR_RESET "Allocated '%s' → Block size: " COLOR_YELLOW "%d" COLOR_RESET 
           " | Used: " COLOR_YELLOW "%d" COLOR_RESET " | Waste: " COLOR_RED "%d\n" COLOR_RESET,
           bestFit->name, bestFit->size, size, bestFit->size - size);
    
    char logMsg[100];
    sprintf(logMsg, "Allocated '%s' (size: %d, root: %s)", name, size, isRoot ? "YES" : "NO");
    addAuditLog(logMsg);
    
    return (void*)bestFit;
}

// Free memory with logging
void free_memory(Node* head, char* name) {
    printf("\n" COLOR_BOLD "═══ FREE REQUEST ═══\n" COLOR_RESET);
    printf("  Name: " COLOR_CYAN "%s\n" COLOR_RESET, name);
    
    if (name == NULL) {
        printf(COLOR_RED "  ✗ ERROR: Cannot free unnamed block.\n" COLOR_RESET);
        return;
    }

    Node* current = head;

    while (current != NULL) {
        if (!current->isFree && strcmp(current->name, name) == 0) {
            int freedSize = current->size;
            current->isFree = true;
            
            printf(COLOR_GREEN "  ✓ SUCCESS: " COLOR_RESET "Freed '%s' (size: %d)\n", 
                   current->name, freedSize);
            
            for (int i = 0; i < current->numReferences; i++) {
                free(current->references[i]);
            }
            free(current->references);
            current->references = NULL;
            current->numReferences = 0;
            current->refCapacity = 0;
            current->isRoot = false;
            
            memset(current->name, 0, sizeof(current->name));

            gcStats.totalManualFrees++;
            
            char logMsg[100];
            sprintf(logMsg, "Manually freed '%s' (size: %d)", name, freedSize);
            addAuditLog(logMsg);
            
            printf(COLOR_YELLOW "  Checking for merge opportunities...\n" COLOR_RESET);
            mergeBlock(head);
            return;
        }
        current = current->next;
    }

    printf(COLOR_RED "  ✗ ERROR: Block '%s' not found.\n" COLOR_RESET, name);
}

void printStatistics() {
    printf("\n");
    printf(COLOR_BOLD COLOR_MAGENTA);
    printf("╔════════════════════════════════════════════════════════════════════════╗\n");
    printf("║                       SYSTEM STATISTICS                                ║\n");
    printf("╚════════════════════════════════════════════════════════════════════════╝\n");
    printf(COLOR_RESET);
    
    printf(COLOR_CYAN "  Memory Operations:\n" COLOR_RESET);
    printf("    • Total Allocations:      " COLOR_GREEN "%d\n" COLOR_RESET, gcStats.totalAllocations);
    printf("    • Manual Frees:           " COLOR_YELLOW "%d\n" COLOR_RESET, gcStats.totalManualFrees);
    
    printf(COLOR_CYAN "\n  Garbage Collection:\n" COLOR_RESET);
    printf("    • Total GC Runs:          " COLOR_GREEN "%d\n" COLOR_RESET, gcStats.totalCollections);
    printf("    • Total Blocks Freed:     " COLOR_YELLOW "%d\n" COLOR_RESET, gcStats.totalFreed);
    printf("    • Last GC Freed:          " COLOR_MAGENTA "%d\n" COLOR_RESET, gcStats.lastFreedCount);
    
    printf("\n");
}

void printMenu() {
    printf("\n");
    printf(COLOR_BOLD COLOR_BLUE);
    printf("╔════════════════════════════════════════════════════════════════════════╗\n");
    printf("║              FIBONACCI HEAP MANAGER WITH MARK-AND-SWEEP GC            ║\n");
    printf("╠════════════════════════════════════════════════════════════════════════╣\n");
    printf("║  1. Allocate Memory           │  5. Remove Reference                  ║\n");
    printf("║  2. Free Memory               │  6. Set/Unset Root Status             ║\n");
    printf("║  3. Display Heap Layout       │  7. Run Garbage Collection            ║\n");
    printf("║  4. Add Reference (A → B)     │  8. Show Statistics                   ║\n");
    printf("║                               │  9. Show Audit Log                    ║\n");
    printf("║  0. Quit                      │                                       ║\n");
    printf("╚════════════════════════════════════════════════════════════════════════╝\n");
    printf(COLOR_RESET);
    printf(COLOR_YELLOW "Enter your choice: " COLOR_RESET);
}

int main() {
    int totalMemory = 16000;
    Node* heap = initializeHeap(totalMemory);
    int choice;
    size_t size;
    char name[20], name2[20];
    int rootChoice;

    printf(COLOR_BOLD COLOR_CYAN);
    printf("\n");
    printf("╔════════════════════════════════════════════════════════════════════════╗\n");
    printf("║                                                                        ║\n");
    printf("║          FIBONACCI HEAP MANAGER WITH GARBAGE COLLECTION               ║\n");
    printf("║                                                                        ║\n");
    printf("╚════════════════════════════════════════════════════════════════════════╝\n");
    printf(COLOR_RESET);
    printf("\n  Total Memory: " COLOR_GREEN "%d bytes\n" COLOR_RESET, totalMemory);

    while (1) {
        printMenu();
        
        if (scanf("%d", &choice) != 1) {
            fprintf(stderr, COLOR_RED "Invalid input. Exiting.\n" COLOR_RESET);
            break;
        }

        switch (choice) {
            case 1:
                printf(COLOR_CYAN "\n Enter variable name: " COLOR_RESET);
                scanf("%s", name);
                printf(COLOR_CYAN " Enter size to allocate: " COLOR_RESET);
                scanf("%zu", &size);
                printf(COLOR_CYAN " Is this a root reference? (1=Yes, 0=No): " COLOR_RESET);
                scanf("%d", &rootChoice);
                allocate_memory(heap, name, size, rootChoice == 1);
                break;
                
            case 2:
                printf(COLOR_CYAN "\n Enter variable name to free: " COLOR_RESET);
                scanf("%s", name);
                free_memory(heap, name);
                break;
                
            case 3:
                traverseHeap(heap);
                break;
                
            case 4:
                printf(COLOR_CYAN "\n Enter source block name: " COLOR_RESET);
                scanf("%s", name);
                printf(COLOR_CYAN " Enter target block name: " COLOR_RESET);
                scanf("%s", name2);
                addReference(heap, name, name2);
                break;
                
            case 5:
                printf(COLOR_CYAN "\n Enter source block name: " COLOR_RESET);
                scanf("%s", name);
                printf(COLOR_CYAN " Enter target block name: " COLOR_RESET);
                scanf("%s", name2);
                removeReference(heap, name, name2);
                break;
                
            case 6:
                printf(COLOR_CYAN "\n Enter block name: " COLOR_RESET);
                scanf("%s", name);
                printf(COLOR_CYAN " Set as root? (1=Yes, 0=No): " COLOR_RESET);
                scanf("%d", &rootChoice);
                setRoot(heap, name, rootChoice == 1);
                break;
                
            case 7:
                garbageCollect(heap);
                break;
                
            case 8:
                printStatistics();
                break;
                
            case 9:
                printAuditLog();
                break;
                
            case 0:
                printf("\n" COLOR_GREEN "Thank you for using Fibonacci Heap Manager!\n" COLOR_RESET);
                printf(COLOR_CYAN "Final Statistics:\n" COLOR_RESET);
                printStatistics();
                return 0;
                
            default:
                printf(COLOR_RED "\n  ✗ Invalid choice. Please try again.\n" COLOR_RESET);
        }
    }
    return 0;
}
