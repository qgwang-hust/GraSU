#
# this file was created by a computer. trust it.
#

# compiler tools
XILINX_VITIS ?= /tools/Xilinx/Vitis/2019.2
XILINX_XRT ?= /opt/xilinx/xrt
XILINX_VIVADO ?= /tools/Xilinx/Vivado/2019.2
XILINX_VIVADO_HLS ?= $(XILINX_VITIS)/Vivado_HLS

HOST_CXX ?= g++
VPP ?= ${XILINX_VITIS}/bin/v++
RM = rm -f
RMDIR = rm -rf

VITIS_PLATFORM = xilinx_u250_xdma_201830_2
VITIS_PLATFORM_PATH = $(VITIS_PLATFORM)

# host compiler global settings
CXXFLAGS += -std=c++0x -DVITIS_PLATFORM=$(VITIS_PLATFORM) -D__USE_XOPEN2K8 -I$(XILINX_XRT)/include/ -I$(XILINX_VIVADO)/include/ -O2 -Wall -c -fmessage-length=0
LDFLAGS += -lxilinxopencl -lpthread -lrt -lstdc++ -lmpfr -lgmp -lhlsmc++-GCC46 -lIp_floating_point_v7_0_bitacc_cmodel -lIp_xfft_v9_1_bitacc_cmodel -lIp_fir_compiler_v7_2_bitacc_cmodel -lIp_dds_compiler_v6_0_bitacc_cmodel -L$(XILINX_XRT)/lib/ -L$(XILINX_VIVADO)/lnx64/tools/fpo_v7_0 -L$(XILINX_VIVADO)/lnx64/lib/csim -L$(XILINX_VIVADO)/lnx64/tools/dds_v6_0 -L$(XILINX_VIVADO)/lnx64/tools/fir_v7_0 -L$(XILINX_VIVADO)/lnx64/tools/fft_v9_1 -Wl,-rpath-link,$(XILINX_XRT)/lib -Wl,-rpath,$(XILINX_VIVADO)/lnx64/lib/csim -Wl,-rpath,$(XILINX_VIVADO)/lnx64/tools/fpo_v7_0 -Wl,-rpath,$(XILINX_VIVADO)/lnx64/tools/fft_v9_1 -Wl,-rpath,$(XILINX_VIVADO)/lnx64/tools/fir_v7_0 -Wl,-rpath,$(XILINX_VIVADO)/lnx64/tools/dds_v6_0

# hardware compiler shared settings
VPP_OPTS = --target hw

#
# OpenCL kernel files
#

BINARY_CONTAINERS += binary_container_1.xclbin

BUILD_SUBDIRS += binary_container_1.build
BINARY_CONTAINER_1_OBJS += binary_container_1.build/bin_search.xo
ALL_KERNEL_OBJS += binary_container_1.build/bin_search.xo
BUILD_SUBDIRS += binary_container_1.build
BINARY_CONTAINER_1_OBJS += binary_container_1.build/dispatch.xo
ALL_KERNEL_OBJS += binary_container_1.build/dispatch.xo
BUILD_SUBDIRS += binary_container_1.build
BINARY_CONTAINER_1_OBJS += binary_container_1.build/process_leaf.xo
ALL_KERNEL_OBJS += binary_container_1.build/process_leaf.xo
BUILD_SUBDIRS += binary_container_1.build
BINARY_CONTAINER_1_OBJS += binary_container_1.build/read_edges.xo
ALL_KERNEL_OBJS += binary_container_1.build/read_edges.xo

ALL_MESSAGE_FILES = $(subst .xo,.mdb,$(ALL_KERNEL_OBJS)) $(subst .xclbin,.mdb,$(BINARY_CONTAINERS))

#
# host files
#

HOST_OBJECTS += src/host.o

HOST_EXE = pma

BUILD_SUBDIRS += src/

#
# primary build targets
#

.PHONY: all clean
all: $(BINARY_CONTAINERS) $(HOST_EXE)

.NOTPARALLEL: clean

clean-host:
	-$(RM) $(HOST_EXE) $(HOST_OBJECTS)

clean-accelerators:
	-$(RM) $(BINARY_CONTAINERS) $(ALL_KERNEL_OBJS) $(ALL_MESSAGE_FILES) 
	-$(RM) *.xclbin.sh *.xclbin.info *.xclbin.link_summary*
	-$(RMDIR) $(BUILD_SUBDIRS)
	-$(RMDIR) .Xil

clean: clean-host clean-accelerators

.PHONY: incremental
incremental: all


nothing:

#
# binary container: binary_container_1.xclbin
#

binary_container_1.build/bin_search.xo: ../src/kernel_binary_search.cpp ../src/kernel_config.h binary_container_1-bin_search-compile.ini common-config.ini
	-@mkdir -p $(@D)
	-@$(RM) $@
	$(VPP) $(VPP_OPTS) --compile -I"$(<D)" --config common-config.ini --config binary_container_1-bin_search-compile.ini -o"$@" "$<"

binary_container_1.build/dispatch.xo: ../src/kernel_dispatch.cpp ../src/kernel_config.h binary_container_1-dispatch-compile.ini common-config.ini
	-@mkdir -p $(@D)
	-@$(RM) $@
	$(VPP) $(VPP_OPTS) --compile -I"$(<D)" --config common-config.ini --config binary_container_1-dispatch-compile.ini -o"$@" "$<"

binary_container_1.build/process_leaf.xo: ../src/kernel_process_leaf.cpp ../src/kernel_config.h binary_container_1-process_leaf-compile.ini common-config.ini
	-@mkdir -p $(@D)
	-@$(RM) $@
	$(VPP) $(VPP_OPTS) --compile -I"$(<D)" --config common-config.ini --config binary_container_1-process_leaf-compile.ini -o"$@" "$<"

binary_container_1.build/read_edges.xo: ../src/kernel_read_edges.cpp ../src/kernel_config.h binary_container_1-read_edges-compile.ini common-config.ini
	-@mkdir -p $(@D)
	-@$(RM) $@
	$(VPP) $(VPP_OPTS) --compile -I"$(<D)" --config common-config.ini --config binary_container_1-read_edges-compile.ini -o"$@" "$<"

binary_container_1.xclbin: $(BINARY_CONTAINER_1_OBJS) binary_container_1-link.ini common-config.ini
	-@echo $(VPP) $(VPP_OPTS) --link --config ../src/stream_connect.ini --config common-config.ini --config binary_container_1-link.ini -o"$@" $(BINARY_CONTAINER_1_OBJS) > binary_container_1.xclbin.sh
	$(VPP) $(VPP_OPTS) --link --config ../src/stream_connect.ini --config common-config.ini --config binary_container_1-link.ini -o"$@" $(BINARY_CONTAINER_1_OBJS)

#
# host rules
#

src/host.o: ../src/host.cpp ../src/config.h ../src/pma_dynamic_graph.hpp
	-@mkdir -p $(@D)
	$(HOST_CXX) $(CXXFLAGS) -o "$@" "$<"

$(HOST_EXE): $(HOST_OBJECTS)
	$(HOST_CXX) -o "$@" $(+) $(LDFLAGS)

