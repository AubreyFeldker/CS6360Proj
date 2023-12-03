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
    // Create uniform random distribution.
    uniform_int_distribution<int> distr(0,2000000000);
    random_device rand;
    mt19937 gen(rand());

    // Create BP tree.


    BPTree<int, std::string> bPTree(128, 64, 64, 32);

    // Populate initial data set.
    for (int i = 0; i < 10000; i++)
    {
        bPTree.insert(distr(gen), "value");
    }

    // Insert another 10M entries.
    // Start the clock on the inserts.
    auto start = high_resolution_clock::now();
    for (int i = 0; i < 10000000; i++)
    {
        bPTree.insert(distr(gen), "value");
    }
    auto stop = high_resolution_clock::now();
    auto duration = duration_cast<microseconds>(stop-start);
    cout << " Duration of BP Tree for inserts: " << duration.count() << endl;

    // Create B+ tree.
    int node_size = 256;
    BPlusTree<int, std::string> bPlusTree(256);

    // Populate initial data set.
    for (int i = 0; i < 10000000; i++)
    {
        bPTree.insert(distr(gen), "value");
    }

    // Insert another 10M entries.
    // Start the clock on the inserts.
    start = high_resolution_clock::now();
    for (int i = 0; i < 10000000; i++)
    {
        bPTree.insert(distr(gen), "value");
    }
    stop = high_resolution_clock::now();
    duration = duration_cast<microseconds>(stop-start);
    cout << " Duration of B+ Tree for inserts: " << duration.count() << endl;


    // Point find queries on BP tree.
    start = high_resolution_clock::now();
    for (int i = 0; i < 100000; i++)
    {
        // Perform find queries.
    }
    stop = high_resolution_clock::now();
    duration = duration_cast<microseconds>(stop-start);
    cout << " Duration of BP Tree for inserts: " << duration.count() << endl;
    

    // Point find queries on B+ tree.
    start = high_resolution_clock::now();
    for (int i = 0; i < 100000; i++)
    {
        // Perform find queries.
    }
    stop = high_resolution_clock::now();
    duration = duration_cast<microseconds>(stop-start);
    cout << " Duration of B+ Tree for finds: " << duration.count() << endl;


    // Time range queries.
    // Generate random max_range.
    int max_size = 100000;
    int range_lengths[10];

    for (int i = 0; i < 10; i++)
    {
        range_lengths[i] = distr(gen);
    }

    // Perform queries at randomly determined ranges for BP tree.
    start = high_resolution_clock::now();
    for (int i = 0; i < sizeof(range_lengths); i++)
    {

    }
    stop = high_resolution_clock::now();
    duration = duration_cast<microseconds>(stop-start);
    cout << " Duration of BP Tree for range queries: " << duration.count() << endl;

    // Perform queries at randomly determined ranges for B+ tree.
    start = high_resolution_clock::now();
    for (int i = 0; i < sizeof(range_lengths); i++)
    {

    }
    stop = high_resolution_clock::now();
    duration = duration_cast<microseconds>(stop-start);
    cout << " Duration of B+ Tree for range queries: " << duration.count() << endl;

    // Insert monotonically increasing values.
    start = high_resolution_clock::now();
    for (int i = 0; i < 10000000; i++)
    {
        bPTree.insert(i * i * i, "value");
    }
    stop = high_resolution_clock::now();
    duration = duration_cast<microseconds>(stop-start);
    cout << " Duration of BP Tree for monotonically increasing inserts: " << duration.count() << endl;

    // Insert monotonically increasing values.
    start = high_resolution_clock::now();
    for (int i = 0; i < 10000000; i++)
    {
        //bplustree.insert(i * i * i)
    }
    stop = high_resolution_clock::now();
    duration = duration_cast<microseconds>(stop-start);
    cout << " Duration of B+ Tree for monotonically increasing inserts: " << duration.count() << endl;

    /* Insert key-value pairs
    bPlusTree.insert(10, "A");
    bPlusTree.insert(20, "B");
    bPlusTree.insert(5, "C");
    bPlusTree.insert(6, "D");
    bPlusTree.insert(12, "E");
    bPlusTree.insert(30, "F");

    bPlusTree.levelOrderTraversal();

    // Find the node with the smallest key greater than or equal to 25
    BPlusTreeNode<int, string> *foundNode = bPlusTree.find(12);

    // Display the keys and values in the found node
    if (foundNode != nullptr)
    {
        cout << "Keys in the found node: ";
        for (const auto &key : foundNode->keys)
        {
            cout << key << " ";
        }
        cout << endl;

        cout << "Values in the found node: ";
        for (const auto &value : foundNode->values)
        {
            cout << value << " ";
        }
        cout << endl;
    }

    

    // Specify the range to reverse values
    int startKey = 10;
    int length = 3;

    // Print the Original values
    std::cout << "Orignal values:" << std::endl;
    bPlusTree.iterate_range(startKey, length, [](const int &key, const std::string &value) {
        std::cout << "Key: " << key << ", Value: " << value << std::endl;
    });

    bPlusTree.iterate_range(10, 5, [](const int &key, std::string &value) {
        value = value + value; // Concatenate the string to itself
        // std::cout << "Key: " << key << ", Value: " << value << std::endl;
    });

    // Print the modified values
    std::cout << "Modified values :" << std::endl;
    bPlusTree.iterate_range(startKey, length, [](const int &key, const std::string &value) {
        std::cout << "Key: " << key << ", Value: " << value << std::endl;
    });

    return 0;
    */
}
