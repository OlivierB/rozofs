# -*- coding: utf-8 -*-
#
# Copyright (c) 2013 Fizians SAS. <http://www.fizians.com>
# This file is part of Rozofs.
#
# Rozofs is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published
# by the Free Software Foundation, version 2.
#
# Rozofs is distributed in the hope that it will be useful, but
# WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
# General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program.  If not, see
# <http://www.gnu.org/licenses/>.

from rozofs.core.platform import Platform
from rozofs.cli.output import ordered_puts
from collections import OrderedDict

def set(platform, args):
    layout = platform.convert_layout(args.layout[0], args.layout[1], args.layout[2])
    platform.set_layout(layout)

def get(platform, args):
    layout = platform. get_layout()
    decode = platform.decode_layout(layout)
    ordered_puts({'layout '+str(layout): OrderedDict([
          ("inverse", decode[0]),
          ("forward", decode[1]),
          ("safe", decode[2])
        ])})

def dispatch(args):
    p = Platform(args.exportd)
    globals()[args.action.replace('-', '_')](p, args)
