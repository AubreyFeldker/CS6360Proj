#include <vector>
#include <functional>

using namespace std;

const int e_size = sizeof(int);

class BPA {
    int log_size; // Maximum number of buffered inserts
    int num_blocks; // Number of blocks in the data structure
    int block_size; // Maximum number of elements per block

    int* bpa_array; // Actual array containing key values
    vector<bool> sorted_blocks; // Bit array for checking if a particular block is already sorted

    int* log_ptr; // Buffered inserts that propogate out to the rest of the array
    int* header_ptr; // Each space in header_ptr + i holds the minimum element for block i
    int* blocks_ptr; // Rest of the elemetns in chunks of block_size elements

    public:
        BPA (int log_size, int num_blocks, int block_size) {
            this->log_size = log_size;
            this->num_blocks = num_blocks;
            this->block_size = block_size;

            bpa_array = new int[log_size + num_blocks + (num_blocks * block_size)];
            log_ptr = bpa_array;
            header_ptr = log_ptr + log_size * e_size;
            blocks_ptr = header_ptr + block_size * e_size;

            sorted_blocks.resize(num_blocks);
        }

        void insert (int element) {
        }

        void find (int element) {

        }

        void iterate_range (int start, int length, function<int(int)> f) {

        }

        void map_range (int start, int length, function<int(int)> f) {

        }
};

int main() {
    BPA tester(5, 10, 6);
}