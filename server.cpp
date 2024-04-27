#include <assert.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>
#include <fcntl.h>
#include <poll.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/ip.h>
#include <string>
#include <vector>
#include <map>
using namespace std; 

// -------------------------------- CH 3 -------------------------------- 
static void msg(const char *msg) {
    fprintf(stderr, "%s\n", msg);
}

static void die(const char *msg) {
    int err = errno;
    fprintf(stderr, "[%d] %s\n", err, msg);
    abort();
}

/*
static void do_something(int connfd) {
    char rbuf[64] = {};
    ssize_t n = read(connfd, rbuf, sizeof(rbuf) - 1);
    if (n < 0) {
        msg("read() error");
        return;
    }
    printf("client says: %s\n", rbuf);

    char wbuf[] = "world";
    write(connfd, wbuf, strlen(wbuf));
}
*/

// -------------------------------- CH 4 -------------------------------- 
const size_t k_max_msg = 4096;
/*
// reads all n bytes from the kernel 
//  we need this bc read() syscall only reads the data available on the kernel
static int read_full(int fd, char *buf, size_t n){
    while(n > 0) {
        ssize_t rv = read(fd, buf, n);
        if (rv <= 0) return -1;
        assert((size_t)rv <= n);
        n -= (size_t)rv;
        buf+= rv;
    }
    return 0;
}

static int write_all(int fd, char *buf, size_t n) {
    while (n > 0) {
        ssize_t rv = write(fd, buf, n);
        if (rv <= 0) return -1; // error
        assert((size_t)rv <= n);
        n -= (size_t)rv;
        buf += rv; // updates the buffer ptr to the next portion that needs to be written
    }
    return 0;
}

// parser func to handle >1 request; "splits" the byte stream
static int32_t one_request(int connfd) {
    char rbuf[4 + k_max_msg + 1]; // 4 bytes contianing msg len + max msg len + null terminator
    errno = 0;
    int32_t err = read_full(connfd, rbuf, 4); // reads the first 4 bytes into rbuf
    if (err) {
        if (errno == 0) {
            msg("EOF");
        } else {
            msg("read() error");
        }
        return err;
    }

    uint32_t len = 0;
    memcpy(&len, rbuf, 4);  // cpy the length of the msg into len (assume little endian)
    if (len > k_max_msg) {
        msg("too long");
        return -1;
    }

    // request body
    err = read_full(connfd, &rbuf[4], len); // read the msg (len bytes) into the rest of rbuf
    if (err) {
        msg("read() error");
        return err;
    }

    // do something
    rbuf[4 + len] = '\0';
    printf("client says: %s\n", &rbuf[4]);

    // reply using the same protocol
    const char reply[] = "world";
    char wbuf[4 + sizeof(reply)];
    len = (uint32_t)strlen(reply); // # bytes of reply
    memcpy(wbuf, &len, 4); // cpy len of reply into buffer
    memcpy(&wbuf[4], reply, len); // cpy reply into buffer
    return write_all(connfd, wbuf, 4 + len); // write len + msg into byte stream
}
*/

// -------------------------------- CH 6 -------------------------------- 
// set fd to non-blocking mode
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

enum { // connection states
    STATE_REQ = 0, // read req
    STATE_RES = 1, // send response
    STATE_END = 2,  // mark the connection for deletion
};

// buffer for read/write since in non-blocking mode, IO operations are often deferred
struct Conn {
    int fd = -1;
    uint32_t state = 0;     // either STATE_REQ or STATE_RES
    // buffer for reading
    size_t rbuf_size = 0;
    uint8_t rbuf[4 + k_max_msg];
    // buffer for writing
    size_t wbuf_size = 0;
    size_t wbuf_sent = 0;
    uint8_t wbuf[4 + k_max_msg];
};

// helper for accept_new_conn
static void conn_put(std::vector<Conn *> &fd2conn, struct Conn *conn) {
    // since we are using fd2conn[fd] = conn, fd2conn must be big enough for this idx
    if (fd2conn.size() <= (size_t)conn->fd) {
        fd2conn.resize(conn->fd + 1);
    }
    fd2conn[conn->fd] = conn;
}

