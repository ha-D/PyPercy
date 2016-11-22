from pypercy import *
import sys

db = '/home/hadi/database'
query = 0
block_size = 100
num_blocks = 10

if len(sys.argv) > 1:
   db = sys.argv[1]
if len(sys.argv) > 2:
   num_blocks = int(sys.argv[2])
   block_size = int(sys.argv[3])
   query = int(sys.argv[4])


server = ZAGServer(db, num_blocks=num_blocks, block_size=block_size)
print("Started Server: ")
print("Database: %s  NumBlocks: %d  BlockSize: %d" % (db, num_blocks, block_size))

client = ZAGClient(num_blocks=num_blocks, block_size=block_size)
req_id = client.make_request(query)
req = client.get_request()
print("\nBlock %d requested" % query)
print("Request ID: %d" % req_id)
print("Request Size: %d" % len(req))

server.process_request(req)
sresp = server.get_response()
print("\nServer response generated")
print("Server Response Size: %d" % len(sresp))

client.parse_response(sresp)
resp = client.get_response()
print("\nResponse Parsed")
print("Response Size: %d" % len(resp))
print("Response:")
print(resp)
