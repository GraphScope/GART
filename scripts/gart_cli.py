#!/usr/bin/env python3

import click
import os
import json
import yaml
import socket
import requests
from urllib.parse import urlparse
import re

CONFIG_FILE_PATH = "/tmp/gart_cli_config.json"
GRAPH_ID = "graph_id"


def save_config(config):
    with open(CONFIG_FILE_PATH, "w") as f:
        json.dump(config, f)


def load_config():
    if os.path.exists(CONFIG_FILE_PATH):
        with open(CONFIG_FILE_PATH, "r") as f:
            try:
                return json.load(f)
            except json.JSONDecodeError:
                return {}
    return {}


@click.group()
@click.pass_context
def cli(ctx):
    """System Manager CLI"""
    # Load the configuration file and store it in the context
    ctx.ensure_object(dict)
    ctx.obj = load_config()


@cli.command()
@click.pass_context
@click.argument("endpoint", required=True, type=str)
def connect(ctx, endpoint):
    """Connect to a new service endpoint."""
    # Save the endpoint to the configuration file
    if not endpoint.startswith(("http://", "https://")):
        endpoint = "http://" + endpoint
    ctx.obj["endpoint"] = endpoint
    save_config(ctx.obj)
    # check if the endpoint is reachable
    parsed_url = urlparse(endpoint)
    host = parsed_url.netloc.split(":")[0]
    port = parsed_url.port
    with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as s:
        try:
            s.settimeout(1)
            s.connect((host, port))
            click.echo(f"Connected to {endpoint}")
            s.close()
        except socket.error as e:
            click.echo(f"Failed to connect to {endpoint}: {e}")
            return


@cli.command()
@click.pass_context
def resume_data_loading(ctx):
    """Resume data loading process."""
    endpoint = ctx.obj.get("endpoint")
    if not endpoint:
        click.echo('Please connect to an endpoint first using the "connect" command.')
        return

    response = requests.post(f"{endpoint}/api/v1/service/resume")
    click.echo(f"Resumed data loading: {response.text}")


@cli.command()
@click.pass_context
def pause_data_loading(ctx):
    """Pause data loading process."""
    endpoint = ctx.obj.get("endpoint")
    if not endpoint:
        click.echo('Please connect to an endpoint first using the "connect" command.')
        return

    response = requests.post(f"{endpoint}/api/v1/service/pause")
    click.echo(f"Paused data loading: {response.text}")


@cli.command()
@click.pass_context
def get_all_available_versions(ctx):
    """Get all available versions of the graph at the moment."""
    endpoint = ctx.obj.get("endpoint")
    if not endpoint:
        click.echo('Please connect to an endpoint first using the "connect" command.')
        return

    response = requests.get(f"{endpoint}/api/v1/graph/{GRAPH_ID}/versions")
    click.echo(f"Available versions: {response.text}")


@cli.command()
@click.pass_context
@click.argument("timestamp", required=True, type=str)
def get_version_by_timestamp(ctx, timestamp):
    """Get the version of the graph at the given timestamp."""
    endpoint = ctx.obj.get("endpoint")
    if not endpoint:
        click.echo('Please connect to an endpoint first using the "connect" command.')
        return
    response = requests.get(
        f"{endpoint}/api/v1/graph/{GRAPH_ID}/versions/timestamp?timestamp={timestamp}")
    click.echo(f"Version at {timestamp}: {response.text}")


