import socket
import sys
import socket_helper
from omegacomplete.utils import safe_recv, safe_sendall

HOST = 'localhost'
PORT = 31337
NUM_CONNECT_FAILURES = 0

def send_command(cmd, arg):
    global NUM_CONNECT_FAILURES

    if (NUM_CONNECT_FAILURES > 10):
        return

    s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    s.settimeout(0.1)
    try:
        s.connect((HOST, PORT))
    except:
        NUM_CONNECT_FAILURES += 1
        return

    line = cmd + ' ' + arg
    safe_sendall(s, line)

    reply = safe_recv(s)
    if reply == "unhandled":
        print("server did not know how to handle request")

    s.close()

def get_current_buffer_contents():
    return " ".join(vim.current.buffer)
