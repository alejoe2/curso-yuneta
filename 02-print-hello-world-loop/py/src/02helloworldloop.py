#!/usr/bin/env python

import time
import argparse

##########################
#       Main
##########################
def main():
    #  Set Vars
    program_version = "Hello World Loop 1.0.0"
    repeat = 100

    #  Parse arguments
    parser = argparse.ArgumentParser(description="Command Line Interface Hello World")
    parser.add_argument("-v", "--version", help="Print program version", action="version", version=program_version)
    group1 = parser.add_argument_group('options for execution')
    group1.add_argument("-r", "--repeat", help="Repeat execution 'Hello World' times. Default repeat=100", type=int, default=100)
    args = parser.parse_args()

    if args.repeat:
        repeat = args.repeat

    showhelloloop(repeat)

##########################
#      Showhelloloop
##########################
def showhelloloop(repeat):
    start_time = time.time()
    
    for i in range(1, repeat+1):
        print("Hello World:", i)
    
    end_time = time.time()

    execution_time = end_time - start_time

    print("Total time taken by CPU:", round(execution_time, 5), "s. Total loops:", repeat)

if __name__ == "__main__":
    main()
