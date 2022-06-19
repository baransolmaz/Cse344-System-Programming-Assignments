#if !defined(AVL_TRANSACTION_H)
#define AVL_TRANSACTION_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

struct Request{
    char *city;
    char *end;
    char *start;
    char *type;
    int checkCity;
};

struct TransactionNode{
    char street[30];
    char type[20];
    int area;
    int height;
    int id;
    int price;
    struct TransactionNode *leftNode;
    struct TransactionNode *rightNode;
};

int getBalanceTransaction(struct TransactionNode *N);                                   // Get the balance factor
int getID(char *key);                                                                   // Returns Transaction ID
int heightTransaction(struct TransactionNode *N);                                       // Calculate height
int max(int a, int b);                                                                  // Returns Max number
int searchTransaction(struct TransactionNode *root, struct Request *req);               // Search Transaction
struct TransactionNode *insertNodeTransaction(struct TransactionNode *root, char *key); // Insert node
struct TransactionNode *leftRotateTransaction(struct TransactionNode *x);               // Left rotate
struct TransactionNode *minValueTransactionNode(struct TransactionNode *node);          // Returns min value Node
struct TransactionNode *newTransactionNode(char *line);                                 // Create a node
struct TransactionNode *rightRotateTransaction(struct TransactionNode *y);              // Right rotate
void freeNodeTransaction(struct TransactionNode *root);                                 // Free Transactions
void printPreOrderTransaction(struct TransactionNode *root);                            // Print the tree

#endif // AVL_TRANSACTION_H