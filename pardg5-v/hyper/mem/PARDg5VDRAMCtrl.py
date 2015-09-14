# Copyright (c) 2014 ACS, ICT.
# All rights reserved.
#
# Authors: Jiuyue Ma

from m5.params import *
from DRAMCtrl import *
from PARDg5VAddrMapper import *

# PARDg5VDRAMCtrl is a PARDg5-V compatible DRAM controller with memory
# virtualize support
class PARDg5VDRAMCtrl(DRAMCtrl):
    type = 'PARDg5VDRAMCtrl'
    cxx_header = "hyper/mem/dram_ctrl.hh"

    # For control plane network
    agent = Param.ManagedResourceAgent(ManagedResourceAgent(),
        "GlobalManager managed resource agent")
    connector = Param.CPConnector(CPConnector(cp_dev=0, cp_fun=0),
        "Control plane network connector")

    # Control Plane Configuration
    original_ranges = VectorParam.GuestAddrRange([],
        "Ranges of memory that should me remapped")
    remapped_ranges = VectorParam.AddrRange([],
        "Ranges of memory that are being mapped to")

# A single DDR3-1600 x64 channel (one command and address bus), with
# timings based on a DDR3-1600 4 Gbit datasheet (Micron MT41J512M8) in
# an 8x8 configuration, amounting to 4 Gbyte of memory.
class PARD_DDR3_1600_x64(PARDg5VDRAMCtrl):
    # 8x8 configuration, 8 devices each with an 8-bit interface
    device_bus_width = 8

    # DDR3 is a BL8 device
    burst_length = 8

    # Each device has a page (row buffer) size of 1 Kbyte (1K columns x8)
    device_rowbuffer_size = '1kB'

    # 8x8 configuration, so 8 devices
    devices_per_rank = 8

    # Use two ranks
    ranks_per_channel = 2

    # DDR3 has 8 banks in all configurations
    banks_per_rank = 8

    # 800 MHz
    tCK = '1.25ns'

    # 8 beats across an x64 interface translates to 4 clocks @ 800 MHz
    tBURST = '5ns'

    # DDR3-1600 11-11-11
    tRCD = '13.75ns'
    tCL = '13.75ns'
    tRP = '13.75ns'
    tRAS = '35ns'
    tRRD = '6ns'
    tXAW = '30ns'
    activation_limit = 4
    tRFC = '260ns'

    tWR = '15ns'

    # Greater of 4 CK or 7.5 ns
    tWTR = '7.5ns'

    # Greater of 4 CK or 7.5 ns
    tRTP = '7.5ns'

    # Default read-to-write bus around to 2 CK, @800 MHz = 2.5 ns
    tRTW = '2.5ns'

    # <=85C, half for >85C
    tREFI = '7.8us'

# A single DDR3-2133 x64 channel refining a selected subset of the
# options for the DDR-1600 configuration, based on the same DDR3-1600
# 4 Gbit datasheet (Micron MT41J512M8). Most parameters are kept
# consistent across the two configurations.
class PARD_DDR3_2133_x64(DDR3_1600_x64):
    # 1066 MHz
    tCK = '0.938ns'

    # 8 beats across an x64 interface translates to 4 clocks @ 1066 MHz
    tBURST = '3.752ns'

    # DDR3-2133 14-14-14
    tRCD = '13.09ns'
    tCL = '13.09ns'
    tRP = '13.09ns'
    tRAS = '33ns'
    tRRD = '5ns'
    tXAW = '25ns'

