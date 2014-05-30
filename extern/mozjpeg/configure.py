#!/usr/bin/env python

import argparse
import datetime

parser = argparse.ArgumentParser()
parser.add_argument('--project', default='libmozjpeg',
    help='Project name, default is %(default)s')
parser.add_argument('--version', default='1.0.1',
    help='Project version, default is %(default)s')
parser.add_argument('--jpeg_lib_version', default='62',
    help='libjpeg version, default is %(default)s', choices=['62', '70', '80'])
parser.add_argument('--arith_enc', default='1',
    help='arithmetic encoding support, default is %(default)s')
parser.add_argument('--arith_dec', default='1',
    help='arithmetic decoding support, default is %(default)s')
parser.add_argument('--mem_srcdst', default='1',
    help='in-memory source/destination manager functions when emulating the libjpeg v6b or v7 API/ABI, default is %(default)s')

args = parser.parse_args() 
build = datetime.date.today().strftime('%Y%m%d')

def cmake_var(config, name, value):
    return config.replace(name, value)


def cmake_define(config, name, value):
    value = bool(value)
    define = '#{directive} {name}'.format(directive='define' if value else 'undef', name=name)
    return config.replace('#cmakedefine ' + name, define)


jconfig = open('win/jconfig.h.in').read()
jconfig = cmake_var(jconfig, '@VERSION@', args.version)
jconfig = cmake_var(jconfig, '@JPEG_LIB_VERSION@', args.jpeg_lib_version)
jconfig = cmake_define(jconfig, 'C_ARITH_CODING_SUPPORTED', args.arith_enc)
jconfig = cmake_define(jconfig, 'D_ARITH_CODING_SUPPORTED', args.arith_dec)
jconfig = cmake_define(jconfig, 'MEM_SRCDST_SUPPORTED', args.mem_srcdst)
open('jconfig.h', 'w').write(jconfig)

config = open('win/config.h.in').read()
config = cmake_var(config, '@VERSION@', args.version)
config = cmake_var(config, '@BUILD@', build)
config = cmake_var(config, '@CMAKE_PROJECT_NAME@', args.project)
open('config.h', 'w').write(config)
