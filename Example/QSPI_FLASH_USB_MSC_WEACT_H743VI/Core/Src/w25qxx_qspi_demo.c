#include "main.h"
#include "w25qxx_qspi.h"
#include <stdio.h>
#include <string.h>

#define TEST_ADDRESS       0x000000
#define TEST_SECTOR        TEST_ADDRESS
#define TEST_BLOCK         TEST_ADDRESS

#define QSPI_BASE_ADDRESS  0x90000000U

#define TEST_SIZE          512
#define MM_TEST_SIZE       64

static uint8_t txBuf[TEST_SIZE];
static uint8_t rxBuf[TEST_SIZE];
static uint8_t mmBuf[MM_TEST_SIZE];

static void DumpHex(const uint8_t *buf, uint32_t len)
{
    for(uint32_t i = 0; i < len; i++)
    {
        printf("%02X ", buf[i]);

        if((i + 1) % 16 == 0)
            printf("\r\n");
    }

    printf("\r\n");
}

static void DumpMemoryMapped(uint32_t address, uint32_t len)
{
    const volatile uint8_t *flash =
        (const volatile uint8_t *)(QSPI_BASE_ADDRESS + address);

    for(uint32_t i = 0; i < len; i++)
    {
        printf("%02X ", flash[i]);

        if((i + 1) % 16 == 0)
            printf("\r\n");
    }

    printf("\r\n");
}

void W25QXX_Demo(void)
{
    printf("\r\n");
    printf("=====================================\r\n");
    printf("      W25QXX QUADSPI DEMO\r\n");
    printf("=====================================\r\n");

    //----------------------------------------------------------
    // Initialize
    //----------------------------------------------------------

    w25qxx_Init();

    printf("Flash ID : 0x%04X\r\n", w25qxx_GetID());

    printf("Status Register 1 : %02X\r\n",
           w25qxx_ReadSR(W25X_ReadStatusReg1));

    printf("Status Register 2 : %02X\r\n",
           w25qxx_ReadSR(W25X_ReadStatusReg2));

    printf("Status Register 3 : %02X\r\n",
           w25qxx_ReadSR(W25X_ReadStatusReg3));

    //----------------------------------------------------------
    // Read
    //----------------------------------------------------------

    printf("\r\nRead first 64 bytes\r\n");

    memset(rxBuf, 0, sizeof(rxBuf));

    W25qxx_Read(rxBuf, TEST_ADDRESS, MM_TEST_SIZE);

    DumpHex(rxBuf, MM_TEST_SIZE);

    //----------------------------------------------------------
    // Erase
    //----------------------------------------------------------

    printf("\r\nErase Sector...\r\n");

    if(W25qxx_EraseSector(TEST_SECTOR) == w25qxx_OK)
        printf("Erase OK\r\n");
    else
        printf("Erase FAIL\r\n");

    memset(rxBuf, 0, sizeof(rxBuf));

    W25qxx_Read(rxBuf, TEST_ADDRESS, MM_TEST_SIZE);

    printf("Verify Erase\r\n");

    DumpHex(rxBuf, MM_TEST_SIZE);

    //----------------------------------------------------------
    // Prepare Pattern
    //----------------------------------------------------------

    for(uint32_t i = 0; i < TEST_SIZE; i++)
        txBuf[i] = (uint8_t)i;

    //----------------------------------------------------------
    // Write
    //----------------------------------------------------------

    printf("\r\nWrite %d bytes...\r\n", TEST_SIZE);

    W25qxx_Write(txBuf, TEST_ADDRESS, TEST_SIZE);

    //----------------------------------------------------------
    // Read Back
    //----------------------------------------------------------

    memset(rxBuf, 0, sizeof(rxBuf));

    W25qxx_Read(rxBuf, TEST_ADDRESS, TEST_SIZE);

    if(memcmp(txBuf, rxBuf, TEST_SIZE) == 0)
        printf("COMPARE PASS\r\n");
    else
        printf("COMPARE FAIL\r\n");

    printf("\r\nFirst 64 bytes after write\r\n");

    DumpHex(rxBuf, MM_TEST_SIZE);

    //----------------------------------------------------------
    // Cross Page Test
    //----------------------------------------------------------

    printf("\r\nPage Boundary Test\r\n");

    for(uint32_t i = 0; i < 300; i++)
        txBuf[i] = (uint8_t)(255 - i);

    W25qxx_EraseSector(TEST_SECTOR);

    W25qxx_Write(txBuf, 200, 300);

    memset(rxBuf, 0, sizeof(rxBuf));

    W25qxx_Read(rxBuf, 200, 300);

    if(memcmp(txBuf, rxBuf, 300) == 0)
        printf("Boundary PASS\r\n");
    else
        printf("Boundary FAIL\r\n");

    //----------------------------------------------------------
    // Block Erase
    //----------------------------------------------------------

    printf("\r\nErase Block (64KB)...\r\n");

    if(W25qxx_EraseBlock(TEST_BLOCK) == w25qxx_OK)
        printf("Block Erase PASS\r\n");
    else
        printf("Block Erase FAIL\r\n");

    //----------------------------------------------------------
    // Restore Pattern
    //----------------------------------------------------------

    for(uint32_t i = 0; i < TEST_SIZE; i++)
        txBuf[i] = (uint8_t)i;

    W25qxx_EraseSector(TEST_SECTOR);
    W25qxx_Write(txBuf, TEST_ADDRESS, TEST_SIZE);

    //----------------------------------------------------------
    // Enter QPI
    //----------------------------------------------------------

    printf("\r\nEnter QPI Mode...\r\n");

    if(w25qxx_EnterQPI() == w25qxx_OK)
        printf("QPI Enabled\r\n");
    else
        printf("QPI Failed\r\n");

    //----------------------------------------------------------
    // MemoryMapped
    //----------------------------------------------------------

    printf("\r\nEnable MemoryMapped Mode...\r\n");

    if(w25qxx_Startup(w25qxx_NormalMode) == w25qxx_OK)
    {
        printf("MemoryMapped Enabled\r\n");

        printf("\r\nMemoryMapped Direct Read\r\n");

        DumpMemoryMapped(TEST_ADDRESS, MM_TEST_SIZE);

        printf("\r\nMemoryMapped memcpy Test\r\n");

        memset(mmBuf, 0, sizeof(mmBuf));

        memcpy(mmBuf,
               (const void *)(QSPI_BASE_ADDRESS + TEST_ADDRESS),
               sizeof(mmBuf));

        DumpHex(mmBuf, sizeof(mmBuf));

        if(memcmp(mmBuf, txBuf, sizeof(mmBuf)) == 0)
            printf("MemoryMapped VERIFY PASS\r\n");
        else
            printf("MemoryMapped VERIFY FAIL\r\n");
    }
    else
    {
        printf("MemoryMapped Failed\r\n");
    }

    printf("\r\n=========== Demo Finished ===========\r\n");
}
