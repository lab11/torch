#!/usr/bin/env python3

import logging
import asyncio

import aiocoap
# from aiocoap import protocol

import ipaddress

logging.basicConfig(level=logging.ERROR)

valid_ips = []

@asyncio.coroutine
def test_address (ipaddr):

    protocol = yield from aiocoap.Context.create_client_context()
    aiocoap.protocol.REQUEST_TIMEOUT = 5

    url = 'coap://[{}]/onoff/Power'.format(ipaddr)

    request = aiocoap.Message(code=aiocoap.GET)
    request.set_request_uri(url)

    try:
        response = yield from protocol.request(request).response
    except Exception as e:
        pass
    else:
        valid_ips.append(ipaddr)
        print('Result: {}: {}'.format(ipaddr, response.payload))

@asyncio.coroutine
def light_power (ip, state):

    protocol = yield from aiocoap.Context.create_client_context()
    aiocoap.protocol.REQUEST_TIMEOUT = 25

    url = 'coap://[{}]/onoff/Power'.format(ip)
    if state:
        payload = 'true'
    else:
        payload = 'false'
    payload = payload.encode('ascii')

    request = aiocoap.Message(code=aiocoap.POST, payload=payload)
    request.set_request_uri(url)

    try:
        response = yield from protocol.request(request).response
    except Exception as e:
        print('failed')
        print(e)
        pass
    else:
        print('turned off: {}'.format(ip))

@asyncio.coroutine
def find_bulbs ():
    routines = []
    block = ipaddress.ip_network('2607:f018:800:111:c298:e554:4f20:0/120')
    for i in range(1,30):
        routines.append(asyncio.async(test_address(block[i])))
    return asyncio.wait(routines)

@asyncio.coroutine
def turn_all_off (ips):
    r = []
    for ip in ips:
        r.append(asyncio.async(light_power(ip, False)))
    return asyncio.wait(r)

@asyncio.coroutine
def turn_all_on (ips):
    r = []
    for ip in ips:
        r.append(asyncio.async(light_power(ip, True)))
    return asyncio.wait(r)

@asyncio.coroutine
def main ():
    yield from asyncio.async(find_bulbs())
    print('found {}'.format(valid_ips))
    yield from asyncio.async(turn_all_off(valid_ips))
    print('Off!')

if __name__ == "__main__":
    loop = asyncio.get_event_loop()
    # block = ipaddress.ip_network('2607:f018:800:111:c298:e554:4f20:0/120')
    # for i in range(1,30):


    asyncio.async(main())




    # loop.run_until_complete()
    loop.run_forever()