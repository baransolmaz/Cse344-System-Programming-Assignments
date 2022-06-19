#include "../libs/avl_city.h"

int heightCity(struct CityNode *N){
    if (N == NULL)
        return 0;
    return N->height;
}
struct CityNode *newCityNode(char *city){
    struct CityNode *node = (struct CityNode *)malloc(sizeof(struct CityNode));
    memset(node->city, '\0', 30);
    for (int i = 0; i < strlen(city); i++)
        node->city[i] = city[i];

    node->dates = NULL;
    node->leftNode = NULL;
    node->rightNode = NULL;
    node->height = 0;
    return (node);
}
struct CityNode *rightRotateCity(struct CityNode *y){
    struct CityNode *x = y->leftNode;
    struct CityNode *T2 = x->rightNode;

    x->rightNode = y;
    y->leftNode = T2;

    y->height = max(heightCity(y->leftNode), heightCity(y->rightNode)) + 1;
    x->height = max(heightCity(x->leftNode), heightCity(x->rightNode)) + 1;

    return x;
}
struct CityNode *leftRotateCity(struct CityNode *x){
    struct CityNode *y = x->rightNode;
    struct CityNode *T2 = y->leftNode;

    y->leftNode = x;
    x->rightNode = T2;

    x->height = max(heightCity(x->leftNode), heightCity(x->rightNode)) + 1;
    y->height = max(heightCity(y->leftNode), heightCity(y->rightNode)) + 1;

    return y;
}
int getBalanceCity(struct CityNode *N){
    if (N == NULL)
        return 0;
    return heightCity(N->leftNode) - heightCity(N->rightNode);
}
struct CityNode *insertNodeCity(struct CityNode *root, struct CityNode *node){
    // Find the correct position to insertNode the node and insertNode it
    if (root == NULL)
        return node;
    int cmp = strcmp(node->city, root->city);
    if (cmp < 0)
        root->leftNode = insertNodeCity(root->leftNode, node);
    else if (cmp > 0)
        root->rightNode = insertNodeCity(root->rightNode, node);
    else
        return root;

    // Update the balance factor of each node and Balance the tree
    root->height = 1 + max(heightCity(root->leftNode), heightCity(root->rightNode));

    int balance = getBalanceCity(root);
    if (balance > 1 && strcmp(node->city, root->leftNode->city) < 0) // key < node->leftNode->key
        return rightRotateCity(root);

    if (balance < -1 && strcmp(node->city, root->rightNode->city) > 0) // key > node->rightNode->key
        return leftRotateCity(root);

    if (balance > 1 && strcmp(node->city, root->leftNode->city) > 0){ // key > node->leftNode->key
        root->leftNode = leftRotateCity(root->leftNode);
        return rightRotateCity(root);
    }

    if (balance < -1 && strcmp(node->city, root->rightNode->city) > 0){ // key<node->rightNode->key
        root->rightNode = rightRotateCity(root->rightNode);
        return leftRotateCity(root);
    }

    return root;
}
struct CityNode *minValueCityNode(struct CityNode *node){
    struct CityNode *current = node;
    while (current->leftNode != NULL)
        current = current->leftNode;

    return current;
}
void freeNodeCity(struct CityNode *root){
    if (root == NULL)
        return;
    freeNodeDate(root->dates);
    freeNodeCity(root->leftNode);
    freeNodeCity(root->rightNode);
    free(root);
}
void printPreOrderCity(struct CityNode *root){
    if (root != NULL){
        printf("|%s\n", root->city);
        printPreOrderDate(root->dates);
        printPreOrderCity(root->leftNode);
        printPreOrderCity(root->rightNode);
    }
}
int searchCity(struct CityNode *root, struct Request *req){
    int count = 0;
    if (root == NULL || req==NULL)
        return count;

    if (req->checkCity == 1){
        if (strncmp(root->city, req->city, strlen(req->city)) > 0)
            count += searchCity(root->leftNode, req);
        if (strncmp(root->city, req->city, strlen(req->city)) < 0)
            count += searchCity(root->rightNode, req);
        if (strncmp(root->city, req->city, strlen(req->city)) == 0)
            count += searchDate(root->dates, req);
        return count;
    }
    count += searchDate(root->dates, req);
    count += searchCity(root->leftNode, req);
    count += searchCity(root->rightNode, req);

    return count;
}