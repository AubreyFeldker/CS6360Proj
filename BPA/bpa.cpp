#include <iostream>
#include <functional>
#pragma GCC diagnostic ignored "-Wconversion-null"

using namespace std;

const int e_size = sizeof(int);

// Structure for the key-value pair for the BPA
template <typename KeyType, typename ValueType>
struct ElementBPA
{
    bool isNull = true;
    KeyType key;
    ValueType value;

    bool operator<(ElementBPA<KeyType, ValueType> ele){
        if (ele.isNull && !isNull)
            return true;
        if (isNull)
            return false;


        return key < ele.key;
    } 
};


template <typename KeyType, typename ValueType>
class BPA {
private:
    ElementBPA<KeyType, ValueType>* bpa_array; // Actual array containing key values.
    bool* sorted_blocks; // Bit array for checking if a particular block is already sorted

    ElementBPA<KeyType, ValueType>* log_ptr; // Buffered inserts that propogate out to the rest of the array
    ElementBPA<KeyType, ValueType>* header_ptr; // Each space in header_ptr + i holds the minimum element for block i
    ElementBPA<KeyType, ValueType>* blocks_ptr; // Rest of the elemetns in chunks of block_size elements

public:
    int log_size; // Maximum number of buffered inserts
    int num_blocks; // Number of blocks in the data structure
    int block_size; // Maximum number of elements per block
    int total_size; // Total size of BPA
    
    BPA* prev = nullptr;  //Pointer to the child BPA to the left
    BPA* next = nullptr;  //Pointer to the child BPA to the right

    BPA (int log_size, int num_blocks, int block_size, ElementBPA<KeyType, ValueType>* bpa = NULL ) {
        this->log_size = log_size;
        this->num_blocks = num_blocks;
        this->block_size = block_size;

        if (bpa)
            bpa_array = bpa;
        else 
            bpa_array = new ElementBPA<KeyType, ValueType>[log_size + num_blocks + (num_blocks * block_size)];

        log_ptr = bpa_array;
        header_ptr = log_ptr + log_size;
        blocks_ptr = header_ptr + block_size;
        total_size = log_size + num_blocks + (num_blocks * block_size);

        sorted_blocks = new bool[num_blocks];
    }

    // Inserts the key value pair, returns false if theres not enough space and the BPA needs to be split
    bool insert (KeyType ekey, ValueType eval) {
        // First check through elements in log and replace the element if they have the same key, else append it to log
        for(int i = 0; i < log_size; i++){
            if(bpa_array[i].isNull || bpa_array[i].key == ekey){
                bpa_array[i].isNull = false;
                bpa_array[i].key = ekey;
                bpa_array[i].value = eval;
                break;
            }
        }

        //If theres still a space left in the log, can return successfully. Case 1.
        if(bpa_array[log_size-1].isNull)
            return true;

        //If the BPA is new (theres no elements in the header) and the log is full, then move min(log size, header size) elements to the header and sort them, then return
        int numToMove = min(log_size, num_blocks);
        if (header_ptr->isNull){
            for(int i = 0; i < numToMove; i++){
                swap(log_ptr[log_size-1-i], header_ptr[i]);
            }
            sort(header_ptr, header_ptr+numToMove);

            return true;
        }

        //If at this point then the log is full and there are some headers (not necessarily all)
        //TODO: Count elements in each block and check if inserting the elements in the log will overflow any of them, then handle



        return false;
    }


    //Finds and returns a pointer to the first value found with the matching key
    ValueType* find (KeyType element) {
        //First iterate through log and return if found
        for(int i = 0; i < log_size; i++){
            if(bpa_array[i].isNull)
                break;
            if (element == bpa_array[i].key)
                return &bpa_array[i].value;
        }
        
        int foundBlock = num_blocks-1;
        //Else iterate through the header and delve into the respective block if necessary.
        for(int i = 0; i < num_blocks; i++){
            // A header might be null, indicating to look through the last used header.
            if (element < header_ptr[i].key || header_ptr[i].isNull){
                foundBlock = i-1;
                break;
            }
            if (element == header_ptr[i].key){
                return &header_ptr[i].value;
            }
        }

        //foundblock is -1 if the element was lower than the first element in the header, meaning it cant exist if its not in the log.
        if (foundBlock == -1)
            return nullptr;

        //Iterate through the block and return if the element is found or upon the first NULL
        ElementBPA<KeyType, ValueType> *blockptr = getBlock(foundBlock);
        for(int i = 0; i < block_size; i++){
            if (blockptr[i].isNull){
                return nullptr;
            }
            if (blockptr[i].key == element){
                return &blockptr[i].value;
            }
        }

        return nullptr;
    }

