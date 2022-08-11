#! /usr/bin/env python3

"""
Designed to gather multiple Pareto points into a single Pareto list;
 it's a simple filter - stdin to stdout,
 but has options to specify input and output files as well as output format

input is a sequence of lines of the form (Pareto output from minimization)
  x1^y1;x2^y2;...
output format is specified by the -f or --format option
"""

from argparse import ArgumentParser
from argparse import RawTextHelpFormatter # to allow newlines in help messages
import sys
import copy

PARETO_SEPARATOR = '^'

def parse_arguments():
    parser = ArgumentParser(formatter_class=RawTextHelpFormatter,
                            description='Usage: collectParetoPoints.py [options] < INPUT > OUTPUT\n'
                            + ' INPUT is a sequence of lines of the form x_1^y_1;x_2^y_2;...\n'
                            + '       each x_i^y_i pair is a Pareto optimum for some pair of objectives\n'
                            + '       matching the output format of the minimization program\n'
                            + ' computes Pareto optima based on the Pareto points in the input'
                            )
    parser.add_argument("-i", "--input",
                        help="input file, so don't use stdin"
                        )
    parser.add_argument("-o", "--output",
                        help="output file, so don't use stdout"
                        )
    parser.add_argument("-f", "--format",
                        choices=['list (default)', 'lines', 'csv'],
                        default='list',
                        help="output format\n"
                            + "  list = same as input\n"
                            + "  lines = each line is a Pareto optimum, values tab separated\n"
                            + "  csv = each line is a Pareto optimum, values comma separated"
                        )
    parser.add_argument("-H", "--header",
                        help="add a header (only relevant for lines or csv format)\n"
                           + " argument is of the form 'objective_1,objective_2', used directly with csv\n"
                           + " converted to tab separated for lines"
                        )
    args = parser.parse_args()
    return args



"""
@return a list of Pareto points in the form [(x1,y1), ...]
"""
def read_points(input):
    pareto_list = []
    line = input.readline().strip()
    while line:
        pareto_list.extend(process_line(line))
        line = input.readline().strip()
    return pareto_list

"""
@param pareto_input a string of the form 'x1^y1;x2^y2;...'
@return a list of the form [(x1,y1), (x2,y2), ...]
"""
def process_line(pareto_input):
    pareto_list = []
    points = pareto_input.split(';')
    for point in points:
        x, y = point.split(PARETO_SEPARATOR)
        pareto_list.append((x, y))
    return pareto_list


"""
@return point_list with points that are dominated by others removed; a
        point (y,z) is dominated if there exists another point (w,x)
        such that w <= y and x <= z
@param point_list a list of the form [(x1,y1), (x2,y2), ...]
"""
def gather_pareto_points(point_list):
    undominated_list = []
    for point in point_list:
        dominated = False
        # deep copy may not be necessay but doesn't hurt
        for undominated_point in copy.deepcopy(undominated_list):
            if dominates(undominated_point, point):
                dominated = True
                break
            elif dominates(point, undominated_point):
                undominated_list.remove(undominated_point)
        if not dominated:
            undominated_list.append(point)
    return undominated_list
                

# @return true if first_point dominates second_point
def dominates(first_point, second_point):
    # convert points to numbers here only

    if float(first_point[0]) <= float(second_point[0]) \
            and float(first_point[1]) <= float(second_point[1]):
        return True
    return False

"""
prints the pareto points in the input format on the output_stream
@param pareto_list a list of pairs
"""
def print_pareto_list(output_stream, pareto_list):
    point = pareto_list[0]
    output_stream.write("{}^{}".format(point[0], point[1]))
    for point in pareto_list[1:]:
        output_stream.write(";{}^{}".format(point[0], point[1]))
    output_stream.write("\n")

"""
prints the pareto points, one per line, on the output_stream
@param pareto_list a list of pairs
@param delimiter the delimiter to put between items of the pair
"""
def print_points(output_stream, pareto_list, delimiter):
    for point in pareto_list:
        output_stream.write("{}{}{}\n".format(point[0], delimiter, point[1]))

if __name__ == '__main__':
    print("starting")
    args = parse_arguments()
    input_stream = sys.stdin
    if args.input:
        input_stream = open(args.input, 'r')
    pareto_list = read_points(input_stream)
    pareto_list = gather_pareto_points(pareto_list)
    output_stream = sys.stdout
    if args.output:
        output_stream = open(args.output, 'w')
    if args.header:
        header_list = args.header.split(',')
        if len(header_list) != 2:
            sys.stderr.write("bad header {}; requires two comma separated strings\n".format(args.header))
            sys.exit()
        if args.format == "lines":
            header = '\t'.join(header_list)
        elif args.format == "csv":
            header = ','.join(header_list)
        output_stream.write("{}\n".format(header))
    if args.format == 'list':
        print_pareto_list(output_stream, pareto_list)
    elif args.format == 'lines':
        print_points(output_stream, pareto_list, '\t')
    elif args.format == 'csv':
        print_points(output_stream, pareto_list, ',')
    else:
        sys.stderr.write("Bad output format {}\n".format(args.format))
