import struct
import platform
import string
import time
import math

receive_buffer = ""
def safe_recvall(conn):
    global receive_buffer
    global oc_is_disabled

    # infinite loop until we find ourselves with a NULL character
    begin_time = time.time()
    while not "\x00" in receive_buffer:
        now = time.time()

        if (math.fabs(now - begin_time) >= 32.0):
            oc_is_disabled = True
            break

        data = ""

        try:
            data = conn.recv(4096)
        except:
            pass

        receive_buffer += data

    # segment buffer and save rest
    sentinel = receive_buffer.index("\x00") + 1
    # don't want the NUL byte
    msg = receive_buffer[:(sentinel - 1)]
    receive_buffer = receive_buffer[sentinel:]

    return msg

def safe_delim_sendall(conn, msg):
    # send the message, must NOT have a '\0' char in the msg
    conn.sendall(msg)

    # send a NULL char as a delimiter
    conn.sendall('\0')

def safe_header_sendall(conn, msg):
    data_length = struct.pack("=I", len(msg))
    conn.sendall(data_length)

    conn.sendall(msg)

def raw_sendall(conn, msg):
    conn.sendall(msg)

def normalize_path_separators(path):
    if (platform.system() == "Windows"):
        return string.replace(path, "/", "\\")
    else:
        return string.replace(path, "\\", "/")
