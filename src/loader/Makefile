LOADER_SOURCES_CXX=$(shell find . -name "*.cpp" -printf "%P\n" -type f)
LOADER_OBJECTS=$(LOADER_SOURCES_S:%.s=$(BUILD_DIR)/loader/%.o) $(LOADER_SOURCES_CXX:%.cpp=$(BUILD_DIR)/loader/%.o)
LOADER_HEADERS=$(shell find . -name "*.hpp" -printf "%P\n" -type f)
LOADER_LIBSTDCXX_IMPORTS=-lstdc++ -lsupc++ -lc -lm -lg

COMMON_SOURCES_CXX=$(shell find ../common -name "*.cpp" -printf "../common/%P\n" -type f)
COMMON_OBJECTS=$(COMMON_SOURCES_CXX:%.cpp=$(BUILD_DIR)/common32/dummyfolder/%.o)

ALL_OBJECTS=$(LOADER_OBJECTS) $(COMMON_OBJECTS)

LOADER_CXXFLAGS=-ffreestanding \
			    -ggdb \
				-I. \
				-I../common \
				-Ithirdparty/include \
				-Wall \
				-Wextra \
				-Werror \
				-Wold-style-cast \
				-Wno-unused-variable \
				-Wno-unused-but-set-variable \
				-Wno-unused-parameter \
				-Wno-missing-field-initializers \
				-Wimplicit-fallthrough -Wno-error=switch \
				-msoft-float -mno-mmx -mno-sse -mno-sse2 \
				-std=c++1z

LOADER_ASFLAGS=--32

LOADER_LDFLAGS=-nostdlib \
			   -O0 \
			   -ggdb \
			   -Tloader.lds \
			   -ffreestanding

LD32=$(TOOLCHAIN32)/bin/$(TARGET32)-g++
AS32=$(TOOLCHAIN32)/bin/$(TARGET32)-as
CXX32=$(TOOLCHAIN32)/bin/$(TARGET32)-g++

LOADER_CRTBEGIN=$(shell $(CXX32) -print-file-name=crtbegin.o)
LOADER_CRTEND=$(shell $(CXX32) -print-file-name=crtend.o)
LOADER_LIBGCC=$(shell $(CXX32) --print-libgcc-file-name)

DIRS=$(shell dirname $(ALL_OBJECTS)) $(BUILD_DIR)/loader/boot

$(BUILD_DIR)/loader.elf: mkdirs $(BUILD_DIR)/loader/boot/multiboot.o $(BUILD_DIR)/loader/crti.o $(BUILD_DIR)/loader/crtn.o $(ALL_OBJECTS)
	$(LD32) -o $@ $(LOADER_LDFLAGS) \
	    $(BUILD_DIR)/loader/boot/multiboot.o \
		$(BUILD_DIR)/loader/crti.o \
		$(LOADER_CRTBEGIN) \
		$(ALL_OBJECTS) \
		$(LOADER_LIBSTDCXX_IMPORTS) \
		$(LOADER_LIBGCC) \
		$(LOADER_CRTEND) \
		$(BUILD_DIR)/loader/crtn.o

$(BUILD_DIR)/loader/%.o: %.s
	$(AS32) -o $@ $(LOADER_ASFLAGS) $<

$(BUILD_DIR)/loader/%.o: %.cpp $(LOADER_HEADERS)
	$(CXX32) -c -o $@ $(LOADER_CXXFLAGS) $<

$(BUILD_DIR)/common32/dummyfolder/%.o: %.cpp $(LOADER_HEADERS)
	$(CXX32) -c -o $@ $(LOADER_CXXFLAGS) $<


mkdirs:
	@mkdir -p $(DIRS)
