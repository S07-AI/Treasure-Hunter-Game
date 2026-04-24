"""
GameUI: View layer for Treasure Runner.

Handles all curses rendering, splash screens, and user input.
No game logic lives here — GameUI queries GameEngine for state.
"""

import curses
import json
import os
from datetime import datetime, timezone

from ..bindings.bindings import Direction
from ..models.exceptions import ImpassableError, NoPortalError, GameEngineError
from ..models.game_engine import GameEngine

# ── Constants ─────────────────────────────────────────────────────────────────

MIN_ROWS = 24
MIN_COLS = 60
TITLE = "Treasure Runner"
EMAIL = "ssafaei@uoguelph.ca"  # replace with your uoguelph email
LEGEND = "@ Player  # Wall  $ Gold  X Portal  L Locked  ^ Switch(off)  + Switch(on)  O Pushable  . Floor"
CONTROLS = "Arrows/WASD: Move   >: Portal   r: Reset   q: Quit"

_DIR_KEYS = {
    curses.KEY_UP:    Direction.NORTH,
    ord('w'):         Direction.NORTH,
    ord('W'):         Direction.NORTH,
    curses.KEY_DOWN:  Direction.SOUTH,
    ord('s'):         Direction.SOUTH,
    ord('S'):         Direction.SOUTH,
    curses.KEY_RIGHT: Direction.EAST,
    ord('d'):         Direction.EAST,
    ord('D'):         Direction.EAST,
    curses.KEY_LEFT:  Direction.WEST,
    ord('a'):         Direction.WEST,
    ord('A'):         Direction.WEST,
}


