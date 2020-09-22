#include "info.h"

int main()
{
    int sock = socket(AF_INET, SOCK_STREAM, 0);

    if (sock < 0)
    {
        perror("Socket error\n");
        exit(-1);
    }

    struct hostent* host = gethostbyname(SOCKET_ADDRESS);

    if (host = NULL)
    {
        perror("Getting host by name error\n");
        close(sock);
        exit(-1);
    }

    struct sockaddr_in server_addr;
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(SOCK_PORT);
    server_addr.sin_addr = *((struct in_addr*) host->h_addr_list[0]);

    if (connect(sock, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0)
    {
        perror("Connect error\n");
        close(sock);
        return -1;
    }

    for (int i = 0; i < QNT_MESSAGE; i++)
    {
        char message[LEN_MESSAGE];
        memset(message, 0, LEN_MESSAGE);
        sprintf(message, "Message #%d from %d", i + 1, getpid());

        if (send(sock, message, strlen(message), 0) < 0)
        {
            perror("Send error\n");
            close(sock);
            return -1;
        }
    }
    
    close(sock);
    return 0;
}