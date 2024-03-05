#ifndef __CVI_FIP_H__
#define __CVI_FIP_H__

#include <unistd.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <inttypes.h>
#include <string.h>

#define SPI_NAND_BASE_DATA_BACKUP_COPY (2)
#define MAX_PAGE_SIZE	(16 * 1024)
#define MAX_SPARE_SIZE	(256)
#define SPI_NAND_FIP_DATA_BLOCK_COUNT (8)
#define SPI_NAND_BASE_RESERVED_ENTRY (4)
#define MAX_BLOCK_CNT			4096
#define NUMBER_PATCH_SET 128
#define HEADER_ADDR 0x10F200000

typedef unsigned char u8;
typedef unsigned int u32;

static inline uint32_t DESC_CONV(char *x)
{
	return ((((((x[0] << 8) | x[1]) << 8) | x[2]) << 8) | x[3]);
}
#define SPI_NAND_VECTOR_SIGNATURE DESC_CONV("SPNV")
#define PRODUCTION_FW_SIGNATURE DESC_CONV("PTFM")

#ifdef NANDBOOT_V2
#define SPI_NAND_VECTOR_VERSION (0x18222001)
#else
#define SPI_NAND_VECTOR_VERSION (0x18352101)
#endif

struct _spi_nand_info {
	uint16_t id;
	uint16_t page_size;

	uint16_t spare_size;
	uint16_t pages_per_block;

	uint16_t block_cnt; // up to 32k block
	uint8_t pages_per_block_shift;
	uint8_t flags;
#ifdef NANDBOOT_V2
	uint8_t ecc_en_feature_offset;
	uint8_t ecc_en_mask;
	uint8_t ecc_status_offset;
	uint8_t ecc_status_mask;
	uint8_t ecc_status_shift;
	uint8_t ecc_status_valid_val;
#endif
};

struct _patch_data_t {
	uint32_t reg;
	uint32_t value;
};

struct _spi_nand_base_vector_t {
	uint32_t signature;
	uint32_t version;

	uint16_t spi_nand_vector_blks[SPI_NAND_BASE_DATA_BACKUP_COPY];

	uint16_t fip_bin_blk_cnt;
	uint16_t fip_bin_blks[SPI_NAND_BASE_DATA_BACKUP_COPY][SPI_NAND_FIP_DATA_BLOCK_COUNT];

	uint16_t erase_count; // erase count for sys base block
	uint16_t rsvd_block_count; // how many blocks reserved for spi_nand_vect and fip.bin
	uint32_t spi_nand_vector_length;  // spi_nand vector struct length, must less than a page

	uint8_t spi_nand_base_block_usage[SPI_NAND_BASE_RESERVED_ENTRY];

	struct _spi_nand_info spi_nand_info;

	uint8_t factory_defect_buf[MAX_BLOCK_CNT / 8]; // factory defect block table, up to 512 bytes

	uint32_t bld_size;
	uintptr_t bld_loading_to_addr;

	uint32_t valid_patch_num;
	struct _patch_data_t patch_data[NUMBER_PATCH_SET];
#ifdef NANDBOOT_V2
	uint16_t crc;
#endif
	uint8_t ecc_en_feature_offset;
	uint8_t ecc_en_mask;
	uint8_t ecc_status_offset;
	uint8_t ecc_status_mask;
	uint8_t ecc_status_shift;
	uint8_t ecc_status_valid_val;
};

//void bbt_dump_buf(char *s, void *buf, int len);

void spi_nand_dump_vec(void);

void get_spi_nand_info(void);

int spi_nand_scan_vector(int fd);

void verify_fip_bin(void);

int spi_nand_flush_fip_bin(int dev_fd, char* path);

int spi_nand_flush_vec(int fd);

int spi_nand_check_write_vector(void *buf);


#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif /* __cplusplus */

#endif	// __CVI_FLASH_H__

