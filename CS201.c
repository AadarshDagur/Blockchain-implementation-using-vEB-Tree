#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <time.h>
#include <uthash.h>


char current_hash[65];
#define MAX_NAME_LENGTH 100
#define HASH_LENGTH 65  // SHA-256 hash (64 characters + null terminator)
#define MAX_BLOCK_KEY 100000000 // Assuming 8 digit block key
#define VEB_UNIVERSE 131072  // For 16-bit universe size (simplified)
char key[11]= "supersecret"; // Key for encryption

// Block structure
typedef struct Block {
    int roll_no;
    char name[MAX_NAME_LENGTH];
    char dob[11];
    long timestamp;
    char previous_hash[HASH_LENGTH];
    int block_key;
    struct Block* next;
} Block;



typedef struct {
    int roll_no;        // Key
    Block *block_data;  // Pointer to the block
    UT_hash_handle hh;  // Handle for the hash table
} BlockMap;

BlockMap *block_map = NULL;


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
    tree->min = tree->max = -1; 
    
    if (u <= 2) {
        tree->summary = NULL;
        tree->clusters = NULL;
    } else {
        int sqrt_u = (int)ceil(sqrt(u));
        tree->summary = createVEBTree(sqrt_u);
        tree->clusters = (vEBTree **)malloc(sqrt_u * sizeof(vEBTree *));
        for (int i = 0; i < sqrt_u; i++) {
            tree->clusters[i] = NULL; // Initialize clusters to NULL
        }
    }
    return tree;
}

// Helper functions to calculate high and low bits (for mapping within clusters)
int high(int x, int u) {
    return x / (int)ceil(sqrt(u));
}

int low(int x, int u) {
    return x % (int)ceil(sqrt(u));
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
    if (tree->clusters[high_x] == NULL) {
        return 0;  // The number is not in this cluster
    }
    return vEBMember(tree->clusters[high_x], low_x);
}

