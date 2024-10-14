#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>


#define MAX_NAME_LENGTH 100
#define HASH_LENGTH 65  // SHA-256 hash (64 characters + null terminator)
#define MAX_BLOCK_KEY 100000000 // Assuming 8 digit block key
#define VEB_UNIVERSE 131072  // For 16-bit universe size (simplified)

// Block structure
typedef struct Block {
    int roll_no;
    char name[MAX_NAME_LENGTH];
    char dob[11]; // Format: YYYY-MM-DD
    char current_hash[HASH_LENGTH];
    char previous_hash[HASH_LENGTH];
    int block_key;
    struct Block* next;
} Block;

// Define structure for vEB Tree
typedef struct vEBTree {
    int u;                   // Universe size
    int min, max;             // Minimum and Maximum values in the tree
    struct vEBTree *summary;  // Summary structure
    struct vEBTree **clusters; // Clusters for dividing universe
} vEBTree;

// Function to create a new vEB tree with a given universe size
vEBTree *createVEBTree(int u) {
    vEBTree *tree = (vEBTree *)malloc(sizeof(vEBTree));
    tree->u = u;
    tree->min = tree->max = -1; // Initially, the tree is empty (no elements)
    
    if (u <= 2) {
        tree->summary = NULL;
        tree->clusters = NULL;
    } else {
        int sqrt_u = (int)sqrt(u);
        tree->summary = createVEBTree(sqrt_u);
        tree->clusters = (vEBTree **)malloc(sqrt_u * sizeof(vEBTree *));
        for (int i = 0; i < sqrt_u; i++) {
            tree->clusters[i] = createVEBTree(sqrt_u);
        }
    }
    return tree;
}

// Helper functions to calculate high and low bits (for mapping within clusters)
int high(int x, int u) {
    return x / (int)sqrt(u);
}

int low(int x, int u) {
    return x % (int)sqrt(u);
}


// Function to check if a number exists in the vEB tree
int vEBMember(vEBTree *tree, int x) {
    if (x == tree->min || x == tree->max) {
        return 1;  // The number is either the min or max of the tree
    }
    if (tree->u <= 2) {
        return 0;  // If universe size is 2 or smaller and it's not min or max, it doesn't exist
    }
    
    int high_x = high(x, tree->u);
    int low_x = low(x, tree->u);
    return vEBMember(tree->clusters[high_x], low_x);
}
Block* arr[131072];

// Utility function to insert an element into a vEB tree (for testing)
void vEBInsert(vEBTree *tree, int x) {
    if (tree->min == -1) {
        tree->min = tree->max = x;
    } else {
        if (x < tree->min) {
            int temp = x;
            x = tree->min;
            tree->min = temp;
        }
        if (tree->u > 2) {
            int high_x = high(x, tree->u);
            int low_x = low(x, tree->u);
            if (tree->clusters[high_x]->min == -1) {
                vEBInsert(tree->summary, high_x);
                tree->clusters[high_x]->min = tree->clusters[high_x]->max = low_x;
            } else {
                vEBInsert(tree->clusters[high_x], low_x);
            }
        }
        if (x > tree->max) {
            tree->max = x;
        }
    }
}

// Create a block with given data
Block* createBlock(int roll_no, const char* name, const char* dob, int block_key, const char* prev_hash) {
    Block* new_block = (Block*)malloc(sizeof(Block));
    new_block->roll_no = roll_no;
    strncpy(new_block->name, name, MAX_NAME_LENGTH);
    strncpy(new_block->dob, dob, 11);
    new_block->block_key = block_key;
     
    // Set the previous hash
    if (prev_hash != NULL) {
        strncpy(new_block->previous_hash, prev_hash, HASH_LENGTH);
    } else {
        strcpy(new_block->previous_hash, "0"); // Genesis block
    }

    // Simulate a hash for the current block (simplified for demonstration)
    snprintf(new_block->current_hash, HASH_LENGTH, "%d%s%s%s%u", roll_no, name, dob, new_block->previous_hash, block_key);

    new_block->next = NULL;
    arr[roll_no]=new_block;
    return new_block;
}

// Append a block to the blockchain
Block* appendBlock(Block* head, Block* new_block) {
    if (head == NULL) {
        return new_block;
    }
    Block* temp = head;
    while (temp->next != NULL) {
        temp = temp->next;
    }
    temp->next = new_block;
    return head;
}

// Display cryptographic version of the data
void displayCryptographicData(int roll_no) {
    printf("Cryptographic version of data for roll number %d\n", roll_no);
}

// Main program to demonstrate blockchain and VEB tree interaction
int main() {
    // Initialize blockchain and VEB tree
    Block* blockchain = NULL;
    vEBTree* veb_tree = createVEBTree(VEB_UNIVERSE); // Universe size for 8-digit block keys

    // Example: Adding blocks
    blockchain = appendBlock(blockchain, createBlock(12345, "Adarsh", "2001-05-12", 12345678, NULL));
    vEBInsert(veb_tree, 12345);
    
    blockchain = appendBlock(blockchain, createBlock(54321, "Rishabh", "2002-11-23", 87654321, blockchain->current_hash));
    vEBInsert(veb_tree, 54321);

    blockchain = appendBlock(blockchain, createBlock(66666, "Aditya", "2012-09-20", 12341234, blockchain->current_hash));
    vEBInsert(veb_tree, 66666);
    
    int block_key;
    int roll_no;

    // User input
    printf("Enter your roll number: ");
    scanf("%d", &roll_no);
    
    // Search block key in veb tree
    if (vEBMember(veb_tree, roll_no)) {        
        printf("Enter your block key: ");
        scanf("%d", &block_key);
        Block *block = arr[roll_no];
        if (block->block_key == block_key) {
            printf("Name: %s\n", block->name);
            printf("DOB: %s\n", block->dob);
        } else {
            displayCryptographicData(roll_no);
        }
    }else{
        printf("User not found\n");
    }

    return 0;
}


// Cryptography of the data stored in block is not implemented
// Hashing of block is not implemented
// Hashing of block keys/passwords is not implemented(for password security)
