# pard
1. Download pard, and copy system folder to pard/pardg5-v

2. Install dependency for gem5:
    sudo apt-get install -y g++ scons swig zlib1g-dev m4 python-dev libreadline-dev 
  
3. Compiling gem5-pard:
    cd pard/gem5-pard
    scons build/X86/gem5.opt build/X86/gem5.debug
  
4. Compiling pardg5-v
    cd pard/pardg5-v
    make all
  
5. Run gem5:
    make run EXTRAS="--caches --l2cache --l2_assoc=16 --l2_size=2MB" CPU_TYPE=atomic
  
6. Testing PRM:
    m5term localhost 3456 or
    telnet localhost 3456
  
7. Install cp module:
    insmod /lib/modules/2.6.28.4-gc395911-dirty/cpa.ko
  
8. Check cp value:
    cat sys/cpa/cpa[0-4]/ident
  
9. Run LDOM:
    utils/command.sh create
    utils/command.sh [startup|shutdown] [LDomID]
    utils/command.sh query [CPName] [LDomID]
        CPName: l2.tags mem_ctrls[0|1] membus membus.mapper bridge.addr_mapper
    utils/command.sh adjust [CPName] [LDomID] [value]
