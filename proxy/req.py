from pypercy import *
import config
import requests

print("Creating Client")
c = ZAGClient(num_blocks=config.NUM_BLOCKS, block_size=config.BLOCK_SIZE)
print("Making reqeuest")
req_id = c.make_request(0)
req = c.get_request()

print("Sending request, length: {}".format(len(req)))
resp = requests.get('http://127.0.0.1', data=req, headers={'pir': '1'}, stream=True)
resp = resp.raw.read()

print("Parsing response, length: {}".format(len(resp)))
c.parse_response(resp)
print("Reading response")
resp = c.get_response()

print("Response: {}".format(len(resp)))
print(resp)

f = open('out.png', 'wb')
f.write(resp)
f.close()
