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
public:
    mutable shared_timed_mutex rw_lock; //R/W mutex for handling thread safety

    virtual ~BPTreeNode() = default;
};

template <typename KeyType, typename ValueType>
class BPTreeNode_Internal: public BPTreeNode<KeyType, ValueType>
{
public:
    vector<BPTreeNode<KeyType, ValueType>*> children;
    BPTreeNode_Internal<KeyType, ValueType>* parent = nullptr;
    
    vector<KeyType> keys;

    virtual ~BPTreeNode_Internal() = default;
};

template <typename KeyType, typename ValueType>
class BPTreeNode_Leaf: public BPTreeNode<KeyType, ValueType>
{
public:
    BPA<KeyType, ValueType> bpa;
    BPTreeNode_Internal<KeyType, ValueType>* parent = nullptr;

    BPTreeNode_Leaf<KeyType, ValueType>* prev = nullptr; //Previous leaf node
    BPTreeNode_Leaf<KeyType, ValueType>* next = nullptr; //Next leaf node

    int num_elts = 0; //Size of the number of elements in the array, including duplicates

    //Constructor
    BPTreeNode_Leaf(int log_size, int num_blocks, int block_size) : bpa(log_size, num_blocks, block_size) {}

    virtual ~BPTreeNode_Leaf() = default;
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
        if (dynamic_cast<BPTreeNode_Leaf<KeyType, ValueType>*>(root) != nullptr) //check if the root is a leaf node already
            return dynamic_cast<BPTreeNode_Leaf<KeyType, ValueType>*>(root);
        
        BPTreeNode_Internal<KeyType, ValueType>* curr_node = dynamic_cast<BPTreeNode_Internal<KeyType, ValueType>*>(root) ;
        BPTreeNode<KeyType, ValueType>* probe_node = root;
        curr_node->rw_lock.lock_shared();

        while (dynamic_cast<BPTreeNode_Leaf<KeyType, ValueType>*>(probe_node) == nullptr) {
            curr_node = dynamic_cast<BPTreeNode_Internal<KeyType, ValueType>*>(probe_node);
            probe_node = curr_node->children[curr_node->children.size() - 1];
            for (int i = 0; i < curr_node->keys.size(); i++) {
                if (key < curr_node->keys[i]) {
                    probe_node = curr_node->children[i];
                    probe_node->rw_lock.lock_shared(); // Hand-over-hand locking
                    curr_node->rw_lock.unlock_shared();

                    break;
                }
            }
        }

