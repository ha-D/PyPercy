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


class CommonTest:
    def make_request(self, log, query, client, servers, **kwargs):
        t, session = gettime(client.make_request, query)
        log.debug("client request time:  {}".format(t))

        reqs = []
        for s in range(len(servers)):
            reqs.append(session.get_request(server_num=s))
            log.debug('client request#{} size:  {}'.format(s+1, len(reqs[-1])))

        for s in range(len(servers)):
            t, _ = gettime(servers[s].process_request, reqs[s])
            log.debug('server#{} process time:  {}'.format(s+1, t))

        for s in range(len(servers)):
            sresp = servers[s].get_response()
            log.debug('server#{} response size: {}'.format(s+1, len(sresp)))

            session.write_response(sresp, server_num=s)

        t, _ = gettime(session.parse_response)
        log.debug('client parse time:    {}'.format(t))

        return session

    def create_nodes(self, num_blocks, block_size, num_servers, *args, **kwargs):
        servers = [self.create_server(block_size=block_size, num_blocks=num_blocks) for s in range(num_servers)]
        client = self.create_client(num_servers=num_servers, block_size=block_size, num_blocks=num_blocks)
        return client, servers

    def create_server(self, num_blocks, block_size):
        pass

    def create_client(self, num_servers, num_blocks, block_size):
        pass

    def check_block(self, content, query, block_size, **kwargs):
        self.assertEqual(db_content[query * block_size: (query+1) * block_size], content,
         'PIR content for block {} must match corresponding block from DB'.format(query))

    @log(logging.DEBUG)
    @params(query=10, num_blocks=160, block_size=2**10, num_servers=2)
    def test_two_server(self, *args, **kwargs):
        client, servers = self.create_nodes(**kwargs)
        session = self.make_request(client=client, servers=servers, *args, **kwargs)
        resp = session.get_result()
        self.check_block(resp, **kwargs)

    @log(logging.DEBUG)
    @params(query=10, num_blocks=160, block_size=2**10, num_servers=3)
    def test_three_server(self, *args, **kwargs):
        client, servers = self.create_nodes(**kwargs)
        session = self.make_request(client=client, servers=servers, *args, **kwargs)
        resp = session.get_result()
        self.check_block(resp, **kwargs)

    @log(logging.DEBUG)
    @params(num_blocks=160, block_size=2**10, num_servers=2)
    def test_multi_block(self, *args, **kwargs):
        query = [2,13,7,9]

        client, servers = self.create_nodes(**kwargs)
        session = self.make_request(client=client, servers=servers, query=query, *args, **kwargs)

        for q in range(len(query)):
            resp = session.get_result(query_num=q)
            self.check_block(resp, query=query[q], **kwargs)

    @log(logging.DEBUG)
    @params(num_blocks=160, block_size=2*10, num_servers=2)
    def test_reuse(self, *args, **kwargs):
        queries = range(100)

        client, servers = self.create_nodes(**kwargs)
        for i in range(100):
            query = queries[i % len(queries)]
            session = self.make_request(client=client, servers=servers, query=query, *args, **kwargs)
            resp = session.get_result()
            self.check_block(resp, query=query, **kwargs)

    @log(logging.DEBUG)
    @params(num_blocks=400, block_size=2*10, num_servers=2)
    def test_multi_block_reuse(self, *args, **kwargs):
        query_sizes = [2,5,9,3,6,4,29,3,5,12,7,5,6,1,3,14]
        queries = [range(i, i + query_sizes[i%len(query_sizes)], 2) for i in range(100)]

        client, servers = self.create_nodes(**kwargs)
        for i in range(100):
            query = queries[i % len(queries)]
            session = self.make_request(client=client, servers=servers, query=query, *args, **kwargs)

            for q in range(len(query)):
                resp = session.get_result(query_num=q)
                self.check_block(resp, query=query[q], **kwargs)


class TestAGPIR(unittest.TestCase):
    @log(logging.DEBUG)
    @params(num_blocks=10, block_size=100, query=0)
    @unittest.skip("")
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

class TestITPIR(unittest.TestCase, CommonTest):
    def create_server(self, num_blocks, block_size):
        return ZZPServer(db, num_blocks=num_blocks, block_size=block_size, w=8)

    def create_client(self, num_servers, num_blocks, block_size):
        return ZZPClient(num_blocks=num_blocks, block_size=block_size, num_servers=num_servers, w=8, t=1)  # TODO: Not sure about t=1, shoud be 2?


class TestChorPIR(unittest.TestCase, CommonTest):
    def create_server(self, num_blocks, block_size):
        return ChorServer(db, num_blocks=num_blocks, block_size=block_size)

    def create_client(self, num_servers, num_blocks, block_size):
        return ChorClient(num_blocks=num_blocks, block_size=block_size, num_servers=num_servers)



def gettime(f, *args, **kwargs):
    start = time.time()
    res = f(*args, **kwargs)
    return time.time() - start, res


if __name__ == '__main__':
    unittest.main()
