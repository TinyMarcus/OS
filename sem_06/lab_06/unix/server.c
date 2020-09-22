#include "info.h"

int sock;

void catch_sigint(int signum)
{
    printf("\nSocket is closing\n");
    close(sock);
    unlink(NAME_SOCKET);
    exit(0);
}

int main(void)
{
    char message[LEN_MESSAGE];
    struct sockaddr_un addr;

    sock = socket(AF_UNIX, SOCK_DGRAM, 0);

    if (sock < 0)
    {
        perror("Socket error\n");
        return(sock);
    }

    addr.sun_family = AF_UNIX;
    strcpy(addr.sun_path, NAME_SOCKET);

    if (bind(sock, (struct sockaddr *)&addr, sizeof(addr)) < 0)
    {
        perror("Bind error\n");
        close(sock);
        unlink(NAME_SOCKET);
        return(-1);
    }

    printf("\nServer is waiting for message\n");
    signal(SIGINT, catch_sigint);

    for (;;)
    {
        int size = recv(sock, message, sizeof(message), 0);

        if (size < 0)
        {
            perror("Recv error\n");
            close(sock);
            unlink(NAME_SOCKET);
            return(size);
        }
        message[size] = 0;
        printf("Server got message: %s\n", message);
    }

    printf("Socket is closing\n");
    close(sock);
    unlink(NAME_SOCKET);
    return(0);
}




