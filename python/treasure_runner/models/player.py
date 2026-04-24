"""
High-level Python wrapper for the C Player type.

Wraps the opaque C Player pointer returned by the game engine
and exposes a Pythonic interface for reading player state.
"""

import ctypes
from ..bindings.bindings import lib, Treasure


class Player:
    """Python wrapper around the C Player opaque pointer."""

    def __init__(self, ptr):
        """Initialize with a pointer to the C player entity."""
        self._ptr = ptr

    def get_room(self) -> int:
        """Return the ID of the room the player is currently in."""
        return lib.player_get_room(self._ptr)

    def get_position(self) -> tuple[int, int]:
        """Return the player's current (x, y) position."""
        x_out = ctypes.c_int(0)
        y_out = ctypes.c_int(0)
        lib.player_get_position(self._ptr, ctypes.byref(x_out), ctypes.byref(y_out))
        return (x_out.value, y_out.value)

    def get_collected_count(self) -> int:
        """Return the number of treasures the player has collected."""
        return lib.player_get_collected_count(self._ptr)

    def has_collected_treasure(self, treasure_id: int) -> bool:
        """Return True if the player has collected the given treasure ID."""
        return lib.player_has_collected_treasure(self._ptr, treasure_id)

    def get_collected_treasures(self) -> list[dict]:
        """Return a list of dicts describing each collected treasure."""
        count_out = ctypes.c_int(0)
        treasures_ptr = lib.player_get_collected_treasures(
            self._ptr, ctypes.byref(count_out)
        )
        count = count_out.value
        result = []
        if not treasures_ptr or count == 0:
            return result
        for i in range(count):
            tptr = treasures_ptr[i]
            if not tptr:
                continue
            tobj = tptr.contents
            name_val = tobj.name.decode("utf-8") if tobj.name else ""
            result.append({
                "id": tobj.id,
                "name": name_val,
                "starting_room_id": tobj.starting_room_id,
                "initial_x": tobj.initial_x,
                "initial_y": tobj.initial_y,
                "x": tobj.x,
                "y": tobj.y,
                "collected": tobj.collected,
            })
        return result
        