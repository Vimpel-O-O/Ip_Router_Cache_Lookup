#ifndef TRIE_H
#define TRIE_H

template <typename Type>
class Trie
{
private:
    int width;
    class Node
    {
    public:
        int data;
        Node *left = nullptr;
        Node *right = nullptr;
        bool has_value = false;

        Node() = default;
        Node(int value) : data(value) {}
    };
    Node *root = new Node();

public:
    Trie(int w) : width(w) {};
    void insert(Type address, int len, int value);
    void remove(Type address, int len);
    int LPM(Type address);
};

#include "trie.tpp"

#endif