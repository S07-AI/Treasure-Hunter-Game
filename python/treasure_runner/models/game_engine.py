"""
High-level Python wrapper for the C GameEngine type.

Manages the lifecycle of the C game engine and exposes
a clean Pythonic interface for game operations.
"""

import ctypes
from ..bindings.bindings import lib, Status, Direction
from .player import Player
from .exceptions import status_to_exception


class GameEngine:
    """Python wrapper around the C GameEngine opaque pointer."""

    def __init__(self, config_path: str):
        """
        Create and initialize the C game engine.

        Raises:
            GameEngineError (or subclass) if game_engine_create fails.
            RuntimeError if the player pointer cannot be obtained.
        """
        eng_out = ctypes.c_void_p(None)
        status = lib.game_engine_create(
            config_path.encode("utf-8"),
            ctypes.byref(eng_out),
        )
        if Status(status) != Status.OK:
            raise status_to_exception(Status(status), "game_engine_create failed")
        self._eng = eng_out

        raw_player = lib.game_engine_get_player(self._eng)
        if raw_player is None:
            raise RuntimeError("game_engine_get_player returned NULL")
        self._player = Player(raw_player)

    @property
    def player(self) -> Player:
        """Return the Python Player wrapper."""
        return self._player

    def destroy(self) -> None:
        """Destroy the C game engine. Safe to call multiple times."""
        if self._eng is not None:
            lib.game_engine_destroy(self._eng)
            self._eng = None

    def move_player(self, direction: Direction) -> None:
        """
        Move the player in the given direction.

        Raises:
            GameEngineError (or subclass) if the move fails.
        """
        status = lib.game_engine_move_player(self._eng, int(direction))
        if Status(status) != Status.OK:
            raise status_to_exception(Status(status), f"move_player failed: {direction}")

    def render_current_room(self) -> str:
        """
        Render the current room as a UTF-8 string.

        Raises:
            GameEngineError (or subclass) on failure.
        """
        str_out = ctypes.c_char_p()
        status = lib.game_engine_render_current_room(self._eng, ctypes.byref(str_out))
        if Status(status) != Status.OK:
            raise status_to_exception(Status(status), "render_current_room failed")
        raw_bytes = str_out.value
        lib.game_engine_free_string(str_out)
        decoded = raw_bytes.decode("utf-8") if raw_bytes is not None else ""
        return decoded

    def get_room_count(self) -> int:
        """
        Return the total number of rooms.

        Raises:
            GameEngineError (or subclass) on failure.
        """
        count_out = ctypes.c_int(0)
        status = lib.game_engine_get_room_count(self._eng, ctypes.byref(count_out))
        if Status(status) != Status.OK:
            raise status_to_exception(Status(status), "get_room_count failed")
        return count_out.value

    def get_room_dimensions(self) -> tuple[int, int]:
        """Return (width, height) of the player's current room."""
        w_out = ctypes.c_int(0)
        h_out = ctypes.c_int(0)
        lib.game_engine_get_room_dimensions(
            self._eng, ctypes.byref(w_out), ctypes.byref(h_out)
        )
        return (w_out.value, h_out.value)

    def get_room_ids(self) -> list[int]:
        """
        Return a list of all room IDs in the world.

        Raises:
            GameEngineError (or subclass) on failure.
        """
        ids_ptr = ctypes.POINTER(ctypes.c_int)()
        count_out = ctypes.c_int(0)
        status = lib.game_engine_get_room_ids(
            self._eng,
            ctypes.byref(ids_ptr),
            ctypes.byref(count_out),
        )
        if Status(status) != Status.OK:
            raise status_to_exception(Status(status), "get_room_ids failed")
        count = count_out.value
        result = [ids_ptr[i] for i in range(count)]
        lib.game_engine_free_string(ids_ptr)
        return result

    def reset(self) -> None:
        """Reset the game to its initial state."""
        status = lib.game_engine_reset(self._eng)
        if Status(status) != Status.OK:
            raise status_to_exception(Status(status), "reset failed")

    def get_total_treasure_count(self) -> int:
        """Return the total number of treasures across all rooms in the world."""
        count_out = ctypes.c_int(0)
        status = lib.game_engine_get_total_treasure_count(
            self._eng, ctypes.byref(count_out)
        )
        if Status(status) != Status.OK:
            raise status_to_exception(Status(status), "get_total_treasure_count failed")
        return count_out.value

    def is_victory(self) -> bool:
        """Return True if the player has collected every treasure in the world."""
        total = self.get_total_treasure_count()
        if total == 0:
            return False
        return self.player.get_collected_count() >= total

    def is_portal_locked(self, x: int, y: int) -> bool:
        """Return True if the portal at (x, y) in the current room is locked."""
        locked_out = ctypes.c_bool(False)
        status = lib.game_engine_is_portal_locked(
            self._eng, x, y, ctypes.byref(locked_out)
        )
        if Status(status) != Status.OK:
            return False
        return locked_out.value
        