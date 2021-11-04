#define FILE_SERVER_C

/** INCLUDES ******************************************************************/
#include <stdio.h>
#include <netinet/in.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>
#include <arpa/inet.h>

#include "server.h"

/** CONSTANTS *****************************************************************/

/** TYPEDEFS ******************************************************************/
struct sockaddr_in servaddr[NUM_OF_CLIENTS];
struct sockaddr_in client[NUM_OF_CLIENTS];

/** MACROS ********************************************************************/

/** VARIABLES *****************************************************************/
int sockfd[NUM_OF_CLIENTS];
int connfd[NUM_OF_CLIENTS];
uint32_t lenn[NUM_OF_CLIENTS];

int opt_val = 1;

/** LOCAL FUNCTION DECLARATIONS ***********************************************/

/** INTERFACE FUNCTION DEFINITIONS ********************************************/

/** LOCAL FUNCTION DEFINITIONS ************************************************/

size_t read_from_socket(int socketfd, uint8_t *buffer, int size)
{
    size_t bytes_read;

    /* Read the data from socket */
    bytes_read = read(socketfd, buffer, size);

    return bytes_read;
}

void ServerInit(void)
{
    for (int cli = 0; cli < NUM_OF_CLIENTS; cli++)
    {
        // socket create and verification
        sockfd[cli] = socket(AF_INET, SOCK_STREAM, 0);
        if (sockfd[cli] == -1)
        {
            printf("socket creation failed...\n");
            exit(0);
        }
        else
        {
            printf("Socket successfully created..\n");
        }

        bzero(&servaddr[cli], sizeof(servaddr[cli]));

        // assign IP, PORT
        servaddr[cli].sin_family = AF_INET;
        servaddr[cli].sin_addr.s_addr = htonl(INADDR_ANY);
        switch (cli)
        {
        case CLIENT_0:
            servaddr[cli].sin_port = htons(PORT_CLIENT_ZERO);
            break;
        case CLIENT_1:
            servaddr[cli].sin_port = htons(PORT_CLIENT_ONE);
            break;
        case CLIENT_2:
            servaddr[cli].sin_port = htons(PORT_CLIENT_TWO);
            break;
        case CLIENT_3:
            servaddr[cli].sin_port = htons(PORT_CLIENT_THREE);
            break;
        default:
            break;
        }

        struct timeval tv;
        tv.tv_sec = 0;
        tv.tv_usec = 100;

        // Added to reuse socket
        setsockopt(sockfd[cli], SOL_SOCKET, SO_REUSEADDR, &opt_val, sizeof opt_val);
        setsockopt(sockfd[cli], SOL_SOCKET, SOCK_NONBLOCK, &opt_val, sizeof opt_val);

        // Binding newly created socket to given IP and verification
        if ((bind(sockfd[cli], (SA *)&servaddr[cli], sizeof(servaddr[cli]))) != 0)
        {
            printf("socket bind failed...\n");
            exit(0);
        }
        else
        {
            printf("Socket successfully binded...\n");
        }

        // Now server is ready to listen and verification
        if ((listen(sockfd[cli], 5)) != 0)
        {
            printf("Listen failed...\n");
            exit(0);
        }
        else
        {
            switch (cli)
            {
            case CLIENT_0:
                printf("Server listening on PORT %d...\n", PORT_CLIENT_ZERO);
                break;
            case CLIENT_1:
                printf("Server listening on PORT %d...\n", PORT_CLIENT_ONE);
                break;
            case CLIENT_2:
                printf("Server listening on PORT %d...\n", PORT_CLIENT_TWO);
                break;
            case CLIENT_3:
                printf("Server listening on PORT %d...\n", PORT_CLIENT_THREE);
                break;
            default:
                break;
            }
        }
        lenn[cli] = sizeof(client[cli]);

        // Accept the data packet from client and verification
        connfd[cli] = accept(sockfd[cli], (SA *)&client[cli], &lenn[cli]);
        //printf("Hi\n");
        if (connfd[cli] < 0)
        {
            printf("server accept failed...\n");
            exit(0);
        }
        else
        {
            printf("Server accepted client with IP: %s from PORT: %d\n",
                   inet_ntoa(client[cli].sin_addr), ntohs(client[cli].sin_port));
        }
    }
}
