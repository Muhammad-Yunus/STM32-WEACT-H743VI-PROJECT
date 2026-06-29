/**
 ******************************************************************************
 * @file    w25qxx_qspi_disk.h
 * @brief   W25Qxx QuadSPI Block Device Driver
 *
 *          This layer converts W25Qxx byte-addressed flash into
 *          a 512-byte sector block device suitable for:
 *
 *              - USB Mass Storage Class (MSC)
 *              - FatFs
 *              - LittleFS
 *
 ******************************************************************************
 */

#ifndef __W25QXX_QSPI_DISK_H
#define __W25QXX_QSPI_DISK_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/

#include "main.h"
#include "w25qxx_qspi.h"

#include <stdint.h>

/* Exported Constants --------------------------------------------------------*/

/* USB MSC standard logical sector size */

#define W25QXX_QSPI_DISK_SECTOR_SIZE          512U

/* W25Q erase block size */

#define W25QXX_QSPI_DISK_ERASE_SIZE           4096U

/* Number of logical sectors inside one erase block */

#define W25QXX_QSPI_DISK_SECTORS_PER_ERASE    \
    (W25QXX_QSPI_DISK_ERASE_SIZE / W25QXX_QSPI_DISK_SECTOR_SIZE)

/* Total flash capacity (W25Q64 = 8MB) */

#define W25QXX_QSPI_DISK_FLASH_SIZE           (8U * 1024U * 1024U)

/* Exported Types ------------------------------------------------------------*/

typedef enum
{
    W25QXX_QSPI_DISK_OK = 0,

    W25QXX_QSPI_DISK_ERROR,

    W25QXX_QSPI_DISK_INVALID_PARAMETER

} W25QXX_QSPI_DiskStatus;

/* Exported Functions --------------------------------------------------------*/

/**
 * @brief Initialize Disk Layer
 */
W25QXX_QSPI_DiskStatus W25QXX_QSPI_Disk_Init(void);

/**
 * @brief Read one or more 512-byte sectors
 *
 * @param sector
 *      Logical sector number
 *
 * @param buffer
 *      Destination buffer
 *
 * @param count
 *      Number of sectors
 */
W25QXX_QSPI_DiskStatus W25QXX_QSPI_Disk_ReadSectors(
        uint32_t sector,
        uint8_t *buffer,
        uint32_t count);

/**
 * @brief Write one or more 512-byte sectors
 *
 * This function performs:
 *
 * Read 4KB
 * Modify requested 512-byte sector(s)
 * Erase 4KB
 * Write back 4KB
 *
 */
W25QXX_QSPI_DiskStatus W25QXX_QSPI_Disk_WriteSectors(
        uint32_t sector,
        const uint8_t *buffer,
        uint32_t count);

/**
 * @brief Erase one logical erase block (4KB)
 *
 * @param erase_block
 *      Erase block number
 */
W25QXX_QSPI_DiskStatus W25QXX_QSPI_Disk_EraseBlock(
        uint32_t erase_block);

/**
 * @brief Get total logical sector count
 */
uint32_t W25QXX_QSPI_Disk_GetSectorCount(void);

/**
 * @brief Get logical sector size
 */
uint32_t W25QXX_QSPI_Disk_GetSectorSize(void);

/**
 * @brief Get erase block size
 */
uint32_t W25QXX_QSPI_Disk_GetEraseSize(void);

/**
 * @brief Convert Logical Sector -> Flash Address
 */
uint32_t W25QXX_QSPI_Disk_SectorToAddress(
        uint32_t sector);

/**
 * @brief Convert Flash Address -> Logical Sector
 */
uint32_t W25QXX_QSPI_Disk_AddressToSector(
        uint32_t address);

/**
 * @brief Get total flash capacity
 */
uint32_t W25QXX_QSPI_Disk_GetCapacity(void);

#ifdef __cplusplus
}
#endif

#endif