# A single DDR4-2400 x64 channel (one command and address bus), with
# timings based on a DDR4-2400 4 Gbit datasheet (Samsung K4A4G085WD)
# in an 8x8 configuration, amounting to 4 Gbyte of memory.
class PARD_DDR4_2400_x64(PARDg5VDRAMCtrl):
    # 8x8 configuration, 8 devices each with an 8-bit interface
    device_bus_width = 8

    # DDR4 is a BL8 device
    burst_length = 8

    # Each device has a page (row buffer) size of 1 Kbyte (1K columns x8)
    device_rowbuffer_size = '1kB'

    # 8x8 configuration, so 8 devices
    devices_per_rank = 8

    # Use a single rank
    ranks_per_channel = 1

    # DDR4 has 16 banks (4 bank groups) in all
    # configurations. Currently we do not capture the additional
    # constraints incurred by the bank groups
    banks_per_rank = 16

    # 1200 MHz
    tCK = '0.833ns'

    # 8 beats across an x64 interface translates to 4 clocks @ 1200 MHz
    tBURST = '3.333ns'

    # DDR4-2400 17-17-17
    tRCD = '14.16ns'
    tCL = '14.16ns'
    tRP = '14.16ns'
    tRAS = '32ns'

    # Here using the average of RRD_S and RRD_L
    tRRD = '4.1ns'
    tXAW = '21ns'
    activation_limit = 4
    tRFC = '260ns'

    tWR = '15ns'

    # Here using the average of WTR_S and WTR_L
    tWTR = '5ns'

    # Greater of 4 CK or 7.5 ns
    tRTP = '7.5ns'

    # Default read-to-write bus around to 2 CK, @1200 MHz = 1.666 ns
    tRTW = '1.666ns'

    # <=85C, half for >85C
    tREFI = '7.8us'

# A single DDR3 x64 interface (one command and address bus), with
# default timings based on DDR3-1333 4 Gbit parts in an 8x8
# configuration, which would amount to 4 GByte of memory.  This
# configuration is primarily for comparing with DRAMSim2, and all the
# parameters except ranks_per_channel are based on the DRAMSim2 config
# file DDR3_micron_32M_8B_x8_sg15.ini. Note that ranks_per_channel has
# to be manually set, depending on size of the memory to be
# simulated. By default DRAMSim2 has 2048MB of memory with a single
# rank. Therefore for 4 GByte memory, set ranks_per_channel = 2
class PARD_DDR3_1333_x64_DRAMSim2(PARDg5VDRAMCtrl):
    # 8x8 configuration, 8 devices each with an 8-bit interface
    device_bus_width = 8

    # DDR3 is a BL8 device
    burst_length = 8

    # Each device has a page (row buffer) size of 1KB
    # (this depends on the memory density)
    device_rowbuffer_size = '1kB'

    # 8x8 configuration, so 8 devices
    devices_per_rank = 8

    # Use two ranks
    ranks_per_channel = 2

    # DDR3 has 8 banks in all configurations
    banks_per_rank = 8

    # 666 MHs
    tCK = '1.5ns'

    tRCD = '15ns'
    tCL = '15ns'
    tRP = '15ns'
    tRAS = '36ns'
    tWR = '15ns'
    tRTP = '7.5ns'

    # 8 beats across an x64 interface translates to 4 clocks @ 666.66 MHz.
    # Note this is a BL8 DDR device.
    tBURST = '6ns'

    tRFC = '160ns'

    # DDR3, <=85C, half for >85C
    tREFI = '7.8us'

    # Greater of 4 CK or 7.5 ns, 4 CK @ 666.66 MHz = 6 ns
    tWTR = '7.5ns'

    # Default read-to-write bus around to 2 CK, @666.66 MHz = 3 ns
    tRTW = '3ns'

    tRRD = '6.0ns'

    tXAW = '30ns'
    activation_limit = 4


# A single LPDDR2-S4 x32 interface (one command/address bus), with
# default timings based on a LPDDR2-1066 4 Gbit part in a 1x32
# configuration.
class PARD_LPDDR2_S4_1066_x32(PARDg5VDRAMCtrl):
    # 1x32 configuration, 1 device with a 32-bit interface
    device_bus_width = 32

    # LPDDR2_S4 is a BL4 and BL8 device
    burst_length = 8

    # Each device has a page (row buffer) size of 1KB
    # (this depends on the memory density)
    device_rowbuffer_size = '1kB'

    # 1x32 configuration, so 1 device
    devices_per_rank = 1

    # Use a single rank
    ranks_per_channel = 1

    # LPDDR2-S4 has 8 banks in all configurations
    banks_per_rank = 8

    # 533 MHz
    tCK = '1.876ns'

    # Fixed at 15 ns
    tRCD = '15ns'

    # 8 CK read latency, 4 CK write latency @ 533 MHz, 1.876 ns cycle time
    tCL = '15ns'

    # Pre-charge one bank 15 ns (all banks 18 ns)
    tRP = '15ns'

    tRAS = '42ns'
    tWR = '15ns'

    # 6 CK read to precharge delay
    tRTP = '11.256ns'

    # 8 beats across an x32 DDR interface translates to 4 clocks @ 533 MHz.
    # Note this is a BL8 DDR device.
    # Requests larger than 32 bytes are broken down into multiple requests
    # in the controller
    tBURST = '7.5ns'

    # LPDDR2-S4, 4 Gbit
    tRFC = '130ns'
    tREFI = '3.9us'

    # Irrespective of speed grade, tWTR is 7.5 ns
    tWTR = '7.5ns'

    # Default read-to-write bus around to 2 CK, @533 MHz = 3.75 ns
    tRTW = '3.75ns'

    # Activate to activate irrespective of density and speed grade
    tRRD = '10.0ns'

    # Irrespective of density, tFAW is 50 ns
    tXAW = '50ns'
    activation_limit = 4

