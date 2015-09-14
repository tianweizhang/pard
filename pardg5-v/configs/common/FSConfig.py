# Copyright (c) 2014 ACS, ICT
# All rights reserved.
#
# Authors: Jiuyue Ma

from m5.objects import *
from m5.util import *

from SysPaths import *

class CowIdeDisk(PARDg5VIdeDisk):
    image = CowDiskImage(child=RawDiskImage(read_only=True),
                         read_only=False)

    def childImage(self, ci):
        self.image.child.image_file = ci

class MemBus(CoherentBus):
    badaddr_responder = BadAddr()
    default = Self.badaddr_responder.pio


def x86IOAddress(port):
    IO_address_space_base = 0x8000000000000000
    return IO_address_space_base + port

def connectX86ClassicSystem(x86_sys, numCPUs):
    # Constants similar to x86_traits.hh
    IO_address_space_base = 0x8000000000000000
    pci_config_address_space_base = 0xc000000000000000
    interrupts_address_space_base = 0xa000000000000000
    APIC_range_size = 1 << 12;

    x86_sys.membus = MemBus()

    # North Bridge
    x86_sys.iobus = NoncoherentBus()

    x86_sys.bridge = PARDg5VBridge(delay='50ns')
    x86_sys.bridge.master = x86_sys.iobus.slave
    #x86_sys.bridge.slave = x86_sys.membus.master
    x86_sys.bridge.slave  = x86_sys.bridge.addr_mapper.master
    x86_sys.membus.master = x86_sys.bridge.addr_mapper.slave

    # Allow the bridge to pass through the IO APIC (two pages),
    # everything in the IO address range up to the local APIC, and
    # then the entire PCI address space and beyond
    x86_sys.bridge.ranges = \
        [
        AddrRange(x86_sys.p5v.south_bridge.io_apic.pio_addr,
                  x86_sys.p5v.south_bridge.io_apic.pio_addr +
                  APIC_range_size - 1),
        AddrRange(IO_address_space_base,
                  interrupts_address_space_base - 1),
        AddrRange(pci_config_address_space_base,
                  Addr.max)
        ]

    # Create a bridge from the IO bus to the memory bus to allow access to
    # the local APIC (two pages)
    x86_sys.apicbridge = Bridge(delay='50ns')
    x86_sys.apicbridge.slave = x86_sys.iobus.master
    x86_sys.apicbridge.master = x86_sys.membus.slave
    x86_sys.apicbridge.ranges = [AddrRange(interrupts_address_space_base,
                                           interrupts_address_space_base +
                                           numCPUs * APIC_range_size
                                           - 1)]

    # connect the io bus
    x86_sys.p5v.attachIO(x86_sys.iobus)

    x86_sys.system_port = x86_sys.membus.slave

def connectX86RubySystem(x86_sys):
    # North Bridge
    x86_sys.iobus = NoncoherentBus()

    # add the ide to the list of dma devices that later need to attach to
    # dma controllers
    x86_sys._dma_ports = [x86_sys.p5v.south_bridge.ide.dma]
    x86_sys.p5v.attachIO(x86_sys.iobus, x86_sys._dma_ports)


