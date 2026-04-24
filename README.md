# Treasure Runner — CIS*2750 A3

A terminal-based dungeon exploration game built with a C backend and a Python frontend. The C layer compiles to a shared library (`libbackend.so`) that is loaded at runtime by Python via `ctypes`, keeping game logic entirely in C while the UI and scripting live in Python.

---

## Project Structure

```
A3-main/
├── assets/
│   ├── default_profile.json    # Default player profile
│   └── starter.ini             # World generation config
├── c/
│   ├── include/                # Public C headers
│   │   ├── game_engine.h
│   │   ├── graph.h
│   │   ├── player.h
│   │   ├── room.h
│   │   ├── types.h
│   │   ├── world_loader.h
│   │   └── datagen.h
│   ├── src/                    # C implementation
│   │   ├── game_engine.c
│   │   ├── graph.c
│   │   ├── player.c
│   │   ├── room.c
│   │   └── world_loader.c
│   ├── tests/                  # C unit tests (Check framework)
│   └── tools/                  # Standalone demo tools
├── dist/                       # Pre-built libpuzzlegen binaries + built libbackend.so
├── python/
│   ├── run_game.py             # Interactive game entry point
│   ├── run_integration.py      # Deterministic integration test runner
│   ├── treasure_runner/
│   │   ├── bindings/           # ctypes bindings to libbackend.so
│   │   ├── models/             # Python-side game engine, player, exceptions
│   │   └── ui/                 # curses-based terminal UI
│   └── tests/                  # Python test suite
├── env.sh                      # Environment setup script
└── Makefile                    # Root build entry point
```

---

## Architecture

```
Python UI (curses)
      │
Python Models (game_engine.py, player.py)
      │
ctypes Bindings (bindings.py)
      │
libbackend.so  ──── libpuzzlegen.so
 (C source)         (pre-built, world generation)
```

The C layer owns all game state: the room graph, player position, treasures, switches, and pushable objects. Python receives an opaque pointer to the `GameEngine` struct and interacts only through the public C API.

---

## Requirements

- GCC, Make
- `pkg-config` + [Check](https://libcheck.github.io/check/) (for C unit tests)
- Python 3.10+
- A terminal that supports curses (Linux / macOS / devcontainer)

The recommended dev environment is the provided devcontainer (`socsguelph/cis2750:w26`), which has all dependencies pre-installed.

---

## Building

**Set up the environment first:**

```bash
source env.sh
```

This exports `TREASURE_RUNNER_ASSETS`, `LD_LIBRARY_PATH` (pointing at `dist/`), and `PYTHONPATH` (pointing at `python/`).

**Build the C shared library:**

```bash
make backend       # compiles libbackend.so into c/lib/
```

**Build and copy everything to `dist/`:**

```bash
make dist          # runs setup-lib + backend, copies libbackend.so to dist/
```

**Clean build artifacts:**

```bash
make clean
```

> `make dist` auto-detects your architecture (`amd64` / `arm64`) and selects the correct pre-built `libpuzzlegen` binary.

---

## Running the Game

```bash
cd python
python run_game.py --config ../assets/starter.ini --profile ../assets/default_profile.json
```

| Flag | Description |
|---|---|
| `--config` | Path to a world `.ini` config file |
| `--profile` | Path to a player profile JSON (created automatically if missing) |

### Controls

| Key | Action |
|---|---|
| Arrow keys / WASD | Move player |
| `>` | Enter portal |
| `r` | Reset to start |
| `q` | Quit |

---

## World Configuration

Edit `assets/starter.ini` to customise the generated world:

```ini
seed=15               # Deterministic generation seed

[world]
num_rooms=3

[room]
width=20
height=15
width_variance=2
height_variance=2

[portals]
portals_per_room=3
connectivity_chance=30

[treasures]
treasures_per_room=3

[pushables]
pushables_per_room=2

[switches]
enabled=true
```

---

## Testing

**C unit tests** (requires Check):

```bash
cd c
make test
```

**Python tests:**

```bash
cd python
python -m pytest tests/
```

**Integration test runner** (deterministic replay):

```bash
cd python
python run_integration.py --config ../assets/starter.ini
```

---

## Map Legend

| Symbol | Meaning |
|---|---|
| `@` | Player |
| `#` | Wall |
| `.` | Floor |
| `$` | Treasure |
| `X` | Portal |
| `O` | Pushable block |
| `^` | Switch (off) |
| `+` | Switch (on) |
| `L` | Locked cell |


# AUTHOR
Shayan Safaei