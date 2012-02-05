import socket
import sys

def main(*args):
    HOST = 'localhost'
    PORT = 31337

    s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    s.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
    s.bind((HOST, PORT))
    s.listen(5)
    while True:
        print('now listenting at: ', s.getsockname())
        sc, sockname = s.accept()
        print('we have accepted a connection from ', sockname)
        print(sc.getsockname(), 'is now connected to ', sc.getpeername())
        message = sc.recv(1024).decode('utf-8')
        print('The client wants to perform the operation: ' + message)
        message = message.split()
        if len(message) == 0:
            continue
        if (message[0] == 'exit'):
            break
        else:
            print('received message: ', message[1])
        sc.sendall(bytes('received', 'utf-8'))
        sc.close()
    return 0

if __name__ == '__main__':
    sys.exit(main(*sys.argv))
