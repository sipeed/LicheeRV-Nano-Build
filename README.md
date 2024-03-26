# LicheeRV-Nano-Build

# download source

```
git clone https://github.com/LicheeRV-Nano-Build --depth=1
cd LicheeRV-Nano-Build
git clone https://github.com/sophgo/host-tools --depth=1
```

## host environment

you can use container:

```
cd host/ubuntu
docker build -t licheervnano-build-ubuntu .
docker run --name licheervnano-build-ubuntu
docker export | sqfstar licheervnano-build-ubuntu.sqfs
singularity shell -e licheervnano-build-ubuntu.sqfs
```

# build it

```
source build/cvisetup.sh
# C906:
defconfig sg2002_licheervnano_sd
# A53:
# defconfig sg2002_licheea53nano_sd
build_all
```
