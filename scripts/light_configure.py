#!/usr/bin/env python3

import logging
import asyncio

import aiocoap

import ipaddress

logging.basicConfig(level=logging.ERROR)

prefix = '2607:f018:800:111:c298:e554:4f'

@asyncio.coroutine
def light_configure (ip, state, freq):

    protocol = yield from aiocoap.Context.create_client_context()
    aiocoap.protocol.REQUEST_TIMEOUT = 25

    messages = []

    messages.append(('/onoff/Power', ['false', 'true'][int(state)]))
    messages.append(('/sdl/luxapose/Frequency', '{}'.format(freq)))

    for message in messages:

        path = message[0]
        payload = message[1]

        url = 'coap://[{}]/{}'.format(ip, path)
        payload = payload.encode('ascii')

        request = aiocoap.Message(code=aiocoap.POST, payload=payload)
        request.set_request_uri(url)

        while True:
            try:
                response = yield from protocol.request(request).response
            except Exception as e:
                print('failed {} for {}'.format(path, ip))
            else:
                print('Successfully configured {}'.format(ip))
                break

@asyncio.coroutine
def configure_lights (lights):
    r = []
    for light in lights:
        device_id = light[0]
        freq      = light[1]

        ipaddr = '{}{}:{}{}'.format(prefix, device_id[0:2], device_id[3:5], device_id[6:8])
        ip = ipaddress.ip_address(ipaddr)

        r.append(asyncio.async(light_configure(ip, True, freq)))
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

            try:
                chandelier_id = int(fields[0])
                device_id     = fields[1]
                freq          = int(fields[2])
                active        = fields[3]

                if 'y' in active or 'Y' in active:
                    lights.append((device_id, freq))

            except:
                print('Could not add {}'.format(l), end='')


    loop = asyncio.get_event_loop()

    asyncio.async(main(lights))

    # loop.run_until_complete()
    loop.run_forever()

