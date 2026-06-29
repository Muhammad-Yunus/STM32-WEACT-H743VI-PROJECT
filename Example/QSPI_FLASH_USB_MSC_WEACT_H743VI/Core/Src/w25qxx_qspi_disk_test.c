#include "main.h"
#include "w25qxx_qspi.h"
#include "w25qxx_qspi_disk.h"
#include <stdio.h>
#include <string.h>

#define DISK_TEST_BASE_SECTOR    0       /* sector 0 = flash address 0 */
#define DISK_TEST_SECTOR_COUNT   4       /* test 4 sectors = 2KB */
#define DISK_TEST_BUF_SIZE       (DISK_TEST_SECTOR_COUNT * 512U)

#define DISK_TEST_AUX_SECTORS    16      /* auxiliary sectors for cross-erase-boundary test */
#define DISK_TEST_AUX_BYTES      (DISK_TEST_AUX_SECTORS * 512U)

static uint8_t txBuf[DISK_TEST_BUF_SIZE];
static uint8_t rxBuf[DISK_TEST_BUF_SIZE];
static uint8_t auxBuf[DISK_TEST_AUX_BYTES];
static uint8_t verifyBuf[DISK_TEST_AUX_BYTES];

static int32_t total_checks = 0;
static int32_t passed_checks = 0;

static uint32_t DumpHex(const uint8_t *buf, uint32_t len)
{
    uint32_t printed = 0;
    for(uint32_t i = 0; i < len; i++)
    {
        printf("%02X ", buf[i]);
        printed += 3;
        if((i + 1) % 16 == 0)
        {
            printf("\r\n");
            printed += 2;
        }
    }
    printf("\r\n");
    printed += 2;
    return printed;
}

static void PrintResult(const char *label, int32_t ok)
{
    total_checks++;
    if(ok) passed_checks++;
    printf("  [%s] %s\r\n", ok ? "PASS" : "FAIL", label);
}

static int32_t MemEq(const uint8_t *a, const uint8_t *b, uint32_t n)
{
    return (memcmp(a, b, n) == 0);
}

/* ============================================================================
 * Test Cases
 * ==========================================================================*/

static void Test_DiskInit(void)
{
    printf("\r\n--- Test 1: Disk Init & Capacity ---\r\n");

    W25QXX_QSPI_DiskStatus s = W25QXX_QSPI_Disk_Init();
    PrintResult("W25QXX_QSPI_Disk_Init returns OK", s == W25QXX_QSPI_DISK_OK);

    uint32_t cap  = W25QXX_QSPI_Disk_GetCapacity();
    uint32_t cnt  = W25QXX_QSPI_Disk_GetSectorCount();
    uint32_t ssz  = W25QXX_QSPI_Disk_GetSectorSize();
    uint32_t esz  = W25QXX_QSPI_Disk_GetEraseSize();

    printf("  Capacity     : %lu bytes (%lu KB)\r\n", cap, cap / 1024U);
    printf("  Sector Count : %lu\r\n", cnt);
    printf("  Sector Size  : %lu\r\n", ssz);
    printf("  Erase Size   : %lu\r\n", esz);

    PrintResult("Sector size == 512",  ssz == 512U);
    PrintResult("Erase size == 4096",  esz == 4096U);
    PrintResult("Capacity == 16MB",    cap == (16UL * 1024UL * 1024UL));
    PrintResult("Sector count == 32768", cnt == 32768U);
    PrintResult("8 sectors per erase", (esz / ssz) == 8U);
}

static void Test_SectorAddressConversion(void)
{
    printf("\r\n--- Test 2: Sector / Address Conversion ---\r\n");

    uint32_t a = W25QXX_QSPI_Disk_SectorToAddress(0);
    uint32_t b = W25QXX_QSPI_Disk_SectorToAddress(1);
    uint32_t c = W25QXX_QSPI_Disk_SectorToAddress(7);
    uint32_t d = W25QXX_QSPI_Disk_SectorToAddress(8);

    PrintResult("Sector 0 -> 0x00000000", a == 0x00000000U);
    PrintResult("Sector 1 -> 0x00000200", b == 0x00000200U);
    PrintResult("Sector 7 -> 0x00000E00", c == 0x00000E00U);
    PrintResult("Sector 8 -> 0x00001000 (erase boundary)", d == 0x00001000U);

    PrintResult("AddressToSector(0x200) == 1",   W25QXX_QSPI_Disk_AddressToSector(0x200) == 1U);
    PrintResult("AddressToSector(0x1000) == 8",  W25QXX_QSPI_Disk_AddressToSector(0x1000) == 8U);
}

