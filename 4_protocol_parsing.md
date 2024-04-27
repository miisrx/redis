**Apr 16, 2024**

# 4: Protocol Parsing

What if we want to handle >1 request at a time? \
We need some sort of "protocol" to split the requests from the TCP byte stream \
The easiest way to do this is to declare the length, then follow with the message

```
+-----+------+-----+------+--------
| len | msg1 | len | msg2 | ...
+-----+------+-----+------+--------
```

## Protocol Design

### Text vs. Binary

Lets say you want to parse "1234". \
In binary, this is a simple fixed-width binary int. \
In text, this is a string and you have to check whether the buffer has ended or end of integer.

Text is:

- more human readable (easier debug)
- more complex because of variable length
  - len calculations
  - bound checks

### streaming data

our protocol includes the length at the beginning, but real-world protocols use less obvious ways to indicate end of message

**chunked transfer encoding** (from HTTP protocol) breaks the data into chunks and sends them each with a length header, then a special message indicates end of stream.

> TODO: try optimixing with buffered IO

- everytime `read`/`write` is called, the data transfer happens between program's memory and the underlying storage device (HDD, SSD); inefficient for system calls to switch between prog + OS
- buffered IO introduces a buffer between the prog and the OS (allows transfer of blocks of data at a time, instead of a single byte)
  - reduces sys calls
  - copying data between prog and buffer is generally faster