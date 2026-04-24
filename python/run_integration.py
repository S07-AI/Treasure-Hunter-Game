#!/usr/bin/env python3
"""Deterministic system integration test runner for Treasure Runner."""

import os
import argparse
from treasure_runner.bindings.bindings import Direction
from treasure_runner.models.game_engine import GameEngine
from treasure_runner.models.exceptions import GameError, ImpassableError


# ============================================================
# Helpers
# ============================================================

def _player_state_str(engine: GameEngine) -> str:
    """Return pipe-separated player state: room=<id>|x=<x>|y=<y>|collected=<n>."""
    room_id = engine.player.get_room()
    x_pos, y_pos = engine.player.get_position()
    collected = engine.player.get_collected_count()
    return f"room={room_id}|x={x_pos}|y={y_pos}|collected={collected}"


def _player_snapshot(engine: GameEngine) -> tuple:
    """Return a hashable snapshot of current player state for cycle detection."""
    room_id = engine.player.get_room()
    x_pos, y_pos = engine.player.get_position()
    collected = engine.player.get_collected_count()
    return (room_id, x_pos, y_pos, collected)


def _dir_name(direction: Direction) -> str:
    """Return the direction name string."""
    return direction.name


def _collected_count(engine: GameEngine) -> int:
    """Return the current collected treasure count."""
    return engine.player.get_collected_count()


# ============================================================
# Entry Move Detection
# ============================================================

def _find_entry_direction(engine: GameEngine) -> Direction:
    """
    Find the first direction from [SOUTH, WEST, NORTH, EAST] where the
    room_id stays the same and the position changes.

    Resets engine after each non-impassable attempt.
    Raises RuntimeError if no valid direction is found.
    """
    candidates = [Direction.SOUTH, Direction.WEST, Direction.NORTH, Direction.EAST]

    for direction in candidates:
        before_room = engine.player.get_room()
        before_pos = engine.player.get_position()

        try:
            engine.move_player(direction)
        except ImpassableError:
            # Do NOT reset — spec says reset only after non-impassable attempts
            continue
        except GameError:
            engine.reset()
            continue

        after_room = engine.player.get_room()
        after_pos = engine.player.get_position()
        engine.reset()

        if after_room == before_room and after_pos != before_pos:
            return direction

    raise RuntimeError("No valid entry direction found")


# ============================================================
# Sweep Phase
# ============================================================

def _run_sweep(
    engine: GameEngine,
    direction: Direction,
    phase_name: str,
    step: int,
    log_lines: list,
) -> int:
    """Execute one directional sweep. Returns updated step counter."""
    log_lines.append(f"SWEEP_START|phase={phase_name}|dir={_dir_name(direction)}")

    seen_states: set = set()
    reason = "BLOCKED"
    move_count = 0

    while True:
        snapshot = _player_snapshot(engine)
        if snapshot in seen_states:
            reason = "CYCLE_DETECTED"
            break
        seen_states.add(snapshot)

        before_str = _player_state_str(engine)
        before_collected = _collected_count(engine)
        step += 1
        move_count += 1

        try:
            engine.move_player(direction)
            after_str = _player_state_str(engine)
            delta = _collected_count(engine) - before_collected

            # Check no-progress: state unchanged despite OK move
            if _player_snapshot(engine) == snapshot:
                log_lines.append(
                    f"MOVE|step={step}|phase={phase_name}|dir={_dir_name(direction)}"
                    f"|result=NO_PROGRESS|before={before_str}|after={after_str}"
                    f"|delta_collected={delta}"
                )
                reason = "BLOCKED"
                break

            log_lines.append(
                f"MOVE|step={step}|phase={phase_name}|dir={_dir_name(direction)}"
                f"|result=OK|before={before_str}|after={after_str}"
                f"|delta_collected={delta}"
            )

        except ImpassableError:
            after_str = _player_state_str(engine)
            delta = _collected_count(engine) - before_collected
            log_lines.append(
                f"MOVE|step={step}|phase={phase_name}|dir={_dir_name(direction)}"
                f"|result=BLOCKED|before={before_str}|after={after_str}"
                f"|delta_collected={delta}"
            )
            reason = "BLOCKED"
            move_count -= 1
            break

        except GameError:
            after_str = _player_state_str(engine)
            delta = _collected_count(engine) - before_collected
            log_lines.append(
                f"MOVE|step={step}|phase={phase_name}|dir={_dir_name(direction)}"
                f"|result=ERROR|before={before_str}|after={after_str}"
                f"|delta_collected={delta}"
            )
            reason = "BLOCKED"
            move_count -= 1
            break

    log_lines.append(f"SWEEP_END|phase={phase_name}|reason={reason}|moves={move_count}")
    return step


