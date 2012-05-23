import socket
import sys
import struct
import timeit
from omegacomplete.utils import safe_recvall, safe_delim_sendall
from omegacomplete.utils import safe_header_sendall, raw_sendall

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
        oc_conn.setsockopt(socket.IPPROTO_TCP, socket.TCP_NODELAY, 1)
    except:
        oc_is_disabled = True


def oc_send_command(cmd):
    global oc_is_disabled
    global oc_conn

    if oc_is_disabled:
        return

    try:
        safe_header_sendall(oc_conn, cmd)

        reply = safe_recvall(oc_conn)
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
        return '1'
    else:
        return '0'

def oc_get_cursor_pos():
    row = str(vim.current.window.cursor[0])
    col = str(vim.current.window.cursor[1])
    return row + ' ' + col

def oc_send_current_buffer():
    global oc_is_diabled
    if oc_is_disabled:
        return

    global oc_conn

    command = "buffer_contents "
    buffer_contents = '\n'.join(vim.current.buffer)
    data_length = struct.pack("=I", len(command) + len(buffer_contents))

    raw_sendall(oc_conn, data_length)
    raw_sendall(oc_conn, command)
    raw_sendall(oc_conn, buffer_contents)

    reply = safe_recvall(oc_conn)
    return reply
