#!/usr/bin/env python3

from flask import Flask
import subprocess
import os

app = Flask(__name__)
port = int(os.getenv("CONTROLLER_FLASK_PORT", 5000))


@app.route("/control/pause", methods=["POST"])
def pause():
    subprocess.run(
        [
            "/bin/bash",
            "-c",
            "/workspace/gart/scripts/pause_resume_data_processing.sh pause",
        ]
    )
    return "Paused", 200


@app.route("/control/resume", methods=["POST"])
def resume():
    subprocess.run(
        [
            "/bin/bash",
            "-c",
            "/workspace/gart/scripts/pause_resume_data_processing.sh resume",
        ]
    )
    return "Resumed", 200


if __name__ == "__main__":
    app.run(host="0.0.0.0", port=port)