static void Test_RangeValidation(void)
{
    uint32_t total = W25QXX_QSPI_Disk_GetSectorCount();
    printf("\r\n--- Test 3: Range Validation ---\r\n");

    W25QXX_QSPI_DiskStatus s1 = W25QXX_QSPI_Disk_ReadSectors(total, rxBuf, 1);
    PrintResult("Reject sector == total (read)",  s1 == W25QXX_QSPI_DISK_INVALID_PARAMETER);

    W25QXX_QSPI_DiskStatus s2 = W25QXX_QSPI_Disk_ReadSectors(total - 1, rxBuf, 2);
    PrintResult("Reject overflow (read)",          s2 == W25QXX_QSPI_DISK_INVALID_PARAMETER);

    W25QXX_QSPI_DiskStatus s3 = W25QXX_QSPI_Disk_ReadSectors(0, rxBuf, 0);
    PrintResult("Reject count == 0 (read)",       s3 == W25QXX_QSPI_DISK_INVALID_PARAMETER);

    W25QXX_QSPI_DiskStatus s4 = W25QXX_QSPI_Disk_ReadSectors(0, NULL, 1);
    PrintResult("Reject NULL buffer (read)",      s4 == W25QXX_QSPI_DISK_INVALID_PARAMETER);
}

static void Test_EraseBlock(void)
{
    printf("\r\n--- Test 4: Erase One Block (4KB @ sector 0) ---\r\n");

    W25QXX_QSPI_DiskStatus s = W25QXX_QSPI_Disk_EraseBlock(0);
    PrintResult("EraseBlock(0) OK", s == W25QXX_QSPI_DISK_OK);

    memset(rxBuf, 0xAA, sizeof(rxBuf));
    W25QXX_QSPI_Disk_ReadSectors(0, rxBuf, DISK_TEST_SECTOR_COUNT);

    int32_t all_ff = 1;
    for(uint32_t i = 0; i < DISK_TEST_BUF_SIZE; i++)
        if(rxBuf[i] != 0xFF) { all_ff = 0; break; }

    PrintResult("Erased block reads all 0xFF", all_ff);

    uint32_t too_far = W25QXX_QSPI_Disk_GetCapacity() / W25QXX_QSPI_DISK_ERASE_SIZE;
    W25QXX_QSPI_DiskStatus s2 = W25QXX_QSPI_Disk_EraseBlock(too_far);
    PrintResult("Reject EraseBlock beyond flash", s2 == W25QXX_QSPI_DISK_INVALID_PARAMETER);
}

static void Test_WriteReadInSameEraseBlock(void)
{
    printf("\r\n--- Test 5: Write & Read 4 sectors (within one erase block) ---\r\n");

    W25QXX_QSPI_Disk_EraseBlock(0);

    for(uint32_t i = 0; i < DISK_TEST_BUF_SIZE; i++)
        txBuf[i] = (uint8_t)(i & 0xFF);

    W25QXX_QSPI_DiskStatus s = W25QXX_QSPI_Disk_WriteSectors(
            DISK_TEST_BASE_SECTOR, txBuf, DISK_TEST_SECTOR_COUNT);
    PrintResult("WriteSectors(0, 4 sectors) OK", s == W25QXX_QSPI_DISK_OK);

    memset(rxBuf, 0, sizeof(rxBuf));
    W25QXX_QSPI_Disk_ReadSectors(DISK_TEST_BASE_SECTOR, rxBuf, DISK_TEST_SECTOR_COUNT);

    PrintResult("Read back matches write", MemEq(txBuf, rxBuf, DISK_TEST_BUF_SIZE));

    printf("  First 64 bytes after write/read:\r\n");
    DumpHex(rxBuf, 64);
}

