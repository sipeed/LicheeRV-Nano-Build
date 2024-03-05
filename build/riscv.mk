riscv-cpio:
	$(call print_target)
	${Q}mkdir -p ${RAMDISK_PATH}/${RAMDISK_OUTPUT_FOLDER}
	${Q}mkdir -p ${KERNEL_PATH}/${KERNEL_OUTPUT_FOLDER}
	${Q}cd $(RAMDISK_PATH)/initramfs/$(INITRAMFS_BASE) ;\
	${Q}find . | cpio --quiet -o -H newc > ${RAMDISK_PATH}/${RAMDISK_OUTPUT_FOLDER}/boot.cpio
	${Q}cp ${RAMDISK_PATH}/${RAMDISK_OUTPUT_FOLDER}/boot.cpio ${KERNEL_PATH}/${KERNEL_OUTPUT_FOLDER}/

opensbi-kernel: export CROSS_COMPILE=$(patsubst "%",%,$(CONFIG_CROSS_COMPILE_KERNEL))
opensbi-kernel: export ARCH=$(patsubst "%",%,$(CONFIG_ARCH))
opensbi-kernel:
	$(call print_target)
	${Q}$(MAKE) -C ${OPENSBI_PATH} PLATFORM=generic \
	    FW_PAYLOAD_PATH=${KERNEL_PATH}/${KERNEL_OUTPUT_FOLDER}/arch/${ARCH}/boot/Image \
	    FW_FDT_PATH=${RAMDISK_PATH}/${RAMDISK_OUTPUT_FOLDER}/${CHIP}_${BOARD}.dtb
	${Q}mkdir -p ${OUTPUT_DIR}/elf
	${Q}cp ${OPENSBI_PATH}/build/platform/generic/firmware/fw_payload.bin ${OUTPUT_DIR}/fw_payload_linux.bin
	${Q}cp ${OPENSBI_PATH}/build/platform/generic/firmware/fw_payload.elf ${OUTPUT_DIR}/elf/fw_payload_linux.elf
