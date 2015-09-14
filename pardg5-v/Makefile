PARDG5_HOME=/opt/stack/pard/pardg5-v
GEM5_SOURCE=/opt/stack/pard/gem5-pard

export M5_PATH=$(PARDG5_HOME)/system/
#TARGET_ARCH=X86_MESI_Two_Level
TARGET_ARCH=PARDX86
BUILD_PARAMS=--file=$(GEM5_SOURCE)/SConstruct EXTRAS=$(HYPER_EXTRAS) -j 16

OPT_BIN=build/$(TARGET_ARCH)/gem5.opt
DEBUG_BIN=build/$(TARGET_ARCH)/gem5.debug
FAST_BIN=build/$(TARGET_ARCH)/gem5.fast
PERF_BIN=build/$(TARGET_ARCH)/gem5.perf
ALL_BIN=$(OPT_BIN) $(DEBUG_BIN) $(FAST_BIN)
HYPER_EXTRAS=hyper
REQUIRED_GEM5_SOURCE=src tests ext

all: build-opt build-debug

build-help:
	@echo
	@echo "PARDg5-V Build System"
	@echo "====================================================="
	@echo "build-opt   -   Build optimizated system"
	@echo "build-debug -   Build debug system"
	@echo "build-fast  -   Build fast system"
	@echo "build-all   -   Build whole system (all above)"
	@echo

build-all:
	@echo "***************************************************************************"
	@echo "*              Building gem5.opt/gem5.debug/gem5.fast                     *"
	@echo "***************************************************************************"
	@for file in $(REQUIRED_GEM5_SOURCE); do ln -s -f $(GEM5_SOURCE)/$$file .; done
	@scons $(BUILD_PARAMS) $(ALL_BIN)
	@for file in $(REQUIRED_GEM5_SOURCE); do rm -f $$file; done
	@echo "Finish building PARDg5-V simulator."

build-opt:
	@echo "***************************************************************************"
	@echo "*                         Building gem5.opt                               *"
	@echo "***************************************************************************"
	@for file in $(REQUIRED_GEM5_SOURCE); do ln -s -f $(GEM5_SOURCE)/$$file .; done
	@scons $(BUILD_PARAMS) $(OPT_BIN)
	@for file in $(REQUIRED_GEM5_SOURCE); do rm -f $$file; done
	@echo "Finish building PARDg5-V simulator."

build-debug:
	@echo "***************************************************************************"
	@echo "*                        Building gem5.debug                              *"
	@echo "***************************************************************************"
	@for file in $(REQUIRED_GEM5_SOURCE); do ln -s -f $(GEM5_SOURCE)/$$file .; done
	@scons $(BUILD_PARAMS) $(DEBUG_BIN)
	@for file in $(REQUIRED_GEM5_SOURCE); do rm -f $$file; done
	@echo "Finish building PARDg5-V simulator."

build-fast:
	@echo "***************************************************************************"
	@echo "*                        Building gem5.fast                               *"
	@echo "***************************************************************************"
	@for file in $(REQUIRED_GEM5_SOURCE); do ln -s -f $(GEM5_SOURCE)/$$file .; done
	@scons $(BUILD_PARAMS) $(FAST_BIN) --no-lto
	@for file in $(REQUIRED_GEM5_SOURCE); do rm -f $$file; done
	@echo "Finish building PARDg5-V simulator."

build-perf:
	@echo "***************************************************************************"
	@echo "*                Building gem5.perf (Google PerfTool support)             *"
	@echo "***************************************************************************"
	@for file in $(REQUIRED_GEM5_SOURCE); do ln -s -f $(GEM5_SOURCE)/$$file .; done
	@scons $(BUILD_PARAMS) $(PERF_BIN)
	@for file in $(REQUIRED_GEM5_SOURCE); do rm -f $$file; done
	@echo "Finish building PARDg5-V simulator."

build-orig:
	@echo "***************************************************************************"
	@echo "*                     Building Original gem5.opt                          *"
	@echo "***************************************************************************"
	@for file in $(REQUIRED_GEM5_SOURCE); do ln -s -f $(GEM5_SOURCE)/$$file .; done
	@scons --file=$(GEM5_SOURCE)/SConstruct -j 16 build/X86_MESI_Two_Level/gem5.opt
	@for file in $(REQUIRED_GEM5_SOURCE); do rm -f $$file; done
	@echo "Finish building PARDg5-V simulator."


clean:
	@for file in $(REQUIRED_GEM5_SOURCE); do ln -s -f $(GEM5_SOURCE)/$$file .; done
	#@scons $(BUILD_PARAMS) $(ALL_BIN) -c -q
	@for file in $(REQUIRED_GEM5_SOURCE); do rm -f $$file; done
	@echo "Finish clean."

build-secure:
	@if [ ! -f ./system/disks/x86root_secure.img ];			\
	then								\
		echo "Generate Secure Disk Image...";			\
		g++ -o SecureDiskImage ./utils/SecureDiskImage.cpp;	\
		./SecureDiskImage;					\
		rm ./SecureDiskImage;					\
	fi

# Include all run configs
export OPT_BIN
export DEBUG_BIN
export FAST_BIN
include configs/RunConfig.mk

