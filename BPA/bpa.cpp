#include <iostream>
#include <functional>
#pragma GCC diagnostic ignored "-Wconversion-null"

using namespace std;

const int e_size = sizeof(int);

class BPA {
private:
    int* bpa_array; // Actual array containing key values. Temporarily public for debugging
    int* sorted_blocks; // Bit array for checking if a particular block is already sorted

    int* log_ptr; // Buffered inserts that propogate out to the rest of the array
    int* header_ptr; // Each space in header_ptr + i holds the minimum element for block i
    int* blocks_ptr; // Rest of the elemetns in chunks of block_size elements

public:
    int log_size; // Maximum number of buffered inserts
    int num_blocks; // Number of blocks in the data structure
    int block_size; // Maximum number of elements per block
    
    BPA* prev = nullptr;  //Pointer to the child BPA to the left
    BPA* next = nullptr;  //Pointer to the child BPA to the right

    BPA (int log_size, int num_blocks, int block_size, int* bpa = NULL ) {
        this->log_size = log_size;
        this->num_blocks = num_blocks;
        this->block_size = block_size;

        if (bpa)
            bpa_array = bpa;
        else 
            bpa_array = new int[log_size + num_blocks + (num_blocks * block_size)];

        log_ptr = bpa_array;
        header_ptr = log_ptr + log_size;
        blocks_ptr = header_ptr + block_size;

        sorted_blocks = new int[num_blocks];
    }


    void insert (int element) {

    }


    //Finds and returns the first element with the matching value
    int find (int element) {
        //First iterate through log and return if found
        for(int i = 0; i < log_size; i++){
            if (element == bpa_array[i])
                return bpa_array[i];
        }
        
        int foundBlock = num_blocks-1;
        //Else iterate through the header and delve into the respective block if necessary.
        for(int i = 0; i < num_blocks; i++){
            if (element == header_ptr[i]){
                return header_ptr[i];
            }
            if (element < header_ptr[i]){
                foundBlock = i-1;
                break;
            }
        }

        //foundblock is -1 if the element was lower than the first element in the header, meaning it cant exist if its not in the log.
        if (foundBlock == -1)
            return NULL;

        //Iterate through the block and return if the element is found or upon the first NULL
        int *blockptr = getBlock(foundBlock);
        for(int i = 0; i < block_size; i++){
            if (blockptr[i] == element){
                return blockptr[i];
            }
            if (blockptr[i] == NULL){
                return NULL;
            }
        }

        return NULL;
    }

    // sort function recommended std::sort(bpa_array, bpa_array+4) for example to sort only the first 4 elements quickly
    void iterate_range (int start, int length, function<int(int)> f) {

    }

    void map_range (int start, int length, function<int(int)> f) {

    }

    // Small helper function, returns pointer to first element in block i
    int* getBlock (int i){
        return blocks_ptr + i * block_size;
    }
};

int main() {
    int *test = new int[3 + 3 + 3*3]{5, NULL, NULL,   2, 8, 15,   4, NULL, NULL,   14, 10, NULL,   21, 24, NULL};
    BPA tester(3, 3, 3, test);

    cout << endl << tester.find(10) << endl;
}