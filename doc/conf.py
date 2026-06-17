# Configuration file for the Sphinx documentation builder.
#
# For the full list of built-in configuration values, see the documentation:
# https://www.sphinx-doc.org/en/master/usage/configuration.html

import os
import subprocess
import sys

# -- Project information -----------------------------------------------------
# https://www.sphinx-doc.org/en/master/usage/configuration.html#project-information

project = "C-ObjectPool"
copyright = "2026, adachng"
author = "adachng"

# -- General configuration ---------------------------------------------------
# https://www.sphinx-doc.org/en/master/usage/configuration.html#general-configuration

sys.path.append(os.path.abspath("breathe/breathe"))
extensions = ["breathe"]
breathe_projects = {project: "xml"}
breathe_default_project = project

templates_path = ["_templates"]
exclude_patterns = ["_build", "Thumbs.db", ".DS_Store"]

# -- Options for HTML output -------------------------------------------------
# https://www.sphinx-doc.org/en/master/usage/configuration.html#options-for-html-output

html_theme = "alabaster"
html_static_path = ["_static"]

read_the_docs_build = os.environ.get("READTHEDOCS", None) == "True"

if read_the_docs_build:
    subprocess.call("doxygen", shell=True)
    os.rename("html", "_static")