// accepts a new connection & creates struct Conn obj
static int32_t accept_new_conn(std::vector<Conn *> &fd2conn, int fd) {
    // accept
    struct sockaddr_in client_addr = {};
    socklen_t socklen = sizeof(client_addr);
    int connfd = accept(fd, (struct sockaddr *)&client_addr, &socklen);
    if (connfd < 0) {
        msg("accept() error");
        return -1;  // error
    }

    fd_set_nb(connfd); // set the new connection fd to nonblocking mode
    // creating the struct Conn
    struct Conn *conn = (struct Conn *)malloc(sizeof(struct Conn));
    if (!conn) {
        close(connfd);
        return -1;
    }
    conn->fd = connfd;
    conn->state = STATE_REQ;
    conn->rbuf_size = 0;
    conn->wbuf_size = 0;
    conn->wbuf_sent = 0;
    conn_put(fd2conn, conn);
    return 0;
}

static void state_req(Conn *conn);
static void state_res(Conn *conn);

static int32_t do_request( const uint8_t *req, uint32_t reqlen,
                            uint32_t *rescode, uint8_t *res, uint32_t *reslen); // CH 7

// inner loop for try_fill_buffer()
//  handles 1 req at a time from read buffer
static bool try_one_request(Conn *conn) {
    // try to parse a request from the buffer
    if (conn->rbuf_size < 4) {
        // not enough data in the buffer. Will retry in the next iteration
        return false;
    }
    uint32_t len = 0;
    memcpy(&len, &conn->rbuf[0], 4);
    if (len > k_max_msg) {
        msg("too long");
        conn->state = STATE_END;
        return false;
    }
    if (4 + len > conn->rbuf_size) {
        // not enough data in the buffer. Will retry in the next iteration
        return false;
    }

    // got one request, generate the response.
    uint32_t rescode = 0;
    uint32_t wlen = 0;
    int32_t err = do_request(
        &conn->rbuf[4], len,
        &rescode, &conn->wbuf[4 + 4], &wlen
    );
    if (err) {
        conn->state = STATE_END;
        return false;
    }
    wlen += 4;
    memcpy(&conn->wbuf[0], &wlen, 4);
    memcpy(&conn->wbuf[4], &rescode, 4);
    conn->wbuf_size = 4 + wlen;

    // remove the request from the buffer.
    // note: frequent memmove is inefficient.
    // note: need better handling for production code.
    size_t remain = conn->rbuf_size - 4 - len;
    if (remain) {
        // move the stuff after this req up to the front
        memmove(conn->rbuf, &conn->rbuf[4 + len], remain); 
    }
    conn->rbuf_size = remain;

    // change state
    conn->state = STATE_RES;
    state_res(conn);

    // continue the outer loop (try_one_request()) if the request was fully processed
    return (conn->state == STATE_REQ);
}

// inner loop for state_req()
//  tries to fill read buffer 
static bool try_fill_buffer(Conn *conn) {
    assert(conn->rbuf_size < sizeof(conn->rbuf));
    ssize_t rv = 0;
    do {
        size_t cap = sizeof(conn->rbuf) - conn->rbuf_size; // size of buffer - cur idx
        rv = read(conn->fd, &conn->rbuf[conn->rbuf_size], cap);
    } while (rv < 0 && errno == EINTR); // EINTR = sys call interrupted by a signal before completion
    
    // handle read errors
    if (rv < 0 && errno == EAGAIN) {
        // got EAGAIN, stop.
        return false;
    }
    if (rv < 0) {
        msg("read() error");
        conn->state = STATE_END;
        return false;
    }
    if (rv == 0) {
        if (conn->rbuf_size > 0) {
            msg("unexpected EOF");
        } else {
            msg("EOF");
        }
        conn->state = STATE_END;
        return false;
    }

    // adjust read buffer of struct Conn
    conn->rbuf_size += (size_t)rv;
    assert(conn->rbuf_size <= sizeof(conn->rbuf));

    // Try to process requests one by one.
    // Why loop?
    //  there can be >1 req in the buffer
    while (try_one_request(conn)) {} 
    return (conn->state == STATE_REQ);
}

