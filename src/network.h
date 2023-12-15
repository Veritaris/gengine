//
// Created by Георгий Имешкенов on 11.12.2023.
//

#define SOCK_PATH "/tmp/gengine.sock"
#define SOCK_FAM AF_UNIX
#define SOCK_TYPE SOCK_STREAM
#define MAXDATABUFFLEN 1024
#define KEY_ENTER 10
#define KEY_ESC 27
#define KEY_SPACE 32

void
handle_socket_bind(int socket) {
    printf("\torigin error code: %d\n", errno);
    switch (errno) {
        case EACCES: {
            printf("\tThe address is protected, and the user is not the superuser.\n");
            break;
        }
        case EADDRINUSE: {
            printf("\tThe socket is already bound to an address.\n");
            break;
        }
        case EBADF: {
            printf("\t%d is not a valid file descriptor.", socket);
            break;
        }
        case EINVAL: {
            printf("\taddrlen is wrong, or addr is not a valid address for this socket's domain.\n");
            break;
        }
        case ENOTSOCK: {
            printf("\tThe file descriptor '%d' does not refer to a socket.", socket);
            break;
        }
        case EAFNOSUPPORT: {
            printf("\tAddress family not supported by protocol family.\n");
            break;
        }
//        UNIX-specific errors (AF_UNIX)
        case EADDRNOTAVAIL: {
            printf("\tA nonexistent interface was requested or the requested address was not local.\n");
            break;
        }
        case EFAULT: {
            printf("\taddr points outside the user's accessible address space.\n");
            break;
        }
        case ELOOP: {
            printf("\tToo many symbolic links were encountered in resolving addr.\n");
            break;
        }
        case ENAMETOOLONG: {
            printf("\taddr is too long.\n");
            break;
        }
        case ENOENT: {
            printf("\tA component in the directory prefix of the socket pathname does not exist.\n");
            break;
        }
        case ENOMEM: {
            printf("\tInsufficient kernel memory was available.\n");
            break;
        }
        case ENOTDIR: {
            printf("\tA component of the path prefix is not a directory.\n");
            break;
        }
        case EROFS: {
            printf("\tThe socket inode would reside on a read-only filesystem.\n");
            break;
        }
        default: {
            printf("\tUnknown error: %d\n", errno);
        }
    }
}
