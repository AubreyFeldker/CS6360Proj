#include <iostream>
#include <vector>
#include <queue>

using namespace std;

// Node structure for the B+ tree
template <typename KeyType, typename ValueType>
struct BPlusTreeNode
{
    bool isLeaf;                           // Indicates whether the node is a leaf or internal node

    std::vector<KeyType> keys;             // Keys stored in the node
    std::vector<ValueType> values;         // Values associated with keys (only in leaf nodes)
    std::vector<BPlusTreeNode *> children; // Add children vector for internal nodes

    BPlusTreeNode *parent; // Pointer to the parent node
    BPlusTreeNode *next;   // Pointer to the next node in the linked list (only in leaf nodes)
    BPlusTreeNode *prev;   // Points to the previous leaf node

    // Constructor with an optional parameter to specify whether the node is a leaf
    BPlusTreeNode(bool leaf = false) : isLeaf(leaf), parent(nullptr), next(nullptr) {}

    // Destructor to recursively delete nodes
    ~BPlusTreeNode()
    {
        for (auto child : children)
        {
            delete child;
        }
    }
};
