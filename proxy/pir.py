import config
from pypercy import *

pir_urls = [
    '127.0.0.1',
    '10.0.2.2'
]

client = ZAGClient(num_blocks=config.NUM_BLOCKS, block_size=config.BLOCK_SIZE)

def make_request(block):
    client.make_request(block)
    return client.get_request()

## MITM
# def start(context, argv):
#     print("Started")
#
# def done(context):
#     print("Done")

# def clientconnect(context, root_layer):
#     print("------------- Client Connected")
#
# def clientdisconnect(context, root_layer):
#     print("Client Disconnect")
#     print("----------------------------------")
#
#
# def serverconnect(context, server_conn):
#     print("Server Connect")
#
# def serverdisconnect(context, server_conn):
#     print("Server disconnect")

def request(flow):

    print("--------------    HTTP Request")
    if 'pir' in flow.request.path:# or flow.request.host in pir_urls:
        print("PIR Request")
        query = int(flow.request.path.replace('/pir/', ''))
        req = make_request(query)
        flow.request.content = req
        flow.request.headers['pir'] = 'yes'
    else:
        print("NON PIR Request")
        query = flow.request.path.replace('/', '')
        flow.request.content = query


def responseheaders(flow):
    print("HTTP Response Headers")

def response(flow):
    print("HTTP Response")

    if flow.response.headers.get('Content-type','') == 'pir':
        print("PIR RESPONSE")
        client.parse_response(flow.response.content)
        resp = client.get_response()
        flow.response.raw = resp

        # print(len(resp))
        # print(type(resp[0]))
        # print("-------------------" )
        # s = ''
        # for i in range(10):
        #      s+=hex(ord(resp[i])) +' '
        # print(s)
        flow.response.content = resp
        flow.response.headers['content-type'] = 'image/png'
    else:
        resp = flow.response.content
        print(len(resp))
        print(type(resp[0]))
        print("-------------------" )
        s = ''
        for i in range(10):
             s+=hex(ord(resp[i])) +' '
        print(s)
        # print(flow.response.content)
    # flow.response.content = "Hello There!"

def error(context):
    print("HTTP Error")
