"""
Root launcher for Datastructure Duplicate file Detection system (DDD) GUI.
Forwards execution to GUI/gui.py.
"""
import os
import sys
import importlib.util

gui_dir = os.path.join(os.path.dirname(os.path.abspath(__file__)), "GUI")
gui_file = os.path.join(gui_dir, "gui.py")

if os.path.exists(gui_file):
    os.chdir(gui_dir)
    sys.path.insert(0, gui_dir)
    spec = importlib.util.spec_from_file_location("gui_engine", gui_file)
    module = importlib.util.module_from_spec(spec)
    sys.modules["gui_engine"] = module
    spec.loader.exec_module(module)

    if __name__ == "__main__":
        module.launch_gui()