@cli.command()
@click.pass_context
@click.argument("config_path", type=click.Path(exists=True))
def submit_config(ctx, config_path):
    """Submit a new configuration file."""
    endpoint = ctx.obj.get("endpoint")
    if not endpoint:
        click.echo('Please connect to an endpoint first using the "connect" command.')
        return

    with open(config_path, "r") as file:
        yaml_content = file.read()
        dict_content = yaml.load(yaml_content, Loader=yaml.FullLoader)
        graph_name = dict_content["loadingConfig"]["database"]
        payload = {
            "name": graph_name,
            "description": graph_name,
            "schema": yaml_content
        }
        try:
            response = requests.post(
                f"{endpoint}/api/v1/graph/yaml",
                headers={"Content-Type": "application/json"},
                data=json.dumps(payload)
            )
            response.raise_for_status()
            click.echo(f"Success: Server responded with {response.status_code} status")
        except requests.exceptions.HTTPError as e:
            click.echo(f"Failed to submit the configuration file: {e}")
        except requests.exceptions.RequestException as e:
            click.echo(f"Failed to submit the configuration file: {e}")
        except Exception as e:
            click.echo(f"Failed to submit the configuration file: {e}")


@cli.command()
@click.pass_context
@click.argument("config_path", type=click.Path(exists=True))
def submit_pgql_config(ctx, config_path):
    """Submit a new pgql configuration file."""
    endpoint = ctx.obj.get("endpoint")
    if not endpoint:
        click.echo('Please connect to an endpoint first using the "connect" command.')
        return

    with open(config_path, "r") as file:
        pgql_content = file.read()
        match = re.search(r'CREATE PROPERTY GRAPH (\w+)', pgql_content)
        if match:
            graph_name = match.group(1)
        else:
            click.echo("Failed to extract graph name from the pgql file.")
            graph_name = "GART_GRAPH"
        payload = {
            "name": graph_name,
            "description": graph_name,
            "schema": pgql_content
        }
        try:
            response = requests.post(
                f"{endpoint}/api/v1/graph/pgql",
                headers={"Content-Type": "application/json"},
                data=json.dumps(payload)
            )
            response.raise_for_status()
            click.echo(f"Success: Server responded with {response.status_code} status")
        except requests.exceptions.HTTPError as e:
            click.echo(f"Failed to submit the pgql configuration file: {e}")
        except requests.exceptions.RequestException as e:
            click.echo(f"Failed to submit the pgql configuration file: {e}")
        except Exception as e:
            click.echo(f"Failed to submit the pgql configuration file: {e}")


@cli.command()
@click.pass_context
@click.argument("algorithm_name")
@click.argument("graph_version", type=str, required=True)
@click.option("--arg", multiple=True, type=(str, str))
def submit_gae_task(ctx, algorithm_name, graph_version, arg):
    """Submit a new GAE task."""
    endpoint = ctx.obj.get("endpoint")
    if not endpoint:
        click.echo('Please connect to an endpoint first using the "connect" command.')
        return

    args_dict = dict(arg)
    args_dict["algorithm_name"] = algorithm_name
    args_dict["graph_version"] = graph_version

    response = requests.post(f"{endpoint}/run-gae-task", json=args_dict)

    if response.status_code == 200:
        click.echo("Algorithm executed successfully!")
        click.echo(response.text)
    else:
        click.echo("Failed to execute algorithm!")
        click.echo(f"Status code: {response.status_code}")
        click.echo(response.text)


@cli.command()
@click.pass_context
@click.argument("graph_version", type=int, required=True)
def change_graph_version_gie(ctx, graph_version):
    """Change the graph version to the given version for GIE."""
    endpoint = ctx.obj.get("endpoint")
    if not endpoint:
        click.echo('Please connect to an endpoint first using the "connect" command.')
        return

    response = requests.post(
        f"{endpoint}/api/v1/graph/{GRAPH_ID}/versions", data={"read_epoch": graph_version}
    )
    click.echo(f"Changed graph version to {graph_version}: {response.text}")


@cli.command()
@click.pass_context
def get_graph_schema(ctx):
    """Get graph schema."""
    endpoint = ctx.obj.get("endpoint")
    if not endpoint:
        click.echo('Please connect to an endpoint first using the "connect" command.')
        return

    response = requests.get(f"{endpoint}/api/v1/graph/{GRAPH_ID}/schema")
    click.echo(f"Graph schema: {response.text}")


if __name__ == "__main__":
    cli(obj={})
