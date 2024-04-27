# 5: The Event Loop & Nonblocking IO

How do we deal with concurrent connections in server-sided network programming?

1. **forking**: creates new processes for each client connection to achieve concurrency
2. **multi-threading**: uses threads instead of processes
3. **event loops**

## Event Loops
uses polling and nonbllocking IO and usually runs on a single thread *(used by most modern production-grade software)*

- **polling:** querying the fds (in our case: listening sockets + client connections) to see if there are any events waiting that can be *immediately* processed without blocking

when we perform an IO operation on the fd, the operation should be performed in **nonblocking** mode

in **blocking mode** *(only these 3 operations have non-blocking mode, other operations should be performed in thread pools; covered later)*

- `read()` blocks the caller when there is no data in the kernel
- `write()` blocks when the write buffer is full
- `accept()` blocks when there are no new connections in kernel queue

in **non-blocking mode**, these operations either success without blocking, or fail with `errno = EAGAIN` ("not ready")

> the `poll` should be the **only blocking operation** in an event loop

### pseudo-code
```C++
all_fds = [...]
while True:
    active_fds = poll(all_fds)
    for each fd in active_fds:
        do_something_with(fd)

def do_something_with(fd):
    if fd is a listening socket:
        add_new_client(fd)
    elif fd is a client connection:
        while work_not_done(fd):
            do_something_to_client(fd)

def do_something_to_client(fd):
    if should_read_from(fd):
        data = read_until_EAGAIN(fd)
        process_incoming_data(data)
    while should_write_to(fd):
        write_until_EAGAIN(fd)
    if should_close(fd):
        destroy_client(fd)

```

### how to set to non-blocking mode?

```C++
static void fd_set_nb(int fd) {
    errno = 0;
    int flags = fcntl(fd, F_GETFL, 0); // sets to non-blocking here
    if (errno) {
        die("fcntl error");
        return;
    }

    flags |= O_NONBLOCK;

    errno = 0;
    (void)fcntl(fd, F_SETFL, flags);
    if (errno) {
        die("fcntl error");
    }
}
```

### `poll`

aside from `poll` there is:

- `select` which is basically the same as `poll` but has smaller max fd (useless in modern apps)
- `epoll` API is stateful
  - when you register another fd with `epoll_ctl` *(used to manipulate the set created by `epoll_create`)*, you specify the types of events (like reading, writing) you're interested in for that descriptor
  - subsequent calls to epoll (`epoll_wait`) don't require you to provide the entire list of file descriptors again
  - kernel efficiently tracks the state and only notifies you about events that have occurred on the registered descriptors
  - > preferable in real world as you may gave too many fds for `poll`

> **TODO:** the exercises at the end of ch6