static void Test_WriteReadAcrossEraseBoundary(void)
{
    printf("\r\n--- Test 6: Write across erase boundary (RMW) ---\r\n");

    W25QXX_QSPI_Disk_EraseBlock(0);
    W25QXX_QSPI_Disk_EraseBlock(1);

    for(uint32_t i = 0; i < DISK_TEST_AUX_BYTES; i++)
        auxBuf[i] = (uint8_t)((i * 7U + 13U) & 0xFF);

    /* Write 12 sectors starting at sector 2 -> spans erase block 0 (sectors 0-7)
       and erase block 1 (sectors 8-15) */
    uint32_t start_sector = 2;
    uint32_t num_sectors  = 12;

    W25QXX_QSPI_DiskStatus s = W25QXX_QSPI_Disk_WriteSectors(
            start_sector, auxBuf, num_sectors);
    PrintResult("Cross-boundary WriteSectors OK", s == W25QXX_QSPI_DISK_OK);

    memset(verifyBuf, 0, sizeof(verifyBuf));
    W25QXX_QSPI_Disk_ReadSectors(start_sector, verifyBuf, num_sectors);

    PrintResult("Cross-boundary data matches",
                MemEq(auxBuf, verifyBuf, num_sectors * 512U));

    /* Verify untouched sectors within the same erase blocks remain 0xFF
       Sectors 0,1 (in block 0) and sectors 14,15 (in block 1) should be untouched */
    memset(rxBuf, 0, sizeof(rxBuf));
    W25QXX_QSPI_Disk_ReadSectors(0, rxBuf, 2);
    int32_t untouched_zero_one = 1;
    for(uint32_t i = 0; i < 2 * 512U; i++)
        if(rxBuf[i] != 0xFF) { untouched_zero_one = 0; break; }
    PrintResult("Untouched sectors 0-1 remain 0xFF", untouched_zero_one);

    memset(rxBuf, 0, sizeof(rxBuf));
    W25QXX_QSPI_Disk_ReadSectors(14, rxBuf, 2);
    int32_t untouched_fourteen_fifteen = 1;
    for(uint32_t i = 0; i < 2 * 512U; i++)
        if(rxBuf[i] != 0xFF) { untouched_fourteen_fifteen = 0; break; }
    PrintResult("Untouched sectors 14-15 remain 0xFF", untouched_fourteen_fifteen);
}

static void Test_OverwritePreservesOtherSectors(void)
{
    printf("\r\n--- Test 7: Overwrite write preserves other sectors in same block ---\r\n");

    /* Pre-condition: Test 6 left sectors 2..13 filled with auxBuf data. */

    /* Build a small pattern, write only sector 5 (still inside block 0) */
    uint8_t pattern[512];
    for(uint32_t i = 0; i < 512; i++)
        pattern[i] = 0xA5;

    W25QXX_QSPI_DiskStatus s = W25QXX_QSPI_Disk_WriteSectors(5, pattern, 1);
    PrintResult("Single-sector write OK", s == W25QXX_QSPI_DISK_OK);

    /* Sector 5 must match new pattern */
    memset(rxBuf, 0, 512);
    W25QXX_QSPI_Disk_ReadSectors(5, rxBuf, 1);
    PrintResult("Sector 5 matches new pattern", MemEq(rxBuf, pattern, 512U));

    /* Adjacent sectors (still inside block 0) must still match original auxBuf data */
    memset(rxBuf, 0, 512);
    W25QXX_QSPI_Disk_ReadSectors(4, rxBuf, 1);
    PrintResult("Sector 4 preserved",
                MemEq(rxBuf, &auxBuf[(4 - 2) * 512U], 512U));

    memset(rxBuf, 0, 512);
    W25QXX_QSPI_Disk_ReadSectors(6, rxBuf, 1);
    PrintResult("Sector 6 preserved",
                MemEq(rxBuf, &auxBuf[(6 - 2) * 512U], 512U));

    /* And sector 13 (last sector in block 1's written range) still intact */
    memset(rxBuf, 0, 512);
    W25QXX_QSPI_Disk_ReadSectors(13, rxBuf, 1);
    PrintResult("Sector 13 (in block 1) preserved",
                MemEq(rxBuf, &auxBuf[(13 - 2) * 512U], 512U));
}

/* ============================================================================
 * Entry Point
 * ==========================================================================*/

void W25QXX_QSPI_Disk_Test(void)
{
    printf("\r\n");
    printf("===========================================\r\n");
    printf("   W25QXX_QSPI_DISK LAYER UNIT TEST        \r\n");
    printf("===========================================\r\n");

    Test_DiskInit();
    Test_SectorAddressConversion();
    Test_RangeValidation();
    Test_EraseBlock();
    Test_WriteReadInSameEraseBlock();
    Test_WriteReadAcrossEraseBoundary();
    Test_OverwritePreservesOtherSectors();

    printf("\r\n--- Summary ---\r\n");
    printf("  Passed: %ld / %ld\r\n", passed_checks, total_checks);
    if(passed_checks == total_checks)
        printf("  ALL TESTS PASSED\r\n");
    else
        printf("  SOME TESTS FAILED\r\n");

    printf("\r\n=========== Disk Test Finished ===========\r\n");
}