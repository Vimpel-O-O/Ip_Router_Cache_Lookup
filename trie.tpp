#include "trie.h"
#include <stdexcept>
using namespace std;

template <typename Type>
void Trie<Type>::insert(Type address, int len, int value)
{
    Node *curr = root;

    int i = 0;
    while (i < len)
    {
        int bit = (address >> (width - 1 - i)) & 1;

        if (bit == 0)
        {
            if (curr->left == nullptr)
            {
                curr->left = new Node();
            }
            curr = curr->left;
        }
        else
        {
            if (curr->right == nullptr)
            {
                curr->right = new Node();
            }
            curr = curr->right;
        }
        i++;
    }

    curr->data = value;
    curr->has_value = true;
}

template <typename Type>
void Trie<Type>::remove(Type address, int len)
{
    Node *curr = root;

    int i = 0;
    while (i < len)
    {
        int bit = (address >> (width - 1 - i)) & 1;

        if (bit == 0)
        {
            if (curr->left == nullptr)
            {
                throw runtime_error("address was not found on remove");
            }
            curr = curr->left;
        }
        else
        {
            if (curr->right == nullptr)
            {
                throw runtime_error("address was not found on remove");
            }
            curr = curr->right;
        }
        i++;
    }

    curr->has_value = false;
}

template <typename Type>
int Trie<Type>::LPM(Type address)
{
    Node *curr = root;
    int best_prefix = -1;
    if (root->has_value)
    {
        best_prefix = root->data;
    }

    int i = 0;
    while (i < width)
    {
        int bit = (address >> (width - 1 - i)) & 1;

        if (bit == 0)
        {
            if (curr->left == nullptr)
            {
                break;
            }
            curr = curr->left;
        }
        else
        {
            if (curr->right == nullptr)
            {
                break;
            }
            curr = curr->right;
        }
        i++;

        if (curr->has_value)
        {
            best_prefix = curr->data;
        }
    }

    return best_prefix;
}
