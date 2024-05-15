# 8 - Hash Maps

There are 2 classes of data structures for key-value storage

- sorting data structures (avg lookup: $O(log(n))$)
  - AVL tree (balanced binary tree)
  - treap (randomized binary tree)
  - trie (n-ary tree)
  - b-tree (balanced n-ary tree)
- hashing data structures (avg lookup: $O(1)$)
  - open addressing
  - chaining

Which do we choose? Depends on whether or not the keys are sorted (note: sorting is a key func for most relational databases).

### mapping keys to slots

- a hastable is an array with a fixed # of slots
- index = `hash(key) % size` (this is why we choose powers of 2 for size of hashtable since the `&` operator is faster than `%`)
- `hash()` turns the key into an int value

### resolving collisions

- open addressing: find another slot if already occupied
  - linear/quadratic probing, double hashing, ...
- chaining: each slot points to a collection of colliding keys
  - linked list (or vector for easier impl)

### $\text{load factor} = \frac{\text{num keys}}{\text{num slots}}$

open addressing:

- load factor $<1$
- deteriorates rapidly near limit

chaining:

- no hard lim on load factor
- avg search scales linearly with load factor

## considerations for impl

### resizing

- a single resize is $O(n)$; too costly regardless of avg time
- soln: progressivley move the keys into the new table so that
  - an insert does a bounded amt of work
  - lookup uses both tables

### CPU cache performance

**case 1:** lookup of single probe

- open addressing: 1 memory read from slot
- chaining: 2 memory reads (the slot & linked list)

**case 2:** multiple probes

- open addressing: depends on strategy; key may be found near initial slot
- chaining: following multiple list nodes

> open addressing is more CPU friendly

### chaining hashtables impl

their implementations are very easy \
array of:

- linked list: most commonly used
- arrays: more CPU friendly
- trees: defend against worst cases

Simple open addressing strategies have a poor search length distribution (have to do a lot of comparisons). Adv open addressing strategies are worse than chaining in worst case because collisions lead to more collisions. Easiest to just impl chaining hastable.

### heap allocs per key

- open addr with data stored in slots (ex. int): 0 allocs
- chaining with data stored in the list node: 1 alloc

$\newline$

- open addr with ptrs to data (ex. string): 1 alloc
- chaining with ptrs to data: 2 alloc

### memory overhead per key

open addr:

- slot contains auxiliary data (ex. deletion bit; flag for occupied or deleted)
- emtpy slots

chaining with linked list:

- each slot is a ptr of constant size
- each slot needs a list node of constant size

### shrinking a hashtable?

premature optimization:

- usage pattern may be periodic
- the saved memory may not be returned to OS due to fragmentation

### general purpose libraries

`std::unordered_map` is guaranteed to allocate keys on the heap, so references to keys remain valid after resizing, and keys need not be movable, BUT this is suboptimal for cases without such requirements

general purpose libraries are often not optimized for extreme cases (latency)

## Intrusive Data Structures

are a way of making generic collections (ie. the data type is unknown)

### examples of generic collections

```C++
template <class T> // slow compilation
struct Node {
    T data;
    struct Node *next;
};

struct Node { // requires 1 extra heap alloc & a lot of type casting
    void *data; // points to anything
    struct Node *next;
};
```

### embedding structures within data 

```C++
struct Node {
    Node *next;
};

struct MyData {
    int foo;
    Node node;  // embedded structure
    // whatever ...
};
```

intrusive chaining hashtables, a lookup returns a list node instead of the data

```C++
Node *my_node = some_lookup_function();
MyData *my_data = container_of(my_node, MyData, node);
//                             ^ptr     ^type   ^member
```

### summary

> always consider latency \& worst cases when handling lots of data

advantages:

- generic
  - doesn't require code to be in the header
  - can use in C
  - can have different data types in same collection
- no internal memory management
  - borrows space from data
  - flexible; data allocs are up to user (ie. user must alloc space for new data)
- no assumptions ab data ownership
  - linked lists assume ownership of the data they store, intrusive data structures store ptrs/refs to existing data elts

disadvantages:

- need type casting
- harder to impl than STL
