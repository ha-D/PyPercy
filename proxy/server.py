import config
from pypercy import *

from http.server import BaseHTTPRequestHandler, HTTPServer
import time


hostName = "0.0.0.0"
hostPort = 80

pirserver = ZAGServer(config.DATABASE, num_blocks=config.NUM_BLOCKS, block_size=config.BLOCK_SIZE)
# pirserver = ZAGServer('/home/hadi/database', num_blocks=10, block_size=816)
class PIRHandler(BaseHTTPRequestHandler):
    def do_GET(self):
        content_len = int(self.headers.get('content-length', 0))
        is_pir = self.headers.get('pir', 0)


        if is_pir:
            body = self.rfile.read(content_len)
            print("PIR request received, length: {}".format(len(body)))
            start_time = time.time()
            pirserver.process_request(body)
            process_time = time.time() - start_time
            print("Process time: {}".format(process_time))
            self.send_response(200)
            self.send_header('Content-type','pir')
            self.end_headers()

            response = pirserver.get_response()
            print("Response length: {}".format(len(response)))
            self.wfile.write(response)
            # self.wfile.write("HELLO BACK")
            return
        else:
            body = self.rfile.read(content_len)
            block = int(body)

            self.send_response(200)
            self.end_headers()

            f = open(config.DATABASE, 'rb')
            f.seek(block * config.BLOCK_SIZE)
            self.wfile.write(f.read(config.BLOCK_SIZE))
            f.close()
            return


myServer = HTTPServer((hostName, hostPort), PIRHandler)
print(time.asctime(), "Server Starts - %s:%s" % (hostName, hostPort))

try:
    myServer.serve_forever()
except KeyboardInterrupt:
    pass

myServer.server_close()
print(time.asctime(), "Server Stops - %s:%s" % (hostName, hostPort))
