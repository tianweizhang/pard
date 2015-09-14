CPU_TYPE=atomic
DEBUG_FLAGS='DummyFlag'	 #DEBUG_FLAGS='IdeBW'
OUT_DIR=m5out
GEM5_EXTRAS=
GEM5_OPTS=$(GEM5_EXTRAS) \
          --debug-flags=$(DEBUG_FLAGS) \
          --outdir=$(OUT_DIR) \
          --carbon-regex="^[A-Za-z_]*\.\(\(switch_\)*cpu[s]*[0-9]*\.ipc\)*\(l2\.\([A-Za-z_]*\.\)*guestCapacity\)*\(l2\.overall_miss_rate\)*\(mem_ctrls[0-9]*\.\([A-Za-z_]*\.\)*bw_total\)*$$"

PARD_CONFIG=configs/PARDg5-V.py $(EXTRAS) \
	    --cpu-type=$(CPU_TYPE)	\
	    --num-cpus=4		\
	    --num-dirs=1		\
	    --mem-size=11GB		\
	    --mem-type=ddr3_1600_x64	\
	    --kernel=$(KERNEL)

KERNEL_22=/home/majiuyue/pardg5/utils/linux-2.6.22/vmlinux
KERNEL_28=/home/majiuyue/pardg5/utils/linux-2.6.28.4/vmlinux
ORIG_K28=$(PARDG5_HOME)/system/binaries/x86_64-vmlinux-2.6.28.4
ORIG_K22=$(PARDG5_HOME)/system/binaries/x86_64-vmlinux-2.6.22.9.smp
ORIG_K28_SMP=$(PARDG5_HOME)/system/binaries/x86_64-vmlinux-2.6.28.4.smp
KERNEL=$(ORIG_K28)

run-help:
	@echo
	@echo "PARDg5-V Run Script"
	@echo "====================================================="
	@echo "single         -   1-guest: Fresh run system"
	@echo "single-restore -   1-guest: Restore from 1-st checkpoint"
	@echo "single-debug   -   1-guest: Fresh run system with gdb"
	@echo "dual           -   2-guest: Fresh run system"
	@echo "dual-restore   -   2-guest: Restore from 1-st checkpoint"
	@echo "dual-debug     -   2-guest: Fresh run system with gdb"
	@echo "quad           -   4-guest: Fresh run system"
	@echo "quad-restore   -   4-guest: Restore from 1-st checkpoint"
	@echo "quad-debug     -   4-guest: Fresh run system with gdb"
	@echo "----------------------------------------------------"
	@echo "Parameter:"
	@echo "  CPU_TYPE: timing, atomic, detailed"
	@echo "  DEBUG_FLAGS: see gem5 debug flags"
	@echo "  EXTRAS: extra configure options"
	@echo

run:
	@$(OPT_BIN) $(GEM5_OPTS) $(PARD_CONFIG) --num-guests=0

fast:
	@$(FAST_BIN) $(PARD_CONFIG) --num-guests=0

debug:
	@gdb --args $(DEBUG_BIN) $(GEM5_OPTS) $(PARD_CONFIG) --num-guests=0


restore: quad-restore

single:
	@$(OPT_BIN) $(GEM5_OPTS) $(PARD_CONFIG) --num-guests=1
single-restore:
	@$(OPT_BIN) $(GEM5_OPTS) $(PARD_CONFIG) --num-guests=1 --checkpoint-restore=1
single-debug:
	@gdb --args $(DEBUG_BIN) $(GEM5_OPTS) $(PARD_CONFIG) --num-guests=1
dual:
	@$(OPT_BIN) $(GEM5_OPTS) $(PARD_CONFIG) --num-guests=2
dual-restore:
	@$(OPT_BIN) $(GEM5_OPTS) $(PARD_CONFIG) --num-guests=2 --checkpoint-restore=1
dual-debug:
	@gdb --args $(DEBUG_BIN) $(GEM5_OPTS) $(PARD_CONFIG) --num-guests=2
quad:
	@$(OPT_BIN) $(GEM5_OPTS) $(PARD_CONFIG) --num-guests=4
quad-restore:
	@$(OPT_BIN) $(GEM5_OPTS) $(PARD_CONFIG) --num-guests=4 --checkpoint-restore=1
quad-debug:
	@gdb --args $(DEBUG_BIN) $(GEM5_OPTS) $(PARD_CONFIG) --num-guests=4

gm:
	@$(OPT_BIN) $(GEM5_OPT) configs/PARDg5-GM.py

MAIN_CONFIG=configs/PARDg5-Main.py $(EXTRAS) \
	    --cpu-type=$(CPU_TYPE)	\
	    --num-cpus=4		\
	    --num-dirs=1		\
	    --mem-size=3GB		\
	    --mem-type=ddr3_1600_x64	\
	    --kernel=$(KERNEL)
main:
	@$(OPT_BIN) $(GEM5_OPT) $(MAIN_CONFIG) --num-guests=4

gdb:
	gdb $(KERNEL)

cpt:
	@echo "[CHKCPT] Checking required directory ..."
	@if [ -e .tmpcpt ]; then rm -rf .tmpcpt; fi
	@if [ ! -d m5out ];  then mkdir m5out;  fi
	@if [ ! -d m5spec ]; then mkdir m5spec; fi
	@mkdir .tmpcpt
	@echo "[CHKCPT] Generating checkpoint with hack_back_ckpt.rcS ..."
	@echo "[CHKCPT] Starting gem5 simulator, log file at ".tmpcpt/gem5.log" ..."
	@M5_PATH=$(M5_PATH) $(OPT_BIN) --outdir=.tmpcpt $(PARDFS_CONFIG) --cpu-type=atomic --num-cpus=2 --mem-size=2048MB --kernel=$(KERNEL) --script=configs/scripts/hack_back_ckpt.rcS > .tmpcpt/gem5.log 2>&1
	@if [ ! -d .tmpcpt/cpt* ];\
	then\
	    echo "";\
	    echo "[CHKCPT] Clearup temporary files ...";\
	    rm -rf .tmpcpt;\
	    echo "[CHKCPT] No checkpoint generated, exit.";\
	    exit;\
	fi
	@echo "[CHKCPT] Hack checkpoint, add PARD section ..."
	@echo "[system.ruby.dir_cntrl0.memBuffer.pard]" >> `find .tmpcpt/ | grep "/m5.cpt"`
	@echo "[CHKCPT] Copy checkpoint files to m5spec/ ..."
	@rm -rf m5spec/cpt* m5out/cpt*
	@cp -r .tmpcpt/cpt* m5spec/
	@echo "[CHKCPT] Copy checkpoint files to m5out/ ..."
	@cp -r .tmpcpt/cpt* m5out/
	@echo "[CHKCPT] Clearup temporary files ..."
	@rm -rf .tmpcpt
	@echo "[CHKCPT] Done."


socat:
	sudo socat TCP:localhost:3500 TUN:192.168.0.100/24,up,tun-type=tap
