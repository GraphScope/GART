#!/usr/bin/env python3

import click
import os
import json
import socket
import requests
from urllib.parse import urlparse

CONFIG_FILE_PATH = "/tmp/gart_cli_config.json"


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

    response = requests.post(f"{endpoint}/control/resume")
    click.echo(f"Resumed data loading: {response.text}")


@cli.command()
@click.pass_context
def pause_data_loading(ctx):
    """Pause data loading process."""
    endpoint = ctx.obj.get("endpoint")
    if not endpoint:
        click.echo('Please connect to an endpoint first using the "connect" command.')
        return

    response = requests.post(f"{endpoint}/control/pause")
    click.echo(f"Paused data loading: {response.text}")


@cli.command()
@click.pass_context
def get_all_available_versions(ctx):
    """Get all available versions of the graph at the moment."""
    endpoint = ctx.obj.get("endpoint")
    if not endpoint:
        click.echo('Please connect to an endpoint first using the "connect" command.')
        return

    response = requests.post(f"{endpoint}/get-all-available-read-epochs")
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
    response = requests.post(
        f"{endpoint}/get-read-epoch-by-timestamp", data={"timestamp": timestamp}
    )
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

    with open(config_path, "rb") as file:
        files = {"file": (config_path, file)}
        try:
            response = requests.post(f"{endpoint}/submit-config", files=files)
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
        f"{endpoint}/change-read-epoch", data={"read_epoch": graph_version}
    )
    click.echo(f"Changed graph version to {graph_version}: {response.text}")


if __name__ == "__main__":
    cli(obj={})
