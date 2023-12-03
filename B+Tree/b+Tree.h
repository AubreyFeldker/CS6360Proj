#include <iostream>
#include <vector>
#include <queue>
#include <functional> // Add this include for std::function
#include "b+TreeNode.h"

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

                    // Print or process the keys and values of the current node
                    if (currentNode->isLeaf)
                    {
                        for (size_t j = 0; j < currentNode->keys.size(); ++j)
                        {
                            std::cout << "(" << currentNode->keys[j] << ", " << currentNode->values[j] << ") ";
                        }
                    }
                    else
                    {
                        for (size_t j = 0; j < currentNode->keys.size(); ++j)
                        {
                            std::cout << "(" << currentNode->keys[j] << ") ";
                        }
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

    // Function to find the smallest key greater than or equal to the given search key
    BPlusTreeNode<KeyType, ValueType> *find(const KeyType &searchKey)
    {
        if (root == nullptr)
        {
            cout << "Tree is empty." << endl;
            return nullptr; // Assuming ValueType has a default constructor
        }

        BPlusTreeNode<KeyType, ValueType> *current = findLeafNode(searchKey);
        int index = findKeyIndex(current, searchKey);

        if (index == -1)
        {
            // Key not found in the leaf node, move to the next leaf node
            current = current->next;
            index = 0;
        }

        if (current != nullptr && index < current->keys.size())
        {
            return current;
        }

        cout << "Key not found." << endl;
        return nullptr; // Assuming ValueType has a default constructor
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

    // Function to iterate over a range of elements in order (by key)
    void iterate_range(const KeyType &start, int length, std::function<void(const KeyType &, ValueType &)> f)
    {
        if (root == nullptr)
        {
            cout << "Tree is empty." << endl;
            return;
        }

        BPlusTreeNode<KeyType, ValueType> *current = findLeafNode(start);
        int index = findKeyIndex(current, start);

        // Iterate through the leaf nodes and apply the function to the specified range
        while (current != nullptr && length > 0)
        {
            for (int i = index; i < current->keys.size() && length > 0; ++i)
            {
                f(current->keys[i], current->values[i]);
                --length;
            }

            // Move to the next leaf node
            current = current->next;
            index = 0;
        }
    }

    // Public method to apply a function to all elements with keys in the range [start, end)
    void map_range(const KeyType &start, const KeyType &end, std::function<void(const KeyType &, ValueType &)> f)
    {
        map_range_recursive(root, start, end, f);
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
        if (leaf->keys.size() >= order)
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
            if (parent->keys.size() >= order)
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

    // Helper function to find the leaf node where the key might be located
    BPlusTreeNode<KeyType, ValueType> *findLeafNode(const KeyType &searchKey)
    {
        BPlusTreeNode<KeyType, ValueType> *current = root;

        while (current != nullptr && !current->isLeaf)
        {
            int index = 0;
            while (index < current->keys.size() && searchKey >= current->keys[index])
            {
                index++;
            }
            current = current->children[index];
        }
        return current;
    }
    // Helper function to find the index of the key in a node
    int findKeyIndex(BPlusTreeNode<KeyType, ValueType> *node, const KeyType &searchKey)
    {
        int index = 0;
        while (index < node->keys.size() && searchKey > node->keys[index])
        {
            index++;
        }

        return index == node->keys.size() ? -1 : index;
    }
    // Private method for recursive range mapping
    void map_range_recursive(BPlusTreeNode<KeyType, ValueType> *node, const KeyType &start, const KeyType &end, std::function<void(const KeyType &, ValueType &)> f)
    {
        if (!node)
        {
            return;
        }

        // If the node is an internal node, recursively traverse its children
        if (!node->isLeaf)
        {
            // Find the child to traverse
            auto childIt = std::upper_bound(node->keys.begin(), node->keys.end(), start);
            int childIndex = std::distance(node->keys.begin(), childIt);

            // Traverse the child
            map_range_recursive(node->children[childIndex], start, end, f);
        }
        else
        {
            // If the node is a leaf, apply the function to keys in the specified range
            auto keyIt = std::lower_bound(node->keys.begin(), node->keys.end(), start);

            while (keyIt != node->keys.end() && *keyIt < end)
            {
                // Apply the function to the key and its associated value
                f(*keyIt, node->values[std::distance(node->keys.begin(), keyIt)]);
                ++keyIt;
            }
        }
    }
};
