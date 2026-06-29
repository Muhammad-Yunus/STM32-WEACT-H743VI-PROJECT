/**
 ******************************************************************************
 * @file    w25qxx_qspi_disk.c
 * @brief   W25Qxx QuadSPI Block Device Layer
 ******************************************************************************
 */

#include "w25qxx_qspi_disk.h"

#include <string.h>

/* ============================================================================
 * Private Variables
 * ==========================================================================*/

/* Scratch buffer for Read-Modify-Write */
static uint8_t gEraseBuffer[W25QXX_QSPI_DISK_ERASE_SIZE];

/* ============================================================================
 * Private Helper Functions
 * ==========================================================================*/

/**
 * @brief Validate sector range
 */
static W25QXX_QSPI_DiskStatus Disk_CheckRange(uint32_t sector,
                                              uint32_t count)
{
    uint32_t total;
    total = W25QXX_QSPI_Disk_GetSectorCount();

    if(count == 0)
    {
        return W25QXX_QSPI_DISK_INVALID_PARAMETER;
    }

    if(sector >= total)
    {
        return W25QXX_QSPI_DISK_INVALID_PARAMETER;
    }

    if((sector + count) > total)
    {
        return W25QXX_QSPI_DISK_INVALID_PARAMETER;
    }

    return W25QXX_QSPI_DISK_OK;
}

/**
 * @brief Convert W25Q driver status to Disk status
 */
static W25QXX_QSPI_DiskStatus Disk_Status(uint8_t status)
{
    switch(status)
    {
        case w25qxx_OK:
            return W25QXX_QSPI_DISK_OK;

        default:
            return W25QXX_QSPI_DISK_ERROR;
    }
}

/* ============================================================================
 * Public Functions
 * ==========================================================================*/

/**
 * @brief Initialize Disk Layer
 */
W25QXX_QSPI_DiskStatus W25QXX_QSPI_Disk_Init(void)
{
    w25qxx_Init();
    (void)w25qxx_GetID();
    (void)w25qxx_EnterQPI();
    return W25QXX_QSPI_DISK_OK;
}

/**
 * @brief Return total flash capacity in bytes
 */
uint32_t W25QXX_QSPI_Disk_GetCapacity(void)
{
    return W25QXX_QSPI_DISK_FLASH_SIZE;
}

/**
 * @brief Return logical sector count
 */
uint32_t W25QXX_QSPI_Disk_GetSectorCount(void)
{
    return W25QXX_QSPI_DISK_FLASH_SIZE /
           W25QXX_QSPI_DISK_SECTOR_SIZE;
}

/**
 * @brief Return logical sector size
 */
uint32_t W25QXX_QSPI_Disk_GetSectorSize(void)
{
    return W25QXX_QSPI_DISK_SECTOR_SIZE;
}

/**
 * @brief Return erase block size
 */
uint32_t W25QXX_QSPI_Disk_GetEraseSize(void)
{
    return W25QXX_QSPI_DISK_ERASE_SIZE;
}

/**
 * @brief Convert Logical Sector -> Flash Address
 */
uint32_t W25QXX_QSPI_Disk_SectorToAddress(uint32_t sector)
{
    return sector * W25QXX_QSPI_DISK_SECTOR_SIZE;
}

/**
 * @brief Convert Flash Address -> Logical Sector
 */
uint32_t W25QXX_QSPI_Disk_AddressToSector(uint32_t address)
{
    return address / W25QXX_QSPI_DISK_SECTOR_SIZE;
}

/**
 * @brief Erase one erase block (4KB)
 */
W25QXX_QSPI_DiskStatus W25QXX_QSPI_Disk_EraseBlock(uint32_t erase_block)
{
    uint32_t address;
    address = erase_block * W25QXX_QSPI_DISK_ERASE_SIZE;

    if(address >= W25QXX_QSPI_DISK_FLASH_SIZE)
    {
        return W25QXX_QSPI_DISK_INVALID_PARAMETER;
    }

    return Disk_Status(W25qxx_EraseSector(address));
}

/* ============================================================================
 * Private Read-Modify-Write Helper
 * ==========================================================================*/

/**
 * @brief
 * Read one 4KB erase block into RAM, modify requested bytes,
 * erase flash block, then write entire 4KB back.
 */
