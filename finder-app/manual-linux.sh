#!/bin/bash
# Script outline to install and build kernel.
# Author: Siddhant Jajoo.

set -e
set -u

OUTDIR=/tmp/aeld
KERNEL_REPO=git://git.kernel.org/pub/scm/linux/kernel/git/stable/linux-stable.git
KERNEL_VERSION=v5.1.10
BUSYBOX_VERSION=1_33_1
FINDER_APP_DIR=$(realpath $(dirname $0))
ARCH=arm64
CROSS_COMPILER=aarch64-none-linux-gnu
CROSS_COMPILE=${CROSS_COMPILER}-

if [ $# -lt 1 ]
then
	# No arguments supplied
	echo "Using default directory ${OUTDIR} for output"
else
	OUTDIR=$1
	echo "Using passed directory ${OUTDIR} for output"
fi

mkdir -p ${OUTDIR}

cd "$OUTDIR"
if [ ! -d "${OUTDIR}/linux-stable" ]; then
    #Clone only if the repository does not exist.
	echo "CLONING GIT LINUX STABLE VERSION ${KERNEL_VERSION} IN ${OUTDIR}"
	git clone ${KERNEL_REPO} --depth 1 --single-branch --branch ${KERNEL_VERSION}
fi
if [ ! -e ${OUTDIR}/linux-stable/arch/${ARCH}/boot/Image ]; then
    cd linux-stable
    echo "Checking out version ${KERNEL_VERSION}"
    git checkout ${KERNEL_VERSION}
	 

	 # deep clean the kernel tree
	 make ARCH=${ARCH} CROSS_COMPILE=${CROSS_COMPILE} mrproper

	 # Configure the virtual arm dev board
	 make ARCH=${ARCH} CROSS_COMPILE=${CROSS_COMPILE} defconfig

	 # Build the kernel image
	 make ARCH=${ARCH} CROSS_COMPILE=${CROSS_COMPILE} -j4 all

	 # Build any kernel modules -> SKIPPED as per assignment instructions
	 # make modules

	 # Build the device tree
	 make ARCH=${ARCH} CROSS_COMPILE=${CROSS_COMPILE} dtbs

	 
fi

echo "Adding the Image in outdir"
cp ${OUTDIR}/linux-stable/arch/${ARCH}/boot/Image ${OUTDIR}

echo "Creating the staging directory for the root filesystem"
cd "$OUTDIR"
if [ -d "${OUTDIR}/rootfs" ]
then
	echo "Deleting rootfs directory at ${OUTDIR}/rootfs and starting over"
    sudo rm  -rf ${OUTDIR}/rootfs
fi

mkdir "rootfs"
cd "${OUTDIR}/rootfs"

# Creating root folders
mkdir -p bin dev etc lib lib64 proc sbin sys tmp usr var
# Creating root folders
mkdir -p -m 777 home
# Creating user folders
mkdir -p usr/bin usr/lib usr/sbin
# Creating runtime files folder
mkdir -p var/log

cd "$OUTDIR"
if [ ! -d "${OUTDIR}/busybox" ]
then
    git clone git://busybox.net/busybox.git
    cd busybox
    git checkout ${BUSYBOX_VERSION}

	 # Reset Busybox for a new version
	 make distclean
	 # Configure Busybox
	 make defconfig
else
    cd busybox
fi

# Build Busybox
make ARCH=${ARCH} CROSS_COMPILE=${CROSS_COMPILE}
# Install Busybox in rootfs folder
make ARCH=${ARCH} CROSS_COMPILE=${CROSS_COMPILE} CONFIG_PREFIX=${OUTDIR}/rootfs install

cd "${OUTDIR}/rootfs"

echo "Library dependencies"
${CROSS_COMPILE}readelf -a bin/busybox | grep "program interpreter"
${CROSS_COMPILE}readelf -a bin/busybox | grep "Shared library"

# Add library dependencies to rootfs
CROSS_READ_ELF_PATH=$(type -aP ${CROSS_COMPILE}readelf)
CROSS_READ_ELF_DIR=$(dirname ${CROSS_READ_ELF_PATH})
CROSS_COMPILER_DIR=$(realpath ${CROSS_READ_ELF_DIR}/../${CROSS_COMPILER})
cp -d ${CROSS_COMPILER_DIR}/libc/lib/ld-linux-aarch64.so.[*0-9] lib
cp ${CROSS_COMPILER_DIR}/libc/lib64/ld-*.so lib64
cp -d ${CROSS_COMPILER_DIR}/libc/lib64/libm.so.[*0-9] lib64
cp ${CROSS_COMPILER_DIR}/libc/lib64/libm-*.so lib64
cp -d ${CROSS_COMPILER_DIR}/libc/lib64/libresolv.so.[*0-9] lib64
cp ${CROSS_COMPILER_DIR}/libc/lib64/libresolv-*.so lib64
cp -d ${CROSS_COMPILER_DIR}/libc/lib64/libc.so.[*0-9] lib64
cp ${CROSS_COMPILER_DIR}/libc/lib64/libc-*.so lib64

# Make device nodes
sudo mknod -m 666 dev/null c 1 3
sudo mknod -m 666 dev/console c 5 1

# Clean and build the writer utility
cd ${FINDER_APP_DIR}
echo "Removing the old writer utility and cross-compiling"
make clean
make ARCH=${ARCH} CROSS_COMPILE=${CROSS_COMPILE}

# Copy the finder related scripts and executables to the /home directory
cp writer ${OUTDIR}/rootfs/home
cp finder.sh ${OUTDIR}/rootfs/home
cp finder-test.sh ${OUTDIR}/rootfs/home
cp autorun-qemu.sh ${OUTDIR}/rootfs/home
mkdir -p ${OUTDIR}/rootfs/conf
cp conf/assignment.txt ${OUTDIR}/rootfs/conf
cp conf/username.txt ${OUTDIR}/rootfs/conf
cd ${OUTDIR}/rootfs/home
ln -s ../conf conf

# Chown the root directory
sudo chown root:root ${OUTDIR}/rootfs
echo 'root:x:0:' > ${OUTDIR}/rootfs/etc/group
echo 'root:x:0:0:root:/root:/bin/sh' > ${OUTDIR}/rootfs/etc/passwd

cd "${OUTDIR}/rootfs"
find . | cpio -H newc -ov --owner root:root > ${OUTDIR}/initramfs.cpio

# Create initramfs.cpio.gz
cd ..
gzip -f initramfs.cpio
