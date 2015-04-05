#!/usr/bin/env python3

from sh import make
from sh import touch

import sys
import os

lights = []

on = '1'
btldr = '0'

light_conf = 'lights.conf'


# env = os.environ.copy()

if len(sys.argv) == 2:
    light_conf = sys.argv[1]

with open(light_conf) as f:
    for l in f:
        fields = l.split()

        try:
            chandelier_id = int(fields[0])
            device_id     = fields[1]
            freq          = int(fields[2])
            active        = fields[3]

        except:
            if len(l) > 1:
                print('Could not add {}'.format(l), end='')
            continue

        print('')
        print('')

        while True:
            print('DEVICE: {}'.format(device_id))
            print('  frequency: {}'.format(freq))
            print('')

            prompt = 'About to program {}. Ready? [Y,s]: '.format(device_id)
            answer = input(prompt)
            if len(answer) == 0 or 'y' in answer or 'Y' in answer:
                # do the programming
                print('  programming...')

                # need startup_gcc.c to recompile
                cmd = '../../platform/torch/startup_gcc.c'
                touch(cmd)

                cmd = 'install ID=c0:98:e5:54:4f:{} ON={} FREQ={} BTLDR={}'.format(device_id, on, freq, btldr)
                print(make(*cmd.split()))
                break

            elif 's' in answer or 'S' in answer:
                print('skipping {}'.format(device_id))
                break



