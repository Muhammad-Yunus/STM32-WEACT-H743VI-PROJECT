/**
 ******************************************************************************
 * @file    w25qxx_disk.c
 * @author  ChatGPT
 * @brief   W25Qxx Block Device Layer
 ******************************************************************************
 */

#include "w25qxx_disk.h"

#include <string.h>

/* ============================================================================
 * Debug Options
 * ==========================================================================*/

/*
 * Enable write verification.
 *
 * 0 = Disable (faster)
 * 1 = Read back after every write and compare
 */
#define W25QXX_DISK_VERIFY_WRITE    0

/* ============================================================================
 * Private Macros
 * ==========================================================================*/



/* ============================================================================
 * Private Variables
 * ==========================================================================*/

/*
 * Scratch buffer used for Read-Modify-Write.
 *
 * One erase block = 4096 Bytes
 */

static uint8_t gEraseBuffer[W25QXX_DISK_ERASE_SIZE];

#if W25QXX_DISK_VERIFY_WRITE
static uint8_t gVerifyBuffer[W25QXX_DISK_ERASE_SIZE];
#endif

/* ============================================================================
 * Private Helper Functions
 * ==========================================================================*/

/**
 * @brief Validate sector range
 */
static W25QXX_DiskStatus Disk_CheckRange(uint32_t sector,
                                         uint32_t count)
{
    uint32_t total;

    total = W25QXX_Disk_GetSectorCount();

    if(count == 0)
    {
        return W25QXX_DISK_INVALID_PARAMETER;
    }

    if(sector >= total)
    {
        return W25QXX_DISK_INVALID_PARAMETER;
    }

    if((sector + count) > total)
    {
        return W25QXX_DISK_INVALID_PARAMETER;
    }

    return W25QXX_DISK_OK;
}

/**
 * @brief Convert W25Q driver status to Disk status
 */
static W25QXX_DiskStatus Disk_Status(uint8_t status)
{
    switch(status)
    {
        case W25Qx_OK:
            return W25QXX_DISK_OK;

        case W25Qx_TIMEOUT:
            return W25QXX_DISK_TIMEOUT;

        default:
            return W25QXX_DISK_ERROR;
    }
}

/* ============================================================================
 * Public Functions
 * ==========================================================================*/

/**
 * @brief Initialize Disk Layer
 */
W25QXX_DiskStatus W25QXX_Disk_Init(void)
{
    uint8_t status;

    status = W25Qx_Init();

    return Disk_Status(status);
}

/**
 * @brief Return total flash capacity in bytes
 */
uint32_t W25QXX_Disk_GetCapacity(void)
{
    return W25Qx_Para.FLASH_Size;
}

/**
 * @brief Return logical sector count
 */
uint32_t W25QXX_Disk_GetSectorCount(void)
{
    return W25Qx_Para.FLASH_Size /
           W25QXX_DISK_SECTOR_SIZE;
}

/**
 * @brief Return logical sector size
 */
uint32_t W25QXX_Disk_GetSectorSize(void)
{
    return W25QXX_DISK_SECTOR_SIZE;
}

/**
 * @brief Return erase block size
 */
uint32_t W25QXX_Disk_GetEraseSize(void)
{
    return W25QXX_DISK_ERASE_SIZE;
}

/**
 * @brief Convert Logical Sector -> Flash Address
 */
uint32_t W25QXX_Disk_SectorToAddress(uint32_t sector)
{
    return sector * W25QXX_DISK_SECTOR_SIZE;
}

/**
 * @brief Convert Flash Address -> Logical Sector
 */
uint32_t W25QXX_Disk_AddressToSector(uint32_t address)
{
    return address / W25QXX_DISK_SECTOR_SIZE;
}

/**
 * @brief Erase one erase block (4KB)
 *
 * erase_block = 0
 * erase_block = 1
 * erase_block = 2
 *
 */
W25QXX_DiskStatus W25QXX_Disk_EraseBlock(uint32_t erase_block)
{
    uint32_t address;

    address = erase_block * W25QXX_DISK_ERASE_SIZE;

    if(address >= W25Qx_Para.FLASH_Size)
    {
        return W25QXX_DISK_INVALID_PARAMETER;
    }

    return Disk_Status(
            W25Qx_Erase_Block(address));
}

/* ============================================================================
 * Private Read-Modify-Write Helper
 * ==========================================================================*/

/**
 * @brief
 * Read one 4KB erase block into RAM, modify requested bytes,
 * erase flash block, then write entire 4KB back.
 *
 * @param flash_address
 *      Absolute flash address
 *
 * @param src
 *      Source data
 *
 * @param length
 *      Number of bytes to update
 *
 * @retval Disk Status
 */
