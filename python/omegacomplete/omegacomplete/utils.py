import struct

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
        msg += conn.recv(4096)

    return msg

def safe_sendall(conn, msg):
    data_length = struct.pack("!I", len(msg))

    conn.sendall(data_length)
    conn.sendall(msg)
