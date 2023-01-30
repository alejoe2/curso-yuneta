# -*- encoding: utf-8 -*-

import os
import sys

from setuptools import setup, find_packages
from os.path import exists


if sys.version_info[:2] < (3, 5):
    raise RuntimeError('Requires Python 3.5 or better')

here = os.path.abspath(os.path.dirname(__file__))
try:
    README = open(os.path.join(here, 'README.rst')).read()
    CHANGES = open(os.path.join(here, 'CHANGES.txt')).read()
except IOError:
    README = CHANGES = ''

from connecttpcfile import __version__

requires = []

setup(
    name="connecttpcfile",
    version=__version__,
    description="Connect to a tcp port and send file",
    author="Luis Echeverria",
    author_email='echeverrialuish@hotmail.com',
    license='MIT License',
    packages=find_packages(),
    include_package_data=True,
    zip_safe=False,
    install_requires=requires,
    tests_require=[],
    py_modules=['connecttpcfile'],
    python_requires='>=3.5',
    entry_points={
        'console_scripts': [
            'connecttpcfile = connecttpcfile:main'
        ]
    }
)
