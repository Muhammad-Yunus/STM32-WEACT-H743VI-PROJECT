/**
 ******************************************************************************
 * @file    w25qxx_demo.c
 * @brief   W25Qxx Driver Demo
 ******************************************************************************
 */

#include "main.h"
#include "w25qxx.h"

#include <stdio.h>
#include <string.h>

void W25Qxx_Demo(void)
{
    uint8_t status;
    uint16_t flash_id;

    uint8_t tx_buffer[256];
    uint8_t rx_buffer[256];

    printf("\r\n");
    printf("=====================================\r\n");
    printf("      W25Qxx Driver Demonstration\r\n");
    printf("=====================================\r\n");

    /*----------------------------------------------------------*/
    /* Initialize Driver                                         */
    /*----------------------------------------------------------*/

    status = W25Qx_Init();

    if(status != W25Qx_OK)
    {
        printf("Init FAILED\r\n");
        return;
    }

    printf("Init OK\r\n");

    /*----------------------------------------------------------*/
    /* Read Flash ID                                             */
    /*----------------------------------------------------------*/

    W25Qx_Read_ID(&flash_id);

    printf("Flash ID          : 0x%04X\r\n", flash_id);

    /*----------------------------------------------------------*/
    /* Print Flash Parameters                                    */
    /*----------------------------------------------------------*/

    printf("\r\nFlash Parameters\r\n");

    printf("-----------------------------\r\n");

    printf("Size             : %lu Bytes\r\n", W25Qx_Para.FLASH_Size);
    printf("Sector Count     : %lu\r\n", W25Qx_Para.SECTOR_COUNT);
    printf("Sector Size      : %lu\r\n", W25Qx_Para.SECTOR_SIZE);
    printf("SubSector Count  : %lu\r\n", W25Qx_Para.SUBSECTOR_COUNT);
    printf("SubSector Size   : %lu\r\n", W25Qx_Para.SUBSECTOR_SIZE);
    printf("Page Size        : %lu\r\n", W25Qx_Para.PAGE_SIZE);

    /*----------------------------------------------------------*/
    /* Prepare Test Pattern                                      */
    /*----------------------------------------------------------*/

    memset(tx_buffer, 0, sizeof(tx_buffer));
    memset(rx_buffer, 0, sizeof(rx_buffer));

    strcpy((char *)tx_buffer,
           "Hello SPI Flash from STM32H743!");

    printf("\r\n");
    printf("Write Buffer : %s\r\n", tx_buffer);

    /*----------------------------------------------------------*/
    /* Erase Sector                                               */
    /*----------------------------------------------------------*/

    printf("\r\nErasing Sector 0 ... ");

    status = W25Qx_Erase_Block(0);

    if(status == W25Qx_OK)
        printf("OK\r\n");
    else
        printf("FAILED\r\n");

    /*----------------------------------------------------------*/
    /* Write Data                                                 */
    /*----------------------------------------------------------*/

    printf("Writing Data ..... ");

    status = W25Qx_Write(tx_buffer,
                         0,
                         strlen((char *)tx_buffer)+1);

    if(status == W25Qx_OK)
        printf("OK\r\n");
    else
        printf("FAILED\r\n");

    /*----------------------------------------------------------*/
    /* Read Back                                                  */
    /*----------------------------------------------------------*/

    printf("Reading Data ..... ");

    status = W25Qx_Read(rx_buffer,
                        0,
                        strlen((char *)tx_buffer)+1);

    if(status == W25Qx_OK)
        printf("OK\r\n");
    else
        printf("FAILED\r\n");

    printf("\r\n");

    printf("Read Buffer : %s\r\n", rx_buffer);

    /*----------------------------------------------------------*/
    /* Compare                                                    */
    /*----------------------------------------------------------*/

    printf("\r\nVerification : ");

    if(strcmp((char *)tx_buffer,
              (char *)rx_buffer)==0)
    {
        printf("PASS\r\n");
    }
    else
    {
        printf("FAILED\r\n");
    }

    /*----------------------------------------------------------*/
    /* Page Write Demo                                            */
    /*----------------------------------------------------------*/

    printf("\r\nPage Write Demo\r\n");

    for(uint16_t i=0;i<256;i++)
    {
        tx_buffer[i]=i;
    }

    W25Qx_Erase_Block(4096);

    W25Qx_Write(tx_buffer,
                4096,
                sizeof(tx_buffer));

    memset(rx_buffer,0,sizeof(rx_buffer));

    W25Qx_Read(rx_buffer,
               4096,
               sizeof(rx_buffer));

    uint8_t error=0;

    for(uint16_t i=0;i<256;i++)
    {
        if(tx_buffer[i]!=rx_buffer[i])
        {
            error=1;
            break;
        }
    }

    if(error)
        printf("256-byte Page Test : FAILED\r\n");
    else
        printf("256-byte Page Test : PASS\r\n");

    /*----------------------------------------------------------*/
    /* Write Enable Demo                                          */
    /*----------------------------------------------------------*/

    status=W25Qx_WriteEnable();

    printf("\r\nWrite Enable : ");

    if(status==W25Qx_OK)
        printf("OK\r\n");
    else
        printf("FAILED\r\n");

    printf("\r\nDemo Finished\r\n");
    printf("=====================================\r\n");
}
