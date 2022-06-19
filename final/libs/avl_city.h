#if !defined(AVL_CITY_H)
#define AVL_CITY_H
#include "avl_date.h"

struct CityNode{
    char city[30];
    int height;
    struct CityNode *leftNode;
    struct CityNode *rightNode;
    struct DateNode *dates;
};

int getBalanceCity(struct CityNode *N); // Get the balance factor
int heightCity(struct CityNode *N);     // Calculate height
int searchCity(struct CityNode *root, struct Request *req); // Search City
struct CityNode *insertNodeCity(struct CityNode *root, struct CityNode *node); // Insert node
struct CityNode *leftRotateCity(struct CityNode *x);                           // Left rotate
struct CityNode *minValueCityNode(struct CityNode *node);                      // Returns min value Node
struct CityNode *newCityNode(char *city);                                      // Create a node
struct CityNode *rightRotateCity(struct CityNode *y);                          // Right rotate
void freeNodeCity(struct CityNode *root);                                      // Free Citys
void printPreOrderCity(struct CityNode *root);                                 // Print the tree
#endif // AVL_CITY_H