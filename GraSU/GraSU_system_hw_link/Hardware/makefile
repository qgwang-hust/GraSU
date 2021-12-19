#
# this file was created by a computer. trust it.
#

# compiler tools
XILINX_VITIS ?= /tools/Xilinx/Vitis/2020.2
XILINX_XRT ?= /opt/xilinx/xrt
XILINX_VIVADO ?= /tools/Xilinx/Vivado/2020.2
XILINX_VIVADO_HLS ?= $(XILINX_VITIS)/Vivado_HLS

VPP_LINKER ?= ${XILINX_VITIS}/bin/v++
RM = rm -f
RMDIR = rm -rf

VITIS_PLATFORM = xilinx_u250_gen3x16_xdma_3_1_202020_1
VITIS_PLATFORM_PATH = $(VITIS_PLATFORM)

VPP_OPTS = --target hw

#
# Hw kernel files
#

BINARY_CONTAINERS += GraSU.xclbin

BUILD_SUBDIRS += GraSU.build
GRASU_OBJS += ../../GraSU_kernels/Hardware/build/bin_search.xo
GRASU_OBJS += ../../GraSU_kernels/Hardware/build/dispatch.xo
GRASU_OBJS += ../../GraSU_kernels/Hardware/build/process_cache.xo
GRASU_OBJS += ../../GraSU_kernels/Hardware/build/process_ddr.xo

ALL_MESSAGE_FILES = $(subst .xclbin,.mdb,$(BINARY_CONTAINERS))

#
# primary build targets
#

.PHONY: all clean
all: $(BINARY_CONTAINERS)


clean:
	-$(RM) $(BINARY_CONTAINERS) $(ALL_MESSAGE_FILES) 
	-$(RM) *.xclbin.sh *.xclbin.info *.xclbin.link_summary*
	-$(RMDIR) $(BUILD_SUBDIRS)
	-$(RMDIR) .Xil

ifeq ($(MAKECMDGOALS),incremental)
.PHONY: GraSU.xclbin
.PHONY: ../../GraSU_kernels/Hardware/build/bin_search.xo
.PHONY: ../../GraSU_kernels/Hardware/build/dispatch.xo
.PHONY: ../../GraSU_kernels/Hardware/build/process_cache.xo
.PHONY: ../../GraSU_kernels/Hardware/build/process_ddr.xo
endif

.PHONY: incremental
incremental: all


nothing:

#
# binary container: GraSU.xclbin
#

GraSU.xclbin: $(GRASU_OBJS) GraSU-link.cfg stream_connect.ini
	-@echo $(VPP_LINKER) $(VPP_OPTS) --link --config stream_connect.ini --kernel_frequency 200 --config GraSU-link.cfg -o"$@" $(GRASU_OBJS) > GraSU.xclbin.sh
	$(VPP_LINKER) $(VPP_OPTS) --link --config stream_connect.ini --kernel_frequency 200 --config GraSU-link.cfg -o"$@" $(GRASU_OBJS)

