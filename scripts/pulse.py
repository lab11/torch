#!/usr/bin/env python3

import asyncio
import ipaddress
import logging
import pprint
import sys


import aiocoap



logging.basicConfig(level=logging.ERROR)

prefix = '2607:f018:800:10f:c298:e554:4f20:'

@asyncio.coroutine
def set_frequency (ipaddr, freq):

    protocol = yield from aiocoap.Context.create_client_context()
    aiocoap.protocol.REQUEST_TIMEOUT = 2

    url = 'coap://[{}]/sdl/luxapose/Frequency'.format(ipaddr)
    payload = '{}'.format(freq).encode('ascii')
    # print(payload)

    request = aiocoap.Message(code=aiocoap.POST, payload=payload)
    request.set_request_uri(url)

    try:
        response = yield from protocol.request(request).response
    except Exception as e:
        print('pulse hiccup')
    else:
        print('pulsed')

@asyncio.coroutine
def main (ipaddr):
    # alternate brightness between 10 and 50
    brightness = 50

    while True:
        brightness = (brightness + 40) % 80

        yield from asyncio.async(set_frequency(ipaddr, brightness))
        yield from asyncio.sleep(2)

if __name__ == "__main__":

    if len(sys.argv) != 2:
        print('Usage: {} <bulb id>')
        sys.exit(1)

    try:
        ipaddr = str(ipaddress.ip_address(prefix + sys.argv[1]))
    except Exception as e:
        print('Must be a valid bulb id')
        sys.exit(1)


    loop = asyncio.get_event_loop()

    loop.run_until_complete(main(ipaddr))
