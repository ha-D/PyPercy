import ctypes
import os.path as op

lib = ctypes.cdll.LoadLibrary(op.join(op.dirname(__file__),'libpypercy.so'))

default_params = {
    'num_blocks': 100,
    'block_size': 100,
    'N': 50,
    'word_size': 20
}


class ZBaseClient(object):
    def __init__(self, **params):
        # self.params = params = default_params.copy()
        # self.params.update(kwargs)
        self.params = params
        self.obj = self.create_client(self.params)

    def create_client(self, params):
        """
        Implemented by subclass
        """

    def make_request(self, block):
        lib.client_make_request.argtypes = [ctypes.c_void_p, ctypes.c_int]
        lib.client_make_request.restype = ctypes.c_int
        return lib.client_make_request(self.obj, block)

    def parse_response(self, raw_response, req_id=0):
        # print(type(req_id), type(raw_response), type(self.obj))
        lib.client_parse_response.argtypes = [ctypes.c_void_p, ctypes.c_int, ctypes.c_char_p, ctypes.c_int]
        lib.client_parse_response.restype = ctypes.c_int
        lib.client_parse_response(self.obj, req_id, raw_response, len(raw_response))

    def read_request(self, chunk_size=1024):
        lib.client_read_request.argtypes = [ctypes.c_void_p, ctypes.c_char_p, ctypes.c_int]
        lib.client_read_request.restype = ctypes.c_int

        s = ctypes.create_string_buffer(chunk_size)
        read_amount = lib.client_read_request(self.obj, s, chunk_size)

        if read_amount < chunk_size:
            return s.raw[:read_amount]

        return s.raw

    def request_chunks(self):
        while True:
            buff = self.read_request()
            if not buff:
                break
            yield buff

    def get_request(self):
        buff = b''
        for chunk in self.request_chunks():
            buff += chunk
        return buff

    def read_response(self, chunk_size=1024):
        lib.client_read_response.argtypes = [ctypes.c_void_p, ctypes.c_char_p, ctypes.c_int]
        lib.client_read_response.restype = ctypes.c_int

        s = ctypes.create_string_buffer(chunk_size)
        read_amount = lib.client_read_response(self.obj, s, chunk_size)

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


class ZBaseServer(object):
    def __init__(self, db_file, db_offset=0, **params):
        # self.params = params = default_params.copy()
        # self.params.update(kwargs)
        self.params = params
        self.db_file = db_file
        self.db_offset = db_offset

        self.obj = self.create_server(db_file, db_offset, params)

    def create_server(self, db_file, db_offset, params):
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

class InvalidParamsError(Exception):
    pass

class ZAGClient(ZBaseClient):
    def __init__(self, num_blocks=100, block_size=100, N=50, word_size=20):
        super().__init__(num_blocks=num_blocks, block_size=block_size, N=N, word_size=word_size)

    def create_client(self, params):
        lib.client_ag_new.argtypes = [ctypes.c_int, ctypes.c_int, ctypes.c_int, ctypes.c_int, ctypes.c_int]
        lib.client_ag_new.restype = ctypes.c_void_p
        return lib.client_ag_new(params['num_blocks'], params['block_size'], params['N'], params['word_size'], 2)


class ZAGServer(ZBaseServer):
    def __init__(self, db_file, db_offset=0, num_blocks=100, block_size=100, N=50, word_size=20):
        super().__init__(db_file, db_offset=db_offset, num_blocks=num_blocks, block_size=block_size, N=N, word_size=word_size)

    def create_server(self, db_file, db_offset, params):

        lib.server_ag_new.argtypes = [ctypes.c_char_p, ctypes.c_int, ctypes.c_int, ctypes.c_int, ctypes.c_int, ctypes.c_int]
        lib.server_ag_new.restype = ctypes.c_void_p
        return lib.server_ag_new(db_file.encode("utf-8"), db_offset, params['num_blocks'], params['block_size'], params['N'], params['word_size'])


class ZITZZPClient(ZBaseClient):
    def __init__(self, num_servers, t=2, num_blocks=100, block_size=100, w=8, tau=0, spir=False):
        self.validate_word_size(word_size)
        super().__init__(num_servers=num_servers, t=t, num_blocks=num_blocks, block_size=block_size, w=w, tau=tau, spir=False)

    def create_client(self, params):
        lib.client_zzp_new.argtypes = [ctypes.c_int, ctypes.c_int, ctypes.c_int, ctypes.c_int, ctypes.c_int, ctypes.c_int, ctypes.c_bool]
        lib.client_zzp_new.restype = ctypes.c_void_p
        return lib.client_zzp_new(params['num_servers'], params['t'], params['num_blocks'], params['block_size'], params['w'], params['tau'], params['spir'])

    def validate_word_size(self, word_size):
        valid_word_sizes = [8, 16, 32, 96, 128, 160, 196, 256, 1024, 1536, 2048]
        if word_size not in valid_word_sizes:
            raise InvalidParamsError("Invalid word_size, must be one of {}".format(valid_word_sizes))


class ZITZZPServer(ZBaseServer):
    def __init__(self, db_file, db_offset=0, num_blocks=100, block_size=100, word_size=20, tau=0, spir=False):
        self.validate_word_size(word_size)
        super().__init__(db_file=db_file, db_offset=db_offset, num_blocks=num_blocks, block_size=block_size, word_size=word_size, tau=tau, spir=False)

    def create_server(self, db_file, db_offset, params):
        lib.server_ag_new.argtypes = [ctypes.c_char_p, ctypes.c_int, ctypes.c_int, ctypes.c_int, ctypes.c_int, ctypes.c_int, ctypes.c_bool]
        lib.server_ag_new.restype = ctypes.c_void_p
        return lib.server_zzp_new(db_file.encode("utf-8"), db_offset, params['num_blocks'], params['block_size'], params['word_size'], params['tau'], params['spir'])

    def validate_word_size(self, word_size):
        valid_word_sizes = [8, 16, 32, 96, 128, 160, 196, 256, 1024, 1536, 2048]
        if word_size not in valid_word_sizes:
            raise InvalidParamsError("Invalid word_size, must be one of {}".format(valid_word_sizes))
