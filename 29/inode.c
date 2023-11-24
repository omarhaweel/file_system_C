#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <math.h>
#include <inode.h>
#include <allocation.h>

#define BUFFER_SIZE 4096

static int num_inodes = 0;

struct inode* load_inodes() {

    struct inode** nodes = NULL;
    int num_nodes = 0;

    char buffer[BUFFER_SIZE];
    size_t rc;

    FILE* fp = fopen("master_file_table", "rb");
    if (!fp) {
        perror("Error opening master_file_table");
        return NULL;
    }

    while ((rc = fread(buffer, 1, BUFFER_SIZE, fp)) > 0 ) {

        size_t jumper = 0;
        while (jumper < rc) {
            struct inode* node = malloc(sizeof(struct inode));
            if (!node) {
                perror("Error allocating node");
                fclose(fp);
                return NULL;
            }
            node->id = *((int*)(buffer + jumper));
            jumper += sizeof(int); // hopp 4 bytes for the id

            int name_length = *((int*)(buffer + jumper));
            jumper += sizeof(int);// hopp 4 bytes for the id
            node->name = malloc(name_length);
            if (!node->name) {
                perror("Error allocating name");
                free(node);
                fclose(fp);
                return NULL;
            }
            strncpy(node->name, buffer + jumper, name_length);


            node->name[name_length - 1] = '\0';// hopp 4 bytes for the id, selv om null-byte er inkludert sist
            jumper += name_length;

            node->is_directory = buffer[jumper++]; // hopp 1 byte
            node->is_readonly = buffer[jumper++]; // hopp 1 byte

            node->filesize = *((int*)(buffer + jumper)); // cast de 4 bytene til int* så dereference
            jumper += sizeof(int); // hopp 4 bytes

            node->num_entries = *((int*)(buffer + jumper));
            jumper += sizeof(int); // hopp 4 bytes

            node->entries = malloc(sizeof(uintptr_t) * node->num_entries);
            if (!node->entries) {
                perror("Error allocating entries");
                free(node->name);
                free(node);
                fclose(fp);
                return NULL;
            }

            for (int i = 0; i < node->num_entries; i++) {
                if (node->is_directory) {
                    node->entries[i] = (uintptr_t)(buffer + jumper); // assign entries[x] as uintptr_t
                } else {
                        node->entries[i] = *(int*)(buffer + jumper); // cast from uintptr_t to int pointer so dereference
                }

                jumper += sizeof(uintptr_t);
            }

            nodes = realloc(nodes, (num_nodes + 1) * sizeof(struct inode*));

            if (!nodes) {
            perror("Error reallocating nodes");
            free(node->entries);
            free(node->name);
            free(node);
            fclose(fp);
            return NULL;
            }

            nodes[num_nodes++] = node;
        }
    }

    fclose(fp);

    for (int i = 0; i < num_nodes; i++) {
        struct inode* node = (struct inode*)nodes[i];
        if (node->is_directory){
            for (int j=0;j<node->num_entries;j++){
                int id=  *(int*) node->entries[j];
                for (int k=0;k<num_nodes;k++){
                    struct inode* nodeToGet = nodes[k];
                    if (nodeToGet->id == id){
                        node->entries[j]=(uintptr_t) nodeToGet;
                        break;
                    }
                }
            }
        }
    }

    struct inode* root = nodes[0];

    free(nodes);

    return root; // return the first node in the linked list of inodes
}


void print_inode(struct inode* node){
    printf("ld:%d\n  name:%s\n num_entries:%d\n file_size:%d\n ", node->id, node->name, node->num_entries, node->filesize);
    printf("\n\n");
}

void print_blocks(struct inode* node){
    for (int i=0;i<node->num_entries;i++){
        printf("blocks er : %ld\n", node->entries[i]);
        printf("\n\n");
    }
}

//////

struct inode* create_dir(struct inode* parent, char* name) {

    // If parent is NULL, create a new root inode
    if (!parent) {
        parent = malloc(sizeof(struct inode));
        if (!parent) {
            perror("kunne ikke allokere");
            EXIT_FAILURE;
        }

        parent->id = num_inodes++;
        parent->name = strdup(name);
        parent->is_directory = 1;
        parent->is_readonly = 0;
        parent->filesize = 0;
        parent->num_entries = 0;
        parent->entries = NULL;

        return parent;
    }

     // lag ny dir
    struct inode* dir = malloc(sizeof(struct inode));
    if (!dir) {
        perror("Error allocating inode");
        return NULL;
    }
    if(!parent->is_directory){
        return NULL;
    }

