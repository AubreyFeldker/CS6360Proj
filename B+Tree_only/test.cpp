#include <iostream>
#include <vector>
#include <queue>
// #include "b+Tree.h"
#include "./CS6360Proj/B+Tree/b+Tree.h"

using namespace std;

int main()
{
    // Create a B+ tree of order 3
    BPlusTree<int, std::string> bPlusTree(3);

    // Insert key-value pairs
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
}
