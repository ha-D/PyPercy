from pypercy import *
import unittest
import sys
import logging
import time

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
log_handler.setLevel(logging.DEBUG)
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
        server = ZAGServer(db, num_blocks=num_blocks, block_size=block_size)
        client = ZAGClient(num_blocks=num_blocks, block_size=block_size)

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
    @params(query=0, num_blocks=10, block_size=100, word_size=2048, num_servers=1, t=2)
    def test_nb10_bs100(self, log, query, num_blocks, block_size, word_size, num_servers, t):
        # server = ZITZZPServer(db, num_blocks=num_blocks, block_size=block_size, word_size=word_size)
        client = ZITZZPClient(num_blocks=num_blocks, block_size=block_size, word_size=word_size, t=2, num_servers=num_servers)
        #
        # t, req_id = gettime(client.make_request, query)
        # log.debug("client request time:  {}".format(t))
        # #
        # req = client.get_request()
        # log.debug('client request size:  {}'.format(len(req)))
        # #
        # t, _ = gettime(server.process_request, req)
        # log.debug('server process time:  {}'.format(t))
        # #
        # sresp = server.get_response()
        # log.debug('server response size: {}'.format(len(sresp)))
        #
        # t, _ = gettime(client.parse_response, sresp)
        # log.debug('client parse time:    {}'.format(t))

        # resp = client.get_response()
        #
        # self.assertEqual(db_content[query * block_size: (query+1) * block_size], resp, 'PIR content matches DB')


def gettime(f, *args, **kwargs):
    start = time.time()
    res = f(*args, **kwargs)
    return time.time() - start, res


if __name__ == '__main__':
    unittest.main()
