"""
run_game.py — Entry point for Treasure Runner.

Parses --config and --profile arguments, then launches the game UI.

Run from the python/ directory, or set PYTHONPATH=python before running.
"""

import os
import argparse

from treasure_runner.models.game_engine import GameEngine
from treasure_runner.ui.game_ui import GameUI


def _parse_args():
    """Parse and return command-line arguments."""
    parser = argparse.ArgumentParser(description="Treasure Runner")
    parser.add_argument(
        "--config",
        required=True,
        help="Path to the world .ini config file",
    )
    parser.add_argument(
        "--profile",
        required=True,
        help="Path to the player profile JSON file (created if missing)",
    )
    return parser.parse_args()


def main():
    """Create the game engine and launch the curses UI."""
    args = _parse_args()
    engine = GameEngine(args.config)
    try:
        ui = GameUI(engine, args.profile)
        ui.run()
    finally:
        engine.destroy()


if __name__ == "__main__":
    raise SystemExit(main())
    