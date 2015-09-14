# Copyright (c) 2014 ACS, ICT.
# All rights reserved.
#
# Author: Jiuyue Ma

import optparse
import sys

from m5.objects import *
from m5.util import addToPath, fatal

addToPath('../../gem5-pard/configs/common')
addToPath('common')

import Simulation
import Options
import PARDg5Main
import PARDg5GM

# Control Panel Settings
class CPNetwork(NoncoherentBus):
    fake_responder = IsaFake(pio_addr=0, pio_size=Addr('16MB'))
    default = Self.fake_responder.pio

def connect_cp_bus(sys, gm):
    # CPN0 for cache/memory; CPN1 for IO.
    gm.cpn = [CPNetwork(), CPNetwork()]
    gm.cpa = [CPAdaptor(pci_bus=0, pci_dev=0x10, pci_func=0, InterruptLine = 10,
                        InterruptPin=1),
              CPAdaptor(pci_bus=0, pci_dev=0x11, pci_func=0, InterruptLine = 11,
                        InterruptPin=1)]
    for i in xrange(2):
        gm.cpa[i].pio = gm.iobus.master
        gm.cpa[i].config = gm.iobus.master
        gm.cpa[i].dma = gm.iobus.slave
        gm.cpa[i].master = gm.cpn[i].slave

    g_cp_dev = 0

    # L2 Tag
    if options.l2cache:
        sys.l2.tags.connector = [CPConnector(cp_dev=g_cp_dev, cp_fun=0,
                                             Type = 67,		# Type 'C'
                                             IDENT = "L2TAG_CP")]
        gm.cpn[0].master = sys.l2.tags.connector[0].slave
        g_cp_dev = g_cp_dev + 1

    # CP of MemBus Address Mapper
    sys.membus.mapper.connector.cp_dev = g_cp_dev
    sys.membus.mapper.connector.Type = 65			# Type 'A'
    sys.membus.mapper.connector.IDENT = "MEMBUS_CP"
    gm.cpn[0].master = sys.membus.mapper.connector.slave
    g_cp_dev = g_cp_dev + 1

    # CP of Memory Controller
    #sys.mem_ctrls[0].addr_mapper.connector.eventq_index = 1
    for mc in sys.mem_ctrls:
        mc.connector.cp_dev = g_cp_dev
        mc.connector.Type = 77					# Type 'M'
        mc.connector.IDENT = "MEMCTRL_CP"
        gm.cpn[0].master = mc.connector.slave
        g_cp_dev = g_cp_dev + 1

    # CP of I/O Bridge
    #sys.bridge.addr_mapper.connector.eventq_index = 1
    sys.bridge.addr_mapper.connector.cp_dev = g_cp_dev
    sys.bridge.addr_mapper.connector.Type = 66			# Type 'B'
    sys.bridge.addr_mapper.connector.IDENT = "IOBRG_CP"
    gm.cpn[1].master = sys.bridge.addr_mapper.connector.slave
    g_cp_dev = g_cp_dev + 1

    return (sys,gm)

# Add options
parser = optparse.OptionParser()
Options.addCommonOptions(parser)
Options.addFSOptions(parser)
PARDg5Main.addPARDg5VOptions(parser)

parser.add_option("--dumpreset-stats", action="store", type="string",
    help="<M,N> dump and reset stats at M ns and every N ns thereafter")
parser.add_option("--max-stats", action="store", type="int", default=10000,
    help="the maximum number of stats to drop (NOT IMPL)")

(options, args) = parser.parse_args()

if args:
    print "Error: script doesn't take any positional arguments"
    sys.exit(1)

# Build System
(main_sys, FutureClass) = PARDg5Main.build_system(options)
gm_sys = PARDg5GM.build_gm_system()
connect_cp_bus(main_sys, gm_sys)
#gm_sys.eventq_index = 1;
#root = Root(full_system=True, system=main_sys)
root = Root(full_system=True, test_system=main_sys, drive_system=gm_sys,
            sim_quantum=500000)

if options.dumpreset_stats:
    when, period = options.dumpreset_stats.split(",", 1)
    main_sys.dumpstats_delay = int(when)
    main_sys.dumpstats_period = int(period)
    print "PARDg5-V: dumpreset-stats @ <",when,",",period,">"

if options.timesync:
    root.time_sync_enable = True

if options.frame_capture:
    VncServer.frame_capture = True

Simulation.setWorkCountOptions(main_sys, options)
Simulation.run(options, root, main_sys, FutureClass)
