#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <math.h>
#include <assert.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <getopt.h>
#include "cvi_fip.h"
#include "mtd-user.h"


#define RW_FIXED_SIZE (2048UL)
#define MAX_CMD_LEN 128
#define MAX_FIP_SIZE (640 * 1024UL)
#define MAX_NAME_LEN 32
#define VER 		"V1.3"

//#define DEBUG

#ifdef DEBUG
#define DBG		printf
#else
#define DBG
#endif

#define ERR printf

#define FIP_MTD_DEV "/dev/mtd0"
#define TOTAL_BLOCK_NUM_FOR_FIP		20

uint8_t pg_buf[MAX_PAGE_SIZE];
char g_spi_nand_sys_vec[MAX_PAGE_SIZE];
mtd_info_t g_spi_nand_info;
char block_table[TOTAL_BLOCK_NUM_FOR_FIP]= {0};

static int erase_func(int fd, uint32_t erase_start_addr)
{
	int ret;
	struct erase_info_user64 ei64;
	struct erase_info_user ei;
	mtd_info_t *info = &g_spi_nand_info;

	ei64.start = (__u64)erase_start_addr;
	ei64.length = (__u64)info->erasesize;

	if (lseek(fd, erase_start_addr, SEEK_SET) != erase_start_addr) {
		printf("[%s] seek failed!\n", __func__);
		return -1;
	}

	ei.start = ei64.start;
	ei.length = ei64.length;
	ret = ioctl(fd, MEMERASE, &ei);
	if (ret < 0) {
		strerror(ret);
		printf("erase block:%#x failed, ret:%d \n", ei.start, ret);
		return ret;
	}

	return 0;
}

int spi_nand_scan_vector(int fd)
{
	int j;
	int ret = 0;
	uint32_t block_addr;
	struct _spi_nand_base_vector_t *sv =
		(struct _spi_nand_base_vector_t *)g_spi_nand_sys_vec;
	mtd_info_t *info = &g_spi_nand_info;

	for (j = 0; j < SPI_NAND_BASE_RESERVED_ENTRY; j++) {

		if (block_table[j])
			continue;

		block_addr = info->erasesize * j;
		memset(pg_buf, 0, MAX_PAGE_SIZE);
		DBG("read sv at : %#x \n", block_addr);

		if (lseek(fd, block_addr, SEEK_SET) != block_addr) {
			printf("[%s] seek failed!\n", __func__);
			return -1;
		}

		ret = read(fd, pg_buf, info->writesize);
		if (ret != info->writesize) {
			printf("read sv failed, ret:%d \n", ret);
			return ret;
		}

		memcpy(sv, pg_buf, sizeof(struct _spi_nand_base_vector_t));
		if (sv->signature == SPI_NAND_VECTOR_SIGNATURE) {
			printf("sv found version 0x%x\n", sv->version);
			return 0;
		}
	}

	ERR("Can't find correct system vector, sv->signature: %s\n", sv->signature);
	return -1;
}

void spi_nand_dump_vec(void)
{
	int i, j;
	struct _spi_nand_base_vector_t *sv =
		(struct _spi_nand_base_vector_t *)g_spi_nand_sys_vec;

	printf("signature: 0x%x\n", sv->signature);
	printf("version 0x%x\n", sv->version);

	for (i = 0; i < SPI_NAND_BASE_DATA_BACKUP_COPY; i++)
		printf("spi_nand_vec_blks 0x%x\n", sv->spi_nand_vector_blks[i]);

	printf("fip_bin_blk_cnt 0x%x\n", sv->fip_bin_blk_cnt);

	for (i = 0; i < SPI_NAND_BASE_DATA_BACKUP_COPY; i++)
		for (j = 0; j < SPI_NAND_FIP_DATA_BLOCK_COUNT; j++)
			if (sv->fip_bin_blks[i][j])
				printf("fip bin blks 0x%x\n", sv->fip_bin_blks[i][j]);

	printf("spi nand info block cnt 0x%x\n", sv->spi_nand_info.block_cnt);
	printf("spi nand info id 0x%x\n", sv->spi_nand_info.id);
	printf("spi nand info pages per block 0x%x\n", sv->spi_nand_info.pages_per_block);
	printf("spi nand info pages per block shift 0x%x\n", sv->spi_nand_info.pages_per_block_shift);
	printf("spi nand info page size 0x%x\n", sv->spi_nand_info.page_size);
	printf("spi nand info spare size 0x%x\n", sv->spi_nand_info.spare_size);
}