def makeX86System(mem_mode, numCPUs = 1, mdesc = None, self = None,
                  Ruby = False):
    if self == None:
        self = X86System()

    if not mdesc:
        # generic system
        mdesc = SysConfig()
    self.readfile = mdesc.script()

    self.mem_mode = mem_mode

    # Physical memory
    # On the PC platform, the memory region 0xC0000000-0xFFFFFFFF is reserved
    # for various devices.  Hence, if the physical memory size is greater than
    # 3GB, we need to split it into two parts.
    excess_mem_size = \
        convert.toMemorySize(mdesc.mem()) - convert.toMemorySize('3GB')
    if excess_mem_size <= 0:
        self.mem_ranges = [AddrRange(mdesc.mem())]
    else:
        warn("Physical memory size specified is %s which is greater than " \
             "3GB.  Twice the number of memory controllers would be " \
             "created."  % (mdesc.mem()))

        self.mem_ranges = [AddrRange('3GB'),
            AddrRange(Addr('4GB'), size = excess_mem_size)]

    # Platform
    self.p5v = PARDg5VPlatform()

    # Create and connect the busses required by each memory system
    if Ruby:
        connectX86RubySystem(self)
    else:
        connectX86ClassicSystem(self, numCPUs)

    self.intrctrl = IntrControl()

    # Disks
    disks = []
    for i in range(0,4):
        disk0 = CowIdeDisk(driveID='master')
        #if i == 3:
        #    disk0.childImage(disk('x86root_secure.img'))
        #else:
        #    disk0.childImage(mdesc.disk())
        disk0.childImage(mdesc.disk())
        disk1 = CowIdeDisk(driveID='slave')
        disk1.childImage(disk('x86root.img'))
        disks.append(disk0);
        disks.append(disk1);
    self.p5v.south_bridge.ide.num_channels = 4
    self.p5v.south_bridge.ide.disks = disks

    #disk0 = CowIdeDisk(driveID='master')
    #disk0.childImage(mdesc.disk())
    #disk1 = CowIdeDisk(driveID='slave')
    #disk1.childImage(disk('cpu2006.img'))
    #disk2 = CowIdeDisk(driveID='master')
    #disk2.childImage(mdesc.disk())
    #disk3 = CowIdeDisk(driveID='slave')
    #disk3.childImage(disk('cpu2006.img'))
    #self.p5v.south_bridge.ide.disks = [disk0, disk1, disk2, disk3]

    # Add in a Bios information structure.
    structures = [X86SMBiosBiosInformation()]
    self.smbios_table.structures = structures

    # Set up the Intel MP table
    base_entries = []
    ext_entries = []
    for i in xrange(numCPUs):
        bp = X86IntelMPProcessor(
                local_apic_id = i,
                local_apic_version = 0x14,
                enable = True,
                bootstrap = (i == 0))
        base_entries.append(bp)
    io_apic = X86IntelMPIOAPIC(
            id = numCPUs,
            version = 0x11,
            enable = True,
            address = 0xfec00000)
    self.p5v.south_bridge.io_apic.apic_id = io_apic.id
    base_entries.append(io_apic)
    isa_bus = X86IntelMPBus(bus_id = 0, bus_type='ISA')
    base_entries.append(isa_bus)
    pci_bus = X86IntelMPBus(bus_id = 1, bus_type='PCI')
    base_entries.append(pci_bus)
    connect_busses = X86IntelMPBusHierarchy(bus_id=0,
            subtractive_decode=True, parent_bus=1)
    ext_entries.append(connect_busses)
    pci_dev4_inta = X86IntelMPIOIntAssignment(
            interrupt_type = 'INT',
            polarity = 'ConformPolarity',
            trigger = 'ConformTrigger',
            source_bus_id = 1,
            source_bus_irq = 0 + (4 << 2),
            dest_io_apic_id = io_apic.id,
            dest_io_apic_intin = 16)
    base_entries.append(pci_dev4_inta)
    def assignISAInt(irq, apicPin):
        assign_to_apic = X86IntelMPIOIntAssignment(
                interrupt_type = 'INT',
                polarity = 'ConformPolarity',
                trigger = 'ConformTrigger',
                source_bus_id = 0,
                source_bus_irq = irq,
                dest_io_apic_id = io_apic.id,
                dest_io_apic_intin = apicPin)
        base_entries.append(assign_to_apic)
    assignISAInt(0, 2)
    assignISAInt(1, 1)
    for i in range(3, 15):
        assignISAInt(i, i)
    self.intel_mp_table.base_entries = base_entries
    self.intel_mp_table.ext_entries = ext_entries