// handles read request
static void state_req(Conn *conn) {
    // Why is there a loop?
    //  pipelining: a client can send multiple reqs without waiting for a response
    //  but we have a limited buffer,
    //  so we fill the buffer, handle req(s) in buffer, repeat
    while (try_fill_buffer(conn)) {}
}

// inner loop for state_req()
//  tries to clear write buffer
static bool try_flush_buffer(Conn *conn) {
    ssize_t rv = 0;
    do { // keep trying to write if there is an interruption 
        size_t remain = conn->wbuf_size - conn->wbuf_sent; // remaining write buffer space
        rv = write(conn->fd, &conn->wbuf[conn->wbuf_sent], remain);
    } while (rv < 0 && errno == EINTR);
    if (rv < 0 && errno == EAGAIN) {
        // got EAGAIN, stop.
        return false;
    }
    if (rv < 0) {
        msg("write() error");
        conn->state = STATE_END;
        return false;
    }
    conn->wbuf_sent += (size_t)rv;
    assert(conn->wbuf_sent <= conn->wbuf_size);
    if (conn->wbuf_sent == conn->wbuf_size) {
        // response was fully sent, change state back
        conn->state = STATE_REQ;
        conn->wbuf_sent = 0;
        conn->wbuf_size = 0;
        return false;
    }
    // still got some data in wbuf, could try to write again
    return true;
}

// handles send response 
static void state_res(Conn *conn) {
    while (try_flush_buffer(conn)) {}
}

static void connection_io(Conn *conn) {
    if (conn->state == STATE_REQ) {
        state_req(conn);
    } else if (conn->state == STATE_RES) {
        state_res(conn);
    } else {
        assert(0);  // not expected
    }
}

// -------------------------------- CH 7 -------------------------------- 
const size_t k_max_args = 1024;

// puts all reqs into a vector<string>
static int32_t parse_req(const uint8_t *data, size_t len, std::vector<std::string> &out) {
    if (len < 4) {
        return -1;
    }
    uint32_t n = 0; // stores total number of commands
    memcpy(&n, &data[0], 4);  
    if (n > k_max_args) {
        return -1;
    }

    size_t pos = 4; 
    while (n--) {
        if (pos + 4 > len) {
            return -1;
        }
        uint32_t sz = 0;
        memcpy(&sz, &data[pos], 4); // get the len of the next cmd (4 bytes)
        if (pos + 4 + sz > len) {
            return -1;
        }
        out.push_back(std::string((char *)&data[pos + 4], sz)); // add the next cmd to vector
        pos += 4 + sz;
    }

    if (pos != len) {
        return -1;  // trailing garbage
    }
    return 0;
}

enum {
    RES_OK = 0,
    RES_ERR = 1,
    RES_NX = 2,
};

// The data structure for the key space. This is just a placeholder
// until we implement a hashtable in the next chapter.
static std::map<std::string, std::string> g_map;

static uint32_t do_get(const std::vector<std::string> &cmd, uint8_t *res, uint32_t *reslen) {
    if (!g_map.count(cmd[1])) { // if there is no next cmd, err
        return RES_NX;
    }
    std::string &val = g_map[cmd[1]]; // what are we getting?
    assert(val.size() <= k_max_msg);
    memcpy(res, val.data(), val.size()); // cpy into res
    *reslen = (uint32_t)val.size();
    return RES_OK;
}

static uint32_t do_set(const std::vector<std::string> &cmd, uint8_t *res, uint32_t *reslen) {
    (void)res;
    (void)reslen;
    g_map[cmd[1]] = cmd[2];
    return RES_OK;
}

static uint32_t do_del(const std::vector<std::string> &cmd, uint8_t *res, uint32_t *reslen) {
    (void)res;
    (void)reslen;
    g_map.erase(cmd[1]);
    return RES_OK;
}

