import socket
import sys
import socket_helper
from omegacomplete.utils import safe_recv, safe_sendall

HOST = 'localhost'
PORT = 31337

def send_command(cmd, arg):
    s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    s.settimeout(0.1)
    try:
        s.connect((HOST, PORT))
    except:
        return

    line = cmd + ' ' + arg
    safe_sendall(s, line)

    reply = safe_recv(s)

    s.close()

def get_current_buffer_contents():
    return " ".join(vim.current.buffer)


def main(*args):
    send_command('complete_word', 'Sy')
    send_command('add_text', 'Sy')
    send_command('exit', 'Sy')
    return 0