        probe_node->rw_lock.unlock_shared();
        return dynamic_cast<BPTreeNode_Leaf<KeyType, ValueType>*>(probe_node);
    }

    // Helper method for gaining all locks down to the internal node being split
    void pess_descent(BPTreeNode_Internal<KeyType, ValueType>* node) {
        if (node->parent != nullptr && node->children.size() == order - 1) //Only have to take this node's parent's lock if a split is going to happen
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

        //if (leaf->parent !=nullptr)
        //    leaf->parent->rw_lock.lock_shared();
        leaf->rw_lock.lock(); //Write lock

        //Can insert into the BPA without issues
        if (leaf->num_elts < leaf->bpa.total_size) {
            leaf->bpa.insert(key, value);
            leaf->num_elts++;
            if (leaf->parent !=nullptr)
                leaf->parent->rw_lock.unlock_shared();
            leaf->rw_lock.unlock(); //Write lock
            return;
        }

        // BPA is full, so must split it
        
        vector<ElementBPA<KeyType, ValueType>> bpa_elts;

        for (int i = 0; i < leaf->num_elts; i++)
            bpa_elts.push_back(leaf->bpa.header_ptr[i]);
        sort(bpa_elts.begin(), bpa_elts.end(), greater<ElementBPA<KeyType, ValueType>>());

        BPTreeNode_Leaf<KeyType, ValueType>* leaf_one = new BPTreeNode_Leaf<KeyType, ValueType>(bpa_log_size, bpa_num_blocks, bpa_block_size);
        BPTreeNode_Leaf<KeyType, ValueType>* leaf_two = new BPTreeNode_Leaf<KeyType, ValueType>(bpa_log_size, bpa_num_blocks, bpa_block_size);

        // Edge case where leaf is also the root, create a new internal node as the root
        if (leaf->parent == nullptr) {
            BPTreeNode_Internal<KeyType, ValueType>* new_node = new BPTreeNode_Internal<KeyType, ValueType>();
            new_node->rw_lock.lock();
            root = new_node;

            leaf_one->rw_lock.lock();
            leaf_two->rw_lock.lock();

            leaf_one->next = leaf_two;
            leaf_one->parent = new_node;
            leaf_two->prev = leaf_one;
            leaf_two->parent = new_node;

            new_node->children.push_back(leaf_one);
            new_node->children.push_back(leaf_two);

            new_node->keys.push_back(bpa_elts[bpa_elts.size() / 2].key);

            new_node->rw_lock.unlock();
            leaf_one->rw_lock.unlock();
            leaf_two->rw_lock.unlock();
        }
        // Normal case, no need to create new root node
        else {

            leaf_one->rw_lock.lock();
            leaf_two->rw_lock.lock();

            leaf_one->prev = leaf->prev;
            leaf_one->parent = leaf->parent;
            leaf_one->next = leaf_two;
            leaf_one->prev = leaf_one;
            leaf_one->parent = leaf->parent;
            leaf_one->next = leaf->next;

            // Get the index at which the original leaf was at
            int split;
            for (split = 0; split < leaf->parent->children.size(); split++) {
                if (leaf->parent->children[split] == leaf) {
                    leaf->parent->children.erase(leaf->parent->children.begin() + split);
                    leaf->parent->children.insert(leaf->parent->children.begin() + split, leaf_one);
                    leaf->parent->children.insert(leaf->parent->children.begin() + split + 1, leaf_two);
                    break;
                }

            }

            //Insert the first key in the righht leaf as the new divider key
            leaf->parent->keys.insert(leaf->parent->keys.begin() + split, leaf_two->bpa.header_ptr[0].key);

            leaf->rw_lock.unlock();
            leaf_one->rw_lock.unlock();
            leaf_two->rw_lock.unlock();

            //Uh oh time for a split!!!
            if (leaf->parent->children.size() == order) {
                split_internal_node(leaf->parent, true);
            }
        }

        leaf_one->rw_lock.lock();
        leaf_two->rw_lock.lock();

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

        leaf = (key >= bpa_elts[bpa_elts.size() / 2].key) ? leaf_two : leaf_one;

        leaf->bpa.insert(key, value); 
        leaf->num_elts++;

        leaf_one->rw_lock.unlock();
        leaf_two->rw_lock.unlock();
    }

    void split_internal_node(BPTreeNode_Internal<KeyType, ValueType>* node, bool first_split = false) {

        if (first_split)
            pess_descent(node);
        int splitIndex = node->keys.size() / 2;

        BPTreeNode_Internal<KeyType, ValueType> *new_node = new BPTreeNode_Internal<KeyType, ValueType>();
        new_node->rw_lock.lock();

        // Root node, need to create a new root
        if(node->parent == nullptr) {
            root = new_node;

            node->parent = new_node;
            new_node->parent = new_node;

            new_node->children.push_back(node);
            new_node->children.push_back(new_node);

            new_node->keys.push_back(node->keys[splitIndex]);
        }
        else {
            // Get the index at which the original leaf was at
            int split;
            for (split = 0; split < node->parent->children.size(); split++) {
                if (node->parent->children[split] == node) {
                    node->parent->children.insert(node->parent->children.begin() + split + 1, new_node);
                    break;
                }

            }
            new_node->parent = node->parent;

            //Insert the first key in the righht leaf as the new divider key
            node->parent->keys.insert(node->parent->keys.begin() + split, node->keys[splitIndex]);
        }

        //For  efficiency we just reuse the old leaf
        new_node->keys.assign(node->keys.begin() + splitIndex + 1, node->keys.end());
        new_node->children.assign(node->children.begin() + splitIndex+1, node->children.end());

        node->keys.erase(node->keys.begin() + splitIndex, node->keys.end());
        node->children.erase(node->children.begin() + splitIndex+1, node->children.end());

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
                    if (leaf->bpa.header_ptr[i].key < start && ! leaf->bpa.sorted_blocks[i])
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