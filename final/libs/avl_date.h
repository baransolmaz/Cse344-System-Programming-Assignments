#if !defined AVL_DATE_H
#define AVL_DATE_H
#include "../libs/avl_transaction.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
struct DateNode{
    char date[11];
    struct TransactionNode *transactions;
    struct DateNode *leftNode;
    struct DateNode *rightNode;
    int height;
};

int cmpDate(char *key, char *date);                                            // Compares Dates
int getBalanceDate(struct DateNode *N);                                        // Get the balance factor
int getDate(char *date, int in);                                               // Returns DD/MM/YYYY -> in= 0/1/2
int heightDate(struct DateNode *N);                                            // Calculate height
int searchDate(struct DateNode *root, struct Request *req);                    // Search Date
struct DateNode *insertNodeDate(struct DateNode *root, struct DateNode *node); // Insert node
struct DateNode *leftRotateDate(struct DateNode *x);                           // Left rotate
struct DateNode *minValueDateNode(struct DateNode *node);                      // Returns min value Node
struct DateNode *newDateNode(char *line);                                      // Create a node
struct DateNode *rightRotateDate(struct DateNode *y);                          // Right rotate
void freeNodeDate(struct DateNode *root);                                      // Free Dates
void printPreOrderDate(struct DateNode *root);                                 // Print the tree

#endif // AVL_DATE_H