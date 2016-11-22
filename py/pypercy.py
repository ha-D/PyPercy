import ctypes
import os.path as op

lib = ctypes.cdll.LoadLibrary(op.join(op.dirname(__file__),'libpypercy.so'))

default_params = {
    'num_blocks': 100,
    'block_size': 100,
    'N': 50,
    'word_size': 20
}


class ZAGClient(object):
    def __init__(self, **kwargs):
        self.params = params = default_params.copy()
        self.params.update(kwargs)


        lib.client_new.argtypes = [ctypes.c_int, ctypes.c_int, ctypes.c_int, ctypes.c_int, ctypes.c_int]
        lib.client_new.restype = ctypes.c_void_p

        self.obj = lib.client_new(params['num_blocks'], params['block_size'], params['N'], params['word_size'], 2)

    def make_request(self, block):
        lib.client_make_request.argtypes = [ctypes.c_void_p, ctypes.c_int]
        lib.client_make_request.restype = ctypes.c_int
        return lib.client_make_request(self.obj, block)

    def parse_response(self, raw_response, req_id=0):
        print(type(req_id), type(raw_response), type(self.obj))
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

class ZAGServer(object):
    def __init__(self, db_file, db_offset=0, **kwargs):
        lib.server_new.argtypes = [ctypes.c_char_p, ctypes.c_int, ctypes.c_int, ctypes.c_int, ctypes.c_int, ctypes.c_int]
        lib.server_new.restype = ctypes.c_void_p
        self.params = params = default_params.copy()
        self.params.update(kwargs)
        self.obj = lib.server_new(db_file.encode("utf-8"), db_offset, params['num_blocks'], params['block_size'], params['N'], params['word_size'])

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
