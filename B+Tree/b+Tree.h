#include <iostream>
#include <vector>
#include <queue>
#include "b+Treenode.h"

using namespace std;

// B+ tree class
template <typename KeyType, typename ValueType>
class BPlusTree
{
private:
    BPlusTreeNode<KeyType, ValueType> *root;
    int order; // Order of the B+ tree

public:
    // Constructor
    BPlusTree(int order) : root(nullptr), order(order) {}

    // Destructor
    ~BPlusTree()
    {
        if (root != nullptr)
        {
            delete root;
        }
    }

    // Public method to insert a key-value pair into the tree
    void insert(const KeyType &key, const ValueType &value)
    {
        if (root == nullptr)
        {
            // Create the root if it doesn't exist
            root = new BPlusTreeNode<KeyType, ValueType>(true);
            root->keys.push_back(key);
            root->values.push_back(value);
        }
        else
        {
            // Call the recursive insert method to handle the insertion
            insertRecursive(root, key, value);
        }
    }

    void levelOrderTraversal() const
    {
        if (root != nullptr)
        {
            std::queue<BPlusTreeNode<KeyType, ValueType> *> nodeQueue;
            nodeQueue.push(root);

            while (!nodeQueue.empty())
            {
                size_t levelSize = nodeQueue.size();

                for (size_t i = 0; i < levelSize; ++i)
                {
                    BPlusTreeNode<KeyType, ValueType> *currentNode = nodeQueue.front();
                    nodeQueue.pop();

                    // Print or process the keys of the current node
                    for (const auto &key : currentNode->keys)
                    {
                        std::cout << key << " ";
                    }

                    // Enqueue the children of the current node
                    for (const auto &child : currentNode->children)
                    {
                        if (child != nullptr)
                        {
                            nodeQueue.push(child);
                        }
                    }
                }

                std::cout << std::endl; // Move to the next level
            }
        }
    }

    // Public method for key deletion
    void remove(const KeyType &key)
    {
        if (root == nullptr)
        {
            std::cout << "Tree is empty. Cannot delete key.\n";
            return;
        }

        // Call the recursive delete method
        removeRecursive(root, key);

        // After deletion, you might want to handle cases where the root becomes empty
        if (root->keys.empty())
        {
            // Adjust the root if necessary
            // This could happen if the last key was deleted
            BPlusTreeNode<KeyType, ValueType> *newRoot = root->children[0];
            delete root;
            root = newRoot;
            if (root != nullptr)
            {
                root->parent = nullptr;
            }
        }
    }

private:
    // Private method to perform the recursive insertion
    void
    insertRecursive(BPlusTreeNode<KeyType, ValueType> *node, const KeyType &key, const ValueType &value)
    {
        if (node->isLeaf)
        {
            // Insert into the leaf node
            insertIntoLeaf(node, key, value);
        }
        else
        {
            // Find the child to traverse to
            int i = 0;
            while (i < node->keys.size() && key > node->keys[i])
            {
                ++i;
            }
            // Recursively insert into the appropriate child
            insertRecursive(node->children[i], key, value);
        }
    }

    // Private method to insert into a leaf node
    void insertIntoLeaf(BPlusTreeNode<KeyType, ValueType> *leaf, const KeyType &key, const ValueType &value)
    {
        // Insert into the leaf node and maintain the sorted order
        auto it = std::lower_bound(leaf->keys.begin(), leaf->keys.end(), key);
        int index = it - leaf->keys.begin();

        leaf->keys.insert(it, key);
        leaf->values.insert(leaf->values.begin() + index, value);

        // Update the linked list pointers
        if (leaf->next != nullptr)
        {
            leaf->next->prev = leaf;
        }

        BPlusTreeNode<KeyType, ValueType> *newLeaf = nullptr;
        if (leaf->keys.size() > order)
        {
            newLeaf = splitLeaf(leaf);
            insertIntoParent(leaf, newLeaf->keys[0], newLeaf);
        }
    }

    // Private method to split a leaf node during insertion
    BPlusTreeNode<KeyType, ValueType> *splitLeaf(BPlusTreeNode<KeyType, ValueType> *leaf)
    {
        int splitIndex = leaf->keys.size() / 2;

        BPlusTreeNode<KeyType, ValueType> *newLeaf = new BPlusTreeNode<KeyType, ValueType>(true);

        newLeaf->keys.assign(leaf->keys.begin() + splitIndex, leaf->keys.end());
        newLeaf->values.assign(leaf->values.begin() + splitIndex, leaf->values.end());

        leaf->keys.erase(leaf->keys.begin() + splitIndex, leaf->keys.end());
        leaf->values.erase(leaf->values.begin() + splitIndex, leaf->values.end());

        newLeaf->next = leaf->next;
        leaf->next = newLeaf;

        return newLeaf;
    }

    // Private method to insert into the parent node after a leaf node split
    void insertIntoParent(BPlusTreeNode<KeyType, ValueType> *leftChild, const KeyType &key, BPlusTreeNode<KeyType, ValueType> *rightChild)
    {
        if (leftChild->parent == nullptr)
        {
            // Create a new root if the current node is the root
            BPlusTreeNode<KeyType, ValueType> *newRoot = new BPlusTreeNode<KeyType, ValueType>();
            newRoot->keys.push_back(key);
            newRoot->children.push_back(leftChild);
            newRoot->children.push_back(rightChild);

            leftChild->parent = newRoot;
            rightChild->parent = newRoot;

            root = newRoot; // Update the tree's root
        }
        else
        {
            // Find the parent node to insert the key and right child
            BPlusTreeNode<KeyType, ValueType> *parent = leftChild->parent;
            auto it = std::lower_bound(parent->keys.begin(), parent->keys.end(), key);
            int index = it - parent->keys.begin();

            // Insert the key and right child into the parent node
            parent->keys.insert(it, key);
            parent->children.insert(parent->children.begin() + index + 1, rightChild);

            rightChild->parent = parent;

            // Check if the parent node needs to be split
            if (parent->keys.size() > order)
            {
                BPlusTreeNode<KeyType, ValueType> *newInternalNode = splitInternal(parent);
                insertIntoParent(parent, newInternalNode->keys[0], newInternalNode);
            }
        }
    }

    // Private method to split an internal (non-leaf) node
    BPlusTreeNode<KeyType, ValueType> *splitInternal(BPlusTreeNode<KeyType, ValueType> *node)
    {
        int splitIndex = node->keys.size() / 2;

        BPlusTreeNode<KeyType, ValueType> *newInternalNode = new BPlusTreeNode<KeyType, ValueType>();
        newInternalNode->keys.assign(node->keys.begin() + splitIndex + 1, node->keys.end());
        newInternalNode->children.assign(node->children.begin() + splitIndex + 1, node->children.end());

        node->keys.erase(node->keys.begin() + splitIndex, node->keys.end());
        node->children.erase(node->children.begin() + splitIndex + 1, node->children.end());

        for (auto child : newInternalNode->children)
        {
            if (child != nullptr)
            {
                child->parent = newInternalNode;
            }
        }

        return newInternalNode;
    }

    // Add other helper methods as needed
};
