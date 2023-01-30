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
    parser = argparse.ArgumentParser(description="Command Line Connect to tpc port")
    parser.add_argument("-v", "--version", help="Print program version", action="version", version=__version__)
    group1 = parser.add_argument_group('options for execution')
    group1.add_argument("-p", "--port", help="define port to use, default port=7000", type=int, default=port)
    group1.add_argument("-i", "--ip", help="define ip to use, default ip='127.0.0.1'", type=str, default=ip)
    group1.add_argument("FILE", help="FILE to send, defaul=file.txt ", type=str)
    args = parser.parse_args()

    if args.port:
        port = args.port
    if args.ip:
        ip = args.ip

    connectport(port, ip, args.FILE)

##########################
#      Conection Port
##########################
def connectport(port, ip, FILE):
    
    # Create a socket object
    client_socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)

    # Connect to the server
    client_socket.connect((ip, port))

    # open the file to be sent
    file = open(FILE, "rb")

    # read the file and send it through the socket
    file_data = (FILE+'\0').encode() + file.read()
    while file_data:
        client_socket.sendall(file_data)
        file_data = file.read()

    # close the socket and file
    client_socket.close()
    file.close()
    

##########################
#      
##########################
if __name__ == "__main__":
    main()