static bool cmd_is(const std::string &word, const char *cmd) {
    return 0 == strcasecmp(word.c_str(), cmd);
}

static int32_t do_request(const uint8_t *req, uint32_t reqlen, 
                            uint32_t *rescode, uint8_t *res, uint32_t *reslen) {
    vector<string> cmd;
    if (0 != parse_req(req, reqlen, cmd)) {
        msg("bad req");
        return -1;
    }
    if (cmd.size() == 2 && cmd_is(cmd[0], "get")) {
        *rescode = do_get(cmd, res, reslen);
    } else if (cmd.size() == 3 && cmd_is(cmd[0], "set")) {
        *rescode = do_set(cmd, res, reslen);
    } else if (cmd.size() == 2 && cmd_is(cmd[0], "del")) {
        *rescode = do_del(cmd, res, reslen);
    } else {
        // cmd is not recognized
        *rescode = RES_ERR;
        const char *msg = "Unknown cmd";
        strcpy((char *)res, msg);
        *reslen = strlen(msg);
        return 0;
    }
    return 0;
}

// -----------------------------------------------------------------

int main() {
    // get a TCP socket fd
    int fd = socket(AF_INET, SOCK_STREAM, 0); // AF_INET is the IP level protocol
    if (fd < 0) {
        die("socket()");
    }
    
    // configure the socket
    int val = 1;
    setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &val, sizeof(val)); // SO_REUSEADDR help res
    
    // bind the socket to an address (associates our local network addr with the socket)
    struct sockaddr_in addr = {};
    addr.sin_family = AF_INET; // IPv4
    addr.sin_port = ntohs(1234); // network to host byte order (short; 2-byte)
    addr.sin_addr.s_addr = ntohl(0); 
    int rv = bind(fd, (const sockaddr *)&addr, sizeof(addr)); // cast sockaddr_in to sockaddr
    if (rv) {
        die("bind()");
    }

    // listen
    rv = listen(fd, SOMAXCONN); // SOMAXCONN = 128 on linux (max queue size)
    if (rv) {
        die("listen()");
    }

    // event loop
    vector<Conn *> fd2conn; // map of all client connections, keyed by fd
    fd_set_nb(fd); // syscall to set the listening fd to nonblocking mode
    vector<struct pollfd> poll_args; // the active fds

    while (true) {
        // prepare the args of poll()
        poll_args.clear(); 
        struct pollfd pfd = {fd, POLLIN, 0}; 
        poll_args.push_back(pfd); // (for convenience) put listening fd in first posn
        // go through connections 
        for (Conn *conn : fd2conn) { 
            if (!conn) continue; // not a valid ptr
            struct pollfd pfd = {};
            pfd.fd = conn->fd;
            pfd.events = (conn->state == STATE_REQ) ? POLLIN : POLLOUT;
            pfd.events = pfd.events | POLLERR; // sets to pollerr if error
            poll_args.push_back(pfd);
        }

        // poll for active fds (marked with POLLIN or POLLOUT)
        int rv = poll(poll_args.data(), (nfds_t)poll_args.size(), 1000); // timeout value = 1000 (not rly relevant rn)
        if (rv < 0) die("poll");

        // process active connections
        for (size_t i = 1; i < poll_args.size(); ++i) {
            if (poll_args[i].revents) { // .revents tell you which event (ie. POLLIN/OUT) triggered poll (active fd)
                Conn *conn = fd2conn[poll_args[i].fd]; 
                connection_io(conn); // handles according to read request/ send reponse
                if (conn->state == STATE_END) {
                    // client closed normally, or smt bad happened
                    // destroy connection
                    fd2conn[conn->fd] = NULL;
                    (void)close(conn->fd);
                    free(conn);
                }
            }
        }

        // try to accept a new connection if the listening fd is active
        if (poll_args[0].revents) {
            (void)accept_new_conn(fd2conn, fd);
        }
    }

    return 0;
}