static W25QXX_DiskStatus ReadModifyWrite(uint32_t flash_address,
                                         const uint8_t *src,
                                         uint32_t length)
{
    uint32_t erase_address;
    uint32_t offset;
    uint8_t status;

    /* Align address to 4KB erase boundary */

    erase_address = flash_address &
                    ~(W25QXX_DISK_ERASE_SIZE - 1);

    offset = flash_address - erase_address;

    /* Check overflow */

    if((offset + length) > W25QXX_DISK_ERASE_SIZE)
    {
        return W25QXX_DISK_INVALID_PARAMETER;
    }

    /*----------------------------------------------------------
     * Read existing erase block
     *---------------------------------------------------------*/

    status = W25Qx_Read(gEraseBuffer,
                        erase_address,
                        W25QXX_DISK_ERASE_SIZE);

    if(status != W25Qx_OK)
    {
        return Disk_Status(status);
    }

    /*----------------------------------------------------------
     * Skip write if data is already identical
     *---------------------------------------------------------*/

    if(memcmp(&gEraseBuffer[offset],
              src,
              length) == 0)
    {
        return W25QXX_DISK_OK;
    }

    /*----------------------------------------------------------
     * Modify requested bytes
     *---------------------------------------------------------*/

    memcpy(&gEraseBuffer[offset],
           src,
           length);

    /*----------------------------------------------------------
     * Erase flash block
     *---------------------------------------------------------*/

    status = W25Qx_Erase_Block(erase_address);

    if(status != W25Qx_OK)
    {
        return Disk_Status(status);
    }

    /*----------------------------------------------------------
     * Write complete erase block back
     *---------------------------------------------------------*/

    status = W25Qx_Write(gEraseBuffer,
                         erase_address,
                         W25QXX_DISK_ERASE_SIZE);

    if(status != W25Qx_OK)
    {
        return Disk_Status(status);
    }

    #if W25QXX_DISK_VERIFY_WRITE

    /*----------------------------------------------------------
     * Verify by reading back entire erase block
     *---------------------------------------------------------*/

    status = W25Qx_Read(gVerifyBuffer,
                        erase_address,
                        W25QXX_DISK_ERASE_SIZE);

    if(status != W25Qx_OK)
    {
        return Disk_Status(status);
    }

    if(memcmp(gEraseBuffer,
              gVerifyBuffer,
              W25QXX_DISK_ERASE_SIZE) != 0)
    {
        return W25QXX_DISK_ERROR;
    }

    #endif

    return W25QXX_DISK_OK;
}

/* ============================================================================
 * Read Sector(s)
 * ==========================================================================*/

/**
 * @brief Read one or more logical sectors
 *
 * sector = LBA
 *
 * count = number of sectors
 *
 * Example:
 *
 * sector = 10
 * count  = 4
 *
 * Reads:
 *
 * 10
 * 11
 * 12
 * 13
 *
 */
W25QXX_DiskStatus W25QXX_Disk_ReadSectors(uint32_t sector,
                                          uint8_t *buffer,
                                          uint32_t count)
{
    uint32_t flash_address;
    uint32_t total_bytes;
    uint8_t status;

    if(buffer == NULL)
    {
        return W25QXX_DISK_INVALID_PARAMETER;
    }

    if(Disk_CheckRange(sector, count) != W25QXX_DISK_OK)
    {
        return W25QXX_DISK_INVALID_PARAMETER;
    }

    flash_address = W25QXX_Disk_SectorToAddress(sector);

    total_bytes = count *
                  W25QXX_DISK_SECTOR_SIZE;

    status = W25Qx_Read(buffer,
                        flash_address,
                        total_bytes);

    return Disk_Status(status);
}

/* ============================================================================
 * Write Sector(s)
 * ==========================================================================*/

/**
 * @brief Write one or more logical sectors
 *
 * This routine automatically performs Read-Modify-Write (RMW)
 * for every affected 4KB erase block.
 *
 * Flash Layout
 *
 *  +----------------------------------------------------------+
 *  |                  4KB Erase Block                         |
 *  +----------------------------------------------------------+
 *  |512|512|512|512|512|512|512|512|
 *   S0 S1 S2 S3 S4 S5 S6 S7
 *
 * USB MSC writes 512-byte logical sectors.
 * NOR Flash erases 4096-byte erase blocks.
 *
 * Therefore every write becomes:
 *
 *      Read 4KB
 *          ↓
 *      Modify requested sector(s)
 *          ↓
 *      Erase 4KB
 *          ↓
 *      Write back 4KB
 *
 */
W25QXX_DiskStatus W25QXX_Disk_WriteSectors(uint32_t sector,
                                           const uint8_t *buffer,
                                           uint32_t count)
{
    uint32_t current_sector;
    uint32_t sectors_left;

    if(buffer == NULL)
    {
        return W25QXX_DISK_INVALID_PARAMETER;
    }

    if(Disk_CheckRange(sector, count) != W25QXX_DISK_OK)
    {
        return W25QXX_DISK_INVALID_PARAMETER;
    }

    current_sector = sector;
    sectors_left   = count;

    while(sectors_left > 0)
    {
        uint32_t flash_address;
        uint32_t sector_offset;
        uint32_t sectors_this_block;
        uint32_t bytes_this_block;
        W25QXX_DiskStatus result;

        /*------------------------------------------------------
         * Current flash address
         *-----------------------------------------------------*/

        flash_address = W25QXX_Disk_SectorToAddress(current_sector);

        /*------------------------------------------------------
         * Sector offset inside erase block
         *-----------------------------------------------------*/

        sector_offset =
            current_sector %
            W25QXX_DISK_SECTORS_PER_ERASE;

        /*------------------------------------------------------
         * Remaining sectors inside this erase block
         *-----------------------------------------------------*/

        sectors_this_block =
            W25QXX_DISK_SECTORS_PER_ERASE -
            sector_offset;

        if(sectors_this_block > sectors_left)
        {
            sectors_this_block = sectors_left;
        }

        bytes_this_block =
            sectors_this_block *
            W25QXX_DISK_SECTOR_SIZE;

        /*------------------------------------------------------
         * Read-Modify-Write
         *-----------------------------------------------------*/

        result = ReadModifyWrite(
                    flash_address,
                    buffer,
                    bytes_this_block);

        if(result != W25QXX_DISK_OK)
        {
            return result;
        }

        /*------------------------------------------------------
         * Next
         *-----------------------------------------------------*/

        current_sector += sectors_this_block;
        sectors_left -= sectors_this_block;
        buffer += bytes_this_block;
    }

    return W25QXX_DISK_OK;
}
