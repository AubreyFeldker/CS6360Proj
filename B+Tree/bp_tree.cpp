#include <iostream>
#include <vector>
#include <functional> // Add this include for std::function
#include "../BPA/bpa.cpp"

using namespace std;

template <typename KeyType, typename ValueType>
class BPTreeNode
{
    BPTreeNode<KeyType, ValueType>* parent = nullptr;

    // Insert r/w locks here
};

template <typename KeyType, typename ValueType>
class BPTreeNode_Internal: public BPTreeNode<KeyType, ValueType>
{
    int order;

    vector<BPTreeNode<KeyType, ValueType>*> children;
    
    vector<KeyType> keys;

    //Coinstructor
    BPTreeNode_Internal(int order) : order(order) {}
};

template <typename KeyType, typename ValueType>
class BPTreeNode_Leaf: public BPTreeNode<KeyType, ValueType>
{
    BPA<KeyType, ValueType> bpa;

    BPTreeNode_Leaf<KeyType, ValueType>* prev = nullptr; //Previous leaf node
    BPTreeNode_Leaf<KeyType, ValueType>* next = nullptr; //Next leaf node

    int num_elts = 0; //Size of the number of elements in the array, including duplicates

    //Constructor
    BPTreeNode_Leaf(int log_size, int num_blocks, int block_size) : bpa(log_size, num_blocks, block_size) {}
};


template <typename KeyType, typename ValueType>
class BPTree {
private:
    BPTreeNode<KeyType, ValueType> *root;
    int order; // Order of the B+ tree

    int bpa_log_size;
    int bpa_num_blocks;
    int bpa_block_size;

    // Helper method to traverse tree until you reach a leaf node
    BPTreeNode_Leaf<KeyType, ValueType>* traverse(KeyType key) {
        BPTreeNode<KeyType, ValueType>* curr_node = root;

        while (typeid(*curr_node).name != "BPTreeNode_Leaf") {
            curr_node = curr_node->children[curr_node->children.size() - 1];
            for (int i = 0; i < curr_node->keys.size(); i++) {
                if (key < curr_node->keys[i]) {
                    curr_node = curr_node->children[i];
                    break;
                }
            }
        }

        return curr_node;
    }

public:
    //Constructor
    BPTree (int order, int log_size, int num_blocks, int block_size) : order(order), bpa_log_size(log_size), bpa_num_blocks(num_blocks), bpa_block_size(block_size) {
        root = new BPTreeNode_Internal();
    }

    void insert(KeyType key, ValueType value) {
        BPTreeNode_Leaf<KeyType, ValueType>* leaf = traverse(key);

        if (leaf->num_elts >= leaf->bpa.total_size) {
            //Spilt the BPA
        }

        leaf->bpa.insert(key, value);
        leaf->num_elts++;
    }

    ValueType* find(KeyType key) {
        BPTreeNode_Leaf<KeyType, ValueType>* leaf = traverse(key);

        return leaf->bpa.find(key);
    }

    void iterate_range (int start, int length, function<ValueType(KeyType)> f) {
        int num_to_process = length;
        BPTreeNode_Leaf<KeyType, ValueType>* leaf = traverse(start);

        while (num_to_process > 0) {
            num_to_process -= leaf->bpa.iterate_range(start, num_to_process, f);
            left = leaf->next;
        }
    }

    void map_range (int start, int length, function<ValueType(KeyType)> f) {
        int num_to_process = length;
        BPTreeNode_Leaf<KeyType, ValueType>* leaf = traverse(start);

        while (num_to_process > 0) {
            num_to_process -= leaf->bpa.map_range(start, num_to_process, f);
            left = leaf->next;
        }
    }
};