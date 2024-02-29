from setuptools import setup, find_packages
from distutils.cmd import Command
import os
import sys
import subprocess

pkg_root = os.path.dirname(os.path.abspath(__file__))
class BuildProto(Command):
    description = "build protobuf file"
    user_options = []

    def initialize_options(self):
        pass

    def finalize_options(self):
        pass

    def run(self):
        cmd = [
            sys.executable,
            os.path.join(
                pkg_root,
                "..",
                '..',
                "proto",
                "proto_generator.py",
            ),
            os.path.join(pkg_root, "src", "gart", "proto"),
            "--python",
        ]
        print(" ".join(cmd))
        subprocess.check_call(
            cmd,
            env=os.environ.copy(),
        )

setup(
    name="gart",
    version='0.1',
    description="GART: Graph Analysis on Relational Transactional Datasets",
    long_description="",
    long_description_content_type="text/markdown",
    author="GraphScope Team",
    author_email="graphscope@alibaba-inc.com",
    url="https://github.com/GraphScope/GART",
    license="Apache License 2.0",
    #package_dir={'': 'python'},
    #packages=find_core_packages('python'),
    packages=find_packages(where='src'),
    package_dir={'': 'src'},
    package_data={},
    classifiers=[
        "Development Status :: 5 - Production/Stable",
        "Intended Audience :: Developers",
        "Intended Audience :: Science/Research",
        "License :: OSI Approved :: Apache Software License",
        "Topic :: Software Development :: Libraries",
        "Topic :: System :: Distributed Computing",
        "Operating System :: MacOS :: MacOS X",
        "Operating System :: POSIX",
        "Programming Language :: Python",
        "Programming Language :: Python :: 3",
        "Programming Language :: Python :: 3.7",
        "Programming Language :: Python :: 3.8",
        "Programming Language :: Python :: 3.9",
        "Programming Language :: Python :: 3.10",
        "Programming Language :: Python :: 3.11",
    ],
    keywords="Graph, Large-Scale, Distributed Computing",
    setup_requires=[
        "setuptools_scm>=5.0.0,<8",
    ],
    cmdclass={
        "build_proto": BuildProto,
    },
    project_urls={
        "Documentation": "https://github.com/GraphScope/GART",
        "Source": "https://github.com/GraphScope/GART",
        "Tracker": "https://github.com/GraphScope/GART/issues",
    },
)
