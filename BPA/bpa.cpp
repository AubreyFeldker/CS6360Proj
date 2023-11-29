#include <iostream>
#include <functional>
#include <algorithm>

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
    ElementBPA<KeyType, ValueType>* bpa_array;  // Actual array containing key values.
    ElementBPA<KeyType, ValueType>* temp_array; // Array used for redistributing the elements
    int* count_per_block;

    

public:
    int log_size; // Maximum number of buffered inserts
    int num_blocks; // Number of blocks in the data structure
    int block_size; // Maximum number of elements per block
    int total_size; // Total number of elements that can be held in the array (minus the log)

    ElementBPA<KeyType, ValueType>* log_ptr; // Buffered inserts that propogate out to the rest of the array
    ElementBPA<KeyType, ValueType>* header_ptr; // Each space in header_ptr + i holds the minimum element for block i
    ElementBPA<KeyType, ValueType>* blocks_ptr; // Rest of the elemetns in chunks of block_size elements

    bool* sorted_blocks; // Bit array for checking if a particular block is already sorted
    bool sorted_log = false;
    
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

        temp_array = new ElementBPA<KeyType, ValueType>[log_size + num_blocks + (num_blocks * block_size)];
        log_ptr = bpa_array;
        header_ptr = log_ptr + log_size;
        blocks_ptr = header_ptr + block_size;
        total_size = num_blocks + (num_blocks * block_size);

        sorted_blocks = new bool[num_blocks];
        count_per_block = new int[num_blocks]();
    }

    //Helper function to facilitate BPA splitting
    bool insert (ElementBPA<KeyType, ValueType> ele) {
        return insert(ele.key, ele.value);
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
        if(bpa_array[log_size-1].isNull) {
            sorted_log = false;
            return true;
        }

        //If the BPA is new (theres no elements in the header) and the log is full, then move min(log size, header size) elements to the header and sort them, then return
        int numToMove = min(log_size, num_blocks);
        if (header_ptr->isNull){
            for(int i = 0; i < numToMove; i++){
                swap(log_ptr[log_size-1-i], header_ptr[i]);
            }
            sort(header_ptr, header_ptr+numToMove);
            sorted_log = true;
            return true;
        }

        //If at this point then the log is full and there are some headers (not necessarily all)]
        int new_destined_per_block[block_size] = {};
        int destination_block[log_size] = {};          //Used to remember which block each element in the log should go for later use
        new_destined_per_block[block_size-1] = log_size;
        //Count how many elements in log will be inserted into each block and check for overflow
        for (int i = 0; i < log_size; i++){
            destination_block[i] = block_size-1;
            //Iterate through the header until target block found
            for(int j = 0; j < num_blocks; j++){
                // A header might be null, indicating to look through the last used header.
                if (log_ptr[i].key < header_ptr[j].key || header_ptr[j].isNull){
                    //cout << "Element " << i << " belongs in block " << j-1 << endl;
                    new_destined_per_block[j-1] += 1;
                    new_destined_per_block[block_size-1] -= 1;
                    destination_block[i] = j-1;
                    break;
                }
            }
        }

        //cout << "count " << new_destined_per_block[block_size-1] << endl;

        //Check if theres enough space in the blocks for all the target insertions
        bool enough_space = true;
        for (int i = 0; i < num_blocks; i++){
            if (count_per_block[i] + new_destined_per_block[i] >= block_size){
                enough_space = false;
                break;
            }
        }

        if (enough_space){
            for (int i = 0; i < log_size; i++){
                //If the subject block header has the same key, replace the value and clear the corresponding element in log
                if (!header_ptr[destination_block[i]].isNull && header_ptr[destination_block[i]].key == log_ptr[i].key){
                    header_ptr[destination_block[i]].value = log_ptr[i].value;
                    log_ptr[i].isNull = true;
                    continue;
                }
                //Otherwise iterate through destination blocks contents, replacing an element with the corresponding key
                // or copying into the first null element. Unset the sorted bit when appending and not replacing.
                ElementBPA<KeyType, ValueType>* subject_block = getBlock(destination_block[i]);
                for (int j = 0; j < block_size; j++){
                    if (subject_block[j].isNull){
                        subject_block[j].isNull = false;
                        subject_block[j].key = log_ptr[i].key;
                        subject_block[j].value = log_ptr[i].value;
                        log_ptr[i].isNull = true;
                        count_per_block[destination_block[i]] += 1;
                        sorted_blocks[destination_block[i]] = false;
                        break;
                    } else if (subject_block[j].key == log_ptr[i].key){
                        subject_block[j].value = log_ptr[i].value;
                        log_ptr[i].isNull = true;
                        break;
                    }
                }
            }
        }


        //If not enough space in one or more blocks, it is necessary to completely redistribute the BPA and select new headers.
        //TODO: Implement hashmap to discard elements with duplicate keys
        if (!enough_space){
            int count = 0;
            for(int i = 0; i < total_size; i++){
                if (!bpa_array[i].isNull)
                    count++;

                temp_array[i] = bpa_array[i];
                bpa_array[i].isNull = true;
            }

            sort(temp_array, temp_array + total_size);
            int new_count_per_block = count / block_size;
            int remains = count % block_size;

            //Copy the modified array back into the correct positions
            for(int i = 0; i < new_count_per_block * num_blocks; i += new_count_per_block){
                int blocknum = i / new_count_per_block;
                header_ptr[blocknum] = temp_array[i];   //Set header of this new copied over block

                //Then copy over the actual elements that belong in the new block
                ElementBPA<KeyType, ValueType>* newBlockStart = getBlock(blocknum);
                for(int j = 1; j < new_count_per_block; j++){
                    newBlockStart[j-1] = temp_array[i+j];
                }
            }
        }


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
    int iterate_range (int start, int length, function<ValueType(KeyType)> f) {
        if (! sorted_log) {
            // Sort the log for iteration
            sort(log_ptr, log_ptr + log_size);
            sorted_log = true;
        }

        int found_block = num_blocks - 1;

        // We find the first block that contains the starting range
        for (int i = 1; i < num_blocks; i++) {
            if (start < header_ptr[i].key || header_ptr[i].isNull) {
                found_block = i-1;
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
            block_space = (block_spot == 0) ?  header_ptr[found_block] : block_ptr[block_spot-1];
            // Check if the item in the log is smaller than the one in the block & the start val, then perform function if so
            if (log_spot < log_size && ! log_ptr[log_spot].isNull && (! keep_block_iter || log_ptr[log_spot].key < block_space.key)) {
                if (log_ptr[log_spot].key >= start) {
                    log_ptr[log_spot].value = f(log_ptr[log_spot].key);
                    iters++;
                }
                log_spot++;
            } else if (keep_block_iter) {
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
                        sort(blocks_ptr, blocks_ptr + block_size);
                        sorted_blocks[found_block] = true;
                    }
                    block_spot = 0;
                }
            }
            else
                break;
        }

        return iters;

    }

    int map_range (int start, int length, function<ValueType(KeyType)> f) {
        for (int i = 0; i < log_size; i++) {
            if (log_ptr[i].key >= start && log_ptr[i].key < start + length)
                log_ptr[i].value = f(log_ptr[i].key);
        }

        int found_block = num_blocks - 1;

        // We find the first block that contains the starting range
        for (int i = 1; i < num_blocks; i++) {
            if (start < header_ptr[i].key || header_ptr[i].isNull) {
                found_block = i-1;
                break;
            }
        }

        // Sort the block if not already sorted
        if (! sorted_blocks[found_block]) {
            sort(blocks_ptr, blocks_ptr + block_size);
            sorted_blocks[found_block] = true;
        }

        ElementBPA<KeyType, ValueType> *block_ptr = getBlock(found_block);
        ElementBPA<KeyType, ValueType> block_space = header_ptr[found_block];
        int block_iterator = 0;
        int iters = 0;

        while (block_space.key < start + length) {
            if (block_space.key >= start) {
                block_space.value = f(block_space.key);
                iters++;
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

        return iters;
    }

    // Small helper function, returns pointer to first element in block i
    ElementBPA<KeyType, ValueType>* getBlock (int i){
        return blocks_ptr + i * (block_size);
    }

    // Dubug helper function, prints contents of the BPA
    void printContents (){
        cout << "{";
        for(int i = 0; i < total_size; i++){
            if (!bpa_array[i].isNull)
                cout << "(" << bpa_array[i].key << ", " << bpa_array[i].value << ")";
            else
                cout << "(" << "Null" << ")";

            if(i+1 < total_size)
                cout << ", ";
        }
        cout << "}" << endl;
    }
};

int add_five(int x) { return x + 5; };

int main() {
    //ElementBPA<int, int> s(1, 2);
    BPA<int, int> tester(4, 4, 4);
    tester.insert(7, 0);
    tester.insert(19, 0);
    tester.insert(15, 0);
    tester.insert(89, 0);

    tester.insert(13, 0);
    tester.insert(8, 0);
    tester.insert(17, 0);
    tester.insert(50, 0);

    tester.insert(90, 0);
    tester.insert(93, 0);
    tester.insert(95, 0);
    tester.insert(101, 0);
    tester.printContents();

    

    //cout << endl << *tester.find(4) << endl;

    tester.map_range(3, 2, &add_five);

    tester.printContents();

    //ElementBPA<int, int> g = new ElementBPA(1, 5);
}