int dump_fip(int dev_fd, char *path)
{
	int fd, page_index, ret;
	uint32_t i, j, pages_per_block, pos;
	char fip_name[32] = {0};
	mtd_info_t *info = &g_spi_nand_info;
	struct _spi_nand_base_vector_t *sv =
		(struct _spi_nand_base_vector_t *) g_spi_nand_sys_vec;

	pages_per_block = 1 << sv->spi_nand_info.pages_per_block_shift;

	int len = strlen(path);
	if (path[len - 1] != '/') {
		path[len] = '/';
		len += 1;
	}

	for (i = 0; i < SPI_NAND_BASE_DATA_BACKUP_COPY; i++) {
		sprintf(fip_name, "fip%d.bin", i);
		memcpy(path + len, fip_name, strlen(fip_name) + 1);
		DBG(" fip path is %s \n", path);
		/* 0755 just for /mnt/data dir */
		fd = open(path, O_WRONLY | O_CREAT | O_TRUNC, 0755);
		if (fd < 0) {
			printf("creat %s file failed!, ret:%d \n", path, fd);
			return -1;
		}

		for (j = 0; j < SPI_NAND_FIP_DATA_BLOCK_COUNT; j++) {

			pos = sv->fip_bin_blks[i][j] * info->erasesize;
			assert(sv->fip_bin_blks[i][j] <= MAX_BLOCK_CNT);

			if (sv->fip_bin_blks[i][j] == 0)
				continue;

			DBG("write blk id %d\n\n", sv->fip_bin_blks[i][j]);

			if (lseek(dev_fd, pos, SEEK_SET) != pos) {
				printf("seek failed!\n");
				return ret;
			}

			memset(pg_buf, 0, MAX_PAGE_SIZE);
			page_index = 0;
			/* read and write a block */
			while (page_index <= (pages_per_block - 1)) {

				DBG("page index is %d \n", page_index);
				ret = read(dev_fd, pg_buf, RW_FIXED_SIZE);
				if(ret < 0) {
					printf("read file failed, ret:%d\n", ret);
					return ret;
				}

				ret = write(fd, pg_buf, RW_FIXED_SIZE);
				if(ret < 0) {
					printf("write file failed, ret:%d \n", ret);
					return ret;
				}

				memset(pg_buf, 0, MAX_PAGE_SIZE);
				pos += info->writesize;
				if (lseek(dev_fd, pos, SEEK_SET) != pos) {
					printf("[%s, %d]==> seek failed!\n", __func__,__LINE__);
					return -1;
				}
				page_index++;
			}
		}
		sync();
		close(fd);
	}
	return 0;
}

int spi_nand_flush_fip_bin(int dev_fd, char* path)
{
	struct stat st = {0};
	struct _spi_nand_base_vector_t *sv =
		(struct _spi_nand_base_vector_t *) g_spi_nand_sys_vec;
	mtd_info_t *info = &g_spi_nand_info;
	int ret, fip_fd;
	void *file_buff = NULL;
	uint32_t len, off, pos, pages_per_block, blk_id, erase_start_addr, i ,j, data_len;

	pages_per_block = 1 << sv->spi_nand_info.pages_per_block_shift;

	fip_fd = open(path, O_RDONLY);
	if (fip_fd < 0) {
		printf("open %s failed, ret:%d \n", path, fip_fd);
		return fip_fd;
	}

	if (stat(path, &st)) {
		printf("stat file failed!\n");
		return -1;
	}
	DBG(" file size is %u\n", st.st_size);

	len = st.st_size;
	file_buff = mmap(NULL, len, PROT_READ, MAP_PRIVATE, fip_fd, 0);
	if (file_buff == NULL) {
		printf("mmap fip.bin failed!\n");
		return -1;
	}

	DBG("%s, fip.bin blk cnt %d\n", __func__, sv->fip_bin_blk_cnt);

	for (i = 0; i < SPI_NAND_BASE_DATA_BACKUP_COPY; i++) {
		off = 0;
		len = st.st_size;
		for (j = 0; j < SPI_NAND_FIP_DATA_BLOCK_COUNT && len; j++) {
			blk_id = sv->fip_bin_blks[i][j];
			erase_start_addr = sv->fip_bin_blks[i][j] * info->erasesize;
			int page_index;

			assert(sv->fip_bin_blks[i][j] <= MAX_BLOCK_CNT);

			if (blk_id == 0)
				continue;

			DBG("erase blk id %d\n\n", sv->fip_bin_blks[i][j]);
			ret = erase_func(dev_fd, erase_start_addr);
			if (ret)
				return ret;

			pos = erase_start_addr;
			if (lseek(dev_fd, pos, SEEK_SET) != pos) {
				printf("[%s, %d] ==> seek failed!\n", __func__,__LINE__);
				return ret;
			}

			page_index = 0;
			memset(pg_buf, 0xff, MAX_PAGE_SIZE);
			/* write a block */
			while (len && (page_index <= pages_per_block - 1)) {

				data_len = len > RW_FIXED_SIZE ? RW_FIXED_SIZE : len;
				memcpy(pg_buf, file_buff + off, data_len);
				off += data_len;
				len -= data_len;
				DBG("date len is %d, page index %d \n", data_len, page_index);
				ret = write(dev_fd, pg_buf, info->writesize);
				if (ret != info->writesize) {
					ERR("Write fail, ret: %d\n", ret);
					return -1;
				}

				page_index++;
				pos += info->writesize;

				if (lseek(dev_fd, pos, SEEK_SET) != pos) {
					printf("[%s, %d] ==> seek failed!\n", __func__,__LINE__);
					return -1;
				}
			}
		}
	}
	munmap(file_buff, len);
	return 0;
}

