import ctypes
import os.path as op
import inspect

lib = ctypes.cdll.LoadLibrary(op.join(op.dirname(__file__),'libpypercy.so'))

default_params = {
    'num_blocks': 100,
    'block_size': 100,
    'N': 50,
    'word_size': 20
}


class BaseParams(object):
    def __init__(self, **params):
        members = [m for m in inspect.getmembers(self) if not m[0].startswith('__')]
        _params = dict(members)
        _params.update(params)

        for member in members:
            if member[0] is None and _params[member[0]] is None:
                raise InvalidParamsError("Parameter '{}' is required".format(member[0]))

        for p in _params:
            setattr(self, p, _params[p])

def get_err_message():
    lib.get_err.restype = ctypes.c_char_p
    mes = lib.get_err()
    mes = mes.decode('utf-8').strip()
    return mes

class PercyError(Exception):
    pass


class InvalidParamsError(PercyError):
    pass

class BaseClient(object):
    params_class = BaseParams

    def __init__(self, num_servers= 1, **kwargs):
        self.num_servers = num_servers
        self.params = self.params_class(**kwargs)
        self.obj = self.create_client()

        if not self.obj:
            raise PercyError(get_err_message())

    def create_client(self):
        """
        Implemented by subclass
        """

    def make_request(self, block):
        lib.client_make_request.argtypes = [ctypes.c_void_p, ctypes.c_int]
        lib.client_make_request.restype = ctypes.c_int
        return lib.client_make_request(self.obj, block)

    def parse_response(self, req_id=0):
        # print(type(req_id), type(raw_response), type(self.obj))
        lib.client_parse_response.argtypes = [ctypes.c_void_p, ctypes.c_int]
        lib.client_parse_response.restype = ctypes.c_int
        lib.client_parse_response(self.obj, req_id)

    def read_request(self, server_num=0, chunk_size=1024):
        lib.client_read_request.argtypes = [ctypes.c_void_p, ctypes.c_int, ctypes.c_char_p, ctypes.c_int]
        lib.client_read_request.restype = ctypes.c_int

        s = ctypes.create_string_buffer(chunk_size)
        read_amount = lib.client_read_request(self.obj, server_num, s, chunk_size)

        if read_amount < chunk_size:
            return s.raw[:read_amount]

        return s.raw

    def write_response(self, raw_response, server_num=0, req_id=0):
        lib.client_write_response.argtypes = [ctypes.c_void_p, ctypes.c_int, ctypes.c_char_p, ctypes.c_int]
        lib.client_write_response.restype = ctypes.c_int
        read_amount = lib.client_write_response(self.obj, server_num, raw_response, len(raw_response))

    def request_chunks(self, server_num=0):
        while True:
            buff = self.read_request(server_num=server_num)
            if not buff:
                break
            yield buff

    def get_request(self, server_num=0):
        buff = b''
        a=0
        for chunk in self.request_chunks(server_num=server_num):
            a+=1
            # print(a, chunk)
            buff += chunk
        return buff

    def get_all_requests(self):
        return [self.get_request(s) for s in range(self.num_servers)]

    def read_result(self, chunk_size=1024):
        lib.client_read_result.argtypes = [ctypes.c_void_p, ctypes.c_char_p, ctypes.c_int]
        lib.client_read_result.restype = ctypes.c_int

        s = ctypes.create_string_buffer(chunk_size)
        read_amount = lib.client_read_result(self.obj, s, chunk_size)

        if read_amount < chunk_size:
            return s.raw[:read_amount]

        return s.raw

    def result_chunks(self):
        while True:
            buff = self.read_result()
            if not buff:
                break
            yield buff

    def get_result(self):
        buff = b''
        for chunk in self.result_chunks():
            buff += chunk
        return buff


class BaseServer(object):
    params_class = BaseParams

    def __init__(self, db_file, **kwargs):
        self.params = self.params_class(**kwargs)
        self.db_file = db_file
        self.obj = self.create_server()

        if not self.obj:
            raise PercyError(get_err_message())

    def create_server(self):
        """
        Implemented by subclass
        """

    def process_request(self, request):
        lib.server_process_request.argtypes = [ctypes.c_void_p, ctypes.c_char_p, ctypes.c_int]
        lib.server_process_request.restype = ctypes.c_int
        return lib.server_process_request(self.obj, request, len(request))

    def read_response(self, chunk_size=1024):
        lib.server_read_response.argtypes = [ctypes.c_void_p, ctypes.c_char_p, ctypes.c_int]
        lib.server_read_response.restype = ctypes.c_int

        s = ctypes.create_string_buffer(chunk_size)
        read_amount = lib.server_read_response(self.obj, s, chunk_size)

        if read_amount < chunk_size:
            return s.raw[:read_amount]

        return s.raw

    def response_chunks(self):
        while True:
            buff = self.read_response()
            if not buff:
                break
            yield buff

    def get_response(self):
        buff = b''
        for chunk in self.response_chunks():
            buff += chunk
        return buff


class AGParams(BaseParams):
    num_blocks = None
    block_size = None
    w = 8
    N = 50


class AGClient(BaseClient):
    params_class = AGParams

    def create_client(self):
        p = self.params
        lib.client_ag_new.argtypes = [ctypes.c_int, ctypes.c_int, ctypes.c_int, ctypes.c_int, ctypes.c_int]
        lib.client_ag_new.restype = ctypes.c_void_p
        return lib.client_ag_new(p.num_blocks, p.block_size, p.N, p.w, 2)


class AGServer(BaseServer):
    params_class = AGParams

    def create_server(self):
        p = self.params
        lib.server_ag_new.argtypes = [ctypes.c_char_p, ctypes.c_int, ctypes.c_int, ctypes.c_int, ctypes.c_int, ctypes.c_int]
        lib.server_ag_new.restype = ctypes.c_void_p
        return lib.server_ag_new(self.db_file.encode("utf-8"), p.num_blocks, p.block_size, p.N, p.w, 1)


class ZZPParams(BaseParams):
    num_blocks = None
    block_size = None
    w = 8
    t = 1


class ZZPClient(BaseClient):
    params_class = ZZPParams

    def create_client(self):
        p = self.params
        lib.client_zzp_new.argtypes = [ctypes.c_int, ctypes.c_int, ctypes.c_int, ctypes.c_int, ctypes.c_int, ctypes.c_int, ctypes.c_bool]
        lib.client_zzp_new.restype = ctypes.c_void_p
        return lib.client_zzp_new(p.num_blocks, p.block_size, p.w, self.num_servers, p.t, 0, 0)


class ZZPServer(BaseServer):
    params_class = ZZPParams

    def create_server(self):
        p = self.params
        lib.server_ag_new.argtypes = [ctypes.c_char_p, ctypes.c_int, ctypes.c_int, ctypes.c_int, ctypes.c_int, ctypes.c_int, ctypes.c_bool]
        lib.server_ag_new.restype = ctypes.c_void_p
        return lib.server_zzp_new(self.db_file.encode("utf-8"), p.num_blocks, p.block_size, p.w)
