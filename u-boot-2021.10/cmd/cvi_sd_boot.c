#include <common.h>
#include <command.h>
#include <dm.h>
#include <mmc.h>

static int do_cvi_sd_boot(struct cmd_tbl *cmdtp, int flag, int argc, char * const argv[])
{
	struct udevice *dev;
	char sddev[10];
	char newroot[100];
	char *root;
	char *pos1, *pos2;

	if (argc == 1) {
		for (uclass_first_device(UCLASS_MMC, &dev); dev; uclass_next_device(&dev)) {
			struct mmc *m = mmc_get_mmc_dev(dev);

			mmc_init(m);

			if (m->has_init && IS_SD(m)) {
				snprintf(sddev, sizeof(sddev), "%d", mmc_get_blk_desc(m)->devnum);

				if (env_set("sddev", sddev)) {
					printf("set env fail\n");
				} else {
					printf("set sdenv=%s\n", sddev);

					root = env_get("root");
					pos1 = strstr(root, "mmcblk");
					pos2 = strstr(root, "p");

					if (pos1 == NULL || pos2 == NULL) {
						return -1;
					}

					pos1 += sizeof("mmcblk") - 1;
					snprintf(newroot, pos1 - root + 1, "%s", root);
					strcat(newroot, sddev);
					strcat(newroot, pos2);

					if (env_set("root", newroot)) {
						printf("set root fail\n");
					} else {
						printf("set root=%s\n", newroot);
					}
				}
			}
		}
	} else if (argc == 2) {
		snprintf(sddev, sizeof(sddev), "%s", argv[1]);

		if (env_set("sddev", sddev)) {
				printf("set env fail\n");
			} else {
				printf("set sdenv=%s\n", sddev);

				root = env_get("root");
				pos1 = strstr(root, "mmcblk");
				pos2 = strstr(root, "p");

				if (pos1 == NULL || pos2 == NULL) {
					return -1;
				}

				pos1 += sizeof("mmcblk") - 1;
				snprintf(newroot, pos1 - root + 1, "%s", root);
				strcat(newroot, sddev);
				strcat(newroot, pos2);

				if (env_set("root", newroot)) {
					printf("set root fail\n");
				} else {
					printf("set root=%s\n", newroot);
				}
			}
	} else {
		printf("Usage:\n%s\n", cmdtp->usage);
	}

	return 0;
}

U_BOOT_CMD(cvi_sd_boot, 2, 0, do_cvi_sd_boot,
		"boot from SD card",
		"cvi_sd_boot [dev]\n"
		"    dev: number of mmc dev"
);
