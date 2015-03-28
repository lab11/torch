#!/usr/bin/env python3

import logging
import pprint
import asyncio

import aiocoap

import ipaddress

logging.basicConfig(level=logging.ERROR)

prefix = '2607:f018:800:111:c298:e554:4f'

@asyncio.coroutine
def get_conf (light):

    protocol = yield from aiocoap.Context.create_client_context()
    aiocoap.protocol.REQUEST_TIMEOUT = 25

    messages = []

    messages.append((
        '/onoff/Power',
        'active',
        lambda x: x.decode('utf8') == 'true'
        ))
    messages.append((
        '/sdl/luxapose/Frequency',
        'freq',
        int
        ))

    for message in messages:

        path = message[0]

        url = 'coap://[{}]/{}'.format(light['ip'], path)

        request = aiocoap.Message(code=aiocoap.GET)
        request.set_request_uri(url)

        tries = 0
        while tries < 3:
            try:
                response = yield from protocol.request(request).response
            except Exception as e:
                print('failed {} for {}'.format(path, light['ip']))
            else:
                light[message[1]] = message[2](response.payload)
                print('light_{}.{} = {}'.format(
                    light['device_id'].split(':')[2],
                    message[1],
                    light[message[1]],
                    ))
                break
            tries += 1

@asyncio.coroutine
def configure_lights (lights):
    r = []
    for light in lights:
        device_id = light['device_id']

        ipaddr = '{}{}:{}{}'.format(prefix, device_id[0:2], device_id[3:5], device_id[6:8])
        ip = ipaddress.ip_address(ipaddr)

        light['ip'] = ip

        r.append(asyncio.async(get_conf(light)))
    return asyncio.wait(r)

@asyncio.coroutine
def main (lights):
    yield from asyncio.async(configure_lights(lights))
    print('Lights done!')

if __name__ == "__main__":
    # Parse lights.conf
    lights = []

    with open('lights.conf') as f:
        for l in f:
            fields = l.split()

            if len(l.strip()) == 0:
                continue
            if fields[0] == 'chandelier_id':
                continue

            try:
                chandelier_id = int(fields[0])
                device_id     = fields[1]
                freq          = int(fields[2])
                active        = fields[3]

                light = {
                        'chandelier_id' : chandelier_id,
                        'device_id'     : device_id,
                        }

                lights.append(light)

            except:
                print('Could not add {}'.format(l), end='')

    lights.sort(key=lambda x: int(x['device_id'].split(':')[2], 16))

    loop = asyncio.get_event_loop()

    loop.run_until_complete(main(lights))

    pprint.pprint(lights)

