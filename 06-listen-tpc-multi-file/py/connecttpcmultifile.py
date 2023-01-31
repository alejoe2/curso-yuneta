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
    group1.add_argument("FILES", help="FILES to send, defaul=listfiles.txt ", type=str)
    args = parser.parse_args()

    if args.port:
        port = args.port
    if args.ip:
        ip = args.ip

    connectport(port, ip, args.FILES)

##########################
#      Conection Port
##########################
def connectport(port, ip, FILES):
    
    # Create a socket object
    client_socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)

    # Connect to the server
    client_socket.connect((ip, port))

    # open list files to be sent
    files = open(FILES, "rb")

    listFile=files.read().decode().split("\n")

    files.close()
    
    for i, filepath in enumerate(listFile):
        
        if filepath !=  "":
            print("Send: ", filepath)
            
            try:
                # open the file to be sent and send
                with open(filepath, 'rb') as f:
                    file_data = (filepath+'\0').encode() + f.read()
                    client_socket.sendall(file_data)
                # receive confirmation
                if(i < len(listFile)-1):
                    confirmation = client_socket.recv(1024)
                    print(f'Received: {confirmation.decode()}')
            
            except FileNotFoundError:
                print("The file is not found\n")
    
    # close the socket and file
    client_socket.close()
    
##########################
#      
##########################
if __name__ == "__main__":
    main()
