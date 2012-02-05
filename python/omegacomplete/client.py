import socket
import sys
import socket_helper

HOST = 'localhost'
PORT = 31337

print(sys.path)

def send_command(cmd, arg):
    s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    try:
        s.connect((HOST, PORT))
    except:
        return
    #print('you are now connected to ', s.getsockname())
    line = cmd + ' ' + arg
    s.sendall(bytes(line, 'utf-8'))
    reply = s.recv(1024).decode('utf-8')
    #print('reply was: ', reply)
    s.close()

def main(*args):
    #send_command('complete_word', 'Sy')
    #send_command('add_text', 'Sy')
    #send_command('exit', 'Sy')
    return 0