    if(parent->is_directory){
        // Check if name already exists in parent's entries
    for (int i = 0; i < parent->num_entries; i++) {
        struct inode* entry = (struct inode*) parent->entries[i];
        if (entry && strcmp(entry->name, name) == 0) {
            return NULL;
        }
    }

    dir->id = num_inodes++;
    dir->name = strdup(name);
    dir->is_directory = 1;
    dir->is_readonly = 0;
    dir->filesize = 0;
    dir->num_entries = 0;
    dir->entries = NULL;

    // legg dir til parent entries og inkrement antall entries
    parent->entries = realloc(parent->entries, sizeof(uintptr_t) * (parent->num_entries + 1));
    if (!parent->entries) {
        perror("Error reallocating entries");
        free(dir->name);
        free(dir);
        return NULL;
    }

    parent->entries[parent->num_entries++] = (uintptr_t) dir;
    }


    return dir;
}

//////////////////////////////////////////////////

struct inode* create_file(struct inode* parent,char* name,char readonly,int size_in_bytes) {

    // ingen parent, lag en parent, med den skal være første inode, og dermed kan ikke legge til directories senere
    if (!parent) {
        parent = malloc(sizeof(struct inode));
        if (!parent) {
            perror("Error allocating inode");
            return NULL;
        }

        parent->id = num_inodes++;
        parent->name = strdup(name);
        parent->is_directory = 0;
        parent->is_readonly = readonly;
        parent->filesize = size_in_bytes;



        parent->num_entries = 0;
        parent->entries = NULL;

        return parent;
    }

    struct inode* fil = malloc(sizeof(struct inode));
    if (!fil) {
        perror("Error allocating inode");
        return NULL;
    }
    // sjekk om en entry i parent->entries har samme navn
    if (parent->is_directory){
         for (int i = 0; i < parent->num_entries; i++) {
        struct inode* entry = (struct inode*) parent->entries[i];
        if (entry && strcmp(entry->name, name) == 0) {
            return NULL;
        }
    }

    fil->id = num_inodes++;
    fil->name = strdup(name);
    fil->is_directory = 0;
    fil->is_readonly = readonly;
    fil->filesize = size_in_bytes;
    fil->num_entries = (int) ceil((fil->filesize + 0.0)/BUFFER_SIZE); //heltallsdivisjon
    fil->entries = malloc(sizeof(uintptr_t) * fil->num_entries);

    for (int i = 0; i < fil->num_entries; i++) {
        fil->entries[i] = allocate_block();
    }

    // legg fil inode til parent->entries og inkrement antall entries
    parent->entries = realloc(parent->entries, sizeof(uintptr_t) * (parent->num_entries + 1));
    if (!parent->entries) {
        perror("Error reallocating entries");
        free(fil->name);
        free(fil);
        return NULL;
    }

    parent->entries[parent->num_entries++] = (uintptr_t) fil;
    }

    return fil;
}

///////////////////////////////////////


struct inode* find_inode_by_name(struct inode* parent, char* name) {
    if (!parent || !parent->is_directory || parent->num_entries==0) {
        return NULL;
    }

    struct inode* target = NULL;

    for (int i = 0; i < parent->num_entries; i++) {
        struct inode* entry = (struct inode*) parent->entries[i]; // cast from uintptr_t
        if (entry && strcmp(entry->name, name) == 0) {
            target = entry;
            break; // gå ut og avslutt
        }
    }

    return target;
}

/////////////////


void fs_shutdown(struct inode* node){

    for (int i = 0; i < node->num_entries; i++) {
        struct inode* entry = (struct inode*) node->entries[i];


        if (entry) {
            if (entry->is_directory) {
                fs_shutdown(entry); // gå inn med rekursjon

            } else { // den er en fil inode , har ikke allokert noe til entries å 4096 bytes så ? gå alloker til dem og husk å free entries til fil inode* her
                free(entry->entries);
                free(entry->name);

                /*for (int i = 0; i < node->num_entries; i++) {
                    free_block(i);
                }*/

                free(entry);
            }
        }
    }

    free(node->entries); // realloc
    free(node->name); // malloc
    free(node); //
}

/////////////////
static int indent = 0;

void debug_fs( struct inode* node )
{
    if( node == NULL ) return;
    for( int i=0; i<indent; i++ )
        printf("  ");
    if( node->is_directory )
    {
        printf("%s (id %d)\n", node->name, node->id );
        indent++;
        for( int i=0; i<node->num_entries; i++ )
        {
            struct inode* child = (struct inode*)node->entries[i];
            debug_fs( child );
        }
        indent--;
    }
    else
    {
        printf("%s (id %d size %db blocks ", node->name, node->id, node->filesize );
        for( int i=0; i<node->num_entries; i++ )
        {
            printf("%d ", (int)node->entries[i]);
        }
        printf(")\n");
    }
}
