#! /bin/sh

# 支持的格式，可以通过修改这个变量自定义支持的格式
# Supported formats, you can customize the list by modifying this variable
supported_formats="yuyv mjpg"

# 支持的分辨率，可以通过修改这个变量自定义支持的分辨率
# Supported resolutions, you can customize the list by modifying this variable
supported_resolutions="480x320 640x360 640x480 1280x720 1920x1080"


parse_uvc_config() {
	input_file="$1"  # 传入的文件路径 / Path to the input file
	parsed_data=""  # 初始化 parsed_data 为空 / Initialize parsed_data as empty

	# 检查文件是否存在并可读取 / Check if the file exists and is readable
	if [ -z "$input_file" ]; then
		# 没有提供文件路径 / No input file provided
		echo "Warn: No input file provided for config." 1>&2  # 输出到标准错误 / Output to stderr
	elif [ ! -f "$input_file" ]; then
		# 文件不存在 / File does not exist
		echo "Warn: File '$input_file' does not exist." 1>&2  # 输出到标准错误 / Output to stderr
	elif [ ! -r "$input_file" ]; then
		# 文件不可读取 / File is not readable
		echo "Warn: File '$input_file' is not readable." 1>&2  # 输出到标准错误 / Output to stderr
	else
		# 使用 awk 处理文件内容 / Using awk to process the file content
		parsed_data=$(awk -v resolutions="$supported_resolutions" -v formats="$supported_formats" '
		BEGIN {
			OFS = " ";
			split(resolutions, supported_res, " ");  # 将支持的分辨率放入数组 / Split the supported resolutions into an array
			split(formats, supported_fmt, " ");  # 将支持的格式放入数组 / Split the supported formats into an array
		}
		/^[^#]/ {  # 只处理非 # 开头的行 / Process lines that do not start with #
			fmt = $1;
			res = $2;

			# 检查格式是否在支持的格式列表中 / Check if the format is in the supported formats list
			is_supported_fmt = 0;
			for (i in supported_fmt) {
				if (fmt == supported_fmt[i]) {
					is_supported_fmt = 1;
					break;
				}
			}
			if (!is_supported_fmt) {
				print "Unsupported format:", fmt > "/dev/stderr";  # 输出不支持的格式到标准错误 / Output unsupported format to stderr
				next;
			}

			if (res ~ /^[0-9]+x[0-9]+$/) {  # 匹配 640x360/1280x720 这种格式 / Match the format like 640x360 or 1280x720
				is_supported_res = 0;
				for (i in supported_res) {
					if (res == supported_res[i]) {
						is_supported_res = 1;
						break;
					}
				}
				if (!is_supported_res) {
					print "Unsupported resolution:", res > "/dev/stderr";  # 输出不支持的分辨率到标准错误 / Output unsupported resolution to stderr
					next;
				}
				split(res, dims, "x");  # 将 1280x720 按 "x" 分割成宽高 / Split 1280x720 into width and height
				w = dims[1];
				h = dims[2];
			} else {
				print "Invalid resolution format:", res > "/dev/stderr";  # 输出无效的分辨率格式到标准错误 / Output invalid resolution format to stderr
				next;
			}
			print fmt, w, h;  # 输出格式 fmt, w, h / Output fmt, w, h
		}
		' "$input_file")
	fi

	# 如果没有解析到任何数据，或者文件不可读取，则返回默认值 / If no data is parsed or file is unreadable, return default values
	if [ -z "$parsed_data" ]; then
		echo "Warn: No valid config, using default instead." 1>&2  # 输出到标准错误 / Output to stderr
		parsed_data="mjpg 640 360"$'\n'"yuyv 640 360"
	fi

	# 确保至少包含 'mjpg' 和 'yuyv' 格式 / Ensure at least one 'mjpg' and 'yuyv' format exists
	has_mjpg=$(echo "$parsed_data" | grep -w "mjpg")
	has_yuyv=$(echo "$parsed_data" | grep -w "yuyv")

	if [ -z "$has_mjpg" ]; then
		echo "Warn: No 'mjpg' config found, adding default 'mjpg 640 360'." 1>&2
		parsed_data="$parsed_data"$'\n'"mjpg 640 360"
	fi

	if [ -z "$has_yuyv" ]; then
		echo "Warn: No 'yuyv' config found, adding default 'yuyv 640 360'." 1>&2
		parsed_data="$parsed_data"$'\n'"yuyv 640 360"
	fi

	echo "$parsed_data"  # 返回解析的数据 / Return the parsed data
}




##########################################################################
# UVC

CONFIGFS_PATH="/sys/kernel/config/usb_gadget/g0"
ORIGINAL_DIR=$(pwd)  # 记录脚本启动时的目录 / Record the original working directory

create_frame() {
	# Example usage:
	# create_frame <width> <height> <format name>

	WIDTH=$1
	HEIGHT=$2
	FORMAT=$3

	if [ "$FORMAT" == "yuyv" ]; then
		wdir=streaming/uncompressed/u/${HEIGHT}p
	elif [ "$FORMAT" == "mjpg" ]; then
		wdir=streaming/mjpeg/m/${HEIGHT}p
	else
		echo "only support format yuyv/mjpeg!"
		exit 1
	fi

	mkdir -p $wdir
	echo $WIDTH > $wdir/wWidth
	echo $HEIGHT > $wdir/wHeight
	echo $(( $WIDTH * $HEIGHT * 2 )) > $wdir/dwMaxVideoFrameBufferSize
	echo $(( $WIDTH * $HEIGHT * 2 * 8 * 5  )) > $wdir/dwMinBitRate
	echo $(( $WIDTH * $HEIGHT * 2 * 8 * 60 )) > $wdir/dwMaxBitRate

	cat > $wdir/dwFrameInterval <<EOF
166666
333333
400000
500000
666666
1000000
2000000
EOF
	echo "333333" > $wdir/dwDefaultFrameInterval

<< EOF
    configure FrameRate
    dwFrameInterfal is in 100-ns units (fps = 1/(dwFrameInterval * 10000000))
      166666 -> 60 fps
      333333 -> 30 fps
      400000 -> 25 fps
      500000 -> 20 fps
      666666 -> 15 fps
     1000000 -> 10 fps
     2000000 ->  5 fps
     5000000 ->  2 fps
    10000000 ->  1 fps
EOF
}


# 挂载 UVC ConfigFS / Mount the UVC ConfigFS
mount_uvc_configfs() {
	echo "Mounting UVC ConfigFS in $(pwd)"

	# 创建 functions/uvc.usb0 目录 / Create the directory for uvc.usb0
	mkdir -p functions/uvc.usb0
	cd functions/uvc.usb0

	# 在 functions/uvc.usb0 目录中创建控制头和符号链接 / Create control header and symlink it in uvc.usb0
	mkdir -p control/header/h
		ln -s control/header/h/ control/class/fs/
		ln -s control/header/h/ control/class/ss/

	# streaming_maxpacket sets wMaxPacketSize. Valid values are 1024/2048/3072
	echo 1024 > streaming_maxpacket
	# streaming_interval sets bInterval. Values range from 1..255
	echo 1 > streaming_interval
	# streaming_maxburst sets bMaxBurst. Valid values are 1..15
	echo 1 > streaming_maxburst

	# 在 functions/uvc.usb0 目录中创建流媒体路径 / Create streaming paths in uvc.usb0
	mkdir -p streaming/uncompressed/u  # YUV 目录 / YUV directory
	mkdir -p streaming/mjpeg/m         # MJPEG 目录 / MJPEG directory
		# 执行解析并直接传递给 while 循环进行处理 / Execute parsing and directly pass to while loop for processing
		parse_uvc_config "$1" | while read fmt w h; do
			echo "Adding: Format: $fmt, Width: $w, Height: $h"  # 输出格式、宽和高 / Output fmt, width, and height
			create_frame $w $h $fmt
		done
	mkdir -p streaming/header/h
		# 将 YUV 和 MJPEG 目录链接到流媒体头中 / Link YUV and MJPEG to streaming header
		ln -s streaming/uncompressed/u/ streaming/header/h/
		ln -s streaming/mjpeg/m/ streaming/header/h/
		# 将 header 链接到 class 中 / Link header to classes
		ln -s streaming/header/h/ streaming/class/fs
		ln -s streaming/header/h/ streaming/class/hs
		ln -s streaming/header/h/ streaming/class/ss

	cd ../..
	ln -s functions/uvc.usb0 configs/c.1

    echo "UVC ConfigFS mounted successfully in $(pwd)"
}

# 卸载 UVC ConfigFS / Unmount the UVC ConfigFS
unmount_uvc_configfs() {
	echo "Unmounting UVC ConfigFS in $(pwd)"

	unlink configs/c.1/uvc.usb0

	cd functions/uvc.usb0

	# 删除流媒体目录 / Remove streaming directories
	unlink streaming/class/fs/h
	unlink streaming/class/hs/h
	unlink streaming/class/ss/h

	unlink streaming/header/h/u
	unlink streaming/header/h/m
	rmdir streaming/header/h

	rmdir streaming/uncompressed/u/*p
	rmdir streaming/mjpeg/m/*p
	rmdir streaming/uncompressed/u
	rmdir streaming/mjpeg/m

	unlink control/class/fs/h
	unlink control/class/ss/h
	rmdir control/header/h

	cd ../..
	# 删除 functions/uvc.usb0 目录 / Remove the uvc.usb0 directory
	rmdir functions/uvc.usb0

	echo UVC ConfigFS unmounted successfully in $(pwd)
}

# 进入工作目录 / Enter the working directory
cd "$CONFIGFS_PATH" || return 1

# 处理传入的参数
# Process the input arguments
case "$1" in
	"mount")
		mount_uvc_configfs "$2"
        ;;
	"unmount")
		unmount_uvc_configfs
        ;;
	"server")
		(	
			echo "Waiting for UDC start..."
			while [ -z "$(cat /sys/kernel/config/usb_gadget/g0/UDC)" ]; do
				echo -n "."
				sleep 1
			done
			sleep 2	# necessary delay time
			echo -e "\nServer is starting..."  # 输出选择了 server 模式 / Output that the server mode is selected
			echo -e "===============================\n\n\n"
			# 在这里添加你希望在 server 模式下执行的其他操作 / Add other operations you want to perform in server mode here
			/etc/init.d/uvc-gadget-server.elf  -u /dev/$(basename /sys/class/udc/*/device/gadget/video4linux/video*)  -d -i /bin/cat_224.jpg 
		) >/tmp/uvc-gadget.log 2>&1 &
	;;
	*)
		echo "Usage: $0 {mount <file>|unmount|server}"  # 提示正确的使用方式 / Prompt the correct usage
	;;
esac

# 返回原始目录 / Return to the original directory
cd "$ORIGINAL_DIR" || return 1