# ============================================================
# Main Integration Runner
# ============================================================

def run(config_path: str, log_path: str) -> int:
    """Execute the full integration run and write the log file."""
    log_lines: list = []

    log_lines.append(f"RUN_START|config={config_path}")

    engine = GameEngine(config_path)

    # STATE: spawn — format: STATE|step=0|phase=SPAWN|state=<player_state>
    spawn_state = _player_state_str(engine)
    log_lines.append(f"STATE|step=0|phase=SPAWN|state={spawn_state}")

    # Find which direction moves the player off the spawn portal
    try:
        entry_dir = _find_entry_direction(engine)
    except RuntimeError:
        log_lines.append(f"RUN_END|steps=0|collected_total=0")
        _write_log(log_path, log_lines)
        return 1

    log_lines.append(f"ENTRY|direction={_dir_name(entry_dir)}")

    # Execute entry move
    before_str = _player_state_str(engine)
    before_collected = _collected_count(engine)
    step = 1
    try:
        engine.move_player(entry_dir)
        after_str = _player_state_str(engine)
        delta = _collected_count(engine) - before_collected
        log_lines.append(
            f"MOVE|step={step}|phase=ENTRY|dir={_dir_name(entry_dir)}"
            f"|result=OK|before={before_str}|after={after_str}"
            f"|delta_collected={delta}"
        )
    except GameError:
        after_str = _player_state_str(engine)
        delta = _collected_count(engine) - before_collected
        log_lines.append(
            f"MOVE|step={step}|phase=ENTRY|dir={_dir_name(entry_dir)}"
            f"|result=ERROR|before={before_str}|after={after_str}"
            f"|delta_collected={delta}"
        )
        log_lines.append("TERMINATED: Initial Move Error")
        log_lines.append(
            f"RUN_END|steps={step}|collected_total={engine.player.get_collected_count()}"
        )
        _write_log(log_path, log_lines)
        return 1

    # Four directional sweeps in order
    sweeps = [
        (Direction.SOUTH, "SWEEP_SOUTH"),
        (Direction.WEST,  "SWEEP_WEST"),
        (Direction.NORTH, "SWEEP_NORTH"),
        (Direction.EAST,  "SWEEP_EAST"),
    ]

    for direction, phase_name in sweeps:
        step = _run_sweep(engine, direction, phase_name, step, log_lines)

    # STATE: final — format: STATE|phase=FINAL|state=<player_state>
    final_state = _player_state_str(engine)
    log_lines.append(f"STATE|step={step}|phase=FINAL|state={final_state}")

    collected_total = engine.player.get_collected_count()
    log_lines.append(f"RUN_END|steps={step}|collected_total={collected_total}")

    engine.destroy()

    _write_log(log_path, log_lines)
    return 0


def _write_log(log_path: str, log_lines: list) -> None:
    """Write log lines to the output file, one record per line."""
    with open(log_path, "w", encoding="utf-8") as f:
        for line in log_lines:
            f.write(line + "\n")


# ============================================================
# CLI
# ============================================================

def parse_args():
    """Parse command-line arguments."""
    parser = argparse.ArgumentParser(description="Treasure Runner integration test logger")
    parser.add_argument("--config", required=True, help="Path to generator config file")
    parser.add_argument("--log", required=True, help="Output log path")
    return parser.parse_args()


def main():
    """Entry point."""
    args = parse_args()
    config_path = os.path.abspath(args.config)
    log_path = os.path.abspath(args.log)
    return run(config_path, log_path)


if __name__ == "__main__":
    raise SystemExit(main())
    