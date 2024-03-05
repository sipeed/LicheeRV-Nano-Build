#include <sys/types.h>
#include <sys/stat.h>
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <errno.h>
#include <unistd.h>
#include <syslog.h>
#include <string.h>
#include <sys/ioctl.h>
#include <mtd/mtd-user.h>

#define FIP_PROTECTOR_VERSION	20220104

#define SPI_NAND_BASE_DATA_BACKUP_COPY (2)
#define SPI_NAND_FIP_DATA_BLOCK_COUNT (8)
#define SPI_NAND_BASE_RESERVED_ENTRY (4)
#define SPI_NAND_SV_RSVD_BLOCK_COUNT (4)

#define FIP_BLOCK_NUM	20
#define ECC_INIT	'9'
#define ECC_NO_ERR	'0'
#define ECC_CORR	'1'
#define ECC_UNCORR	'2'
#define BLK_UNUSED	0
#define BLK_USED	1

#define IOCTL_BASE_BASE		's'
#define IOCTL_READ_CHIP_ID	_IOR(IOCTL_BASE_BASE, 1, unsigned int)
#define IOCTL_READ_CHIP_VERSION	_IOR(IOCTL_BASE_BASE, 2, unsigned int)

static inline unsigned int DESC_CONV(char *x)
{
	return ((((((x[0] << 8) | x[1]) << 8) | x[2]) << 8) | x[3]);
}


static inline unsigned int CHECK_MASK_BIT(void *_mask, unsigned int bit)
{
	unsigned int w = bit / 8;
	unsigned int off = bit % 8;

	return ((unsigned char *)_mask)[w] & (1 << off);
}

static inline void SET_MASK_BIT(void *_mask, unsigned int bit)
{
	unsigned int byte = bit / 8;
	unsigned int offset = bit % 8;
	((unsigned char *)_mask)[byte] |= (1 << offset);
}

static inline void CLEAR_MASK_BIT(void *_mask, unsigned int bit)
{
	unsigned int byte = bit / 8;
	unsigned int offset = bit % 8;
	((unsigned char *)_mask)[byte] &= ~(1 << offset);
}

#define SPI_NAND_VECTOR_SIGNATURE DESC_CONV("SPNV")

struct cvi_erase_info {
	unsigned int start;
	unsigned int length;
};

struct _spi_nand_info {
	unsigned short id;
	unsigned short page_size;

	unsigned short spare_size;
	unsigned short pages_per_block;

	unsigned short block_cnt; // up to 32k block
	unsigned char pages_per_block_shift;
	unsigned char flags;
};

struct spi_nand_base_vector_t {
	unsigned int signature;
	unsigned int version;
	unsigned short spi_nand_vector_blks[SPI_NAND_BASE_DATA_BACKUP_COPY];
	unsigned short fip_bin_blk_cnt;
	unsigned short fip_bin_blks[SPI_NAND_BASE_DATA_BACKUP_COPY][SPI_NAND_FIP_DATA_BLOCK_COUNT];
	unsigned short erase_count; // erase count for sys base block
	unsigned short rsvd_block_count; // how many blocks reserved for spi_nand_vect and fip.bin
	unsigned int spi_nand_vector_length;  // spi_nand vector struct length, must less than a page
	unsigned char spi_nand_base_block_usage[SPI_NAND_BASE_RESERVED_ENTRY];
	struct _spi_nand_info spi_nand_info;
};

static unsigned int block_size;
static struct cvi_erase_info ei;               // the erase block structure
static int mtd_fd;
static mtd_info_t mtd_info;           // the MTD structure
static struct spi_nand_base_vector_t *sv;
unsigned char log_on = 0;