# A single WideIO x128 interface (one command and address bus), with
# default timings based on an estimated WIO-200 8 Gbit part.
class PARD_WideIO_200_x128(PARDg5VDRAMCtrl):
    # 1x128 configuration, 1 device with a 128-bit interface
    device_bus_width = 128

    # This is a BL4 device
    burst_length = 4

    # Each device has a page (row buffer) size of 4KB
    # (this depends on the memory density)
    device_rowbuffer_size = '4kB'

    # 1x128 configuration, so 1 device
    devices_per_rank = 1

    # Use one rank for a one-high die stack
    ranks_per_channel = 1

    # WideIO has 4 banks in all configurations
    banks_per_rank = 4

    # 200 MHz
    tCK = '5ns'

    # WIO-200
    tRCD = '18ns'
    tCL = '18ns'
    tRP = '18ns'
    tRAS = '42ns'
    tWR = '15ns'
    # Read to precharge is same as the burst
    tRTP = '20ns'

    # 4 beats across an x128 SDR interface translates to 4 clocks @ 200 MHz.
    # Note this is a BL4 SDR device.
    tBURST = '20ns'

    # WIO 8 Gb
    tRFC = '210ns'

    # WIO 8 Gb, <=85C, half for >85C
    tREFI = '3.9us'

    # Greater of 2 CK or 15 ns, 2 CK @ 200 MHz = 10 ns
    tWTR = '15ns'

    # Default read-to-write bus around to 2 CK, @200 MHz = 10 ns
    tRTW = '10ns'

    # Activate to activate irrespective of density and speed grade
    tRRD = '10.0ns'

    # Two instead of four activation window
    tXAW = '50ns'
    activation_limit = 2

# A single LPDDR3 x32 interface (one command/address bus), with
# default timings based on a LPDDR3-1600 4 Gbit part in a 1x32
# configuration
class PARD_LPDDR3_1600_x32(PARDg5VDRAMCtrl):
    # 1x32 configuration, 1 device with a 32-bit interface
    device_bus_width = 32

    # LPDDR3 is a BL8 device
    burst_length = 8

    # Each device has a page (row buffer) size of 4KB
    device_rowbuffer_size = '4kB'

    # 1x32 configuration, so 1 device
    devices_per_rank = 1

    # Use a single rank
    ranks_per_channel = 1

    # LPDDR3 has 8 banks in all configurations
    banks_per_rank = 8

    # 800 MHz
    tCK = '1.25ns'

    # Fixed at 15 ns
    tRCD = '15ns'

    # 12 CK read latency, 6 CK write latency @ 800 MHz, 1.25 ns cycle time
    tCL = '15ns'

    tRAS = '42ns'
    tWR = '15ns'

    # Greater of 4 CK or 7.5 ns, 4 CK @ 800 MHz = 5 ns
    tRTP = '7.5ns'

    # Pre-charge one bank 15 ns (all banks 18 ns)
    tRP = '15ns'

    # 8 beats across a x32 DDR interface translates to 4 clocks @ 800 MHz.
    # Note this is a BL8 DDR device.
    # Requests larger than 32 bytes are broken down into multiple requests
    # in the controller
    tBURST = '5ns'

    # LPDDR3, 4 Gb
    tRFC = '130ns'
    tREFI = '3.9us'

    # Irrespective of speed grade, tWTR is 7.5 ns
    tWTR = '7.5ns'

    # Default read-to-write bus around to 2 CK, @800 MHz = 2.5 ns
    tRTW = '2.5ns'

    # Activate to activate irrespective of density and speed grade
    tRRD = '10.0ns'

    # Irrespective of size, tFAW is 50 ns
    tXAW = '50ns'
    activation_limit = 4
