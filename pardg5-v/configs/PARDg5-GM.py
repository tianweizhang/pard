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

addToPath('common')
addToPath('../../gem5-latest/configs/common')

from M5FSConfig import *
from SysPaths import *
from Benchmarks import *
import Simulation
import CacheConfig
import MemConfig
from Caches import *
import Options

from PARDg5GM import *

# Add options
parser = optparse.OptionParser()
Options.addCommonOptions(parser)
Options.addFSOptions(parser)
(options, args) = parser.parse_args()
if args:
    print "Error: script doesn't take any positional arguments"
    sys.exit(1)

test_sys = build_gm_system()
root = Root(full_system=True, system=test_sys)
Simulation.run(options, root, test_sys, FutureClass)
