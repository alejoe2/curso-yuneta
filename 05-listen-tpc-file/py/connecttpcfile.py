#!/usr/bin/env python

import argparse
import socket
import os

try:
    from . import __version__
except:  # pragma: no cover
    __version__ = '1.0.0'

##########################
#       Main
##########################
def main():
    #  Set Vars
    port = 7000
    ip = "127.0.0.1"
    repeat = 1

    #  Parse arguments
    parser = argparse.ArgumentParser(description="Command Line Connect to tpc port")
    parser.add_argument("-v", "--version", help="Print program version", action="version", version=__version__)
    group1 = parser.add_argument_group('options for execution')
    group1.add_argument("-p", "--port", help="define port to use, default port=7000", type=int, default=port)
    group1.add_argument("-i", "--ip", help="define ip to use, default ip='127.0.0.1'", type=str, default=ip)
    group1.add_argument("-r", "--repeat", help="Repeat execution 'Hello World' times. Default repeat=2", type=int, default=repeat)
    group1.add_argument("FILE", help="FILE to send, default=file.txt ", type=str)
    args = parser.parse_args()

    if args.port:
        port = args.port
    if args.ip:
        ip = args.ip
    if args.repeat:
        repeat = args.repeat

    connectport(port, ip, args.FILE, repeat)


##########################
#      Connection Port
##########################
def connectport(port, ip, FILE, repeat):

    # Create a socket object
    client_socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)

    # Connect to the server
    client_socket.connect((ip, port))

    local_name = client_socket.getsockname()
    print("Local socket name:", local_name)

    peer_name = client_socket.getpeername()
    print("Peer socket name:", peer_name)

    for i in range(1, repeat+1):

        print("Send file:", FILE)

        # Envía longitud nombre fichero
        file_name_len = len(FILE)
        to_send = file_name_len.to_bytes(4, byteorder='big')
        client_socket.sendall(to_send)

        # Envía longitud fichero
        file_size = os.path.getsize(FILE)
        print("file_size:", file_size)
        to_send = file_size.to_bytes(4, byteorder='big')
        client_socket.sendall(to_send)

        # Envía nombre fichero
        client_socket.sendall(FILE.encode())

        # Envía fichero
        with open(FILE, 'rb') as f:
            client_socket.sendall(f.read())

        #confirmation = client_socket.recv(1024)
        #print(f'Received: {confirmation.decode()}')

    # close the socket
    client_socket.close()


##########################
#
##########################
if __name__ == "__main__":
    main()
