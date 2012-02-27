import struct
import platform
import string

def safe_recv(conn):
    "This function guarantees the receival of variable length data over sockets."
    data_length = ""
    while len(data_length) != 4:
        piece = conn.recv(1)
        if len(piece) == 0:
            return ""
        data_length += piece

    assert len(data_length) == 4

    data_length = struct.unpack("!I", data_length)[0]

    msg = ""
    while len(msg) != data_length:
        msg += conn.recv( min(4096, data_length) )

    assert len(msg) == data_length

    return msg

def safe_sendall(conn, msg):
    data_length = struct.pack("!I", len(msg))

    conn.sendall(data_length)
    conn.sendall(msg)

def normalize_path_separators(path):
    if (platform.system() == "Windows"):
        return string.replace(path, "/", "\\")
    else:
        return string.replace(path, "\\", "/")

def log(msg):
    print msg
