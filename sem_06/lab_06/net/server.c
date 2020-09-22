#include "info.h"

int clients [MAX_CLIENTS] = { 0 }; 
int listener_sock ;

void closeConnections () 
{
    for ( int i = 0; i < MAX_CLIENTS; i++)
    {
        if (clients[i] != 0)
        {
            close(clients[i]);
        }
    } 
}

int handleConnection () 
{
    struct sockaddr_in client_addr;
    int client_size = sizeof(client_addr);
    int client_sock = accept(listener_sock, (struct sockaddr*)&client_addr , (socklen_t*) &client_size);

    if (client_sock < 0)
    {
        perror ("Error in accept ()\n"); 
        closeConnections(); 
        close(listener_sock);
        return -1; 
    }

    printf("\nServer got new connection:\nip = %s:%d\n", inet_ntoa( client_addr.sin_addr), ntohs(client_addr.sin_port));

    for ( int i = 0; i < MAX_CLIENTS; i++) 
    {
        if (clients[i] == 0) 
        {
            clients[i] = client_sock;
            printf("Client number − %d\n", i + 1);
            return 0; 
        }
    }
    return −1;
}

void getMessage(int client_sock , int client_index) 
{
    char msg[MSG_LEN];
    struct sockaddr_in client_addr;
    int addr_size = sizeof(client_addr);
    int msg_size = recv(client_sock , msg, MSG_LEN, 0); 
    
    if (msg_size == 0)
    {
        getpeername(client_sock, (struct sockaddr*)&client_addr, (socklen_t*)&addr_size);
        printf("\nClient %d closed connection %s:%d \n", client_index + 1,
                            inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port)); 
        close(client_sock);
        clients[client_index] = 0;
    }
    else
    {
        msg[msg_size] = ’\0’;
        printf ("\nServer got message from client %d: %s\n" , client_index + 1, msg);
    } 
}

int main( void ) 
{
    listener_sock = socket(AF_INET, SOCK_STREAM, 0);
    
    if (listener_sock < 0) 
    {
        perror("Error in sock()\n");
        return -1; 
    }

    struct sockaddr_in addr ;
    addr.sin_family = AF_INET; 
    addr.sin_port = htons(SOCK_PORT);
    addr.sin_addr.s_addr = INADDR_ANY;

    if (bind(listener_sock, (struct sockaddr*)&addr, sizeof(addr)) < 0) 
    {
        perror("Error in bind ()\n") ;
        close(listener_sock);
        return -1; 
    }

    if (listen(listener_sock , 3) < 0) 
    {
        perror("Error in listen ()\n");
        close(listener_sock);
        return -1; 
    }

    printf("Server started on ip: %s:%d\n",inet_ntoa(addr.sin_addr), ntohs(addr.sin_port));
 
    for (;;) 
    {
        fd_set fds; 
        int max_fd;
        FD_ZERO(&fds);
        FD_SET(listener_sock, &fds); 
        max_fd = listener_sock;

        for (int i = 0; i < MAX_CLIENTS; i++)
        {
            if (clients[i] > 0)
            {
                FD_SET(clients[i], &fds);
            }

            if (clients[i] > max_fd)
            {
                max_fd = clients[i];
            }
        }
    
        if (select(max_fd + 1, &fds , NULL, NULL, NULL) < 0)
        {
            perror("Error in select ()\n"); 
            closeConnections();
            close(listener_sock);
            return -1; 
        }

        if (FD_ISSET(listener_sock, &fds)) 
        {
            if (handleConnection() < 0) 
            {
                perror ("No space for new connections\n") ; 
                closeConnections();
                close(listener_sock);
                return -1;
            }
        }
        
        for ( int i = 0; i < MAX_CLIENTS; i++)
        {
            if ((clients[i] > 0) && FD_ISSET(clients[i], &fds))
            {
                getMessage(clients[i], i);
            }
        } 
    }
    
    closeConnections(); 
    close(listener_sock); 
    return 0;
}