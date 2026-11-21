"""
Root launcher for Datastructure Duplicate file Detection system (DDD) CLI.
Forwards execution to CLI/main.py.
"""
import os
import sys
import importlib.util

cli_dir = os.path.join(os.path.dirname(os.path.abspath(__file__)), "CLI")
cli_file = os.path.join(cli_dir, "main.py")

if os.path.exists(cli_file):
    os.chdir(cli_dir)
    sys.path.insert(0, cli_dir)
    spec = importlib.util.spec_from_file_location("cli_engine", cli_file)
    module = importlib.util.module_from_spec(spec)
    sys.modules["cli_engine"] = module
    spec.loader.exec_module(module)

    if __name__ == "__main__":
        module.main()
