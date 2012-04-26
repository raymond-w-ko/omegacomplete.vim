import socket
import sys
from omegacomplete.utils import safe_recv, safe_sendall

# these are probably global to the VIM Python interpreter, so prefix with "oc_"
# to prevent conflicts with any other Python plugins

oc_host = 'localhost'
oc_port = 31337
oc_is_disabled = False
oc_conn = None

def oc_init_connection():
    global oc_is_disabled
    global oc_conn

    try:
        oc_conn = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        oc_conn.settimeout(0.1)
        oc_conn.connect((oc_host, oc_port))

        oc_conn.setblocking(0)
    except:
        oc_is_disabled = True


def oc_send_command(cmd):
    global oc_is_disabled
    global oc_conn

    if oc_is_disabled:
        return

    try:
        safe_sendall(oc_conn, cmd)

        reply = safe_recv(oc_conn)
        return reply

    except:
        oc_is_disabled = True


def oc_get_current_buffer_contents():
    global oc_is_disabled

    if oc_is_disabled:
        return ""

    return "\n".join(vim.current.buffer)

def oc_get_current_line():
    global oc_is_disabled

    if oc_is_disabled:
        return ""

    return vim.current.line

def oc_disable_check():
    global oc_is_disabled

    if oc_is_disabled:
        return "1"
    else:
        return "0"
