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
    parser.add_argument("--output", default="output", help="Output file name")

    subparsers = parser.add_subparsers(dest="algorithm", help="graph algorithm to run")

    parser_sssp = subparsers.add_parser("sssp", help="Single Source Shortest Path")
    parser_sssp.add_argument(
        "--source", default="(0, 0)", help="The source node id (default: %(default)s)"
    )

    parser_wsssp = subparsers.add_parser(
        "wsssp", help="Weighted Single Source Shortest Path"
    )
    parser_wsssp.add_argument(
        "--source", default="(0, 0)", help="The source node id (default: %(default)s)"
    )
    parser_wsssp.add_argument(
        "--weight",
        default="wa_work_from",
        help="The weight property (default: %(default)s)",
    )

    return parser


if __name__ == "__main__":
    args = get_parser().parse_args()
    g = DiGraph(f"{args.host}:{args.port}")

    if args.algorithm.lower() == "sssp":
        source = ast.literal_eval(args.source)
        print(f"Running Single Source Shortest Path algorithm on {source} ...")
        length = nx.single_source_shortest_path_length(g, source)

        with open(args.output, "w", encoding="UTF-8") as f:
            for node, dist in length.items():
                f.write(f"{node}: {dist}\n")

    if args.algorithm.lower() == "wsssp":
        source = ast.literal_eval(args.source)
        print(
            f"Running Weighted Single Source Shortest Path algorithm on {source} by {args.weight} ..."
        )
        length = nx.single_source_dijkstra_path_length(g, source, weight=args.weight)

        with open(args.output, "w", encoding="UTF-8") as f:
            for node, dist in length.items():
                f.write(f"{node}: {dist}\n")
