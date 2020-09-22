#include "info.h"

int main()
{
    int sock = socket(AF_UNIX, SOCK_DGRAM, 0);
    if (sock < 0)
    {
        perror("Socket error\n");
        return(sock);
    }

    struct sockaddr_un server_addr;
    server_addr.sun_family = AF_UNIX;
    strcpy(server_addr.sun_path, NAME_SOCKET);

    srand(time(NULL));

    for (int i = 0; i < QNT_MESSAGE; i++)
    {
        sleep(rand() % 3 + 1);
        char message[LEN_MESSAGE];
        sprintf(message, "Message %d from client %d", i + 1, getpid());
        sendto(sock, message, strlen(message), 0,
                        (struct sockaddr *)&server_addr, sizeof(server_addr));
    }

    close(sock);
    return 0;
}