static W25QXX_QSPI_DiskStatus ReadModifyWrite(uint32_t flash_address,
                                              const uint8_t *src,
                                              uint32_t length)
{
    uint32_t erase_address;
    uint32_t offset;
    uint8_t status;

    erase_address = flash_address &
                    ~(W25QXX_QSPI_DISK_ERASE_SIZE - 1);

    offset = flash_address - erase_address;

    if((offset + length) > W25QXX_QSPI_DISK_ERASE_SIZE)
    {
        return W25QXX_QSPI_DISK_INVALID_PARAMETER;
    }

    /* Read existing erase block */
    status = W25qxx_Read(gEraseBuffer,
                         erase_address,
                         W25QXX_QSPI_DISK_ERASE_SIZE);

    if(status != w25qxx_OK)
    {
        return Disk_Status(status);
    }

    /* Skip write if data is already identical */
    if(memcmp(&gEraseBuffer[offset],
              src,
              length) == 0)
    {
        return W25QXX_QSPI_DISK_OK;
    }

    /* Modify requested bytes */
    memcpy(&gEraseBuffer[offset],
           src,
           length);

    /* Erase flash block */
    status = W25qxx_EraseSector(erase_address);
    if(status != w25qxx_OK)
    {
        return Disk_Status(status);
    }

    /* Write complete erase block back */
    W25qxx_WriteNoCheck(gEraseBuffer,
                        erase_address,
                        W25QXX_QSPI_DISK_ERASE_SIZE);

    return W25QXX_QSPI_DISK_OK;
}

/* ============================================================================
 * Read Sector(s)
 * ==========================================================================*/

/**
 * @brief Read one or more logical sectors
 */
W25QXX_QSPI_DiskStatus W25QXX_QSPI_Disk_ReadSectors(uint32_t sector,
                                                     uint8_t *buffer,
                                                     uint32_t count)
{
    uint32_t flash_address;
    uint32_t total_bytes;
    uint8_t status;

    if(buffer == NULL)
    {
        return W25QXX_QSPI_DISK_INVALID_PARAMETER;
    }

    if(Disk_CheckRange(sector, count) != W25QXX_QSPI_DISK_OK)
    {
        return W25QXX_QSPI_DISK_INVALID_PARAMETER;
    }

    flash_address = W25QXX_QSPI_Disk_SectorToAddress(sector);
    total_bytes = count * W25QXX_QSPI_DISK_SECTOR_SIZE;

    status = W25qxx_Read(buffer, flash_address, total_bytes);

    return Disk_Status(status);
}

/* ============================================================================
 * Write Sector(s)
 * ==========================================================================*/

/**
 * @brief Write one or more logical sectors
 */
W25QXX_QSPI_DiskStatus W25QXX_QSPI_Disk_WriteSectors(uint32_t sector,
                                                      const uint8_t *buffer,
                                                      uint32_t count)
{
    uint32_t current_sector;
    uint32_t sectors_left;

    if(buffer == NULL)
    {
        return W25QXX_QSPI_DISK_INVALID_PARAMETER;
    }

    if(Disk_CheckRange(sector, count) != W25QXX_QSPI_DISK_OK)
    {
        return W25QXX_QSPI_DISK_INVALID_PARAMETER;
    }

    current_sector = sector;
    sectors_left   = count;

    while(sectors_left > 0)
    {
        uint32_t flash_address;
        uint32_t sector_offset;
        uint32_t sectors_this_block;
        uint32_t bytes_this_block;
        W25QXX_QSPI_DiskStatus result;

        sector_offset = current_sector % W25QXX_QSPI_DISK_SECTORS_PER_ERASE;
        sectors_this_block = W25QXX_QSPI_DISK_SECTORS_PER_ERASE - sector_offset;

        if(sectors_this_block > sectors_left)
        {
            sectors_this_block = sectors_left;
        }

        flash_address = W25QXX_QSPI_Disk_SectorToAddress(current_sector);
        bytes_this_block = sectors_this_block * W25QXX_QSPI_DISK_SECTOR_SIZE;

        result = ReadModifyWrite(flash_address, buffer, bytes_this_block);

        if(result != W25QXX_QSPI_DISK_OK)
        {
            return result;
        }

        current_sector += sectors_this_block;
        sectors_left -= sectors_this_block;
        buffer += bytes_this_block;
    }

    return W25QXX_QSPI_DISK_OK;
}
