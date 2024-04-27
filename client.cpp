#include <assert.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/ip.h>
#include <string>
#include <vector>

// -------------------------------- CH 3 -------------------------------- 
static void msg(const char *msg) {
    fprintf(stderr, "%s\n", msg);
}

static void die(const char *msg) {
    int err = errno;
    fprintf(stderr, "[%d] %s\n", err, msg);
    abort();
}

// -------------------------------- CH 4 -------------------------------- 
const size_t k_max_msg = 4096;

// look @ server.cpp for comments
static int32_t read_full(int fd, char *buf, size_t n) {
    while (n > 0) {
        ssize_t rv = read(fd, buf, n);
        if (rv <= 0) {
            return -1;  // error, or unexpected EOF
        }
        assert((size_t)rv <= n);
        n -= (size_t)rv;
        buf += rv;
    }
    return 0;
}

// look @ server.cpp for comments
static int32_t write_all(int fd, const char *buf, size_t n) {
    while (n > 0) {
        ssize_t rv = write(fd, buf, n);
        if (rv <= 0) {
            return -1;  // error
        }
        assert((size_t)rv <= n);
        n -= (size_t)rv;
        buf += rv;
    }
    return 0;
}

/*
// making requests and receiving responses
static int32_t query(int fd, const char *text) {
    // make request 
    uint32_t len = (uint32_t)strlen(text); // len of msg
    if (len > k_max_msg) return -1;
    
    char wbuf[4 + k_max_msg];
    memcpy(wbuf, &len, 4); // cpy len of msg into first 4 bytes (assume little endian)
    memcpy(&wbuf[4], text, len); // cpy msg into the remaining space

    if (int32_t err = write_all(fd, wbuf, 4 + len)) // send msg to server
        return err;

    // receive message
    char rbuf[4 + k_max_msg + 1]; // create buffer for read
    errno = 0;
    int32_t err = read_full(fd, rbuf, 4); // read from fd describing server socket (only first 4 bytes for msg len)
    if (err) {
        if (errno == 0) msg("EOF");
        else msg("read() error");
        return err;
    }
    
    memcpy(&len, rbuf, 4); // copy the received msg len to first 4 byte of rbuf
    if (len > k_max_msg) {
        msg("too long"); 
        return -1;
    }

    err = read_full(fd, &rbuf[4], len); // read len bytes of msg from fd
    if (err) {
        msg("read() error");
        return err;
    }

    // do something with the msg
    rbuf[4 + len] = '\0';
    printf("server says: %s\n", &rbuf[4]);
    return 0;
}
*/

// -------------------------------- CH 6 -------------------------------- 
static int32_t send_req(int fd, const std::vector<std::string> &cmd) {
    uint32_t len = 4; // space for 
    for (const std::string &s : cmd) {
        len += 4 + s.size();
    }
    if (len > k_max_msg) {
        return -1;
    }

    char wbuf[4 + k_max_msg]; // create a write buffer
    memcpy(&wbuf[0], &len, 4);  // add total length of req (assume little endian)
    uint32_t n = cmd.size(); // number of cmds
    memcpy(&wbuf[4], &n, 4); 
    size_t cur = 8; 
    for (const std::string &s : cmd) {
        uint32_t p = (uint32_t)s.size();
        memcpy(&wbuf[cur], &p, 4); // cpy the len of msg
        memcpy(&wbuf[cur + 4], s.data(), s.size()); // cpy msg
        cur += 4 + s.size();
    }
    return write_all(fd, wbuf, 4 + len); // write it to the wbuf
}

static int32_t read_res(int fd) {
    // receive message len
    char rbuf[4 + k_max_msg + 1]; // create buffer for read
    errno = 0;
    int32_t err = read_full(fd, rbuf, 4); // read from fd describing server socket (only first 4 bytes for msg len)
    if (err) {
        if (errno == 0) msg("EOF");
        else msg("read() error");
        return err;
    }
    
    uint32_t len;
    memcpy(&len, rbuf, 4); // copy the received msg len to first 4 byte of rbuf (assume little endian)
    if (len > k_max_msg) {
        msg("too long"); 
        return -1;
    }

    // receive message body
    err = read_full(fd, &rbuf[4], len); // read len bytes of msg from fd
    if (err) {
        msg("read() error");
        return err;
    }

    // print the result
    uint32_t rescode = 0;
    if (len < 4) {
        msg("bad response");
        return -1;
    }
    memcpy(&rescode, &rbuf[4], 4);
    printf("server says: [%u] %.*s\n", rescode, len - 4, &rbuf[8]);
    return 0;
}

int main(int argc, char **argv) {
    int fd = socket(AF_INET, SOCK_STREAM, 0); // get socket fd
    if (fd < 0) {
        die("socket()");
    }

    // connect socket to an address
    struct sockaddr_in addr = {}; 
    addr.sin_family = AF_INET;
    addr.sin_port = ntohs(1234);
    addr.sin_addr.s_addr = ntohl(INADDR_LOOPBACK);  // 127.0.0.1
    int rv = connect(fd, (const struct sockaddr *)&addr, sizeof(addr));
    if (rv) {
        die("connect");
    }

    // multiple pipelined requests
    std::vector<std::string> cmd;
    for (int i = 1; i < argc; ++i) {
        cmd.push_back(argv[i]);
    }
    int32_t err = send_req(fd, cmd);
    if (err) {
        goto L_DONE;
    }
    err = read_res(fd);
    if (err) {
        goto L_DONE;
    }


L_DONE:
    close(fd);
    return 0;
}