class GameUI:
    """View layer: all curses rendering, splash screens, and user input."""

    def __init__(self, engine: GameEngine, profile_path: str):
        self._engine = engine
        self._profile_path = profile_path
        self._profile = self._load_or_create_profile()
        self._message = "Find the treasure and explore the dungeon!"
        self._running = True
        self._victory = False
        self._rooms_visited: set = {engine.player.get_room()}

    # ── Profile I/O ───────────────────────────────────────────────────────────

    def _load_or_create_profile(self) -> dict:
        """Load profile from disk, or create a new one prompting for a name."""
        if os.path.exists(self._profile_path):
            with open(self._profile_path, 'r', encoding='utf-8') as file_handle:
                return json.load(file_handle)
        name = input("Enter your player name: ").strip() or "Player"
        return {
            "player_name": name,
            "games_played": 0,
            "max_treasure_collected": 0,
            "most_rooms_world_completed": 0,
            "timestamp_last_played": datetime.now(timezone.utc).isoformat(),
        }

    def _save_profile(self) -> None:
        """Update stats and persist the profile to disk."""
        collected = self._engine.player.get_collected_count()
        rooms = len(self._rooms_visited)
        self._profile["games_played"] += 1
        if collected > self._profile["max_treasure_collected"]:
            self._profile["max_treasure_collected"] = collected
        if rooms > self._profile["most_rooms_world_completed"]:
            self._profile["most_rooms_world_completed"] = rooms
        self._profile["timestamp_last_played"] = (
            datetime.now(timezone.utc).isoformat()
        )
        profile_dir = os.path.dirname(os.path.abspath(self._profile_path))
        os.makedirs(profile_dir, exist_ok=True)
        with open(self._profile_path, 'w', encoding='utf-8') as file_handle:
            json.dump(self._profile, file_handle, indent=2)

    def _profile_lines(self) -> list:
        """Return formatted profile info as a list of display strings."""
        profile_data = self._profile
        return [
            f"Player:        {profile_data['player_name']}",
            f"Games Played:  {profile_data['games_played']}",
            f"Max Treasure:  {profile_data['max_treasure_collected']}",
            f"Most Rooms:    {profile_data['most_rooms_world_completed']}",
            f"Last Played:   {profile_data['timestamp_last_played']}",
        ]

    # ── Curses Helpers ────────────────────────────────────────────────────────

    def _check_size(self, stdscr) -> None:
        """Raise RuntimeError if the terminal is too small for the game."""
        rows, cols = stdscr.getmaxyx()
        if rows < MIN_ROWS or cols < MIN_COLS:
            raise RuntimeError(
                f"Terminal too small ({rows}x{cols}). "
                f"Minimum required: {MIN_ROWS}x{MIN_COLS}."
            )

    def _safe_addstr(self, stdscr, row: int, col: int, text: str) -> None:
        """Write text at (row, col), silently ignoring out-of-bounds errors."""
        rows, cols = stdscr.getmaxyx()
        if row < 0 or row >= rows or col < 0 or col >= cols:
            return
        try:
            stdscr.addstr(row, col, text[:max(0, cols - col - 1)])
        except curses.error:
            pass

    def _center(self, stdscr, row: int, text: str) -> None:
        """Write text centred horizontally on the given row."""
        _, cols = stdscr.getmaxyx()
        col = max(0, (cols - len(text)) // 2)
        self._safe_addstr(stdscr, row, col, text)

    # ── Splash Screens ────────────────────────────────────────────────────────

    def _draw_splash(self, stdscr, heading: str) -> None:
        """Display a full-screen splash with heading and profile summary."""
        stdscr.clear()
        rows, _ = stdscr.getmaxyx()
        lines = (
            [TITLE, heading, ""]
            + self._profile_lines()
            + ["", "Press any key to continue..."]
        )
        start = max(0, (rows - len(lines)) // 2)
        for i, line in enumerate(lines):
            self._center(stdscr, start + i, line)
        stdscr.refresh()
        stdscr.getch()

    def _victory_stat_lines(self) -> list:
        """Return the win-screen stat lines for the victory splash."""
        collected = self._engine.player.get_collected_count()
        total = self._engine.get_total_treasure_count()
        rooms_vis = len(self._rooms_visited)
        total_rooms = self._engine.get_room_count()
        return [
            "",
            f"Treasures collected: {collected}/{total}",
            f"Rooms visited:       {rooms_vis}/{total_rooms}",
            "",
            "Press any key to exit...",
        ]

    def _draw_victory_splash(self, stdscr) -> None:
        """Display a full-screen victory screen with final stats."""
        stdscr.clear()
        rows, _ = stdscr.getmaxyx()
        lines = (
            [TITLE, "*** YOU WIN! ***", ""]
            + self._profile_lines()
            + self._victory_stat_lines()
        )
        start = max(0, (rows - len(lines)) // 2)
        for i, line in enumerate(lines):
            self._center(stdscr, start + i, line)
        stdscr.refresh()
        stdscr.getch()

    # ── In-game Drawing ───────────────────────────────────────────────────────

    def _draw_message_bar(self, stdscr) -> None:
        """Render the message bar at row 0."""
        _, cols = stdscr.getmaxyx()
        self._safe_addstr(stdscr, 0, 0, self._message.ljust(cols - 1))

    def _draw_room(self, stdscr) -> None:
        """Render the room label (row 1) and room grid (rows 2+)."""
        room_id = self._engine.player.get_room()
        self._safe_addstr(stdscr, 1, 0, f"Room {room_id}")
        room_str = self._engine.render_current_room()
        for i, line in enumerate(room_str.split('\n')):
            self._safe_addstr(stdscr, 2 + i, 0, line)

    def _status_bar_text(self) -> str:
        """Return the player status bar string for the footer."""
        collected = self._engine.player.get_collected_count()
        total_treasure = self._engine.get_total_treasure_count()
        total_rooms = self._engine.get_room_count()
        rooms_vis = len(self._rooms_visited)
        name = self._profile["player_name"]
        return (
            f"{TITLE} | {name} | Treasure: {collected}/{total_treasure} | "
            f"Rooms: {rooms_vis}/{total_rooms} | {EMAIL}"
        )

    def _draw_footer(self, stdscr) -> None:
        """Render controls, legend, and status bar at the bottom rows."""
        rows, cols = stdscr.getmaxyx()
        self._safe_addstr(stdscr, rows - 3, 0, CONTROLS[:cols - 1])
        self._safe_addstr(stdscr, rows - 2, 0, LEGEND[:cols - 1])
        self._safe_addstr(stdscr, rows - 1, 0, self._status_bar_text()[:cols - 1])

    # ── Input Handling ────────────────────────────────────────────────────────

    def _try_move(self, direction: Direction) -> str:
        """Attempt a directional move; return a feedback message."""
        collected_before = self._engine.player.get_collected_count()
        try:
            self._engine.move_player(direction)
            self._rooms_visited.add(self._engine.player.get_room())
            collected_after = self._engine.player.get_collected_count()
            if collected_after > collected_before:
                total = self._engine.get_total_treasure_count()
                return f"Treasure collected! ({collected_after}/{total})"
            return "Moved."
        except ImpassableError:
            return "You can't go that way."
        except NoPortalError:
            return "No portal in that direction."

    def _try_portal(self) -> str:
        """Try every direction to find and traverse a portal into a new room."""
        current_room = self._engine.player.get_room()
        for direction in Direction:
            try:
                self._engine.move_player(direction)
                if self._engine.player.get_room() != current_room:
                    self._rooms_visited.add(self._engine.player.get_room())
                    return "You stepped through a portal!"
                # Moved but stayed in same room — undo by moving back
                opposite = {
                    Direction.NORTH: Direction.SOUTH,
                    Direction.SOUTH: Direction.NORTH,
                    Direction.EAST: Direction.WEST,
                    Direction.WEST: Direction.EAST,
                }
                try:
                    self._engine.move_player(opposite[direction])
                except (ImpassableError, NoPortalError, GameEngineError):
                    pass
            except (ImpassableError, NoPortalError, GameEngineError):
                pass
        return "No portal to enter from here."

    def _handle_key(self, key: int) -> None:
        """Dispatch a keypress and update the current message accordingly."""
        if key in (ord('q'), ord('Q')):
            self._running = False
            return
        if key in (ord('r'), ord('R')):
            self._engine.reset()
            self._rooms_visited = {self._engine.player.get_room()}
            self._victory = False
            self._message = "Game reset to starting state."
            return
        if key == ord('>'):
            self._message = self._try_portal()
            return
        direction = _DIR_KEYS.get(key)
        if direction is not None:
            self._message = self._try_move(direction)

    # ── Main Loop & Entry Point ───────────────────────────────────────────────

    def _game_loop(self, stdscr) -> None:
        """Run the main game loop until the player quits or wins."""
        curses.curs_set(0)
        stdscr.keypad(True)
        while self._running:
            stdscr.clear()
            self._draw_message_bar(stdscr)
            self._draw_room(stdscr)
            self._draw_footer(stdscr)
            stdscr.refresh()
            self._handle_key(stdscr.getch())
            # Check victory after every action
            if not self._victory and self._engine.is_victory():
                self._victory = True
                self._running = False

    def run(self) -> None:
        """Launch the full game: startup splash → game loop → end splash → save."""
        def _inner(stdscr):
            self._check_size(stdscr)
            self._draw_splash(stdscr, "Welcome!")
            self._game_loop(stdscr)
            self._save_profile()
            if self._victory:
                self._draw_victory_splash(stdscr)
            else:
                self._draw_splash(stdscr, "Thanks for playing!")
        curses.wrapper(_inner)
        