// Utility function to insert an element into a vEB tree
void vEBInsert(vEBTree *tree, int x) {
    if (tree->min == -1) {
        tree->min = tree->max = x;  // Tree was empty, now has one element
    } else {
        if (x < tree->min) {
            int temp = x;
            x = tree->min;
            tree->min = temp;
        }
        if (tree->u > 2) {
            int high_x = high(x, tree->u);
            int low_x = low(x, tree->u);
            if (tree->clusters[high_x] == NULL) {
                tree->clusters[high_x] = createVEBTree((int)ceil(sqrt(tree->u)));
            }
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

// Function to free a vEB tree
void freeVEBTree(vEBTree *tree) {
    if (tree == NULL) return;

    if (tree->u > 2) {
        int sqrt_u = (int)ceil(sqrt(tree->u));

        // Recursively free all clusters
        for (int i = 0; i < sqrt_u; i++) {
            if (tree->clusters[i] != NULL) {
                freeVEBTree(tree->clusters[i]);
            }
        }

        // Free the clusters array and the summary
        free(tree->clusters);
        freeVEBTree(tree->summary);
    }

    // Finally, free the tree itself
    free(tree);
}




// Simple hash function that produces a hexadecimal hash
unsigned long calculateHash(Block *block) {
    // Create a simple hash by concatenating the fields
    char combined[200]; // Buffer to hold combined string
    snprintf(combined, sizeof(combined), "%d%s%s%ld%s%d",
             block->roll_no, block->name, block->dob,
             block->timestamp, block->previous_hash);
    
    // Generate a simple hash (for demonstration)
    unsigned long hash = 5381;
    int c;
    char *ptr = combined;

    while ((c = *ptr++)) {
        hash = ((hash << 5) + hash) + c; // hash * 33 + c
    }

    return hash;

}

void generateHashString(unsigned long hashValue, char *hashStr) {
    // Convert the hash value to a hexadecimal string
    snprintf(hashStr, 65, "%lx", hashValue);

    // Ensure that the hash is represented as a 64-character hexadecimal string
    for (int i = strlen(hashStr); i < 64; i++) {
        hashStr[i] = '0'; // Fill remaining with zeros
    }
    hashStr[64] = '\0'; // Null-terminate
}

int hashed_key(int key) {
    unsigned long hash = 5381;
    int c;

    // Treating the key as an array of bytes
    unsigned char *ptr = (unsigned char*)&key; // Use unsigned char for byte-wise operations

    for (size_t i = 0; i < sizeof(int); i++) {
        c = ptr[i];
        hash = ((hash << 5) + hash) + c; // hash * 33 + c
    }

    return hash;
}

// XOR cipher for encryption and decryption
char xor_cipher(char data, char key) {
    return data ^ key;
}

// Decrypt the data
char* decrypt(char* data) {
    char* decrypted = (char*)malloc(strlen(data) + 1);
    for (int i = 0; i < strlen(data); i++) {
        decrypted[i] = xor_cipher(data[i], key[i%11]);
    }
    return decrypted;
}

void add_block_to_map(int roll_no, Block *block) {
    BlockMap *entry = (BlockMap *)malloc(sizeof(BlockMap));
    entry->roll_no = roll_no;
    entry->block_data = block;
    HASH_ADD_INT(block_map, roll_no, entry);
}

Block *find_block_in_map(int roll_no) {
    BlockMap *entry;
    HASH_FIND_INT(block_map, &roll_no, entry);
    return entry ? entry->block_data : NULL;
}

// Free the block map
void freeBlockMap() {
    BlockMap *current_entry, *tmp;
    HASH_ITER(hh, block_map, current_entry, tmp) {
        HASH_DEL(block_map, current_entry);
        free(current_entry);
    }
}

// Create a block with given data
Block* createBlock(int roll_no, const char* name, const char* dob, int block_key, const char* hash) {
    Block* new_block = (Block*)malloc(sizeof(Block));
    new_block->roll_no = roll_no;
    for (int i = 0; i < MAX_NAME_LENGTH; i++) {
        new_block->name[i] = xor_cipher(name[i], key[i%11]);
    }
    for (int i = 0; i < 11; i++) {
        new_block->dob[i] = xor_cipher(dob[i], key[i%11]);
    }
    new_block->block_key = block_key;
    
    // Set the previous hash
    if (hash != NULL) {
        strncpy(new_block->previous_hash, hash, HASH_LENGTH);
    } else {
        strcpy(new_block->previous_hash, "0"); // Genesis block
    }


    new_block->timestamp = time(NULL);
    unsigned long hashValue = calculateHash(new_block);
    generateHashString(hashValue, current_hash);

    new_block->next = NULL;
    add_block_to_map(roll_no, new_block);
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

// Free the blockchain memory
void freeBlockchain(Block* head) {
    while (head) {
        Block* temp = head;
        head = head->next;
        free(temp);
    }
}


// Main program to demonstrate blockchain and VEB tree interaction
int main() {

    // Initialize blockchain and VEB tree
    Block* blockchain = NULL;
    vEBTree* veb_tree = createVEBTree(VEB_UNIVERSE); // Universe size for 8-digit block keys
    blockchain = appendBlock(blockchain, createBlock(12345, "Adarsh", "2001-05-12", hashed_key(12345678), NULL));
    vEBInsert(veb_tree, 12345);

    blockchain = appendBlock(blockchain, createBlock(54321, "Rishabh", "2002-11-23", hashed_key(87654321), current_hash));
    vEBInsert(veb_tree, 54321);

    blockchain = appendBlock(blockchain, createBlock(65060, "Anil", "2004-09-08", hashed_key(10101010), current_hash));
    vEBInsert(veb_tree, 65060);

    blockchain = appendBlock(blockchain, createBlock(66666, "Aditya", "2012-09-20", hashed_key(12341234), current_hash));
    vEBInsert(veb_tree, 66666);


    // Load additional blocks from a CSV file
    FILE *file = fopen("random_entries.csv", "r");
    if (file == NULL) {
        printf("Could not open file.\n");
        return 1;
    }

    char line[200];
    while (fgets(line, sizeof(line), file)) {
        
        char *id_str = strtok(line, ",");
        char *name = strtok(NULL, ",");
        char *date = strtok(NULL, ",");
        char *num_str = strtok(NULL, ",");

        int id = atoi(id_str);
        int num = atoi(num_str);

        if (id && name && date && num) {
            blockchain = appendBlock(blockchain, createBlock(id, name, date, hashed_key(num), current_hash));
            vEBInsert(veb_tree, id);
        }
    }

    fclose(file);

    // Main menu loop
    int choice;
    while (1) {
        printf("--------------------\n");
        printf("Menu:\n");
        printf("1. Check your data\n");
        printf("2. Enter a new block\n");
        printf("3. Check if blockchain is tampered\n");
        printf("4. Exit\n");
        printf("Enter your choice: ");
        scanf("%d", &choice);
        printf("--------------------\n");

        switch (choice) {
            case 1: {
                int block_key;
                int roll_no;

                // User input for roll number
                printf("Enter your roll number: ");
                scanf("%d", &roll_no);
                
                // Search block key in veb tree
                if (vEBMember(veb_tree, roll_no)) {
                    printf("Enter your block key: ");
                    scanf("%d", &block_key);
                    Block* block = find_block_in_map(roll_no);
                    if (block->block_key == hashed_key(block_key)) {
                        printf("--------------------\n");
                        printf("Welcome %s\n", decrypt(block->name));
                        printf("Name: %s\n", decrypt(block->name));
                        printf("DOB: %s\n", decrypt(block->dob));
                        printf("Hash: %s\n", block->previous_hash);
                        printf("Block key: %d\n", block->block_key);

                    } else {
                        printf("--------------------\n");
                        printf("Invalid block key\n");
                        printf("Name: %s\n", block->name);
                        printf("DOB: %s\n", block->dob);
                        printf("Hash: %s\n", block->previous_hash);
                        printf("Block key: %d\n", block->block_key);
                    }
                } else {
                    printf("User not found\n");
                }
                break;
            }
            case 2:{
                char name[MAX_NAME_LENGTH];
                char dob[11]; 
                int block_key;

                printf("Enter your name: ");
                scanf(" %[^\n]%*c", name);

                printf("Enter your date of birth (YYYY-MM-DD): ");
                scanf("%10s", dob);

                printf("Enter an 8-digit block key: ");
                scanf("%d", &block_key);

                int roll_num;
                for (int i=10001;i<=99999;i++){
                    if (vEBMember(veb_tree, i)) {
                        continue;
                    }else{
                        roll_num=i;
                        break;
                    }
                }
            

                // Create a new block
                Block* new_block = createBlock(roll_num, name, dob, hashed_key(block_key), current_hash);
                blockchain = appendBlock(blockchain, new_block);
                vEBInsert(veb_tree, roll_num); 
                

                printf("New block created successfully with roll no %d!\n",roll_num);
                break;
            }
            case 3: {
                // Check if blockchain is tampered
                int c = 0;
                Block* current_block = blockchain;
                while (current_block->next != NULL) {
                    char hash[65];
                    unsigned long hashValue = calculateHash(current_block);
                    generateHashString(hashValue, hash);
                    if (strcmp(hash, current_block->next->previous_hash) != 0) {
                        printf("Block %d is not valid\n", current_block->roll_no);
                        printf("Blockchain is tampered\n");
                        c = 1;
                        break;
                    }
                    printf("Block %d is valid\n", current_block->roll_no);
                    current_block = current_block->next;
                }
                if (c == 0) {
                    printf("Blockchain is not tampered\n");
                }
                break;
            }
            case 4: {
                printf("Exiting...\n");
                freeVEBTree(veb_tree);
                freeBlockchain(blockchain);
                freeBlockMap();
                return 0;
            }
            default: {
                printf("Invalid choice. Please try again.\n");
                break;
            }

        }

    }

    return 0;
}
