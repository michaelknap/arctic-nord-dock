# Makefile for Arctic Nord Dock

# Version
VERSION ?= 1.0.0

# Compiler and flags (default compiler is clang; can be overridden)
CC ?= clang
CFLAGS = -O3 -Wall -Wextra -pedantic -std=c23 `pkg-config --cflags x11` \
		  -fstack-protector-strong -D_FORTIFY_SOURCE=2 -fPIE
LDFLAGS = `pkg-config --libs x11` -lm -pie -Wl,-z,relro -Wl,-z,now

# Directories
SRC_DIR = src
BUILD_DIR = build

# Source files (all .c files inside SRC_DIR)
SRCS = $(wildcard $(SRC_DIR)/*.c)

# Object files (each .o file inside BUILD_DIR)
OBJS = $(patsubst $(SRC_DIR)/%.c,$(BUILD_DIR)/%.o,$(SRCS))

# Target executable (placed in BUILD_DIR)
TARGET = $(BUILD_DIR)/arctic-nord-dock

# Installation directories (PREFIX is configurable; DESTDIR supports staged installs)
PREFIX ?= /opt/arctic-nord-dock
DESTDIR ?=
BINDIR = $(DESTDIR)$(PREFIX)

# Desktop and icon file directories
DESKTOPDIR = $(DESTDIR)/usr/share/applications
ICONDIR = $(DESTDIR)/usr/share/icons/hicolor/scalable/apps

# Files to install
DESKTOP_FILE = $(SRC_DIR)/arctic-nord-dock.desktop.in
ICON_FILE = $(SRC_DIR)/icons/icon.svg

# Default target: build the executable
all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) -o $@ $(OBJS) $(LDFLAGS)

# Compile source files into object files inside BUILD_DIR
$(BUILD_DIR)/%.o: $(SRC_DIR)/%.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

# Ensure BUILD_DIR exists before compiling
$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

# Clean: remove the entire build directory.
clean:
	rm -rf $(BUILD_DIR)

# Strip: remove symbols from the binary.
strip: $(TARGET)
	strip --strip-all $(TARGET)

# Run: build and run the executable.
run: $(TARGET)
	./$(TARGET)

# Static analysis targets
check: cppcheck clangtidy clangcheck

cppcheck:
	cppcheck --suppress=missingIncludeSystem --enable=all --inconclusive --std=c23 --force $(SRC_DIR)

clangtidy:
	clang-tidy $(SRCS) --extra-arg=-std=c23 -checks='*,-clang-analyzer-*,-cppcoreguidelines-avoid-non-const-global-variables,-readability-identifier-length,-cppcoreguidelines-avoid-magic-numbers,-readability-magic-numbers,-llvmlibc-restrict-system-libc-headers,-bugprone-easily-swappable-parameters,modernize-use-nullptr,readability-implicit-bool-conversion'

clangcheck:
	scan-build --status-bugs $(MAKE) clean all

# Install target: install binary, desktop file, and icon.
install: all strip
	@echo "Installing binary to $(BINDIR)..."
	mkdir -p $(BINDIR)
	install -Dm755 $(TARGET) $(BINDIR)/arctic-nord-dock

	@echo "Processing and installing desktop file..."
	mkdir -p $(DESKTOPDIR)
	sed "s|@BINDIR@|$(PREFIX)|g" $(DESKTOP_FILE) > $(BUILD_DIR)/arctic-nord-dock.desktop
	install -Dm644 $(BUILD_DIR)/arctic-nord-dock.desktop $(DESKTOPDIR)/arctic-nord-dock.desktop

	@echo "Installing icon file to $(ICONDIR)..."
	mkdir -p $(ICONDIR)
	install -Dm644 $(ICON_FILE) $(ICONDIR)/arctic-nord-dock.svg

	@echo "Updating icon cache..."
	xdg-icon-resource forceupdate

# Uninstall target: remove installed files.
uninstall:
	@echo "Removing binary from $(BINDIR)..."
	rm -f $(BINDIR)/arctic-nord-dock
	@echo "Removing desktop file from $(DESKTOPDIR)..."
	rm -f $(DESKTOPDIR)/arctic-nord-dock.desktop
	@echo "Removing icon file from $(ICONDIR)..."
	rm -f $(ICONDIR)/arctic-nord-dock.svg
	@echo "Updating icon cache..."
	xdg-icon-resource forceupdate

# Package target for Debian:
# Stage installation into a temporary directory, create DEBIAN/control, then build a .deb package.
deb: clean
	@echo "Staging installation for .deb package..."
	# Create the staging directory structure.
	mkdir -p pkg/$(PREFIX)
	mkdir -p pkg/DEBIAN
	mkdir -p pkg/usr/share/applications
	mkdir -p pkg/usr/share/icons/hicolor/scalable/apps
	# Stage the installation.
	$(MAKE) install DESTDIR=$(CURDIR)/pkg
	@echo "Creating control file..."
	@echo "Package: arctic-nord-dock" > pkg/DEBIAN/control
	@echo "Version: $(VERSION)" >> pkg/DEBIAN/control
	@echo "Section: utility" >> pkg/DEBIAN/control
	@echo "Priority: optional" >> pkg/DEBIAN/control
	@echo "Architecture: $(shell dpkg --print-architecture)" >> pkg/DEBIAN/control
	@echo "Maintainer: Michael Knap" >> pkg/DEBIAN/control
	@echo "Description: Arctic Nord Dock is a lightweight dock for copying palette values in various formats." >> pkg/DEBIAN/control
	@echo "Building .deb package..."
	dpkg-deb --build pkg $(BUILD_DIR)/arctic-nord-dock-$(VERSION)-$(shell arch).deb
	@echo "Package created: $(BUILD_DIR)/arctic-nord-dock-$(VERSION)-$(shell arch).deb"
	@rm -rf pkg


# Source distribution tarball (for Arch or other packaging systems)
dist: clean
	@echo "Creating source distribution tarball..."
	mkdir -p $(BUILD_DIR)
	tar czf $(BUILD_DIR)/arctic-nord-dock-$(VERSION)-$(shell arch).tar.gz --exclude=$(BUILD_DIR) .
	@echo "Tarball created: $(BUILD_DIR)/arctic-nord-dock-$(VERSION)-$(shell arch).tar.gz"


.PHONY: all clean run strip install uninstall cppcheck clangcheck clangtidy check deb dist
