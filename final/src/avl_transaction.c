#include "../libs/avl_transaction.h"


int max(int a, int b){
    return (a > b) ? a : b;
}
int getID(char *key){
    char id[20];
    memset(id, '\0', 20);
    for (int i = 0; i < strlen(key); i++){
        if (key[i] == ' ')
            return atoi(id);
        else
            id[i] = key[i];
    }
    return -1;
}
int heightTransaction(struct TransactionNode *N){
    if (N == NULL)
        return 0;
    return N->height;
}

// Create a node 
struct TransactionNode *newTransactionNode(char *line){
    struct TransactionNode *node = (struct TransactionNode *)malloc(sizeof(struct TransactionNode));
    node->id = atoi(strtok_r(line, " ", &line));
    char *type = strtok_r(line, " ", &line);
    memset(node->type, '\0', 20);
    for (int i = 0; i < strlen(type); i++)
        node->type[i] = type[i];
    char *street = strtok_r(line, " ", &line);
    memset(node->street, '\0', 30);
    for (int i = 0; i < strlen(street); i++)
        node->street[i] = street[i];
    node->area = atoi(strtok_r(line, " ", &line));
    node->price = atoi(strtok_r(line, " ", &line));
    node->leftNode = NULL;
    node->rightNode = NULL;
    node->height = 0;
    return (node);
}

// Right rotate
struct TransactionNode *rightRotateTransaction(struct TransactionNode *y){
    struct TransactionNode *x = y->leftNode;
    struct TransactionNode *T2 = x->rightNode;

    x->rightNode = y;
    y->leftNode = T2;

    y->height = max(heightTransaction(y->leftNode), heightTransaction(y->rightNode)) + 1;
    x->height = max(heightTransaction(x->leftNode), heightTransaction(x->rightNode)) + 1;

    return x;
}

// Left rotate
struct TransactionNode *leftRotateTransaction(struct TransactionNode *x){
    struct TransactionNode *y = x->rightNode;
    struct TransactionNode *T2 = y->leftNode;

    y->leftNode = x;
    x->rightNode = T2;

    x->height = max(heightTransaction(x->leftNode), heightTransaction(x->rightNode)) + 1;
    y->height = max(heightTransaction(y->leftNode), heightTransaction(y->rightNode)) + 1;

    return y;
}

// Get the balance factor
int getBalanceTransaction(struct TransactionNode *N){
    if (N == NULL)
        return 0;
    return heightTransaction(N->leftNode) - heightTransaction(N->rightNode);
}

// Insert node
struct TransactionNode *insertNodeTransaction(struct TransactionNode *root, char *key){
    // Find the correct position to insertNode the node and insertNode it
    if (root == NULL)
        return (newTransactionNode(key));
    int id = getID(key);
    int cmp = id - (root->id);
    if (cmp < 0)
        root->leftNode = insertNodeTransaction(root->leftNode, key);
    else if (cmp > 0)
        root->rightNode = insertNodeTransaction(root->rightNode, key);
    else
        return root;
    // Update the balance factor of each node and Balance the tree
    root->height = 1 + max(heightTransaction(root->leftNode), heightTransaction(root->rightNode));

    int balance = getBalanceTransaction(root);
    if (balance > 1 && id - (root->leftNode->id) < 0) // key < node->leftNode->key
        return rightRotateTransaction(root);

    if (balance < -1 && id - (root->rightNode->id) > 0) // key > node->rightNode->key
        return leftRotateTransaction(root);

    if (balance > 1 && id - (root->leftNode->id) > 0){ // key > node->leftNode->key
        root->leftNode = leftRotateTransaction(root->leftNode);
        return rightRotateTransaction(root);
    }

    if (balance < -1 && id - (root->rightNode->id) > 0){ // key<node->rightNode->key
        root->rightNode = rightRotateTransaction(root->rightNode);
        return leftRotateTransaction(root);
    }

    return root;
}

struct TransactionNode *minValueTransactionNode(struct TransactionNode *node){
    struct TransactionNode *current = node;
    while (current->leftNode != NULL)
        current = current->leftNode;

    return current;
}

void freeNodeTransaction(struct TransactionNode *root){
    if (root == NULL)
        return;
    freeNodeTransaction(root->leftNode);
    freeNodeTransaction(root->rightNode);
    free(root);
}

void printPreOrderTransaction(struct TransactionNode *root){
    if (root != NULL){
        printf("|\t|-------\\->%d\n", root->id);
        printf("|\t|\t|-------\\->%s\n", root->type);
        printf("|\t|\t|-------\\->%s\n", root->street);
        printf("|\t|\t|-------\\->%d\n", root->area);
        printf("|\t|\t|-------\\->%d\n", root->price);
        printPreOrderTransaction(root->leftNode);
        printPreOrderTransaction(root->rightNode);
    }
}

int searchTransaction(struct TransactionNode *root, struct Request *req){
    int count = 0;
    if (root == NULL||req==NULL)
        return count;

    count += searchTransaction(root->leftNode, req);
    count += searchTransaction(root->rightNode, req);
    //printf("Current: %s , Req: %s\n ", root->type, req->type);
    if (strcmp(root->type, req->type) == 0)//{printf("T-id: %d\n", root->id);
        count++;//}

    return count;
}