def makeLinuxX86Guest(numCPUs = 1, memSize = 0x40000000):
    self = LinuxX86PARDg5VGuest()

    # kernel & boot flag
    self.boot_osflags = 'earlyprintk=ttyS0 console=ttyS0 lpj=7999923 ' + \
                        'root=/dev/sda1 no_timer_check'
    self.kernel = binary('x86_64-vmlinux-2.6.22.9')

    # E820
    entries = [
        # Mark the first megabyte of memory as reserved
        X86E820Entry(addr = 0, size = '639kB', range_type = 1),
        X86E820Entry(addr = 0x9fc00, size = '385kB', range_type = 2),
        # available physical memory
        X86E820Entry(addr = 0x100000,
                     size = '%dB' % (memSize - 0x100000),
                     range_type = 1),
        # Reserve the last 16kB of the 32-bit address space for the
        # m5op interface
        X86E820Entry(addr=0xFFFF0000, size='64kB', range_type=2),
    ]
    self.e820_table.entries = entries

    # Bios information structure.
    structures = [X86SMBiosBiosInformation()]
    self.smbios_table.structures = structures

    # Set up the Intel MP table
    base_entries = []
    ext_entries = []
    for i in xrange(numCPUs):
        bp = X86IntelMPProcessor(
                local_apic_id = i,
                local_apic_version = 0x14,
                enable = True,
                bootstrap = (i == 0))
        base_entries.append(bp)
    io_apic = X86IntelMPIOAPIC(
            id = numCPUs,
            version = 0x11,
            enable = True,
            address = 0xfec00000)
    base_entries.append(io_apic)
    isa_bus = X86IntelMPBus(bus_id = 0, bus_type='ISA')
    base_entries.append(isa_bus)
    pci_bus = X86IntelMPBus(bus_id = 1, bus_type='PCI')
    base_entries.append(pci_bus)
    connect_busses = X86IntelMPBusHierarchy(bus_id=0,
            subtractive_decode=True, parent_bus=1)
    ext_entries.append(connect_busses)
    pci_dev4_inta = X86IntelMPIOIntAssignment(
            interrupt_type = 'INT',
            polarity = 'ConformPolarity',
            trigger = 'ConformTrigger',
            source_bus_id = 1,
            source_bus_irq = 0 + (4 << 2),
            dest_io_apic_id = io_apic.id,
            dest_io_apic_intin = 16)
    base_entries.append(pci_dev4_inta)

    def assignISAInt(irq, apicPin):
        assign_to_apic = X86IntelMPIOIntAssignment(
                interrupt_type = 'INT',
                polarity = 'ConformPolarity',
                trigger = 'ConformTrigger',
                source_bus_id = 0,
                source_bus_irq = irq,
                dest_io_apic_id = io_apic.id,
                dest_io_apic_intin = apicPin)
        base_entries.append(assign_to_apic)
    assignISAInt(0, 2)
    assignISAInt(1, 1)
    for i in range(3, 15):
        assignISAInt(i, i)
    self.intel_mp_table.base_entries = base_entries
    self.intel_mp_table.ext_entries = ext_entries

    return self

def makeLinuxX86System(mem_mode, numCPUs = 1, mdesc = None,
                       Ruby = False, numGuests = 1):
    self = X86PARDg5VSystem()

    # Build up the x86 system and then specialize it for Linux
    makeX86System(mem_mode, numCPUs, mdesc, self, Ruby)

    # We assume below that there's at least 1MB of memory. We'll require 2
    # just to avoid corner cases.
    phys_mem_size = sum(map(lambda r: r.size(), self.mem_ranges))
    assert(phys_mem_size >= 0x200000)
    assert(len(self.mem_ranges) <= 2)

    if numGuests>0:
        guests = []
        for guestID in range(0,numGuests):
            guest = makeLinuxX86Guest(numCPUs=1, memSize=0x20000000)
            guests.append(guest)
        self.guests = guests

    return self