#define fp_print(fmt, ...)					\
({								\
	if (log_on) {					\
		printf(fmt, ##__VA_ARGS__);			\
	}							\
})

void dump_buf(char *s, void *buf, int len)
{
	int i;
	int line_counter = 0;
	int sep_flag = 0;
	int addr = 0;

	printf("%s 0x%p\n", s, buf);

	printf("%07x:\t", addr);

	for (i = 0; i < len; i++) {
		if (line_counter++ > 15) {
			line_counter = 0;
			sep_flag = 0;
			addr += 16;
			i--;
			printf("\n%07x:\t", addr);
			continue;
		}

		if (sep_flag++ > 1) {
			sep_flag = 1;
			printf(" ");
		}

		printf("%02x", *((char *)buf++));
	}

	printf("\n");
}

void dump_fip_protector_info(unsigned char *fip_ecc)
{
	fp_print("[FP] ===== Execute result after running fip protector ====\n");
	fp_print("[FP] SV ECC info : ");
	for(int i = 0; i < FIP_BLOCK_NUM; i++)
		fp_print("%c", fip_ecc[i]);
	fp_print("\n");

	fp_print("[FP] sv signature=0x%x\n", sv->signature);

#if 1
	fp_print("[FP] fip_bin_blk[0] %x %x %x %x %x\n", sv->fip_bin_blks[0][0], sv->fip_bin_blks[0][1], sv->fip_bin_blks[0][2],
	sv->fip_bin_blks[0][3], sv->fip_bin_blks[0][4]);
	fp_print("[FP] fip_bin_blk[1] %x %x %x %x %x\n", sv->fip_bin_blks[1][0], sv->fip_bin_blks[1][1], sv->fip_bin_blks[1][2],
	sv->fip_bin_blks[1][3], sv->fip_bin_blks[1][4]);
#endif
	//block_size = sv->spi_nand_info.page_size * sv->spi_nand_info.pages_per_block;
	fp_print("[FP] info in SV: page_size=%d page_per_block=%d, block_size=0x%x, fip_bin_blk_cnt=%d, rsvd_bk_cnt=%d\n", sv->spi_nand_info.page_size,
		sv->spi_nand_info.pages_per_block, block_size, sv->fip_bin_blk_cnt, sv->rsvd_block_count);

	fp_print("[FP] nand base block usage = %2x %2x %2x %2x\n",
				sv->spi_nand_base_block_usage[0],
				sv->spi_nand_base_block_usage[1],
				sv->spi_nand_base_block_usage[2],
				sv->spi_nand_base_block_usage[3]);
}

void reset_ecc(unsigned int blk_id)
{
	char cmd_msg[25];

	sprintf(cmd_msg, "echo %d > /proc/cvsnfc\n", blk_id);
	system(cmd_msg);
}

void fip_write(int blk_id, unsigned char *buf)
{
	lseek(mtd_fd, blk_id * block_size, SEEK_SET);
	write(mtd_fd, buf, block_size);
}

void fip_read(int blk_id, unsigned char *buf)
{
	lseek(mtd_fd, blk_id * block_size, SEEK_SET);
	read(mtd_fd, buf, block_size);
}

void fip_erase(int blk_id)
{
	lseek(mtd_fd, 0, SEEK_SET);
	ei.start = blk_id * block_size;
	/* erase destination block first before writing */
    ioctl(mtd_fd, MEMERASE, &ei);
	reset_ecc(blk_id);
}

void recheck_ecc(unsigned char *fip_ecc)
{
	int ecc_fd;

	ecc_fd = open("/proc/cvsnfc", O_RDONLY, 0);
	memset(fip_ecc, ECC_INIT, FIP_BLOCK_NUM);
	read(ecc_fd, fip_ecc, FIP_BLOCK_NUM);

	close(ecc_fd);
}

void rescan_ecc(unsigned char *fip_ecc)
{
	int ecc_fd;
	int i;
	char read_buf[block_size];

	system("echo 32 > /proc/cvsnfc");

	for (i = 0; i < FIP_BLOCK_NUM; i++)
		fip_read(i, read_buf);

	ecc_fd = open("/proc/cvsnfc", O_RDONLY, 0);
	memset(fip_ecc, ECC_INIT, FIP_BLOCK_NUM);
	read(ecc_fd, fip_ecc, FIP_BLOCK_NUM);

	close(ecc_fd);

	fp_print("[FP] %s ECC info : ", __func__);
	for(int i = 0; i < FIP_BLOCK_NUM; i++)
		fp_print("%c", fip_ecc[i]);
	fp_print("\n");
}

int get_sv_info(void)
{
	int ecc_fd;
	int sv_fd;
	unsigned char fip_ecc[FIP_BLOCK_NUM];
	int i;

#if 1
	sv_fd = open("/dev/mtd0", O_RDONLY, 0);
	sv = malloc(sizeof(struct spi_nand_base_vector_t));

	system("echo 32 > /proc/cvsnfc");

	for (i = 0; i < FIP_BLOCK_NUM; i++) {
		char read_buf[block_size];

		fip_read(i, read_buf);
		//lseek(sv_fd, i * block_size, SEEK_SET);
		//read(sv_fd, sv, sizeof(struct spi_nand_base_vector_t));
	}

	ecc_fd = open("/proc/cvsnfc", O_RDONLY, 0);
	memset(fip_ecc, ECC_INIT, FIP_BLOCK_NUM);
	read(ecc_fd, fip_ecc, FIP_BLOCK_NUM);

	memset(sv, 0, sizeof(struct spi_nand_base_vector_t));
#endif

#if 1
	fp_print("[FP] SV ECC info : ");
	for(int i = 0; i < FIP_BLOCK_NUM; i++)
		fp_print("%c", fip_ecc[i]);
	fp_print("\n");
#endif

	for (i = 0; i < SPI_NAND_SV_RSVD_BLOCK_COUNT; i++) {
		if ((fip_ecc[i] == ECC_CORR) || (fip_ecc[i] == ECC_NO_ERR)) {
			lseek(sv_fd, i * block_size, SEEK_SET);
			read(sv_fd, sv, sizeof(struct spi_nand_base_vector_t));
			//fp_print("[FP] sv signature = 0x%x\n", sv->signature);
			//dump_buf("sv: ", sv, 64);
			if (sv->signature == SPI_NAND_VECTOR_SIGNATURE)
				break;
		}
	}

	if (i == SPI_NAND_SV_RSVD_BLOCK_COUNT) {
		fp_print("[FP] No SV can be found\n");
		close(sv_fd);
		close(ecc_fd);
		return -1;
	}

	fp_print("[FP] sv signature=0x%x\n", sv->signature);

#if 1
	fp_print("[FP] fip_bin_blk[0] %x %x %x %x %x\n", sv->fip_bin_blks[0][0], sv->fip_bin_blks[0][1], sv->fip_bin_blks[0][2],
	sv->fip_bin_blks[0][3], sv->fip_bin_blks[0][4]);
	fp_print("[FP] fip_bin_blk[1] %x %x %x %x %x\n", sv->fip_bin_blks[1][0], sv->fip_bin_blks[1][1], sv->fip_bin_blks[1][2],
	sv->fip_bin_blks[1][3], sv->fip_bin_blks[1][4]);
#endif
	//block_size = sv->spi_nand_info.page_size * sv->spi_nand_info.pages_per_block;
	fp_print("[FP] info in SV: page_size=%d page_per_block=%d, block_size=0x%x, fip_bin_blk_cnt=%d, rsvd_bk_cnt=%d\n", sv->spi_nand_info.page_size,
		sv->spi_nand_info.pages_per_block, block_size, sv->fip_bin_blk_cnt, sv->rsvd_block_count);

	fp_print("[FP] nand base block usage = %2x %2x %2x %2x\n",
				sv->spi_nand_base_block_usage[0],
				sv->spi_nand_base_block_usage[1],
				sv->spi_nand_base_block_usage[2],
				sv->spi_nand_base_block_usage[3]);

	close(sv_fd);
	close(ecc_fd);
	return 0;
}

int fip_update(unsigned char *buf, unsigned int write_blk, unsigned char *fip_ecc)
{
	unsigned char read_buf[block_size];

	for (int retry = 0; retry < 3; retry++) { /* 3 retry chance in order to let self update can success */

    	fp_print("[FP] %s Eraseing Block[%x] start from 0x%x with length 0x%x\n", __func__, write_blk, ei.start, ei.length);
		fip_erase(write_blk);

		fp_print("[FP] %s Writing block[%x] with size 0x%x\n", __func__, write_blk, block_size);
		fip_write(write_blk, buf);

//	lseek(mtd_fd, 0, SEEK_SET);
		fip_read(write_blk, read_buf);
		recheck_ecc(fip_ecc);

		if(!memcmp(buf, read_buf, block_size) && (fip_ecc[write_blk] == ECC_NO_ERR)) {
			fp_print("[FP] %s try %d, update success!!\n", __func__, retry);
			return 0;
		}
	}

#if 0
	/* erase current block if failed */
	//fip_ecc[write_blk] == ECC_UNCORR;
	lseek(mtd_fd, 0, SEEK_SET);
	ei.start = write_blk * block_size;
    fp_print("[FP] %s Eraseing Block[%x] start from 0x%x with length 0x%x due to update failed\n", __func__, write_blk, ei.start, ei.length);
    ioctl(mtd_fd, MEMERASE, &ei);
	CLEAR_MASK_BIT(sv->spi_nand_base_block_usage, write_blk);
#endif
	fp_print("[FP] %s with blk_id %d fialed\n", __func__, write_blk);
	return -1;
}

int fip_blk_move(unsigned char *buf, unsigned int old_blk, unsigned int write_blk, unsigned char *fip_ecc)
{
	unsigned char read_buf[block_size];

	fp_print("[FP] %s Eraseing destination block[%x] start from 0x%x with length 0x%x\n", __func__, write_blk, ei.start, ei.length);
	fip_erase(write_blk);

	fp_print("[FP] %s Writing destination block[%x] with size 0x%x\n", __func__, write_blk, block_size);

	/* go to destination block and then write data */
	fip_write(write_blk, buf);

	/* return to begin address*/

	fip_read(write_blk, read_buf);

	recheck_ecc(fip_ecc);

	if (!memcmp(buf, read_buf, block_size) && (fip_ecc[write_blk] == ECC_NO_ERR)) {
		SET_MASK_BIT(sv->spi_nand_base_block_usage, write_blk);
    	fp_print("[FP] Erase original abnormal Block[%x] start from 0x%x with length 0x%x\n",old_blk, ei.start, ei.length);
		fip_erase(old_blk);
		CLEAR_MASK_BIT(sv->spi_nand_base_block_usage, old_blk);
		recheck_ecc(fip_ecc);
		fp_print("[FP] %s success!!\n", __func__);
		return 0;
	} else {
		/* clean target block if verify failed */
    	fp_print("[FP] %s Eraseing destination block[%x] start from 0x%x with length 0x%x due to failed\n", __func__, write_blk, ei.start, ei.length);
		fip_erase(write_blk);

		return -1;
	}
}

int fip_blk_recover(int fip_id, int blk_id, int type, unsigned char *fip_ecc)
{
	unsigned char read_buf[block_size];
	int select_blk;
	int old_blk;


	if ( type == ECC_CORR) {
//		lseek(mtd_fd, 0, SEEK_SET);
		fip_read(sv->fip_bin_blks[fip_id][blk_id], read_buf);
		//lseek(mtd_fd, sv->fip_bin_blks[fip_id][blk_id] * block_size, SEEK_SET);
		//read(mtd_fd, read_buf, block_size);
	} else if (type == ECC_UNCORR) {
		int backup_blk;

		if (fip_id == 0)
			backup_blk = sv->fip_bin_blks[1][blk_id];
		else
			backup_blk = sv->fip_bin_blks[0][blk_id];

		fp_print("[FP] %s, ECC_UNCORR,  find backup block = 0x%x\n", __func__, backup_blk);
		fip_read(backup_blk, read_buf);
//		lseek(mtd_fd, 0, SEEK_SET);
		//lseek(mtd_fd, backup_blk * block_size, SEEK_SET);
		//read(mtd_fd, read_buf, block_size);
		//dump_buf("read_buf: ", read_buf, 0x10);
	}

	old_blk = sv->fip_bin_blks[fip_id][blk_id];

	/* 1st round, find those unused blocks with ECC_NO_ERR */
	for (select_blk = SPI_NAND_SV_RSVD_BLOCK_COUNT; select_blk < sv->rsvd_block_count; select_blk++) {
		if ((fip_ecc[select_blk] == ECC_NO_ERR) && (CHECK_MASK_BIT(sv->spi_nand_base_block_usage, select_blk) == BLK_UNUSED) &&
			(select_blk != sv->fip_bin_blks[0][blk_id]) && (select_blk != sv->fip_bin_blks[1][blk_id])) {

				if (!fip_blk_move(read_buf, old_blk, select_blk, fip_ecc)) {
					fp_print("[FP] R1 move fip[%d][%d] from block 0x%x to block 0x%x\n", fip_id, blk_id, old_blk, select_blk);
					sv->fip_bin_blks[fip_id][blk_id] = select_blk;
					return 0;
				}
		}
	}
	fp_print("[FP] %s move fip[%d][%d] from block %d to block %d 1st round failed\n", __func__, fip_id, blk_id, old_blk, select_blk);

	/* 2nd round, erase those unused blocks wtih ECC_CORR and then find an valid block again*/
	for (select_blk = SPI_NAND_SV_RSVD_BLOCK_COUNT; select_blk < sv->rsvd_block_count; select_blk++) {
		if ((fip_ecc[select_blk] == ECC_CORR) && (CHECK_MASK_BIT(sv->spi_nand_base_block_usage, select_blk) == BLK_UNUSED) &&
			(select_blk != sv->fip_bin_blks[0][blk_id]) && (select_blk != sv->fip_bin_blks[1][blk_id])) {

				if (!fip_blk_move(read_buf, old_blk, select_blk, fip_ecc)) {
					fp_print("[FP] R2 move fip[%d][%d] from block 0x%x to block 0x%x\n", fip_id, blk_id, old_blk, select_blk);
					sv->fip_bin_blks[fip_id][blk_id] = select_blk;
					return 0;
				}
		}
	}

	fp_print("[FP] %s move fip[%d][%d] from block %d to block %d 2nd round failed\n", __func__, fip_id, blk_id, old_blk, select_blk);
	/* 3rd round, erase old block itself and try to use it again */
	if (!fip_update(read_buf, old_blk, fip_ecc)) {
		fp_print("[FP] R3 erase and update fip[%d][%d] itself\n", fip_id, blk_id);
		return 0;
	}

	fp_print("[FP] %s move fip[%d][%d] from block %d to block %d 3rd round failed\n", __func__, fip_id, blk_id, old_blk, select_blk);
	/* 4th round, erase those unused blocks with ECC_UNCORR and then find an valid block again */
	for (select_blk = SPI_NAND_SV_RSVD_BLOCK_COUNT; select_blk < sv->rsvd_block_count; select_blk++) {
		if ((fip_ecc[select_blk] == ECC_UNCORR) && (CHECK_MASK_BIT(sv->spi_nand_base_block_usage, select_blk) == BLK_UNUSED) &&
			(select_blk != sv->fip_bin_blks[0][blk_id]) && (select_blk != sv->fip_bin_blks[1][blk_id])) {

				if (!fip_blk_move(read_buf, old_blk, select_blk, fip_ecc)) {
					fp_print("[FP] R4 move fip[%d][%d] from block 0x%x to block 0x%x\n", fip_id, blk_id, old_blk, select_blk);
					sv->fip_bin_blks[fip_id][blk_id] = select_blk;
					return 0;
				}
		}
	}
	fp_print("[FP] %s move fip[%d][%d] from block %d to block %d 4th round failed\n", __func__, fip_id, blk_id, old_blk, select_blk);
	return -1;
}

int sv_blk_recover(int sv_id, int type, unsigned char *fip_ecc)
{
	unsigned char read_buf[block_size];
	int select_blk;
	int old_blk;


	if ( type == ECC_CORR) {
//		lseek(mtd_fd, 0, SEEK_SET);
		fip_read(sv->spi_nand_vector_blks[sv_id], read_buf);
		//lseek(mtd_fd, sv->spi_nand_vector_blks[sv_id] * block_size, SEEK_SET);
		//read(mtd_fd, read_buf, block_size);
	} else if (type == ECC_UNCORR) {
		int backup_blk;

		if (sv_id == 0)
			backup_blk = sv->spi_nand_vector_blks[1];
		else
			backup_blk = sv->spi_nand_vector_blks[0];

		fip_read(backup_blk, read_buf);
//		lseek(mtd_fd, 0, SEEK_SET);
		//lseek(mtd_fd, backup_blk * block_size, SEEK_SET);
		//read(mtd_fd, read_buf, block_size);
	}

	old_blk = sv->spi_nand_vector_blks[sv_id];

	for (select_blk = 0; select_blk < SPI_NAND_SV_RSVD_BLOCK_COUNT; select_blk++) {
		if ((fip_ecc[select_blk] == ECC_NO_ERR) && (CHECK_MASK_BIT(sv->spi_nand_base_block_usage, select_blk) == BLK_UNUSED) &&
			(select_blk != sv->spi_nand_vector_blks[0]) && (select_blk != sv->spi_nand_vector_blks[1])) {
				sv->spi_nand_vector_blks[sv_id] = select_blk;
				CLEAR_MASK_BIT(sv->spi_nand_base_block_usage, old_blk);
				SET_MASK_BIT(sv->spi_nand_base_block_usage, select_blk);
				memcpy(read_buf, sv, sizeof(struct spi_nand_base_vector_t));

				if (!fip_blk_move(read_buf, old_blk, select_blk, fip_ecc)) {
					fp_print("[FP] R1 move sv[%d] from block %d to block %d\n", sv_id, old_blk, select_blk);
					return 0;
				} else {
					/* move failed, restore to original status */
					sv->spi_nand_vector_blks[sv_id] = old_blk;
					SET_MASK_BIT(sv->spi_nand_base_block_usage, old_blk);
					CLEAR_MASK_BIT(sv->spi_nand_base_block_usage, select_blk);
				}
		}
	}
	fp_print("[FP] move sv[%d] from block 0x%x to block 0x%x 1st round failed\n", sv_id, old_blk, select_blk);

	for (select_blk = 0; select_blk < SPI_NAND_SV_RSVD_BLOCK_COUNT; select_blk++) {
		if ((fip_ecc[select_blk] == ECC_CORR) && (CHECK_MASK_BIT(sv->spi_nand_base_block_usage, select_blk) == BLK_UNUSED) &&
			(select_blk != sv->spi_nand_vector_blks[0]) && (select_blk != sv->spi_nand_vector_blks[1])) {
				sv->spi_nand_vector_blks[sv_id] = select_blk;
				CLEAR_MASK_BIT(sv->spi_nand_base_block_usage, old_blk);
				SET_MASK_BIT(sv->spi_nand_base_block_usage, select_blk);
				memcpy(read_buf, sv, sizeof(struct spi_nand_base_vector_t));

				if (!fip_blk_move(read_buf, old_blk, select_blk, fip_ecc)) {
					fp_print("[FP] R2 move sv[%d] from block %d to block %d\n", sv_id, old_blk, select_blk);
					return 0;
				} else {
					/* move failed, restore to original status */
					sv->spi_nand_vector_blks[sv_id] = old_blk;
					SET_MASK_BIT(sv->spi_nand_base_block_usage, old_blk);
					CLEAR_MASK_BIT(sv->spi_nand_base_block_usage, select_blk);
				}
		}
	}

	fp_print("[FP] move sv[%d] from block 0x%x to block 0x%x 2nd round failed\n", sv_id, old_blk, select_blk);

	if (!fip_update(read_buf, old_blk, fip_ecc)) {
		fp_print("[FP] R3 erase and update sv[%d] itself\n", sv_id);
		return 0;
	}

	fp_print("[FP] move sv[%d] from block 0x%x to block 0x%x 3rd round failed\n", sv_id, old_blk, select_blk);

	for (select_blk = 0; select_blk < SPI_NAND_SV_RSVD_BLOCK_COUNT; select_blk++) {
		if ((fip_ecc[select_blk] == ECC_UNCORR) && (CHECK_MASK_BIT(sv->spi_nand_base_block_usage, select_blk) == BLK_UNUSED) &&
			(select_blk != sv->spi_nand_vector_blks[0]) && (select_blk != sv->spi_nand_vector_blks[1])) {
				sv->spi_nand_vector_blks[sv_id] = select_blk;
				CLEAR_MASK_BIT(sv->spi_nand_base_block_usage, old_blk);
				SET_MASK_BIT(sv->spi_nand_base_block_usage, select_blk);
				memcpy(read_buf, sv, sizeof(struct spi_nand_base_vector_t));

				if (!fip_blk_move(read_buf, old_blk, select_blk, fip_ecc)) {
					fp_print("[FP] R4 move sv[%d] from block %d to block %d\n", sv_id, old_blk, select_blk);
					return 0;
				} else {
					/* move failed, restore to original status */
					sv->spi_nand_vector_blks[sv_id] = old_blk;
					SET_MASK_BIT(sv->spi_nand_base_block_usage, old_blk);
					CLEAR_MASK_BIT(sv->spi_nand_base_block_usage, select_blk);
				}
		}
	}

	fp_print("[FP] move sv[%d] from block 0x%x to block 0x%x 4th round failed\n", sv_id, old_blk, select_blk);

	return -1;
}

int sv_blk_update(unsigned char *fip_ecc, int sv_corrupt_mark)
{
	unsigned char read_buf[block_size];
	int select_blk;
	int old_blk;
	int ret;

sv_update_retry:

	if (sv_corrupt_mark == 0) { /* no sv blk need to recovery */
		//lseek(mtd_fd, 0, SEEK_SET);
		fip_read(sv->spi_nand_vector_blks[0], read_buf);
		//lseek(mtd_fd, sv->spi_nand_vector_blks[0] * block_size, SEEK_SET);
		//read(mtd_fd, read_buf, block_size);
		memcpy(read_buf, sv, sizeof(struct spi_nand_base_vector_t));

		ret = fip_update(read_buf, sv->spi_nand_vector_blks[0], fip_ecc);

		if (ret < 0) {
			sv_corrupt_mark += 1; /* SV0 is corrupted */
		}

		ret = fip_update(read_buf, sv->spi_nand_vector_blks[1], fip_ecc);

		if (ret < 0) {
			sv_corrupt_mark += 2; /* SV0 is corrupted */
		}

		if (sv_corrupt_mark > 0)
			goto sv_update_retry;

	} else if (sv_corrupt_mark == 1) { /* SV0 get ECC */
		fp_print("[FP] SV0, block %d gets ECC %s\n", sv->spi_nand_vector_blks[0],
		(fip_ecc[sv->spi_nand_vector_blks[0]] == ECC_CORR)? "CORR":"UNCORR");

		ret = sv_blk_recover(0, fip_ecc[sv->spi_nand_vector_blks[0]], fip_ecc);
		if (ret < 0)
			return ret;
		else
			sv_corrupt_mark = 0;
		fip_read(sv->spi_nand_vector_blks[1], read_buf);
		//lseek(mtd_fd, sv->spi_nand_vector_blks[1] * block_size, SEEK_SET);
		//read(mtd_fd, read_buf, block_size);
		memcpy(read_buf, sv, sizeof(struct spi_nand_base_vector_t));
		ret = fip_update(read_buf, sv->spi_nand_vector_blks[1], fip_ecc);

		if (ret < 0) {
			sv_corrupt_mark += 2; /* SV1 is corrupted */
		}

		if (sv_corrupt_mark > 0)
			goto sv_update_retry;

	} else if (sv_corrupt_mark == 2) { /* SV1 get ECC */
		fp_print("[FP] SV1, block %d gets ECC %s\n", sv->spi_nand_vector_blks[1],
		(fip_ecc[sv->spi_nand_vector_blks[1]] == ECC_CORR)? "CORR":"UNCORR");

		ret = sv_blk_recover(1, fip_ecc[sv->spi_nand_vector_blks[1]], fip_ecc);
		if (ret < 0)
			return ret;
		else
			sv_corrupt_mark = 0;

		fip_read(sv->spi_nand_vector_blks[0], read_buf);
		//lseek(mtd_fd, sv->spi_nand_vector_blks[0] * block_size, SEEK_SET);
		//read(mtd_fd, read_buf, block_size);
		memcpy(read_buf, sv, sizeof(struct spi_nand_base_vector_t));
		ret = fip_update(read_buf, sv->spi_nand_vector_blks[0], fip_ecc);

		if (ret < 0) {
			sv_corrupt_mark += 1; /* SV0 is corrupted */
		}

		if (sv_corrupt_mark > 0)
			goto sv_update_retry;

	} else { /* Both SV0 and SV1 get ECC */
		fp_print("[FP] SV0, block %d gets ECC %s\n", sv->spi_nand_vector_blks[0],
		(fip_ecc[sv->spi_nand_vector_blks[0]] == ECC_CORR)? "CORR":"UNCORR");
		ret = sv_blk_recover(0, fip_ecc[sv->spi_nand_vector_blks[0]], fip_ecc);
		if (ret < 0)
			return ret;
		else
			sv_corrupt_mark = 0;


		fp_print("[FP] SV1, block %d gets ECC %s\n", sv->spi_nand_vector_blks[1],
		(fip_ecc[sv->spi_nand_vector_blks[1]] == ECC_CORR)? "CORR":"UNCORR");
		ret = sv_blk_recover(1, fip_ecc[sv->spi_nand_vector_blks[1]], fip_ecc);
		if (ret < 0)
			return ret;
		else
			sv_corrupt_mark = 0;
	}
}

int main(int argc, char const *argv[])
{
	//struct spi_nand_base_vector_t *sv;
	//int sv_fd;
    pid_t pid, sid; /* Our process ID and Session ID */
	int ret;
	int i;
	int protect_period = 3600;
	int execute_once = 1;
	int fd;
	unsigned int chip_version = 0x18320001;

	if (argc <= 5) {
		for (i = 1; i < argc; i++) {
			if (!strcmp(argv[i], "-d"))
				log_on = 1;
			else if (!strcmp(argv[i], "-v")) {
				printf("fip_protector version is %d\n", FIP_PROTECTOR_VERSION);
				if (argc == 2)
					exit(EXIT_SUCCESS);
			}
			else if (!strcmp(argv[i], "-t")) {
				char accept[] = "1234567890";

				if (strspn(argv[i+1], accept) == 0)
					protect_period = 3600; /* use default 3600 seconds */
				else {
					printf("Set fip_protector period to %d\n", atoi(argv[i+1]));
					protect_period = atoi(argv[i+1]);
				}
				execute_once = 0;
			}
		}
	} else {
		printf("usage: fip_protector [option] \n");
		printf("       -d - enable debug log\n");
		printf("       -v - show fip_protector version\n");
		printf("       -t [period] - set protector period, unit is second\n");
		printf("\n");
		return -1;
	}

    pid = fork();

    if (pid < 0) {
		fp_print("[FP] creat pid failed\n");
        exit(EXIT_FAILURE);
    }

    if (pid > 0) {
		fp_print("[FP] get pid = %d\n", pid);
        exit(EXIT_SUCCESS);
    }

    fd = open("/dev/cvi-base", O_RDWR | O_SYNC);
    if (fd == -1) {
        fp_print("[FP] Open cvi-base [0]\n");
    }

    if (ioctl(fd, IOCTL_READ_CHIP_VERSION, &chip_version) < 0) {
        fp_print("[FP] Get CHIP_VERSION 0x%x\n", chip_version);
    } else
		fp_print("[FP] chip version=0x%x\n", chip_version);

	close(fd);

	if (chip_version == 0x3)
		exit(EXIT_SUCCESS);

    /* Change the file mode mask */
    umask(0);

    /* Create a new SID for the child process */
    sid = setsid();

    if (sid < 0) {
        /* Log the failure */
		fp_print("[FP] creat sid failed\n");
        exit(EXIT_FAILURE);
    }

    /* Change the current working directory */
    if ((chdir("/")) < 0) {
        /* Log the failure */
		fp_print("[FP] change to root failed\n");
        exit(EXIT_FAILURE);
    }

    /* Daemon-specific initialization goes here */
	mtd_fd = open("/dev/mtd0", O_RDWR);
	ioctl(mtd_fd, MEMGETINFO, &mtd_info);   // get the device info
	fp_print("[FP] MTD Type: %x\nMTD total size: 0x%x bytes\nMTD erase size: 0x%x bytes\n",
	mtd_info.type, mtd_info.size, mtd_info.erasesize);
	ei.length = mtd_info.erasesize;   //set the erase block size
	block_size = mtd_info.erasesize;
	close(mtd_fd);

	ret = get_sv_info(); // get SV information

	if (ret < 0) {
		printf("No SV can be found\n");
		exit(EXIT_FAILURE);
	}

	//fp_print("[FP] sv0 in blk %d, sv1 in blk %d\n", sv->spi_nand_vector_blks[0], sv->spi_nand_vector_blks[1]);

    while (1) {
		int ecc_fd;
		unsigned char fip_ecc[FIP_BLOCK_NUM];
		int idx, i, j;
		unsigned char read_buf[block_size];
		int sv_need_update;
		int sv_corrupt_mark;

		sv_need_update = 0;
		sv_corrupt_mark = 0; /* 1 means sv0 corrupted, 2 means sv1, 3 means both */

		mtd_fd = open("/dev/mtd0", O_RDWR);

		fp_print("[FP] clear fip ecc table and do rescan again\n");
		system("echo 32 > /proc/cvsnfc");

		//scan mtdblock0 block 0 ~ 19 (include all sv and fip)
		for (idx = 0; idx < sv->rsvd_block_count; idx++) {
			fip_read(idx, read_buf);
			//lseek(mtd_fd, (idx * block_size), SEEK_SET);
			//read(mtd_fd, read_buf, block_size);
		}

		ecc_fd = open("/proc/cvsnfc", O_RDONLY, 0);
		memset(fip_ecc, ECC_INIT, FIP_BLOCK_NUM);
		read(ecc_fd, fip_ecc, FIP_BLOCK_NUM);
		fp_print("[FP] ECC info : ");
		for(int i = 0; i < FIP_BLOCK_NUM; i++)
			fp_print("%c", fip_ecc[i]);
		fp_print("\n");

		// do check and recovery for fip blocks
		for (i = 0; i < SPI_NAND_BASE_DATA_BACKUP_COPY; i++)
			for (j = 0; j < sv->fip_bin_blk_cnt; j++) {
				idx = sv->fip_bin_blks[i][j];
				//fp_print("sv[%d][%d] = 0x%x\n", i, j, idx);
				if ((fip_ecc[idx] == ECC_CORR) || (fip_ecc[idx] == ECC_UNCORR)) {
					int ret;

					fp_print("[FP] detect ECC on block[0x%x]!!\n", idx);
					sv_need_update = 1;
					ret = fip_blk_recover(i, j, fip_ecc[idx], fip_ecc);

					if (ret < 0) {
						fp_print("[FP] no valid block can be used to do protector\n");
						exit(EXIT_FAILURE);
					}

				} else if (fip_ecc[idx] == ECC_INIT)
					fp_print("[FP] fip_ecc[%d] is not scaned\n", idx);
				//else
				//	fp_print("[FP] fip_ecc[%d] is good\n", idx);
			}

		// do SV check, recovery and update in final

		if ((fip_ecc[sv->spi_nand_vector_blks[0]] == ECC_CORR) || (fip_ecc[sv->spi_nand_vector_blks[0]] == ECC_UNCORR)) {
			sv_need_update = 1;
			sv_corrupt_mark += 1;
		}

		if ((fip_ecc[sv->spi_nand_vector_blks[1]] == ECC_CORR) || (fip_ecc[sv->spi_nand_vector_blks[1]] == ECC_UNCORR)) {
			sv_need_update = 1;
			sv_corrupt_mark += 2;
		}

		fp_print("[FP] sv_need_update=%d, sv_corrupt_mark=%d\n", sv_need_update, sv_corrupt_mark);
		if (sv_need_update == 1) {
			int ret;

			fp_print("[FP] do sv_blk_update\n");
			ret = sv_blk_update(fip_ecc, sv_corrupt_mark);

			if (ret < 0) {
				fp_print("[FP] No valid block can be used to update SV!!\n");
				exit(EXIT_FAILURE);
			}
		} else
			fp_print("[FP] Nothing to change\n");

		fp_print("[FP] finish scan and recovery\n");
		dump_fip_protector_info(fip_ecc);
		close(ecc_fd);
		close(mtd_fd);
		if (!execute_once) {
			fp_print("[FP] trigger fip_protector after %d seconds\n", protect_period);
			sleep(protect_period);
		} else
			break;
	}
	fp_print("[FP] exit!!\n");
	exit(EXIT_SUCCESS);
}

