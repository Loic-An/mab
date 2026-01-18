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
SRC := $(wildcard src/*.cpp)
HEADERS_DIR := inc

CFLAGS := -Wall -Wextra -O2 -I$(HEADERS_DIR)
LDFLAGS := -lfreenect -lfreenect_sync -lusb-1.0 -ludev -lpthread

program: $(SRC)
	$(CXX) $(CFLAGS) $(SRC) -o $(PROGRAM) $(LDFLAGS)

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
deps: sys_deps libusb freenect clean

clean:
	rm -rf $(BUILD)
	rm -f $(PROGRAM)

# ================================
# System dependencies
# ================================
sys_deps:
	sudo apt update
	sudo apt install -y libudev-dev cmake build-essential autoconf automake libtool pkg-config

# ================================
# libusb (shared)
# ================================
libusb: $(BUILD)/libusb/.git
	cd $(BUILD)/libusb && ./autogen.sh
	cd $(BUILD)/libusb && ./configure --prefix=$(PREFIX) --enable-shared --disable-static
	$(MAKE) -C $(BUILD)/libusb
	sudo $(MAKE) -C $(BUILD)/libusb install

$(BUILD)/libusb/.git:
	mkdir -p $(BUILD)
	git clone $(LIBUSB_REPO) $(BUILD)/libusb

# ================================
# libfreenect (shared)
# ================================
freenect: $(BUILD)/freenect/.git libusb
	mkdir -p $(BUILD)/freenect
	cd $(BUILD)/freenect && cmake \
	    -DCMAKE_INSTALL_PREFIX=$(PREFIX) \
	    -DBUILD_SHARED_LIBS=ON \
	    -DBUILD_FREENECT_SHARED=ON \
	    -DBUILD_FREENECT_STATIC=OFF \
	    -DBUILD_FREENECT_SYNC=ON \
	    -DBUILD_FREENECT_CV=ON \
	    -DBUILD_FREENECT_AUDIO=ON \
	    -DBUILD_FREENECT_REG=ON \
		-DBUILD_REDIST_PACKAGE=OFF \
	    -DWITH_UDEV=ON
	$(MAKE) -C $(BUILD)/freenect
	sudo $(MAKE) -C $(BUILD)/freenect install

$(BUILD)/freenect/.git:
	mkdir -p $(BUILD)
	git clone $(FREENECT_REPO) $(BUILD)/freenect