#include <iostream>
#include <vector>
#include <functional>
#include <mutex>
#include <shared_mutex> //Thread safety capabilities
#include "../BPA/bpa.cpp"

using namespace std;

template <typename KeyType, typename ValueType>
class BPTreeNode
{
    mutable shared_mutex rw_lock; //R/W mutex for handling thread safety
    BPTreeNode<KeyType, ValueType>* parent = nullptr;
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
        curr_node->rw_lock.lock_shared();

        while (typeid(curr_node).name() != "BPTreeNode_Leaf") {
            curr_node = curr_node->children[curr_node->children.size() - 1];
            for (int i = 0; i < curr_node->keys.size(); i++) {
                if (key < curr_node->keys[i]) {
                    curr_node->rw_lock.lock_shared(); // Hand-over-hand locking
                    curr_node->parent->rw_lock.unlock_shared();

                    curr_node = curr_node->children[i];
                    break;
                }
            }
        }

        return curr_node;
    }

    // Helper method for gaining all locks down to the internal node being split
    void pess_descent(BPTreeNode_Internal<KeyType, ValueType>* node) {
        if (node->parent != nullptr)
            pess_descent(node->parent);
        node->rw_lock.lock();
    }

public:
    //Constructor
    BPTree (int order, int log_size, int num_blocks, int block_size) : order(order), bpa_log_size(log_size), bpa_num_blocks(num_blocks), bpa_block_size(block_size) {
        root = new BPTreeNode_Leaf<KeyType, ValueType>(log_size, num_blocks, block_size);
    }

    void insert(KeyType key, ValueType value) {
        BPTreeNode_Leaf<KeyType, ValueType>* leaf = traverse(key);

        leaf->parent->rw_lock.lock_shared();
        leaf->rw_lock.lock(); //Write lock

        //Can insert into the BPA without issues
        if (leaf->num_elts < leaf->bpa.total_size) {
            leaf->bpa.insert(key, value);
            leaf->num_elts++;
            leaf->rw_lock.unlock(); //Write lock
            return;
        }

        // BPA is full, so must split it
        
        vector<ElementBPA<KeyType, ValueType> *> bpa_elts;

        for (int i = 0; i < leaf->num_elts; i++)
            bpa_elts.push_back(&(leaf->bpa.header_ptr[i]));
        sort(bpa_elts.front(), bpa_elts.back());

        BPTreeNode_Leaf<KeyType, ValueType>* leaf_one = new BPTreeNode_Leaf<KeyType, ValueType>(bpa_log_size, bpa_num_blocks, bpa_block_size);
        BPTreeNode_Leaf<KeyType, ValueType>* leaf_two = new BPTreeNode_Leaf<KeyType, ValueType>(bpa_log_size, bpa_num_blocks, bpa_block_size);

        // Edge case where leaf is also the root, create a new internal node as the root
        if (leaf->parent == nullptr) {
            BPTreeNode_Internal<KeyType, ValueType>* new_node = new BPTreeNode_Internal<KeyType, ValueType>(order);
            new_node->rw_lock.lock();
            root = new_node;

            leaf_one->rw_lock.lock();
            leaf_two->rw_lock.lock();

            leaf_one->next = leaf_two;
            leaf_one->parent = root;
            leaf_two->prev = leaf_one;
            leaf_two->parent = root;

            root->children.push_back(leaf_one);
            root->children.push_back(leaf_two);

            root->keys.push_back(bpa_elts[bpa_elts.size() / 2].key);

            root->rw_lock.unlock();
        }
        // Normal case, no need to create new root node
        else {
            leaf->rw_lock.lock();
            leaf_one->rw_lock.lock();
            leaf_two->rw_lock.lock();

            leaf_one->prev = leaf->prev;
            leaf_one->parent = leaf->parent;
            leaf_one->next = leaf_two;
            leaf_one->prev = leaf_one;
            leaf_one->parent = leaf->parent;
            leaf_one->next = leaf->next;

            // Get the index at which the original leaf was at
            auto it = find(leaf->parent->children.begin(), leaf->parent->children.end(), leaf);

            leaf->parent->children.assign(it, leaf_one);
            leaf->parent->children.insert(it+1, leaf_two);

            //Insert the first key in the righht leaf as the new divider key
            leaf->parent->keys.insert(leaf->parent->keys.begin() + (it - leaf->parent->children.begin()), leaf_two->bpa.header_ptr[0].key);

            leaf->rw_lock.unlock();
            leaf_one->rw_lock.unlock();
            leaf_two->rw_lock.unlock();

            //Uh oh time for a split!!!
            if (leaf->parent->children.size() == order) {
                split_internal_node(leaf->parent, true);
            }
        }

        //Redistributing the values from the original BPA into the two in a way such that
        for(int i = 0; i < bpa_elts.size() / 2; i++) {
            leaf_one->bpa.insert(bpa_elts[(i % bpa_num_blocks) * bpa_num_blocks + (i / bpa_num_blocks)]);
            leaf_one->num_elts++;
            leaf_two->bpa.insert(bpa_elts[(i % bpa_num_blocks) * bpa_num_blocks + (i / bpa_num_blocks) + bpa_elts.size() / 2]);
            leaf_two->num_elts++;
        }

        if (bpa_elts.size() % 2 == 1) {
            leaf_two->bpa.insert(bpa_elts[bpa_elts.size() - 1]);
            leaf_two->num_elts++;
        }

        delete leaf;
        // Retraverse tree to get new key for insertion
        leaf = traverse(key);    

        leaf->rw_lock.lock();
        leaf->bpa.insert(key, value); 
        leaf->num_elts++;
        leaf->rw_lock.unlock();
    }

    void split_internal_node(BPTreeNode_Internal<KeyType, ValueType>* node, bool first_split = false) {

        if (first_split)
            pess_traverse(node);
        int splitIndex = node->keys.size() / 2;

        BPTreeNode_Internal<KeyType, ValueType> *new_node = new BPTreeNode_Internal<KeyType, ValueType>(order);
        new_node->rw_lock.lock();

        // Root node, need to create a new root
        if(node->parent == nullptr) {
            root = new BPTreeNode_Internal<KeyType, ValueType>(order);

            node->parent = root;
            new_node->parent = root;

            root->children.push_back(node);
            root->children.push_back(new_node);

            root->keys.push_back(node->keys[splitIndex]);
        }
        else {
            // Get the index at which the original leaf was at
            auto it = find(node->parent->children.begin(), node->parent->children.end(), node);

            node->parent->children.assign(it, node);
            node->parent->children.insert(it+1, new_node);

            new_node->parent = node->parent;

            //Insert the first key in the righht leaf as the new divider key
            node->parent->keys.insert(node->parent->keys.begin() + (it - node->parent->children.begin()), node->keys[splitIndex]);
        }

        //For  efficiency we just reuse the old leaf
        new_node->keys.assign(node->keys.begin() + splitIndex + 1, node->keys.end());
        new_node->children.assign(node->children.begin() + splitIndex+1, node->children.end());

        node->keys.erase(node->keys.begin() + splitIndex, node->keys.end());
        node->values.erase(node->children.begin() + splitIndex+1, node->children.end());

        node->rw_lock.unlock();
        new_node->rw_lock.unlock();

        //Uh oh time for a split!!! again!!!!!!!!!!!!!!!
        if (node->parent->children.size() == order)
            split_internal_node(node->parent);
        else { //Unlock the rest of the nodes up to the root
            while (new_node->parent != nullptr) { //if not root
                new_node = new_node->parent;
                new_node->rw_lock.unlock();
            }
        }
    }

    ValueType* find(KeyType key) {
        BPTreeNode_Leaf<KeyType, ValueType>* leaf = traverse(key);

        leaf->rw_lock.lock_shared();
        ValueType* val = leaf->bpa.find(key);
        leaf->rw_lock.unlock_shared();
        return val;
    }

    void iterate_range (int start, int length, function<ValueType(KeyType)> f) {
        int num_to_process = length;
        BPTreeNode_Leaf<KeyType, ValueType>* leaf = traverse(start);

        bool need_write = true; //Check if we need to take a write lock here
        bool last_need_write;

        while (num_to_process > 0) {
            last_need_write = need_write;
            need_write = true;
            if (leaf->bpa.sorted_log) {
                for(int i = 1; i < bpa_num_blocks; i++) {
                    if (leaf->bpa.header_ptr[i] < start && ! leaf->bpa.sorted_blocks[i])
                        break;
                }

                need_write = false;
            }

            (need_write) ? leaf->rw_lock.lock() : leaf->rw_lock.lock_shared();
            if (num_to_process != length)
                (last_need_write) ? leaf->prev->rw_lock.unlock() : leaf->prev->rw_lock.unlock_shared();

            num_to_process -= leaf->bpa.iterate_range(start, num_to_process, f);

            if (leaf-> next == nullptr)
                break;
            leaf = leaf->next;
        }
        (need_write) ? leaf->rw_lock.unlock() : leaf->rw_lock.unlock_shared();
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