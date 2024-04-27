# 7: Basic Server: get, set, del

The “command” in our design is a list of strings, like `set` `key` `val`. We’ll encode the “command” with the following scheme.

```
+------+-----+------+-----+------+-----+-----+------+
| nstr | len | str1 | len | str2 | ... | len | strn |
+------+-----+------+-----+------+-----+-----+------+
```

The response is a 32-bit status code followed by the response string.

```
+-----+---------+
| res | data... |
+-----+---------+
```

```C++
enum {
    RES_OK = 0,
    RES_ERR = 1,
    RES_NX = 2,
};
```
