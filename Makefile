# ================================
# Configuration
# ================================
PREFIX := /usr/local
BUILD := ./build

# Git repos
LIBUSB_REPO := https://github.com/libusb/libusb.git
FREENECT_REPO := https://github.com/OpenKinect/libfreenect.git

# ================================
# Top-level targets (compile all .c files)
# ================================
PROGRAM := mab
SRC := $(wildcard src/*.c)
HEADERS_DIR := inc

CFLAGS := -Wall -Wextra -O2 -I$(HEADERS_DIR)
LDFLAGS := -lfreenect -lusb-1.0 -ludev -lpthread

program: $(SRC)
	$(CC) $(CFLAGS) $(SRC) -o $(PROGRAM) $(LDFLAGS)

run:
	./$(PROGRAM)

on_pi:
	git restore .
	git pull
	make program
	clear
	git log -1 --pretty=format:"commit %h:%s"
	echo "Running program..."
	make run

# ================================
# Dependencies
# ================================
deps: sys_deps libusb freenect lib_headers distclean

cleanlib:
	rm -rf lib

clean:
	rm -rf $(BUILD)

distclean: clean
	sudo rm -rf libusb libfreenect

# ================================
# Collect headers into one folder
# ================================
lib_headers: libfreenect/.git
	mkdir -p lib

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
