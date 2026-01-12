# ================================
# Configuration
# ================================
PREFIX := /usr/local
BUILD := $(CURDIR)/build

# Git repos
LIBUSB_REPO := https://github.com/libusb/libusb.git
FREENECT_REPO := https://github.com/OpenKinect/libfreenect.git
PIGPIO_REPO := https://github.com/joan2937/pigpio.git

# ================================
# Top-level targets (compile all .c files)
# ================================
PROGRAM := mab
SRC := main.c

CFLAGS := -Wall -Wextra -O2
LDFLAGS := -lfreenect -lusb-1.0 -ludev -lpigpio -lpthread -lm

program: $(SRC)
	$(CC) $(CFLAGS) $(SRC) -o $(PROGRAM) $(LDFLAGS)
# ================================
# Dependencies
# ================================
deps: sys_deps libusb freenect pigpio lib_headers distclean

cleanlib:
	rm -rf lib

clean:
	rm -rf $(BUILD)

distclean: clean
	rm -rf libusb libfreenect pigpio

# ================================
# Collect headers into one folder
# ================================
lib_headers: libfreenect/.git pigpio/.git
	mkdir -p lib

    # Pigpio headers
	cp -r pigpio/*.h lib/
    # libfreenect core headers
	cp -r libfreenect/src/*.h lib/
	cp -r libfreenect/include/*.h lib/
	cp -r libfreenect/wrappers/c_sync/*.h lib/
    # cp -r libfreenect/wrappers/cpp/*.h lib/


# ================================
# System dependencies
# ================================
sys_deps:
	sudo apt update
	sudo apt install -y libudev-dev cmake build-essential autoconf automake libtool pkg-config

# ================================
# libusb (shared)
# ================================
libusb: libusb/.git
	cd libusb && ./autogen.sh
	cd libusb && ./configure --prefix=$(PREFIX) --enable-shared --disable-static
	$(MAKE) -C libusb
	sudo $(MAKE) -C libusb install

libusb/.git:
	git clone $(LIBUSB_REPO) libusb

# ================================
# libfreenect (shared)
# ================================
freenect: libfreenect/.git libusb
	mkdir -p $(BUILD)/freenect
	cd $(BUILD)/freenect && cmake ../../libfreenect \
	    -DCMAKE_INSTALL_PREFIX=$(PREFIX) \
	    -DBUILD_SHARED_LIBS=ON \
	    -DBUILD_FREENECT_SHARED=ON \
	    -DBUILD_FREENECT_STATIC=OFF \
	    -DBUILD_FREENECT_SYNC=ON \
	    -DBUILD_FREENECT_CV=ON \
	    -DBUILD_FREENECT_AUDIO=ON \
	    -DBUILD_FREENECT_REG=ON \
	    -DWITH_UDEV=ON
	$(MAKE) -C $(BUILD)/freenect
	sudo $(MAKE) -C $(BUILD)/freenect install

libfreenect/.git:
	git clone $(FREENECT_REPO) libfreenect

# ================================
# pigpio (shared)
# ================================
pigpio: pigpio/.git
	mkdir -p $(BUILD)/pigpio
	cd $(BUILD)/pigpio && cmake ../../pigpio \
	    -DCMAKE_INSTALL_PREFIX=$(PREFIX) \
	    -DBUILD_SHARED_LIBS=ON
	$(MAKE) -C $(BUILD)/pigpio
	sudo $(MAKE) -C $(BUILD)/pigpio install

pigpio/.git:
	git clone $(PIGPIO_REPO) pigpio
