# Copyright (c) 2014 ACS, ICT.
# All rights reserved.
#
# Author: Jiuyue Ma

import optparse
import sys

import m5
from m5.defines import buildEnv
from m5.objects import *
from m5.util import addToPath, fatal

addToPath('../../gem5-latest/configs/common')
addToPath('common')

from FSConfig import *
from SysPaths import *
from Benchmarks import *
import Simulation
import CacheConfig
import MemConfig
from Caches import *
import Options

TestCPUClass = None
test_mem_mode = None
FutureClass = None

def addPARDg5VOptions(parser):
    parser.add_option("--num-guests", action="store", type="int", default=1,
        help="the number of guest simulate")

def build_system(options):
    # system under test can be any CPU
    (TestCPUClass, test_mem_mode, FutureClass) = Simulation.setCPUClass(options)
    # Match the memories with the CPUs, based on the options for the test system
    TestMemClass = Simulation.setMemClass(options)
    bm = [SysConfig(disk=options.disk_image, mem=options.mem_size)]
    np = options.num_cpus
    
    if np<2:
        print "Error: >=2 cores is required"
        sys.exit(1)

    return (build_main_system(np, test_mem_mode, options, bm, TestCPUClass), FutureClass)

def build_main_system(np, test_mem_mode, options, bm, TestCPUClass):
    self = makeLinuxX86System(test_mem_mode, options.num_cpus, bm[0],
            options.ruby, numGuests = options.num_guests)

    # Set the cache line size for the entire system
    self.cache_line_size = options.cacheline_size

    # Create a top-level voltage domain
    self.voltage_domain = VoltageDomain(voltage = options.sys_voltage)

    # Create a source clock for the system and set the clock period
    self.clk_domain = SrcClockDomain(clock =  options.sys_clock,
            voltage_domain = self.voltage_domain)

    # Create a CPU voltage domain
    self.cpu_voltage_domain = VoltageDomain()

    # Create a source clock for the CPUs and set the clock period
    self.cpu_clk_domain = SrcClockDomain(clock = options.cpu_clock,
                                             voltage_domain =
                                             self.cpu_voltage_domain)

    if options.kernel is not None and options.num_guests>0:
        for guest in self.guests:
            guest.kernel = binary(options.kernel)
    #self.guests[0].kernel = binary('/home/majiuyue/pardg5/utils/linux-2.6.22/vmlinux')

    if options.script is not None:
        #for guest in self.guests:
        #    guest.readfile = options.script
        self.readfile = options.script

    if options.lpae:
        self.have_lpae = True

    if options.virtualisation:
        self.have_virtualization = True

    self.init_param = options.init_param

    # For now, assign all the CPUs to the same clock domain
    self.cpu = [TestCPUClass(clk_domain=self.cpu_clk_domain, cpu_id=i)
                    for i in xrange(np)]

    if options.caches or options.l2cache:
        # By default the IOCache runs at the system clock
        self.iocache = IOCache(addr_ranges = self.mem_ranges)
        self.iocache.cpu_side = self.iobus.master
        self.iocache.mem_side = self.membus.slave
    else:
        self.iobridge = Bridge(delay='50ns', ranges = self.mem_ranges)
        self.iobridge.slave = self.iobus.master
        self.iobridge.master = self.membus.slave
    #self.iobridge = Bridge(delay='50ns', ranges = self.mem_ranges)
    #self.iobridge.slave = self.iobus.master
    #self.iobridge.master = self.membus.slave

    # Sanity check
    if options.fastmem:
        if TestCPUClass != AtomicSimpleCPU:
            fatal("Fastmem can only be used with atomic CPU!")
        if (options.caches or options.l2cache):
            fatal("You cannot use fastmem in combination with caches!")

    for i in xrange(np):
        if options.fastmem:
            self.cpu[i].fastmem = True
        if options.checker:
            self.cpu[i].addCheckerCpu()
        self.cpu[i].createThreads()

    CacheConfig.config_cache(options, self)
    MemConfig.config_mem(options, self)

    # PARDg5V: L1Cache config
    if options.caches:
        for id in range(0, np):
            self.cpu[id].icache.guest_id = id
            self.cpu[id].dcache.guest_id = id
            self.cpu[id].itb_walker_cache.guest_id = id
            self.cpu[id].dtb_walker_cache.guest_id = id
            self.cpu[id].icache.tags.waymasks = [0xFF, 0xFF, 0xFF, 0xFF]
            self.cpu[id].dcache.tags.waymasks = [0xFF, 0xFF, 0xFF, 0xFF]
            self.cpu[id].itb_walker_cache.tags.waymasks = [0xFF, 0xFF, 0xFF, 0xFF]
            self.cpu[id].dtb_walker_cache.tags.waymasks = [0xFF, 0xFF, 0xFF, 0xFF]
        self.iocache.tags.waymasks = [0xFFFFFFFFFFFFFFFF, 0xFFFFFFFFFFFFFFFF, 0xFFFFFFFFFFFFFFFF, 0xFFFFFFFFFFFFFFFF]

    if options.l2cache:
        # although L2 is a 8-way caches, we make enough mask for 16-way
        self.l2.tags.waymasks = [0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF]

    # PARDg5V: set memctrl address remapping
    self.membus.mapper.original_ranges = \
        [
        GuestAddrRange("2048MB", guestID=0),
        GuestAddrRange("2048MB", guestID=1),
        GuestAddrRange("2048MB", guestID=2),
        GuestAddrRange("2048MB", guestID=3),
        ] 
    self.membus.mapper.remapped_ranges = \
        [
        AddrRange( "4096MB", size="2048MB"),
        AddrRange( "6144MB", size="2048MB"),
        AddrRange( "8192MB", size="2048MB"),
        AddrRange("10240MB", size="2048MB"),
        ] 

    # PARDg5V: mask INTR
    self.p5v.south_bridge.io_apic.pin_mask = [0xFFFFFF, 0xFFFFFF, 0xFFFFFF, 0xFFFFFF]

    # PARDg5V: set north bridge address remapping
    self.bridge.addr_mapper.original_ranges = \
        [
        # Guest@0:
        #    Guest, COM_1 => Host, COM_1
        GuestAddrRange(x86IOAddress(0x3f8), size=8, guestID=0),
        GuestAddrRange(x86IOAddress(0x2f8), size=8, guestID=0),
        GuestAddrRange(x86IOAddress(0x3e8), size=8, guestID=0),
        GuestAddrRange(x86IOAddress(0x2e8), size=8, guestID=0),
        #    Guest, I8042(keyboard) => <NONE-EXIST>
        GuestAddrRange(x86IOAddress(0x60), size=8, guestID=0),
        #    Guest, ATA_2 => <NONE-EXIST>
        GuestAddrRange(x86IOAddress(0x170), size=8, guestID=0),
        GuestAddrRange(x86IOAddress(0x374), size=8, guestID=0),
        GuestAddrRange(x86IOAddress(0x1008), size=8, guestID=0),
        
        # Guest@1:
        #    Guest, COM_1 => Host, COM_2
        GuestAddrRange(x86IOAddress(0x3f8), size=8, guestID=1),
        GuestAddrRange(x86IOAddress(0x2f8), size=8, guestID=1),
        GuestAddrRange(x86IOAddress(0x3e8), size=8, guestID=1),
        GuestAddrRange(x86IOAddress(0x2e8), size=8, guestID=1),
        #    Guest, I8042(keyboard) => <NONE-EXIST>
        GuestAddrRange(x86IOAddress(0x60), size=8, guestID=1),
        #    Guest, ATA_2 => <NONE-EXIST>
        GuestAddrRange(x86IOAddress(0x170), size=8, guestID=1),
        GuestAddrRange(x86IOAddress(0x374), size=8, guestID=1),
        GuestAddrRange(x86IOAddress(0x1008), size=8, guestID=1),

        # Guest@2:
        #    Guest, COM_1 => Host, COM_4
        GuestAddrRange(x86IOAddress(0x3f8), size=8, guestID=2),
        GuestAddrRange(x86IOAddress(0x2f8), size=8, guestID=2),
        GuestAddrRange(x86IOAddress(0x3e8), size=8, guestID=2),
        GuestAddrRange(x86IOAddress(0x2e8), size=8, guestID=2),
        #    Guest, I8042(keyboard) => <NONE-EXIST>
        GuestAddrRange(x86IOAddress(0x60), size=8, guestID=2),
        #    Guest, ATA_2 => <NONE-EXIST>
        GuestAddrRange(x86IOAddress(0x170), size=8, guestID=2),
        GuestAddrRange(x86IOAddress(0x374), size=8, guestID=2),
        GuestAddrRange(x86IOAddress(0x1008), size=8, guestID=2),
        
        # Guest@3:
        #    Guest, COM_1 => Host, COM_4; COM_2-4 => <NONE-EXIST>
        GuestAddrRange(x86IOAddress(0x3f8), size=8, guestID=3),
        GuestAddrRange(x86IOAddress(0x2f8), size=8, guestID=3),
        GuestAddrRange(x86IOAddress(0x3e8), size=8, guestID=3),
        GuestAddrRange(x86IOAddress(0x2e8), size=8, guestID=3),
        #    Guest, I8042(keyboard) => <NONE-EXIST>
        GuestAddrRange(x86IOAddress(0x60), size=8, guestID=3),
        #    Guest, ATA_2 => <NONE-EXIST>
        GuestAddrRange(x86IOAddress(0x170), size=8, guestID=3),
        GuestAddrRange(x86IOAddress(0x374), size=8, guestID=3),
        GuestAddrRange(x86IOAddress(0x1008), size=8, guestID=3),
        ]
    NONE_EXIST = AddrRange(x86IOAddress(0xcf8), size=8)
    self.bridge.addr_mapper.remapped_ranges = \
        [
        # Guest@0
        #    Guest, COM_1 => Host, COM_1
        AddrRange(x86IOAddress(0x3f8), size=8),
        NONE_EXIST, NONE_EXIST, NONE_EXIST,
        #    Guest, I8042(keyboard) => <NONE-EXIST>
        NONE_EXIST,
        #    Guest, ATA_2 => <NONE-EXIST>
        NONE_EXIST, NONE_EXIST, NONE_EXIST,

        # Guest@1
        #    Guest, COM_1 => Host, COM_2
        AddrRange(x86IOAddress(0x2f8), size=8),
        NONE_EXIST, NONE_EXIST, NONE_EXIST,
        #    Guest, I8042(keyboard) => <NONE-EXIST>
        NONE_EXIST,
        #    Guest, ATA_2 => <NONE-EXIST>
        NONE_EXIST, NONE_EXIST, NONE_EXIST,

        # Guest@2
        #    Guest, COM_1 => Host, COM_3
        AddrRange(x86IOAddress(0x3e8), size=8),
        NONE_EXIST, NONE_EXIST, NONE_EXIST,
        #    Guest, I8042(keyboard) => <NONE-EXIST>
        NONE_EXIST,
        #    Guest, ATA_2 => <NONE-EXIST>
        NONE_EXIST, NONE_EXIST, NONE_EXIST,

        # Guest@3
        #    Guest, COM_1 => Host, COM_2
        AddrRange(x86IOAddress(0x2e8), size=8),
        NONE_EXIST, NONE_EXIST, NONE_EXIST,
        #    Guest, I8042(keyboard) => <NONE-EXIST>
        NONE_EXIST,
        #    Guest, ATA_2 => <NONE-EXIST>
        NONE_EXIST, NONE_EXIST, NONE_EXIST,
        ]

    # PARDg5V: set IDE disk GuestID
    self.p5v.south_bridge.ide.guestNum = 4
    self.p5v.south_bridge.ide.disks[0].guestID = 0
    self.p5v.south_bridge.ide.disks[1].guestID = 0
    self.p5v.south_bridge.ide.disks[2].guestID = 1
    self.p5v.south_bridge.ide.disks[3].guestID = 1
    self.p5v.south_bridge.ide.disks[4].guestID = 2
    self.p5v.south_bridge.ide.disks[5].guestID = 2
    self.p5v.south_bridge.ide.disks[6].guestID = 3
    self.p5v.south_bridge.ide.disks[7].guestID = 3
    #self.p5v.south_bridge.ide.encrypt_enable_array = [0, 0, 0, 1]
    self.p5v.south_bridge.ide.encrypt_enable_array = [0, 0, 0, 0]

    # PARDg5V: set Uart GuestID
    self.p5v.com_1.guestID = 0
    self.p5v.com_2.guestID = 1
    self.p5v.com_3.guestID = 2
    self.p5v.com_4.guestID = 3

    return self

