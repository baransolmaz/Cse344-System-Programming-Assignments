#include "../libs/avl_date.h"

int getDate(char *date, int in){
    char d[3][5];
    for (int i = 0; i < 3; i++)
        memset(d[i], '\0', 5);
    int row = 0, column = 0;
    for (int i = 0; i < strlen(date); i++)
        if (date[i] == '-'){
            row++;
            column = 0;
        }else{
            d[row][column] = date[i];
            column++;
        }
    
    return atoi(d[in]);
}
int cmpDate(char *key, char *date){
    int key_day = getDate(key, 0);
    int key_mounth = getDate(key, 1);
    int key_year = getDate(key, 2);
    int date_day = getDate(date, 0);
    int date_mounth = getDate(date, 1);
    int date_year = getDate(date, 2);
    if (key_year > date_year)
        return -1;
    else if (key_year < date_year)
        return 1;
    else if (key_mounth > date_mounth) // Same Year
        return -1;
    else if (key_mounth < date_mounth) // Same Year
        return 1;
    else if (key_day > date_day) // Same Year , same mounth
        return -1;
    else if (key_day < date_day) // Same Year , same mounth
        return 1;
    else
        return 0;
}
int heightDate(struct DateNode *N){
    if (N == NULL)
        return 0;
    return N->height;
}
struct DateNode *newDateNode(char *date){
    struct DateNode *node = (struct DateNode *)malloc(sizeof(struct DateNode));
    for (int i = 0; i < 10; i++)
        node->date[i] = date[i];
    node->date[10] = '\0';
    node->leftNode = NULL;
    node->rightNode = NULL;
    node->transactions = NULL;
    node->height = 0;
    return (node);
}
struct DateNode *rightRotateDate(struct DateNode *y){
    struct DateNode *x = y->leftNode;
    struct DateNode *T2 = x->rightNode;

    x->rightNode = y;
    y->leftNode = T2;

    y->height = max(heightDate(y->leftNode), heightDate(y->rightNode)) + 1;
    x->height = max(heightDate(x->leftNode), heightDate(x->rightNode)) + 1;

    return x;
}
struct DateNode *leftRotateDate(struct DateNode *x){
    struct DateNode *y = x->rightNode;
    struct DateNode *T2 = y->leftNode;

    y->leftNode = x;
    x->rightNode = T2;

    x->height = max(heightDate(x->leftNode), heightDate(x->rightNode)) + 1;
    y->height = max(heightDate(y->leftNode), heightDate(y->rightNode)) + 1;

    return y;
}
int getBalanceDate(struct DateNode *N){
    if (N == NULL)
        return 0;
    return heightDate(N->leftNode) - heightDate(N->rightNode);
}
struct DateNode *insertNodeDate(struct DateNode *root, struct DateNode *node){
    // Find the correct position to insertNode the node and insertNode it
    if (root == NULL)
        return node;

    int cmp = cmpDate(node->date, root->date);
    if (cmp < 0)
        root->leftNode = insertNodeDate(root->leftNode, node);
    else if (cmp > 0)
        root->rightNode = insertNodeDate(root->rightNode, node);
    else
        return root;
    // Update the balance factor of each node and Balance the tree
    root->height = 1 + max(heightDate(root->leftNode), heightDate(root->rightNode));

    int balance = getBalanceDate(root);
    if (balance > 1 && cmpDate(node->date, root->leftNode->date) < 0) // key < node->leftNode->key
        return rightRotateDate(root);

    if (balance < -1 && cmpDate(node->date, root->rightNode->date) > 0) // key > node->rightNode->key
        return leftRotateDate(root);

    if (balance > 1 && cmpDate(node->date, root->leftNode->date) > 0){ // key > node->leftNode->key
        root->leftNode = leftRotateDate(root->leftNode);
        return rightRotateDate(root);
    }

    if (balance < -1 && cmpDate(node->date, root->rightNode->date) > 0){ // key<node->rightNode->key
        root->rightNode = rightRotateDate(root->rightNode);
        return leftRotateDate(root);
    }
    return root;
}
struct DateNode *minValueDateNode(struct DateNode *node){
    struct DateNode *current = node;
    while (current->leftNode != NULL)
        current = current->leftNode;

    return current;
}
void freeNodeDate(struct DateNode *root){
    if (root == NULL)
        return;
    freeNodeTransaction(root->transactions);
    freeNodeDate(root->leftNode);
    freeNodeDate(root->rightNode);
    free(root);
}
int searchDate(struct DateNode *root, struct Request *req){
    int count = 0;
    if (root == NULL|| req==NULL)
        return count;

    count += searchDate(root->leftNode, req);
    count += searchDate(root->rightNode, req);

    if (cmpDate(req->start, root->date) >= 0 && cmpDate(req->end, root->date) <= 0)
        count += searchTransaction(root->transactions, req);

    return count;
}
void printPreOrderDate(struct DateNode *root){
    if (root != NULL){
        printf("|-------\\->%s\n", root->date);
        printPreOrderTransaction(root->transactions);
        printPreOrderDate(root->leftNode);
        printPreOrderDate(root->rightNode);
    }
}