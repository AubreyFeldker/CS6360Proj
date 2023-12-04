#include <iostream>
#include <vector>
#include <queue>
#include <chrono>
#include <random>
#include "b+Tree.h"
#include "bp_tree.cpp"

using namespace std;
using namespace std::chrono;

int main()
{
    // Setup experiments by initializing values to arrays.
    int num_blocks[5] = {4,8,16,32,64};
    int block_size[5] = {4,8,16,32,64};
    int bplus_size[5] = {256,1024,4096,16384,65536};

    // Loop through the experiments.
    for (int k = 0; k < 5; k++)
    {

        // Create uniform random distribution.
        uniform_int_distribution<int> distr(0,200000000);
        random_device rand;
        mt19937 gen(rand());

        // Create BP tree.
        cout << "Testing BP Tree with header size: " << num_blocks[k] << " and block_size: " << block_size[k] << endl;
        cout << "Testing B+ Tree with size: " << bplus_size[k] << endl;
        BPTree<int, int> bPTree(16, 3, num_blocks[k], block_size[k]);

        // Populate initial data set.
        for (int i = 0; i < 10000000; i++)
        {
            bPTree.insert(distr(gen), i);
        }
        
        // Insert another 1M entries.
        // Start the clock on the inserts.
        auto start = high_resolution_clock::now();
        for (int i = 0; i < 10000000; i++)
        {
            bPTree.insert(distr(gen), i);
        }
        auto stop = high_resolution_clock::now();
        auto duration = duration_cast<microseconds>(stop-start);
        cout << " Duration of BP Tree for inserts: " << duration.count() << " microseconds." << endl;

        // Create B+ tree.
        BPlusTree<int, int> bPlusTree(bplus_size[k]);

        // Populate initial data set.
        for (int i = 0; i < 10000000; i++)
        {
            bPlusTree.insert(distr(gen), i);
        }

        // Insert another 10M entries.
        // Start the clock on the inserts.
        start = high_resolution_clock::now();
        for (int i = 0; i < 10000000; i++)
        {
            bPlusTree.insert(distr(gen), i);
        }
        stop = high_resolution_clock::now();
        duration = duration_cast<microseconds>(stop-start);
        cout << " Duration of B+ Tree for inserts: " << duration.count() << " microseconds." <<endl;


        // Point find queries on BP tree.
        start = high_resolution_clock::now();
        int foundBP[11000];

        for (int i = 0; i < 10000; i++)
        {
            // Perform find queries.
            bPTree.find(distr(gen));
            
        }
        stop = high_resolution_clock::now();
        duration = duration_cast<microseconds>(stop-start);
        cout << " Duration of BP Tree for finds: " << duration.count() << " microseconds." <<endl;
        

        // Point find queries on B+ tree.
        start = high_resolution_clock::now();
        int foundBPlus[10000];
        for (int i = 0; i < 10000; i++)
        {
            // Perform find queries.
            bPlusTree.find(distr(gen));

        }
        stop = high_resolution_clock::now();
        duration = duration_cast<microseconds>(stop-start);
        cout << " Duration of B+ Tree for finds: " << duration.count() << " microseconds." << endl;


        // Time range queries.
        // Generate random max_range.
        int max_size = 10000;
        int range_lengths[1000];
        uniform_int_distribution<int> distr_range(0,max_size);
        
        for (int i = 0; i < 1000; i++)
        {
            range_lengths[i] = distr_range(gen);
        }

        // Perform queries at randomly determined ranges for BP tree.
        start = high_resolution_clock::now();
        for (int i = 0; i < sizeof(range_lengths); i++)
        {
            bPTree.iterate_range(distr(gen), range_lengths[i], &add_five);
        }
        stop = high_resolution_clock::now();
        duration = duration_cast<microseconds>(stop-start);
        cout << " Duration of BP Tree for range queries: " << duration.count() << " microseconds." << endl;

        // Perform queries at randomly determined ranges for B+ tree.
        start = high_resolution_clock::now();
        for (int i = 0; i < sizeof(range_lengths); i++)
        {
            const int key = distr(gen);
            bPlusTree.iterate_range(key, range_lengths[i], [](const int &key,  int &value)
            {value += 5;});
        }
        stop = high_resolution_clock::now();
        duration = duration_cast<microseconds>(stop-start);
        cout << " Duration of B+ Tree for range queries: " << duration.count() << " microseconds." << endl;

        // Insert monotonically increasing values.
        start = high_resolution_clock::now();
        for (int i = 0; i < 1000000; i++)
        {
            bPTree.insert(i + i, i);
        }
        stop = high_resolution_clock::now();
        duration = duration_cast<microseconds>(stop-start);
        cout << " Duration of BP Tree for monotonically increasing inserts: " << duration.count() << " microseconds." << endl;

        // Insert monotonically increasing values.
        start = high_resolution_clock::now();
        for (int i = 0; i < 1000000; i++)
        {
            bPlusTree.insert(i + i, i);
        }
        stop = high_resolution_clock::now();
        duration = duration_cast<microseconds>(stop-start);
        cout << " Duration of B+ Tree for monotonically increasing inserts: " << duration.count() << " microseconds." << endl;
    }
}
