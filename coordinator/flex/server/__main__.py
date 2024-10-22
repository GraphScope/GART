#!/usr/bin/env python3

import connexion

from flex.server import encoder
from datetime import datetime


def main():
    current_timestamp = datetime.now().strftime('%Y-%m-%d %H:%M:%S')
    with open("/tmp/cluster_create_time.txt", "w") as f:
        f.write(current_timestamp)
    app = connexion.App(__name__, specification_dir='./openapi/')
    app.app.json_encoder = encoder.JSONEncoder
    app.add_api('openapi.yaml',
                arguments={'title': 'GraphScope FLEX HTTP SERVICE API'},
                pythonic_params=True)

    app.run(port=18080)


if __name__ == '__main__':
    main()
