# 10: AVL Tree

Redis is a key-value store, where the "value"s are:
- list: doulbe ended queue
- hasmap
- sorted set

## Sorted Set

is a collection of `(score, name)` pairs indexed:

- find score by name
- range query (get a subset of the sorted pairs)
  - get closest pair to a target
  - iterate asc/desc from the starting pair
- rank query
  - get rank of a pair
  - get pair by rank
  - offset a pair

a sorted set may be usefule for displaying a scoreboard  

continue: Rotations Keep the Order
