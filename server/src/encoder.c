/** @file       encoder.h
 *  @brief      encoder source file.
 *  @copyright  (c) 2021-denizugur - All Rights Reserved
 *              Permission to use, reproduce, copy, prepare derivative works,
 *              modify, distribute, perform, display or sell this software and/or
 *              its documentation for any purpose is prohibited without the express
 *              written consent of denizugur.
 *  @author     Deniz UGUR
 *  @contact	deniz.ugur@ozu.edu.tr
 *  @date       04.11.2021
 */
#define FILE_ENCODER_C

/** INCLUDES ******************************************************************/
#include <stdio.h>
#include <stdlib.h>

#include "encoder.h"
#include "server.h"

/** CONSTANTS *****************************************************************/

/** TYPEDEFS ******************************************************************/
encoder_data_t Encoder;

/** MACROS ********************************************************************/

/** VARIABLES *****************************************************************/
uint8_t receiveBuffer[BUFFER_LENGTH];
bool dataValid = false;

/** LOCAL FUNCTION DECLARATIONS ***********************************************/

/** INTERFACE FUNCTION DEFINITIONS ********************************************/

/** LOCAL FUNCTION DEFINITIONS ************************************************/

void receiveEncoderData(int socketfd, uint8_t clientID)
{
    size_t length;

    length = read_from_socket(socketfd, (uint8_t *)&receiveBuffer, sizeof(receiveBuffer));

    if (length != 0)
    {
        receiveBuffer[length] = '\0';
        dataValid = parseEncoderData(receiveBuffer, length);
        if (dataValid)
        {
            parseData((char *)&receiveBuffer, clientID);
        }
    }
    else
    {
        printf("Empty buffer!\n");
    }
    bzero(receiveBuffer, length);

#ifdef DEBUG
    for (int i = 0; i < NUM_OF_ENCODERS; i++)
        printf("%.6f ", Encoder.encoderAngleSet0[i]);
    for (int i = 0; i < NUM_OF_ENCODERS; i++)
        printf("%.6f ", Encoder.encoderAngleSet1[i]);
    printf("\n");
#endif
}

bool parseEncoderData(uint8_t const *buff, size_t len)
{
    static unsigned char ndx = 0;
    static bool recvInProgress = false;
    int ll = len;
    bool flag1 = false;
    bool flag2 = false;
    char startMarker = '<';
    char endMarker = '>';
    char rc;

    while (len--)
    {
        rc = buff[ndx];
        if (ndx == 0 && rc == startMarker)
        {
            recvInProgress = true;
            flag1 = true;
        }

        if (recvInProgress == true)
        {
            if (rc != endMarker)
            {
                receiveBuffer[ndx] = rc;
                ndx++;
                if (ndx >= ll)
                    ndx = ll - 1;
            }
            else
            {
                receiveBuffer[ndx] = '\0';
                receiveBuffer[0] = ' ';
                recvInProgress = false;
                ndx = 0;
                flag2 = true;
            }
        }
    }

    if (flag1 & flag2)
        return true;
    else
        return false;
}

double parseData(char *data, uint8_t clientID)
{
    char *token;
    char *rest = data;
    int ai = 0;
    int si = 0;

    while ((token = strtok_r(rest, " ", &rest)))
    {
        switch (clientID)
        {
        case CLIENT_0:
            if ((ai + si) % 2 == 0)
                Encoder.encoderAngleSet0[ai++] = strtod(token, NULL);
            else
                Encoder.encoderSpeedSet0[si++] = strtod(token, NULL);
            break;
        case CLIENT_1:
            if ((ai + si) % 2 == 0)
                Encoder.encoderAngleSet1[ai++] = strtod(token, NULL);
            else
                Encoder.encoderSpeedSet1[si++] = strtod(token, NULL);
            break;
        case CLIENT_2:
            if ((ai + si) % 2 == 0)
                Encoder.encoderAngleSet2[ai++] = strtod(token, NULL);
            else
                Encoder.encoderSpeedSet2[si++] = strtod(token, NULL);
            break;
        case CLIENT_3:
            if ((ai + si) % 2 == 0)
                Encoder.encoderAngleSet3[ai++] = strtod(token, NULL);
            else
                Encoder.encoderSpeedSet3[si++] = strtod(token, NULL);
            break;
        default:
            break;
        }
    }
}
