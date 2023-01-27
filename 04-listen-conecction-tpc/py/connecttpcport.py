#!/usr/bin/env python

import argparse
import socket

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

    #  Parse arguments
    parser = argparse.ArgumentParser(description="Command Line Interface Hello World")
    parser.add_argument("-v", "--version", help="Print program version", action="version", version=__version__)
    group1 = parser.add_argument_group('options for execution')
    group1.add_argument("-p", "--port", help="define port to use, default port=7000", type=int, default=port)
    group1.add_argument("-i", "--ip", help="define ip to use, default ip='127.0.0.1'", type=str, default=ip)
    group1.add_argument("MESSAGE", help="MESSAGE to send", type=str)
    args = parser.parse_args()

    if args.port:
        port = args.port
    if args.ip:
        ip = args.ip

    connectport(port, ip, args.MESSAGE)

##########################
#      Conection Port
##########################
def connectport(port, ip, MESSAGE):
    
    # Create a socket object
    client_socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)

    # Connect to the server
    client_socket.connect((ip, port))

    # Send data to the server
    client_socket.sendall(MESSAGE.encode())

    # Close the socket
    client_socket.close()

##########################
#      
##########################
if __name__ == "__main__":
    main()
