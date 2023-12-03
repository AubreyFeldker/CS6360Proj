#include <iostream>
#include <vector>
#include <queue>
// #include "b+Tree.h"
#include "bp_tree.cpp"

using namespace std;

int main()
{
    // Create a B+ tree of order 3
    BPTree<int, int> tree = BPTree<int, int>(3, 3, 3, 5);

    // Insert key-value pairs
    tree.insert(10, 1);
    tree.insert(20, 4);
    tree.insert(5, 8);
    tree.insert(6, 5);
    tree.insert(12, 128);
    tree.insert(30, 33);

    return 0;

}
