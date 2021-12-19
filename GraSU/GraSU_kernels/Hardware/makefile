#
# this file was created by a computer. trust it.
#

# compiler tools
XILINX_VITIS ?= /tools/Xilinx/Vitis/2020.2
XILINX_XRT ?= /opt/xilinx/xrt
XILINX_VIVADO ?= /tools/Xilinx/Vivado/2020.2
XILINX_VIVADO_HLS ?= $(XILINX_VITIS)/Vivado_HLS

VPP ?= ${XILINX_VITIS}/bin/v++
RM = rm -f
RMDIR = rm -rf

VITIS_PLATFORM = xilinx_u250_gen3x16_xdma_3_1_202020_1
VITIS_PLATFORM_PATH = $(VITIS_PLATFORM)

TARGET = hw
VPP_OPTS = --target $(TARGET)

#
# Hw kernel files
#


BUILD_SUBDIRS += build
HW_KERNEL_OBJS += build/bin_search.xo
SRC_KERNEL_OBJS += build/bin_search.xo
BUILD_SUBDIRS += build
HW_KERNEL_OBJS += build/dispatch.xo
SRC_KERNEL_OBJS += build/dispatch.xo
BUILD_SUBDIRS += build
HW_KERNEL_OBJS += build/process_cache.xo
SRC_KERNEL_OBJS += build/process_cache.xo
BUILD_SUBDIRS += build
HW_KERNEL_OBJS += build/process_ddr.xo
SRC_KERNEL_OBJS += build/process_ddr.xo

ALL_MESSAGE_FILES = $(subst .xo,.o,.mdb,$(SRC_KERNEL_OBJS))

#
# primary build targets
#

.PHONY: all clean
all: $(HW_KERNEL_OBJS)


clean:
	-$(RM) $(SRC_KERNEL_OBJS) $(ALL_MESSAGE_FILES) 
	-$(RMDIR) $(BUILD_SUBDIRS)
	-$(RMDIR) .Xil

.PHONY: incremental
incremental: all


nothing:

#
# Hw Kernels compile;
#

build/bin_search.xo: ../src/kernel_bin_search.cpp ../src/kernel_config.h bin_search-compile.cfg
	-@mkdir -p $(@D)
	-@$(RM) $@
	$(VPP) $(VPP_OPTS) --compile -I"$(<D)" --config bin_search-compile.cfg -o"$@" "$<"

build/dispatch.xo: ../src/kernel_dispatch.cpp ../src/kernel_config.h dispatch-compile.cfg
	-@mkdir -p $(@D)
	-@$(RM) $@
	$(VPP) $(VPP_OPTS) --compile -I"$(<D)" --config dispatch-compile.cfg -o"$@" "$<"

build/process_cache.xo: ../src/kernel_process_cache.cpp ../src/kernel_config.h process_cache-compile.cfg
	-@mkdir -p $(@D)
	-@$(RM) $@
	$(VPP) $(VPP_OPTS) --compile -I"$(<D)" --config process_cache-compile.cfg -o"$@" "$<"

build/process_ddr.xo: ../src/kernel_process_ddr.cpp ../src/kernel_config.h process_ddr-compile.cfg
	-@mkdir -p $(@D)
	-@$(RM) $@
	$(VPP) $(VPP_OPTS) --compile -I"$(<D)" --config process_ddr-compile.cfg -o"$@" "$<"

