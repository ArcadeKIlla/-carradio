#!/usr/bin/env python3
# -*- coding: utf-8 -*-

from setuptools import setup, find_packages

setup(
    name="vfd_sh1106_adapter",
    version="1.0.0",
    description="Adapter to replace VFD display with SH1106 OLED display for CarRadio project",
    author="Your Name",
    author_email="your.email@example.com",
    url="https://github.com/yourusername/carradio_sh1106_python",
    packages=find_packages(),
    install_requires=[
        "luma.oled>=3.8.1",
        "pillow>=8.0.0",
    ],
    classifiers=[
        "Development Status :: 4 - Beta",
        "Intended Audience :: Developers",
        "License :: OSI Approved :: MIT License",
        "Programming Language :: Python :: 3",
        "Programming Language :: Python :: 3.6",
        "Programming Language :: Python :: 3.7",
        "Programming Language :: Python :: 3.8",
        "Programming Language :: Python :: 3.9",
        "Topic :: Software Development :: Libraries",
        "Topic :: System :: Hardware",
    ],
    python_requires=">=3.6",
)
