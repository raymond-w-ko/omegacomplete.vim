import socket
import sys
from omegacomplete.utils import safe_recv, safe_sendall

HOST = 'localhost'
PORT = 31337
NUM_CONNECT_FAILURES = 0
CONNECTION = None

def send_command(cmd, arg):
    global NUM_CONNECT_FAILURES
    global CONNECTION

    if (NUM_CONNECT_FAILURES > 10):
        return
        
    # if connection is not made yet, then make it
    if (CONNECTION == None):
        CONNECTION = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        try:
            CONNECTION.connect((HOST, PORT))
            CONNECTION.setblocking(0)
        except:
            NUM_CONNECT_FAILURES += 1
            return

    line = cmd + ' ' + arg
    safe_sendall(CONNECTION, line)

    reply = safe_recv(CONNECTION)

def get_current_buffer_contents():
    return " ".join(vim.current.buffer)