    // sort function recommended std::sort(bpa_array, bpa_array+4) for example to sort only the first 4 elements quickly
    void iterate_range (int start, int length, function<ValueType(KeyType)> f) {
        // Sort the log for iteration
        sort(log_ptr, log_ptr + log_size);

        int found_block = num_blocks - 1;

        // We find the first block that contains the starting range
        for (int i = 1; i < num_blocks; i++) {
            if (start < header_ptr[i].key || header_ptr[i].isNull) {
                foundBlock = i-1;
                break;
            }
        }

        ElementBPA<KeyType, ValueType> *block_ptr = getBlock(found_block);

        int log_spot = 0;
        int block_spot = 0;

        if (! sorted_blocks[found_block]) {
            sort(block_ptr, block_ptr + block_size);
            sorted_blocks[found_block] = true;
        }

        int iters = 0;
        bool keep_block_iter = true;
        ElementBPA<KeyType, ValueType> block_space;

        // Do *length* iterations of the function onto the monotonically increasing elements in the BPA
        while (iters < length) {
            block_space = (block_spot == 0) ?  header_ptr[found_block] : block_ptr[block_spot-1]
            // Check if the item in the log is smaller than the one in the block & the start val, then perform function if so
            if (log_spot < log_size && ! log_ptr[log_spot].isNull && (! keep_block_iter || log_ptr[log_spot].key < block_space.key)) {
                if (log_ptr[log_spot].key >= start) {
                    log_ptr[log_spot].value = f(log_ptr[log_spot].key);
                    iters++;
                }
                log_spot++;
            }
            else if (keep_block_iter) {
                if (block_space.key >= start) {
                    block_space.value = f(block_space.key);
                    iters++;
                }
                block_spot++;

                // Finished this block, must go to the next one... if there is a next one
                if (block_spot > block_size || block_ptr[block_spot-1].isNull) {
                    found_block++;
                    
                    // Final block in the BPA, can't continue iterating here!
                    if (found_block >= num_blocks) {
                        keep_block_iter = false;
                        continue;
                    }

                    block_ptr = getBlock(found_block);
                    if (! sorted_blocks[found_block]) {
                        sort(block_ptr, block_ptr + block_size);
                        sorted_blocks[found_block] = true;
                    }
                    block_spot = 0;
                }
            }
            else
                break;

            
        }

        return;

    }

    void map_range (int start, int length, function<ValueType(KeyType)> f) {
        for (int i = 0; i < log_size; i++) {
            if (log_ptr[i].key >= start && log_ptr[i].key < start + length)
                log_ptr[i].value = f(log_ptr[i].key)
        }

        int found_block = num_blocks - 1;

        // We find the first block that contains the starting range
        for (int i = 1; i < num_blocks; i++) {
            if (start < header_ptr[i].key || header_ptr[i].isNull) {
                foundBlock = i-1;
                break;
            }
        }

        // Sort the block if not already sorted
        if (! sorted_blocks[found_block]) {
            sort(block_ptr, block_ptr + block_size);
            sorted_blocks[found_block] = true;
        }

        ElementBPA<KeyType, ValueType> *block_ptr = getBlock(found_block);
        ElementBPA<KeyType, ValueType> block_space = header_ptr[found_block];
        int block_iterator = 0;

        while (block_space.key < start + length) {
            if (block_space.key >= start) {
                block_space.value = f(block_space.key);
            }

            block_space = block_ptr[block_iterator];
            block_iterator++;

            if (block_iterator >= block_size) {
                found_block++;
                block_ptr = getBlock(found_block);
                block_space = header_ptr[found_block];
                block_iterator = 0;

                if (! sorted_blocks[found_block]) {
                    sort(block_ptr, block_ptr + block_size);
                    sorted_blocks[found_block] = true;
                }
            }
        }

        return;
    }

    // Small helper function, returns pointer to first element in block i
    ElementBPA<KeyType, ValueType>* getBlock (int i){
        return blocks_ptr + i * block_size;
    }

    // Dubug helper function, prints contents of the BPA
    void printContents (){
        cout << "{";
        for(int i = 0; i < total_size; i++){
            cout << "(" << bpa_array[i].key << ", " << bpa_array[i].value << ")";
            if(i+1 < total_size)
                cout << ", ";
        }
        cout << "}" << endl;
    }
};

int main() {
    //ElementBPA<int, int> s(1, 2);
    BPA<int, int> tester(3, 4, 3);

    tester.printContents();
    tester.insert(2, 20);
    tester.insert(3, 30);
    tester.insert(1, 10);
    
    tester.insert(4, 40);
    tester.printContents();

    cout << endl << *tester.find(4) << endl;

    //ElementBPA<int, int> g = new ElementBPA(1, 5);
}