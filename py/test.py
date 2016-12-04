from pypercy import *
import unittest
import sys
import logging
import time

LOG_LEVEL = logging.INFO

db = '/home/hadi/database'
with open(db, 'rb') as f:
    db_content = f.read()

def params(**kwargs):
    def wrapper(f):
        def inner(self, **ikwargs):
            kwargs.update(ikwargs)
            return f(self, **kwargs)
        inner.__name__ = f.__name__
        return inner
    return wrapper

log_handler = logging.StreamHandler()
log_handler.setLevel(LOG_LEVEL)
log_formatter = logging.Formatter("%(name)s - %(message)s")
log_handler.setFormatter(log_formatter)
def log(level=logging.DEBUG):
    def wrapper(f):
        def inner(self, *args, **kwargs):
            logger = logging.getLogger("{}.{}".format(self.__class__.__name__, f.__name__))
            logger.addHandler(log_handler)
            logger.setLevel(level)
            kwargs['log'] = logger
            return f(self, *args, **kwargs)
        return inner
    return wrapper


class TestAGPIR(unittest.TestCase):
    @log(logging.DEBUG)
    @params(num_blocks=10, block_size=100, query=0)
    @unittest.skip("fuck off")
    def test_nb10_bs100(self, log, num_blocks, block_size, query):
        server = AGServer(db, num_blocks=num_blocks, block_size=block_size)
        client = AGClient(num_blocks=num_blocks, block_size=block_size)

        t, req_id = gettime(client.make_request, query)
        log.debug("client request time:  {}".format(t))

        req = client.get_request()
        log.debug('client request size:  {}'.format(len(req)))

        t, _ = gettime(server.process_request, req)
        log.debug('server process time:  {}'.format(t))

        sresp = server.get_response()
        log.debug('server response size: {}'.format(len(sresp)))

        t, _ = gettime(client.parse_response, sresp)
        log.debug('client parse time:    {}'.format(t))

        resp = client.get_response()

        self.assertEqual(db_content[query * block_size: (query+1) * block_size], resp, 'PIR content matches DB')

class TestITPIR(unittest.TestCase):
    @log(logging.DEBUG)
    @params(query=0, num_blocks=100, block_size=100, w=8, num_servers=2, t=2)
    def test_nb100_bs100(self, log, query, num_blocks, block_size, w, num_servers, t):
        server1 = ZZPServer(db, num_blocks=num_blocks, block_size=block_size, w=w)
        server2 = ZZPServer(db, num_blocks=num_blocks, block_size=block_size, w=w)
        client = ZZPClient(num_blocks=num_blocks, block_size=block_size, w=w, num_servers=num_servers)
        #
        t, req_id = gettime(client.make_request, query)
        log.debug("client request time:  {}".format(t))

        req1 = client.get_request(server_num=0)
        log.debug('client request#1 size:  {}'.format(len(req1)))
        req2 = client.get_request(server_num=1)
        log.debug('client request#2 size:  {}'.format(len(req2)))

        t, _ = gettime(server1.process_request, req1)
        log.debug('server#1 process time:  {}'.format(t))
        t, _ = gettime(server2.process_request, req2)
        log.debug('server#2 process time:  {}'.format(t))

        sresp1 = server1.get_response()
        log.debug('server#1 response size: {}'.format(len(sresp1)))
        sresp2 = server2.get_response()
        log.debug('server#2 response size: {}'.format(len(sresp2)))

        client.write_response(sresp1, server_num=0)
        client.write_response(sresp2, server_num=1)

        t, _ = gettime(client.parse_response)
        log.debug('client parse time:    {}'.format(t))

        resp = client.get_result()

        self.assertEqual(db_content[query * block_size: (query+1) * block_size], resp, 'PIR content matches DB')


def gettime(f, *args, **kwargs):
    start = time.time()
    res = f(*args, **kwargs)
    return time.time() - start, res


if __name__ == '__main__':
    unittest.main()
