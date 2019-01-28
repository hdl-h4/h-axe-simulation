#! /usr/bin/env python3

import json
import os.path
from pandas import read_csv

from numpy import arange
from os import system as os_sys
from sys import argv
from sys import stderr
from sys import stdout


# Arguments
from argparse import ArgumentParser
def parse_cmd(argv=None):
    parser = ArgumentParser(
        description='Plot CPU & network utilization from simulation logs.')
    parser.add_argument('input', type=str, nargs='+', help='The input data directory or file.')
    parser.add_argument('-o','--output',type=str,dest='output',help='The output directory.')
    return parser.parse_args(argv)

# No X11
from matplotlib import use as pltUse
pltUse('Agg')
from matplotlib import pyplot as plt

class Painter:
    def __init__(self, args):
        self.columns = ['CPU', 'MEMORY', 'DISKIO', 'NETWORK']
        self.figsize = (8, 6)
        self.xlabel = "Time"
        self.ylabel = "ResourceUtilization"
        self.linewidth = 1
        self.in_files = args.input
        self.output = args.output

        # Retrieve input files
        self.in_files = []
        for input in args.input:
            if os.path.isdir(input):
                self.in_files.extend([
                    os.path.join(input, f) for f in os.listdir(input)
                    if os.path.isfile(os.path.join(input, f))
                ])
            elif os.path.isfile(input):
                self.in_files.append(input)
            else:
                stderr.write("Input path %s does not exist\n" % input)
        if len(self.in_files) is 0:
            exit(0)

    def plot_all(self):
        for in_file in self.in_files:
            if not in_file.endswith('.csv'):
                continue
            # load data
            data = read_csv(in_file, sep="\t", comment='#', names=self.columns)
            print(data)
            # plot
            output_file = os.path.join(self.output, os.path.splitext(os.path.basename(in_file))[0] + '.png')
            self.plot(len(data), data[:][self.columns], output_file)

    def plot(self, size, df, output_file):
        fig, ax = plt.subplots(figsize=self.figsize)
        ax.set_xlabel(self.xlabel)
        ax.set_ylabel(self.ylabel)
        ax.set_xlim([0, size-1])
        ax.set_ylim([0, 1])

        # TODO xlim
        df.plot(linewidth=self.linewidth, ax=ax)
        lgd = plt.legend(bbox_to_anchor=(1, 1), loc=1, borderaxespad=0.1)
        plt.savefig(output_file, bbox_extra_artists=(lgd, ), bbox_inches='tight')
        print("Saved fig {0}".format(output_file))
        plt.close()


def main(args):
    # Create output dir in case it does not exist
    os_sys('mkdir -p %s' % args.output)

    Painter(args).plot_all()


if __name__ == "__main__":
    args = parse_cmd(argv[1:])
    main(args)
