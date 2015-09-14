# Copyright (c) 2014 ACS, ICT.
# All rights reserved.
#
# Author: Jiuyue Ma

import optparse
import sys

from m5.objects import *
from m5.util import addToPath, fatal

addToPath('../../gem5-latest/configs/common')
addToPath('common')

import Simulation
import Options
import PARDg5Main

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
root = Root(full_system=True, system=main_sys)

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
