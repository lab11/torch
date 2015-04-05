#!/usr/bin/env python3

lights = []

with open('lights.conf') as f:
    for l in f:
        fields = l.split()

        try:
            chandelier_id = int(fields[0])
            device_id     = fields[1]
            freq          = int(fields[2])
            active        = fields[3]

            if freq in lights:
                print('DUPLICATE {} at {}', freq, device_id)

            lights.append(freq)

        except:
            print('Could not add {}'.format(l), end='')

