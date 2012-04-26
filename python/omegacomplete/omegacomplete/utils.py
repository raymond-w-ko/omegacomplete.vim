import struct
import platform
import string
import time

receive_buffer = ""
def safe_recv(conn):
    global receive_buffer

    # infinite loop until we find ourselves with a NULL character
    data = ""
    while not "\x00" in receive_buffer:
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

def safe_sendall(conn, msg):
    # send the message, must NOT have a '\0' char in the msg
    conn.sendall(msg)

    # send a NULL char as a delimiter
    delimiter = '\0'
    conn.sendall(delimiter)

def normalize_path_separators(path):
    if (platform.system() == "Windows"):
        return string.replace(path, "/", "\\")
    else:
        return string.replace(path, "\\", "/")
