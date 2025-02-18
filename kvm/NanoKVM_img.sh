
cd kvm
rm -r ./kvmapp
rm -r ./latest
curl -O https://cdn.sipeed.com/nanokvm/latest.zip
unzip latest.zip
rm latest.zip
mv ./latest ./kvmapp
touch ./kvmapp/kvm_new_img
chmod -R +x ./kvmapp
cd ../

./host/mount_ext4.sh $1 mountpoint
cp -r ./kvm/kvmapp/ mountpoint/
cp ./kvm/frp_0.59.0_linux_riscv64/frpc mountpoint/usr/bin/
cp ./kvm/tailscale_1.80.2_riscv64/tailscale mountpoint/usr/bin/
cp ./kvm/tailscale_1.80.2_riscv64/tailscaled mountpoint/usr/sbin/
chmod +x mountpoint/usr/bin/frpc
chmod +x mountpoint/usr/bin/tailscale
chmod +x mountpoint/usr/sbin/tailscaled
cp -f kvm/kvmapp/system/ko/soph_mipi_rx.ko ./mountpoint/mnt/system/ko/
chmod +x mountpoint/mnt/system/ko/soph_mipi_rx.ko
rm -rf mountpoint/etc/init.d
cp -rf kvm/init.d mountpoint/etc/
mkdir mountpoint/etc/kvm/
touch mountpoint/etc/kvm/ssh_stop
umount mountpoint

# ./kvm/NanoKVM_img.sh /home/bugu/LicheeRV-Nano-Build/install/soc_sg2002_licheervnano_sd/images/2025-02-17-19-08-3649fe.img
# cp /home/bugu/LicheeRV-Nano-Build/install/soc_sg2002_licheervnano_sd/images/2025-02-17-19-08-3649fe.img /home/bugu/LicheeRV-Nano-Build/install/soc_sg2002_licheervnano_sd/images/20250217_NanoKVM_Rev1_4_0.img
# xz /home/bugu/LicheeRV-Nano-Build/install/soc_sg2002_licheervnano_sd/images/20250217_NanoKVM_Rev1_4_0.img
