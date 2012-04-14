import socket
import sys
from omegacomplete.utils import safe_recv, safe_sendall

# these are probably global to the VIM Python interpreter, so prefix with "oc_"
# to prevent conflicts with any other Python plugins

oc_host = 'localhost'
oc_port = 31337
oc_num_connect_failures = 0
oc_conn = None

def send_command(cmd, arg):
    global oc_num_connect_failures
    global oc_conn

    if (oc_num_connect_failures > 10):
        return
        
    # if oc_conn is not made yet, then make it
    if (oc_conn == None):
        oc_conn = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        try:
            oc_conn.connect((oc_host, oc_port))
            oc_conn.setblocking(0)
        except:
            oc_num_connect_failures += 1
            return

    line = cmd + ' ' + arg
    safe_sendall(oc_conn, line)

    reply = safe_recv(oc_conn)

def get_current_buffer_contents():
    return " ".join(vim.current.buffer)
