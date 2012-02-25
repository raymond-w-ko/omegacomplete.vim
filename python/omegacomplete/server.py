import socket
import sys
import string
from omegacomplete.utils import safe_recv, safe_sendall

class OmegaCompleteServer:
    def __init__(self):
        pass

    def handle(self, cmd, arg):
        return cmd + "( " + arg + " )"

def main(*args):
    HOST = "localhost"
    PORT = 31337
    ocs = OmegaCompleteServer()

    s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    s.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
    s.bind((HOST, PORT))
    s.listen(5)
    while True:
        (conn, address) = s.accept()
        msg = safe_recv(conn)
        if len(msg) == 0:
            continue

        print("msg == " + msg)
        msg = string.split(msg, None, 1)

        if (msg[0] == "exit" or msg[0] == "quit"):
            safe_sendall(conn, "exiting")
            conn.close()
            break

        if (len(msg) != 2):
            safe_sendall(conn, "invalid command")
            conn.close()
            break

        safe_sendall(conn, ocs.handle(msg[0], msg[1]))
        conn.close()

    return 0

if __name__ == '__main__':
    sys.exit(main(*sys.argv))
