/** @file       server.h
 *  @brief      server header file.
 *  @copyright  (c) 2021-denizugur - All Rights Reserved
 *              Permission to use, reproduce, copy, prepare derivative works,
 *              modify, distribute, perform, display or sell this software and/or
 *              its documentation for any purpose is prohibited without the express
 *              written consent of denizugur.
 *  @author     Deniz UGUR
 *  @contact	deniz.ugur@ozu.edu.tr
 *  @date       04.11.2021
 */
#ifndef FILE_SERVER_H
#define FILE_SERVER_H

/** INCLUDES ******************************************************************/
#include <stddef.h>
#include "main.h"

/** CONSTANTS *****************************************************************/
#define SA struct sockaddr

/** TYPEDEFS ******************************************************************/
typedef enum
{
    PORT_CLIENT_ZERO = 8080U,
    PORT_CLIENT_ONE = 8081U,
    PORT_CLIENT_TWO = 8082U,
    PORT_CLIENT_THREE = 8083U,
} port_nums_t;

typedef enum
{
    CLIENT_0 = 0x00,
    CLIENT_1,
    CLIENT_2,
    CLIENT_3,
} client_ID_t;
/** MACROS ********************************************************************/

#ifndef FILE_SERVER_C
#define INTERFACE extern
#else
#define INTERFACE
#endif

/** VARIABLES *****************************************************************/
INTERFACE int sockfd[NUM_OF_CLIENTS];
INTERFACE int connfd[NUM_OF_CLIENTS];
INTERFACE uint32_t lenn[NUM_OF_CLIENTS];
INTERFACE struct sockaddr_in servaddr[NUM_OF_CLIENTS];
INTERFACE struct sockaddr_in client[NUM_OF_CLIENTS];

/** FUNCTIONS *****************************************************************/

INTERFACE void ServerInit(void);

INTERFACE size_t read_from_socket(int socketfd, uint8_t *buffer, int size);

#undef INTERFACE // Should not let this roam free

#endif // FILE_SERVER_H