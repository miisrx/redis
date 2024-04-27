**Apr 11,2024**

# 2: Socket Programming Concepts
> a **socket** is a channel for 2 parties to communicate over a network 
- the one initiating the channel is the **client**
- the **server** waits for new clients

there are *2* types of channels:  
- packet-based (eg. UDP): a packet is a message of a certain size
- **byte-stream-based** (eg. TCP): ordered sequence of bytes
    - a **protocol** is used to interpret the bytes (eg. the file format ".pdf" tells us how to interpret the file)

> applications refer to sockets by opaque OS handles (file descriptors)
- they are identifiers that the OS provides to applications to refer to resources such as: files, network connections (sockets), devices
- Imagine you have a room full of boxes, each containing a different item. You are given a set of tags, but these tags don't tell you what's inside the boxes, just a unique identifier for each box. 

## Socket Handles
- Listening sockets: Obtained by listening on an address.
- Connection sockets: Obtained by accepting a client connection from a listening socket.


## DNS lookup
process that translates human-readable domain names (eg. https://www.youtube.com/) to machine-readable IP addresses (eg. 8.8.8.8)
- when you type a domain name in browser, your computer queries a DNS server (which acts like a giant phonebook) to return the associated IP address 