int spi_nand_flush_vec(int fd)
{
	int ret, i;
	uint32_t block_addr;
	mtd_info_t *info = &g_spi_nand_info;
	struct _spi_nand_base_vector_t *sv =
		(struct _spi_nand_base_vector_t *) g_spi_nand_sys_vec;

	memset(pg_buf, 0xff, MAX_PAGE_SIZE);
	memcpy(pg_buf, sv, sizeof(struct _spi_nand_base_vector_t));

	for (i = 0; i < SPI_NAND_BASE_DATA_BACKUP_COPY; i++) {

		/* skip bad block */
		if (block_table[i])
			continue;

		block_addr = i * info->erasesize;
		if (lseek(fd, block_addr, SEEK_SET) != block_addr) {
			printf("[%s, %d] ==> seek failed!\n", __func__,__LINE__);
			return -1;
		}

		ret = write(fd, pg_buf, info->writesize);
		if (ret < 0) {
			printf("write sv data failed, ret:%d\n", ret);
			return ret;
		}
	}
	sync();
	return 0;
}

int check_bad_block(int fd)
{
	int i;
	int ret = 0;
	loff_t block_addr;

	mtd_info_t *spi_nand_info = &g_spi_nand_info;
	memset(spi_nand_info, 0x0, sizeof(mtd_info_t));

	ret = ioctl(fd, MEMGETINFO, spi_nand_info);
	if (ret < 0) {
		printf("get nand info failed!\n");
		return ret;
	}

	for (i = 0; i < TOTAL_BLOCK_NUM_FOR_FIP; i++) {
		block_addr = i * spi_nand_info->erasesize;
		if (ioctl(fd, MEMGETBADBLOCK, &block_addr)) {
			printf("bad block at 0x%llx \n", block_addr);
			block_table[i] = 1;
		}
	}

	printf("page size:0x%x\n", spi_nand_info->writesize);
	printf("erase size:0x%x\n", spi_nand_info->erasesize);

	return ret;
}

static void display_help(int status)
{
	fprintf(status == EXIT_SUCCESS ? stdout : stderr,
			"Usage: update_fip [OPTION] [path]\n"
			"Write or  dump fip.\n"
			"./update_fip -d -p /mnt/data \n"
			"./update_fip -u -p /mnt/data/fip.bin \n"
			"\n"

			"  -d                      Dump fip and backup of fip\n"
			"  --update                Update fip\n"
			"  -p, --path              The path of fip\n"
			"  -h, --help              Display this help and exit\n"
			"  -v, --version           Output version information and exit\n"
	       );
	exit(status);
}

static bool dump_fip_image = false;
static bool update_fip = false;
static char path[256] = {0};

void process_options(int argc, char * const argv[])
{
	int error = 0;

	for (;;) {
		int option_index = 0;
		static const char short_options[] = "vhdup:";
		static const struct option long_options[] = {
			/* Order of these args with val==0 matters; see option_index. */
			{"version", no_argument, 0, 'v'},
			{"dump", no_argument, 0, 'd'},
			{"update", no_argument, 0, 'u'},
			{"path", required_argument, 0, 'p'},
			{"help", no_argument, 0, 'h'},
			{0, 0, 0, 0},
		};

		int c = getopt_long(argc, argv, short_options,
				long_options, &option_index);

		if (c == -1)
			break;

		switch (c) {
			case 'v':
				printf("Version: %s \n\n", VER);
				exit(EXIT_SUCCESS);
				break;

			case 'u':
				update_fip = true;
				break;

			case 'd':
				printf("Get option d \n");
				dump_fip_image = true;
				break;

			case 'p':
				strcpy((void *)path, optarg);
				printf("path: %s\n", path);
				break;

			case 'h':
				display_help(EXIT_SUCCESS);
				break;

			case '?':
				error++;
				display_help(EXIT_SUCCESS);
				break;
		}
	}

	if (update_fip && dump_fip_image) {
		printf("can not do update either dump operation\n");
		exit(EXIT_FAILURE);
	}

	if (update_fip && !strlen(path))
		display_help(EXIT_SUCCESS);

	if (dump_fip_image && !strlen(path))
		display_help(EXIT_SUCCESS);

	return;
}

int main(int argc, char * const argv[])
{
	int fd;
	int ret = 0;

	process_options(argc, argv);
	/* rw-rw---- */
	fd = open(FIP_MTD_DEV, O_RDWR, S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP);
	if (fd < 0) {
		printf("open %s fialed!\n", FIP_MTD_DEV);
		return -1;
	}

	if(check_bad_block(fd)) {
		ret = -1;
		goto close_dev;
	}

	ret = spi_nand_scan_vector(fd);
	if (ret) {
		printf("unable to get sys vector\n");
		ret = -1;
		goto close_dev;
	}

	if (update_fip) {
		ret = spi_nand_flush_fip_bin(fd, path);
		//ret = spi_nand_flush_vec(fd);
		goto close_dev;
	}

	if (dump_fip_image)
		ret = dump_fip(fd, path);

close_dev:
	close(fd);

	return ret;
}


