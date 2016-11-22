from requests import Request, Session
import requests
from pypercy import *


a = ZPercyClient()
a.make_request(0)

print(requests.post('http://127.0.0.1:8000', data=a.request_chunks()).text)
