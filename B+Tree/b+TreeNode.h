#include <iostream>
#include <vector>
#include <queue>
#include "../BPA/bpa.cpp"

using namespace std;

// Node structure for the B+ tree
template <typename KeyType, typename ValueType>
struct BPlusTreeNode
{
    bool isLeaf;                           // Indicates whether the node is a leaf or internal node
    BPA bpa;
    std::vector<BPlusTreeNode *> children; // Add children vector for internal nodes

    BPlusTreeNode *parent; // Pointer to the parent node
    BPlusTreeNode *next;   // Pointer to the next node in the linked list (only in leaf nodes)
    BPlusTreeNode *prev;   // Points to the previous leaf node

    // Constructor with an optional parameter to specify whether the node is a leaf
    BPlusTreeNode(bool leaf = false, int log_size, int num_blocks, int block_size) : isLeaf(leaf), bpa(log_size, num_blocks, block_size), parent(nullptr), next(nullptr) {}

    // Destructor to recursively delete nodes
    ~BPlusTreeNode()
    {
        for (auto child : children)
        {
            delete child;
        }
    }
};
