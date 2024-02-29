import argparse
import ast
import networkx as nx

from gart import DiGraph


def get_parser():
    parser = argparse.ArgumentParser(
        description="NX Algorithm on GART DiGraph",
        formatter_class=argparse.ArgumentDefaultsHelpFormatter,
    )

    parser.add_argument(
        "--host", default="127.0.0.1", help="NetworkX server host address"
    )
    parser.add_argument("--port", default=50051, help="NetworkX server port")
    parser.add_argument(
        "--algorithm", default="sssp", help="NetworkX algorithm (sssp, ...)"
    )
    parser.add_argument(
        "--args", default="(0, 1)", help="Args for the algorithm (e.g. (0, 1) for sssp)"
    )
    parser.add_argument("--output", default="output", help="Output file name")

    return parser


if __name__ == "__main__":
    args = get_parser().parse_args()
    g = DiGraph(f"{args.host}:{args.port}")

    if args.algorithm.lower() == "sssp":
        source = ast.literal_eval(args.args)
        length = nx.single_source_shortest_path_length(g, source)

        with open(args.output, "w", encoding="UTF-8") as f:
            for node, dist in length.items():
                f.write(f"{node}: {dist}\n")
