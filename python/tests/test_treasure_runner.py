"""
Unit tests for the Treasure Runner Python layer.

Covers:
  - bindings: enums, Treasure struct, Status/Direction values, lib signatures
  - exceptions: hierarchy, status_to_exception, status_to_status_exception
  - models/player.py: all Player methods via mocked lib
  - models/game_engine.py: all GameEngine methods via mocked lib
"""

import ctypes
import unittest
from unittest.mock import MagicMock, patch, call


# ============================================================
# Helpers
# ============================================================

def _make_treasure(tid=1, name=b"Gold", starting_room_id=0,
                   initial_x=2, initial_y=3, x=2, y=3, collected=False):
    from treasure_runner.bindings import Treasure
    t = Treasure()
    t.id = tid
    t.name = name
    t.starting_room_id = starting_room_id
    t.initial_x = initial_x
    t.initial_y = initial_y
    t.x = x
    t.y = y
    t.collected = collected
    return t


# ============================================================
# Status enum
# ============================================================

class TestStatusEnum(unittest.TestCase):

    def test_ok_is_zero(self):
        from treasure_runner.bindings import Status
        self.assertEqual(Status.OK, 0)

    def test_invalid_argument(self):
        from treasure_runner.bindings import Status
        self.assertEqual(Status.INVALID_ARGUMENT, 1)

    def test_null_pointer(self):
        from treasure_runner.bindings import Status
        self.assertEqual(Status.NULL_POINTER, 2)

    def test_no_memory(self):
        from treasure_runner.bindings import Status
        self.assertEqual(Status.NO_MEMORY, 3)

    def test_bounds_exceeded(self):
        from treasure_runner.bindings import Status
        self.assertEqual(Status.BOUNDS_EXCEEDED, 4)

    def test_internal_error(self):
        from treasure_runner.bindings import Status
        self.assertEqual(Status.INTERNAL_ERROR, 5)

    def test_room_impassable(self):
        from treasure_runner.bindings import Status
        self.assertEqual(Status.ROOM_IMPASSABLE, 6)

    def test_room_no_portal(self):
        from treasure_runner.bindings import Status
        self.assertEqual(Status.ROOM_NO_PORTAL, 7)

    def test_room_not_found(self):
        from treasure_runner.bindings import Status
        self.assertEqual(Status.ROOM_NOT_FOUND, 8)

    def test_ge_no_such_room(self):
        from treasure_runner.bindings import Status
        self.assertEqual(Status.GE_NO_SUCH_ROOM, 9)

    def test_game_engine_status_is_status(self):
        from treasure_runner.bindings import Status, GameEngineStatus
        self.assertIs(GameEngineStatus, Status)


# ============================================================
# Direction enum
# ============================================================

class TestDirectionEnum(unittest.TestCase):

    def test_north_is_zero(self):
        from treasure_runner.bindings import Direction
        self.assertEqual(Direction.NORTH, 0)

    def test_south(self):
        from treasure_runner.bindings import Direction
        self.assertEqual(Direction.SOUTH, 1)

    def test_east(self):
        from treasure_runner.bindings import Direction
        self.assertEqual(Direction.EAST, 2)

    def test_west(self):
        from treasure_runner.bindings import Direction
        self.assertEqual(Direction.WEST, 3)

    def test_four_directions(self):
        from treasure_runner.bindings import Direction
        self.assertEqual(len(Direction), 4)

    def test_name_strings(self):
        from treasure_runner.bindings import Direction
        self.assertEqual(Direction.NORTH.name, "NORTH")
        self.assertEqual(Direction.SOUTH.name, "SOUTH")
        self.assertEqual(Direction.EAST.name, "EAST")
        self.assertEqual(Direction.WEST.name, "WEST")


# ============================================================
# Treasure struct
# ============================================================

class TestTreasureStruct(unittest.TestCase):

    def test_all_fields_exist(self):
        from treasure_runner.bindings import Treasure
        names = [f[0] for f in Treasure._fields_]
        for field in ["id", "name", "starting_room_id", "initial_x",
                      "initial_y", "x", "y", "collected"]:
            self.assertIn(field, names)

    def test_id_field(self):
        t = _make_treasure(tid=42)
        self.assertEqual(t.id, 42)

    def test_position_fields(self):
        t = _make_treasure(x=9, y=4)
        self.assertEqual(t.x, 9)
        self.assertEqual(t.y, 4)

    def test_initial_position_fields(self):
        t = _make_treasure(initial_x=1, initial_y=2)
        self.assertEqual(t.initial_x, 1)
        self.assertEqual(t.initial_y, 2)

    def test_starting_room_id(self):
        t = _make_treasure(starting_room_id=7)
        self.assertEqual(t.starting_room_id, 7)

    def test_collected_false(self):
        t = _make_treasure(collected=False)
        self.assertFalse(t.collected)

    def test_collected_true(self):
        t = _make_treasure(collected=True)
        self.assertTrue(t.collected)

    def test_name_bytes(self):
        t = _make_treasure(name=b"Diamond")
        self.assertEqual(t.name, b"Diamond")


# ============================================================
# Exception hierarchy
# ============================================================

class TestExceptionHierarchy(unittest.TestCase):

    def test_game_error_is_base_exception(self):
        from treasure_runner.models.exceptions import GameError
        self.assertTrue(issubclass(GameError, Exception))

    def test_game_engine_error_inherits_game_error(self):
        from treasure_runner.models.exceptions import GameEngineError, GameError
        self.assertTrue(issubclass(GameEngineError, GameError))

    def test_invalid_argument_inherits_game_engine_error(self):
        from treasure_runner.models.exceptions import InvalidArgumentError, GameEngineError
        self.assertTrue(issubclass(InvalidArgumentError, GameEngineError))

    def test_out_of_bounds_inherits_game_engine_error(self):
        from treasure_runner.models.exceptions import OutOfBoundsError, GameEngineError
        self.assertTrue(issubclass(OutOfBoundsError, GameEngineError))

    def test_impassable_inherits_game_engine_error(self):
        from treasure_runner.models.exceptions import ImpassableError, GameEngineError
        self.assertTrue(issubclass(ImpassableError, GameEngineError))

    def test_no_such_room_inherits_game_engine_error(self):
        from treasure_runner.models.exceptions import NoSuchRoomError, GameEngineError
        self.assertTrue(issubclass(NoSuchRoomError, GameEngineError))

    def test_no_portal_inherits_game_engine_error(self):
        from treasure_runner.models.exceptions import NoPortalError, GameEngineError
        self.assertTrue(issubclass(NoPortalError, GameEngineError))

    def test_internal_error_inherits_game_engine_error(self):
        from treasure_runner.models.exceptions import InternalError, GameEngineError
        self.assertTrue(issubclass(InternalError, GameEngineError))

    def test_status_error_inherits_game_error(self):
        from treasure_runner.models.exceptions import StatusError, GameError
        self.assertTrue(issubclass(StatusError, GameError))

    def test_all_status_subclasses_inherit_status_error(self):
        from treasure_runner.models.exceptions import (
            StatusInvalidArgumentError, StatusNullPointerError,
            StatusNoMemoryError, StatusBoundsExceededError,
            StatusImpassableError, StatusInternalError, StatusError,
        )
        for cls in [StatusInvalidArgumentError, StatusNullPointerError,
                    StatusNoMemoryError, StatusBoundsExceededError,
                    StatusImpassableError, StatusInternalError]:
            with self.subTest(cls=cls):
                self.assertTrue(issubclass(cls, StatusError))

    def test_exceptions_are_raisable(self):
        from treasure_runner.models.exceptions import (
            GameError, ImpassableError, NoSuchRoomError, InternalError
        )
        for exc_cls in [GameError, ImpassableError, NoSuchRoomError, InternalError]:
            with self.subTest(exc=exc_cls):
                with self.assertRaises(exc_cls):
                    raise exc_cls("test")


# ============================================================
# status_to_exception
# ============================================================

class TestStatusToException(unittest.TestCase):

    def _ste(self, status_val, msg=None):
        from treasure_runner.models.exceptions import status_to_exception
        if msg:
            return status_to_exception(status_val, msg)
        return status_to_exception(status_val)

    def test_invalid_argument(self):
        from treasure_runner.bindings import Status
        from treasure_runner.models.exceptions import InvalidArgumentError
        exc = self._ste(Status.INVALID_ARGUMENT)
        self.assertIsInstance(exc, InvalidArgumentError)

    def test_bounds_exceeded(self):
        from treasure_runner.bindings import Status
        from treasure_runner.models.exceptions import OutOfBoundsError
        exc = self._ste(Status.BOUNDS_EXCEEDED)
        self.assertIsInstance(exc, OutOfBoundsError)

    def test_room_impassable(self):
        from treasure_runner.bindings import Status
        from treasure_runner.models.exceptions import ImpassableError
        exc = self._ste(Status.ROOM_IMPASSABLE)
        self.assertIsInstance(exc, ImpassableError)

    def test_no_portal(self):
        from treasure_runner.bindings import Status
        from treasure_runner.models.exceptions import NoPortalError
        exc = self._ste(Status.ROOM_NO_PORTAL)
        self.assertIsInstance(exc, NoPortalError)

    def test_no_such_room(self):
        from treasure_runner.bindings import Status
        from treasure_runner.models.exceptions import NoSuchRoomError
        exc = self._ste(Status.GE_NO_SUCH_ROOM)
        self.assertIsInstance(exc, NoSuchRoomError)

    def test_internal_error(self):
        from treasure_runner.bindings import Status
        from treasure_runner.models.exceptions import InternalError
        exc = self._ste(Status.INTERNAL_ERROR)
        self.assertIsInstance(exc, InternalError)

    def test_null_pointer_maps_to_internal(self):
        from treasure_runner.bindings import Status
        from treasure_runner.models.exceptions import InternalError
        exc = self._ste(Status.NULL_POINTER)
        self.assertIsInstance(exc, InternalError)

    def test_message_preserved(self):
        from treasure_runner.bindings import Status
        exc = self._ste(Status.INVALID_ARGUMENT, "my message")
        self.assertIn("my message", str(exc))

    def test_unknown_status_gives_game_engine_error(self):
        from treasure_runner.models.exceptions import GameEngineError
        exc = self._ste(9999)
        self.assertIsInstance(exc, GameEngineError)


# ============================================================
# status_to_status_exception
# ============================================================

class TestStatusToStatusException(unittest.TestCase):

    def _stse(self, status_val):
        from treasure_runner.models.exceptions import status_to_status_exception
        return status_to_status_exception(status_val)

    def test_invalid_argument(self):
        from treasure_runner.bindings import Status
        from treasure_runner.models.exceptions import StatusInvalidArgumentError
        self.assertIsInstance(self._stse(Status.INVALID_ARGUMENT), StatusInvalidArgumentError)

    def test_null_pointer(self):
        from treasure_runner.bindings import Status
        from treasure_runner.models.exceptions import StatusNullPointerError
        self.assertIsInstance(self._stse(Status.NULL_POINTER), StatusNullPointerError)

    def test_no_memory(self):
        from treasure_runner.bindings import Status
        from treasure_runner.models.exceptions import StatusNoMemoryError
        self.assertIsInstance(self._stse(Status.NO_MEMORY), StatusNoMemoryError)

    def test_bounds_exceeded(self):
        from treasure_runner.bindings import Status
        from treasure_runner.models.exceptions import StatusBoundsExceededError
        self.assertIsInstance(self._stse(Status.BOUNDS_EXCEEDED), StatusBoundsExceededError)

    def test_impassable(self):
        from treasure_runner.bindings import Status
        from treasure_runner.models.exceptions import StatusImpassableError
        self.assertIsInstance(self._stse(Status.ROOM_IMPASSABLE), StatusImpassableError)

    def test_internal(self):
        from treasure_runner.bindings import Status
        from treasure_runner.models.exceptions import StatusInternalError
        self.assertIsInstance(self._stse(Status.INTERNAL_ERROR), StatusInternalError)

    def test_unknown_falls_back(self):
        from treasure_runner.models.exceptions import StatusError
        self.assertIsInstance(self._stse(9999), StatusError)


# ============================================================
# Player model (mocked lib)
# ============================================================

class TestPlayer(unittest.TestCase):

    def setUp(self):
        self.patcher = patch("treasure_runner.models.player.lib")
        self.mock_lib = self.patcher.start()
        from treasure_runner.models.player import Player
        self.Player = Player
        self.ptr = ctypes.c_void_p(0xDEAD)

    def tearDown(self):
        self.patcher.stop()

    def _make(self):
        return self.Player(self.ptr)

    def test_init_stores_ptr(self):
        p = self._make()
        self.assertEqual(p._ptr, self.ptr)

    def test_get_room_delegates(self):
        self.mock_lib.player_get_room.return_value = 3
        p = self._make()
        self.assertEqual(p.get_room(), 3)
        self.mock_lib.player_get_room.assert_called_once_with(self.ptr)

    def test_get_room_returns_int(self):
        self.mock_lib.player_get_room.return_value = 0
        p = self._make()
        self.assertIsInstance(p.get_room(), int)

    def test_get_position_returns_tuple(self):
        def fake(ptr, xr, yr):
            xr._obj.value = 5
            yr._obj.value = 8
            return 0
        self.mock_lib.player_get_position.side_effect = fake
        p = self._make()
        result = p.get_position()
        self.assertIsInstance(result, tuple)
        self.assertEqual(len(result), 2)

    def test_get_position_values(self):
        def fake(ptr, xr, yr):
            xr._obj.value = 10
            yr._obj.value = 20
            return 0
        self.mock_lib.player_get_position.side_effect = fake
        p = self._make()
        x, y = p.get_position()
        self.assertEqual(x, 10)
        self.assertEqual(y, 20)

    def test_get_collected_count_zero(self):
        self.mock_lib.player_get_collected_count.return_value = 0
        p = self._make()
        self.assertEqual(p.get_collected_count(), 0)

    def test_get_collected_count_nonzero(self):
        self.mock_lib.player_get_collected_count.return_value = 5
        p = self._make()
        self.assertEqual(p.get_collected_count(), 5)
        self.mock_lib.player_get_collected_count.assert_called_once_with(self.ptr)

    def test_has_collected_true(self):
        self.mock_lib.player_has_collected_treasure.return_value = True
        p = self._make()
        self.assertTrue(p.has_collected_treasure(7))
        self.mock_lib.player_has_collected_treasure.assert_called_once_with(self.ptr, 7)

    def test_has_collected_false(self):
        self.mock_lib.player_has_collected_treasure.return_value = False
        p = self._make()
        self.assertFalse(p.has_collected_treasure(999))

    def test_get_collected_treasures_empty_on_null(self):
        self.mock_lib.player_get_collected_treasures.return_value = None
        p = self._make()
        self.assertEqual(p.get_collected_treasures(), [])

    def test_get_collected_treasures_empty_on_zero_count(self):
        def fake(ptr, count_ref):
            count_ref._obj.value = 0
            return None
        self.mock_lib.player_get_collected_treasures.side_effect = fake
        p = self._make()
        self.assertEqual(p.get_collected_treasures(), [])

    def test_get_collected_treasures_returns_list(self):
        from treasure_runner.bindings import Treasure
        t = _make_treasure(tid=3, name=b"Ruby", x=1, y=2, collected=True)
        t_ptr = ctypes.pointer(t)
        arr_type = ctypes.POINTER(Treasure) * 1
        arr = arr_type(t_ptr)

        def fake(ptr, count_ref):
            count_ref._obj.value = 1
            return arr

        self.mock_lib.player_get_collected_treasures.side_effect = fake
        p = self._make()
        result = p.get_collected_treasures()
        self.assertIsInstance(result, list)
        self.assertEqual(len(result), 1)

    def test_get_collected_treasures_dict_keys(self):
        from treasure_runner.bindings import Treasure
        t = _make_treasure(tid=1, name=b"Gold")
        t_ptr = ctypes.pointer(t)
        arr_type = ctypes.POINTER(Treasure) * 1
        arr = arr_type(t_ptr)

        def fake(ptr, count_ref):
            count_ref._obj.value = 1
            return arr

        self.mock_lib.player_get_collected_treasures.side_effect = fake
        p = self._make()
        d = p.get_collected_treasures()[0]
        for key in ["id", "name", "starting_room_id", "initial_x",
                    "initial_y", "x", "y", "collected"]:
            self.assertIn(key, d)

    def test_get_collected_treasures_name_decoded(self):
        from treasure_runner.bindings import Treasure
        t = _make_treasure(name=b"Sapphire")
        t_ptr = ctypes.pointer(t)
        arr_type = ctypes.POINTER(Treasure) * 1
        arr = arr_type(t_ptr)

        def fake(ptr, count_ref):
            count_ref._obj.value = 1
            return arr

        self.mock_lib.player_get_collected_treasures.side_effect = fake
        p = self._make()
        name = p.get_collected_treasures()[0]["name"]
        self.assertIsInstance(name, str)
        self.assertEqual(name, "Sapphire")

    def test_get_collected_treasures_null_name(self):
        from treasure_runner.bindings import Treasure
        t = _make_treasure(name=None)
        t.name = None
        t_ptr = ctypes.pointer(t)
        arr_type = ctypes.POINTER(Treasure) * 1
        arr = arr_type(t_ptr)

        def fake(ptr, count_ref):
            count_ref._obj.value = 1
            return arr

        self.mock_lib.player_get_collected_treasures.side_effect = fake
        p = self._make()
        name = p.get_collected_treasures()[0]["name"]
        self.assertEqual(name, "")

    def test_get_collected_treasures_id_value(self):
        from treasure_runner.bindings import Treasure
        t = _make_treasure(tid=42)
        t_ptr = ctypes.pointer(t)
        arr_type = ctypes.POINTER(Treasure) * 1
        arr = arr_type(t_ptr)

        def fake(ptr, count_ref):
            count_ref._obj.value = 1
            return arr

        self.mock_lib.player_get_collected_treasures.side_effect = fake
        p = self._make()
        self.assertEqual(p.get_collected_treasures()[0]["id"], 42)


# ============================================================
# GameEngine model (mocked lib)
# ============================================================

class TestGameEngine(unittest.TestCase):

    def setUp(self):
        self.patcher = patch("treasure_runner.models.game_engine.lib")
        self.mock_lib = self.patcher.start()

        def fake_create(path, eng_ref):
            eng_ref._obj.value = 0xCAFE
            return 0  # OK

        self.mock_lib.game_engine_create.side_effect = fake_create
        self.mock_lib.game_engine_get_player.return_value = ctypes.c_void_p(0xBEEF)

    def tearDown(self):
        self.patcher.stop()

    def _engine(self):
        from treasure_runner.models.game_engine import GameEngine
        return GameEngine("/fake/config.ini")

    # --- construction ---

    def test_create_calls_lib(self):
        self._engine()
        self.mock_lib.game_engine_create.assert_called_once()

    def test_create_encodes_path(self):
        self._engine()
        args = self.mock_lib.game_engine_create.call_args[0]
        self.assertEqual(args[0], b"/fake/config.ini")

    def test_create_failure_raises(self):
        from treasure_runner.models.exceptions import GameEngineError
        self.mock_lib.game_engine_create.side_effect = None
        self.mock_lib.game_engine_create.return_value = 1  # INVALID_ARGUMENT
        from treasure_runner.models.game_engine import GameEngine
        with self.assertRaises(GameEngineError):
            GameEngine("/bad.ini")

    def test_null_player_raises_runtime(self):
        self.mock_lib.game_engine_get_player.return_value = None
        from treasure_runner.models.game_engine import GameEngine
        with self.assertRaises(RuntimeError):
            GameEngine("/fake.ini")

    def test_player_property_is_player(self):
        from treasure_runner.models.player import Player
        eng = self._engine()
        self.assertIsInstance(eng.player, Player)

    # --- destroy ---

    def test_destroy_calls_lib(self):
        eng = self._engine()
        eng.destroy()
        self.mock_lib.game_engine_destroy.assert_called_once()

    def test_destroy_idempotent(self):
        eng = self._engine()
        eng.destroy()
        eng.destroy()
        self.assertEqual(self.mock_lib.game_engine_destroy.call_count, 1)

    def test_destroy_nulls_pointer(self):
        eng = self._engine()
        eng.destroy()
        self.assertIsNone(eng._eng)

    # --- move_player ---

    def test_move_ok_no_raise(self):
        from treasure_runner.bindings import Direction
        self.mock_lib.game_engine_move_player.return_value = 0
        eng = self._engine()
        eng.move_player(Direction.NORTH)  # no exception

    def test_move_passes_direction_int(self):
        from treasure_runner.bindings import Direction
        self.mock_lib.game_engine_move_player.return_value = 0
        eng = self._engine()
        eng.move_player(Direction.EAST)
        args = self.mock_lib.game_engine_move_player.call_args[0]
        self.assertEqual(args[1], int(Direction.EAST))

    def test_move_impassable_raises(self):
        from treasure_runner.bindings import Direction
        from treasure_runner.models.exceptions import ImpassableError
        self.mock_lib.game_engine_move_player.return_value = 6  # ROOM_IMPASSABLE
        eng = self._engine()
        with self.assertRaises(ImpassableError):
            eng.move_player(Direction.SOUTH)

    def test_move_no_such_room_raises(self):
        from treasure_runner.bindings import Direction
        from treasure_runner.models.exceptions import NoSuchRoomError
        self.mock_lib.game_engine_move_player.return_value = 9  # GE_NO_SUCH_ROOM
        eng = self._engine()
        with self.assertRaises(NoSuchRoomError):
            eng.move_player(Direction.WEST)

    def test_move_internal_error_raises(self):
        from treasure_runner.bindings import Direction
        from treasure_runner.models.exceptions import InternalError
        self.mock_lib.game_engine_move_player.return_value = 5  # INTERNAL_ERROR
        eng = self._engine()
        with self.assertRaises(InternalError):
            eng.move_player(Direction.NORTH)

    # --- render_current_room ---

    def test_render_returns_string(self):
        def fake_render(eng, str_ref):
            str_ref._obj.value = b"###\n.@.\n###\n"
            return 0
        self.mock_lib.game_engine_render_current_room.side_effect = fake_render
        eng = self._engine()
        result = eng.render_current_room()
        self.assertIsInstance(result, str)

    def test_render_content(self):
        def fake_render(eng, str_ref):
            str_ref._obj.value = b"ROOM"
            return 0
        self.mock_lib.game_engine_render_current_room.side_effect = fake_render
        eng = self._engine()
        self.assertEqual(eng.render_current_room(), "ROOM")

    def test_render_frees_string(self):
        def fake_render(eng, str_ref):
            str_ref._obj.value = b"x"
            return 0
        self.mock_lib.game_engine_render_current_room.side_effect = fake_render
        eng = self._engine()
        eng.render_current_room()
        self.mock_lib.game_engine_free_string.assert_called()

    def test_render_null_bytes_returns_empty(self):
        def fake_render(eng, str_ref):
            str_ref._obj.value = None
            return 0
        self.mock_lib.game_engine_render_current_room.side_effect = fake_render
        eng = self._engine()
        self.assertEqual(eng.render_current_room(), "")

    def test_render_error_raises(self):
        from treasure_runner.models.exceptions import GameEngineError
        self.mock_lib.game_engine_render_current_room.return_value = 5
        eng = self._engine()
        with self.assertRaises(GameEngineError):
            eng.render_current_room()

    # --- get_room_count ---

    def test_get_room_count_returns_int(self):
        def fake(eng, count_ref):
            count_ref._obj.value = 3
            return 0
        self.mock_lib.game_engine_get_room_count.side_effect = fake
        eng = self._engine()
        self.assertEqual(eng.get_room_count(), 3)

    def test_get_room_count_error_raises(self):
        from treasure_runner.models.exceptions import GameEngineError
        self.mock_lib.game_engine_get_room_count.return_value = 1
        eng = self._engine()
        with self.assertRaises(GameEngineError):
            eng.get_room_count()

    # --- get_room_dimensions ---

    def test_get_room_dimensions_returns_tuple(self):
        def fake(eng, w_ref, h_ref):
            w_ref._obj.value = 20
            h_ref._obj.value = 15
            return 0
        self.mock_lib.game_engine_get_room_dimensions.side_effect = fake
        eng = self._engine()
        w, h = eng.get_room_dimensions()
        self.assertEqual(w, 20)
        self.assertEqual(h, 15)

    def test_get_room_dimensions_is_two_tuple(self):
        def fake(eng, w_ref, h_ref):
            w_ref._obj.value = 10
            h_ref._obj.value = 10
            return 0
        self.mock_lib.game_engine_get_room_dimensions.side_effect = fake
        eng = self._engine()
        result = eng.get_room_dimensions()
        self.assertEqual(len(result), 2)

    # --- get_room_ids ---

    def test_get_room_ids_returns_list(self):
        arr = (ctypes.c_int * 2)(10, 20)
        arr_ptr = ctypes.cast(arr, ctypes.POINTER(ctypes.c_int))

        def fake(eng, ids_ref, count_ref):
            ctypes.memmove(ids_ref, ctypes.byref(arr_ptr),
                           ctypes.sizeof(ctypes.POINTER(ctypes.c_int)))
            ctypes.memmove(count_ref, ctypes.byref(ctypes.c_int(2)),
                           ctypes.sizeof(ctypes.c_int))
            return 0

        self.mock_lib.game_engine_get_room_ids.side_effect = fake
        eng = self._engine()
        result = eng.get_room_ids()
        self.assertIsInstance(result, list)
        self.assertEqual(len(result), 2)

    def test_get_room_ids_frees_array(self):
        arr = (ctypes.c_int * 1)(5)
        arr_ptr = ctypes.cast(arr, ctypes.POINTER(ctypes.c_int))

        def fake(eng, ids_ref, count_ref):
            ctypes.memmove(ids_ref, ctypes.byref(arr_ptr),
                           ctypes.sizeof(ctypes.POINTER(ctypes.c_int)))
            ctypes.memmove(count_ref, ctypes.byref(ctypes.c_int(1)),
                           ctypes.sizeof(ctypes.c_int))
            return 0

        self.mock_lib.game_engine_get_room_ids.side_effect = fake
        eng = self._engine()
        eng.get_room_ids()
        self.mock_lib.game_engine_free_string.assert_called()

    def test_get_room_ids_error_raises(self):
        from treasure_runner.models.exceptions import GameEngineError
        self.mock_lib.game_engine_get_room_ids.return_value = 5
        eng = self._engine()
        with self.assertRaises(GameEngineError):
            eng.get_room_ids()

    # --- reset ---

    def test_reset_calls_lib(self):
        self.mock_lib.game_engine_reset.return_value = 0
        eng = self._engine()
        eng.reset()
        self.mock_lib.game_engine_reset.assert_called_once()

    def test_reset_error_raises(self):
        from treasure_runner.models.exceptions import GameEngineError
        self.mock_lib.game_engine_reset.return_value = 5
        eng = self._engine()
        with self.assertRaises(GameEngineError):
            eng.reset()


# ============================================================
# Binding signatures (lib argtypes/restype)
# ============================================================

class TestLibSignatures(unittest.TestCase):

    def _lib(self):
        from treasure_runner.bindings.bindings import lib
        return lib

    def test_game_engine_create_argtypes(self):
        self.assertIsNotNone(self._lib().game_engine_create.argtypes)

    def test_game_engine_create_restype(self):
        self.assertIsNotNone(self._lib().game_engine_create.restype)

    def test_game_engine_destroy_argtypes(self):
        self.assertIsNotNone(self._lib().game_engine_destroy.argtypes)

    def test_move_player_argtypes(self):
        self.assertIsNotNone(self._lib().game_engine_move_player.argtypes)

    def test_player_get_room_argtypes(self):
        self.assertIsNotNone(self._lib().player_get_room.argtypes)

    def test_player_get_position_argtypes(self):
        self.assertIsNotNone(self._lib().player_get_position.argtypes)

    def test_player_get_collected_treasures_restype(self):
        self.assertIsNotNone(self._lib().player_get_collected_treasures.restype)

    def test_game_engine_free_string_argtypes(self):
        self.assertIsNotNone(self._lib().game_engine_free_string.argtypes)

    def test_render_current_room_argtypes(self):
        self.assertIsNotNone(self._lib().game_engine_render_current_room.argtypes)

    def test_get_room_ids_argtypes(self):
        self.assertIsNotNone(self._lib().game_engine_get_room_ids.argtypes)

    def test_game_engine_reset_argtypes(self):
        self.assertIsNotNone(self._lib().game_engine_reset.argtypes)

    def test_player_get_collected_count_argtypes(self):
        self.assertIsNotNone(self._lib().player_get_collected_count.argtypes)

    def test_player_has_collected_treasure_argtypes(self):
        self.assertIsNotNone(self._lib().player_has_collected_treasure.argtypes)

    """
Unit tests for the Treasure Runner Python layer.

Covers:
  - bindings: enums, Treasure struct, Status/Direction values, lib signatures
  - exceptions: hierarchy, status_to_exception, status_to_status_exception
  - models/player.py: all Player methods via mocked lib
  - models/game_engine.py: all GameEngine methods via mocked lib
"""

import ctypes
import unittest
from unittest.mock import MagicMock, patch, call


# ============================================================
# Helpers
# ============================================================

def _make_treasure(tid=1, name=b"Gold", starting_room_id=0,
                   initial_x=2, initial_y=3, x=2, y=3, collected=False):
    from treasure_runner.bindings import Treasure
    t = Treasure()
    t.id = tid
    t.name = name
    t.starting_room_id = starting_room_id
    t.initial_x = initial_x
    t.initial_y = initial_y
    t.x = x
    t.y = y
    t.collected = collected
    return t


# ============================================================
# Status enum
# ============================================================

class TestStatusEnum(unittest.TestCase):

    def test_ok_is_zero(self):
        from treasure_runner.bindings import Status
        self.assertEqual(Status.OK, 0)

    def test_invalid_argument(self):
        from treasure_runner.bindings import Status
        self.assertEqual(Status.INVALID_ARGUMENT, 1)

    def test_null_pointer(self):
        from treasure_runner.bindings import Status
        self.assertEqual(Status.NULL_POINTER, 2)

    def test_no_memory(self):
        from treasure_runner.bindings import Status
        self.assertEqual(Status.NO_MEMORY, 3)

    def test_bounds_exceeded(self):
        from treasure_runner.bindings import Status
        self.assertEqual(Status.BOUNDS_EXCEEDED, 4)

    def test_internal_error(self):
        from treasure_runner.bindings import Status
        self.assertEqual(Status.INTERNAL_ERROR, 5)

    def test_room_impassable(self):
        from treasure_runner.bindings import Status
        self.assertEqual(Status.ROOM_IMPASSABLE, 6)

    def test_room_no_portal(self):
        from treasure_runner.bindings import Status
        self.assertEqual(Status.ROOM_NO_PORTAL, 7)

    def test_room_not_found(self):
        from treasure_runner.bindings import Status
        self.assertEqual(Status.ROOM_NOT_FOUND, 8)

    def test_ge_no_such_room(self):
        from treasure_runner.bindings import Status
        self.assertEqual(Status.GE_NO_SUCH_ROOM, 9)

    def test_game_engine_status_is_status(self):
        from treasure_runner.bindings import Status, GameEngineStatus
        self.assertIs(GameEngineStatus, Status)


# ============================================================
# Direction enum
# ============================================================

class TestDirectionEnum(unittest.TestCase):

    def test_north_is_zero(self):
        from treasure_runner.bindings import Direction
        self.assertEqual(Direction.NORTH, 0)

    def test_south(self):
        from treasure_runner.bindings import Direction
        self.assertEqual(Direction.SOUTH, 1)

    def test_east(self):
        from treasure_runner.bindings import Direction
        self.assertEqual(Direction.EAST, 2)

    def test_west(self):
        from treasure_runner.bindings import Direction
        self.assertEqual(Direction.WEST, 3)

    def test_four_directions(self):
        from treasure_runner.bindings import Direction
        self.assertEqual(len(Direction), 4)

    def test_name_strings(self):
        from treasure_runner.bindings import Direction
        self.assertEqual(Direction.NORTH.name, "NORTH")
        self.assertEqual(Direction.SOUTH.name, "SOUTH")
        self.assertEqual(Direction.EAST.name, "EAST")
        self.assertEqual(Direction.WEST.name, "WEST")


# ============================================================
# Treasure struct
# ============================================================

class TestTreasureStruct(unittest.TestCase):

    def test_all_fields_exist(self):
        from treasure_runner.bindings import Treasure
        names = [f[0] for f in Treasure._fields_]
        for field in ["id", "name", "starting_room_id", "initial_x",
                      "initial_y", "x", "y", "collected"]:
            self.assertIn(field, names)

    def test_id_field(self):
        t = _make_treasure(tid=42)
        self.assertEqual(t.id, 42)

    def test_position_fields(self):
        t = _make_treasure(x=9, y=4)
        self.assertEqual(t.x, 9)
        self.assertEqual(t.y, 4)

    def test_initial_position_fields(self):
        t = _make_treasure(initial_x=1, initial_y=2)
        self.assertEqual(t.initial_x, 1)
        self.assertEqual(t.initial_y, 2)

    def test_starting_room_id(self):
        t = _make_treasure(starting_room_id=7)
        self.assertEqual(t.starting_room_id, 7)

    def test_collected_false(self):
        t = _make_treasure(collected=False)
        self.assertFalse(t.collected)

    def test_collected_true(self):
        t = _make_treasure(collected=True)
        self.assertTrue(t.collected)

    def test_name_bytes(self):
        t = _make_treasure(name=b"Diamond")
        self.assertEqual(t.name, b"Diamond")


# ============================================================
# Exception hierarchy
# ============================================================

class TestExceptionHierarchy(unittest.TestCase):

    def test_game_error_is_base_exception(self):
        from treasure_runner.models.exceptions import GameError
        self.assertTrue(issubclass(GameError, Exception))

    def test_game_engine_error_inherits_game_error(self):
        from treasure_runner.models.exceptions import GameEngineError, GameError
        self.assertTrue(issubclass(GameEngineError, GameError))

    def test_invalid_argument_inherits_game_engine_error(self):
        from treasure_runner.models.exceptions import InvalidArgumentError, GameEngineError
        self.assertTrue(issubclass(InvalidArgumentError, GameEngineError))

    def test_out_of_bounds_inherits_game_engine_error(self):
        from treasure_runner.models.exceptions import OutOfBoundsError, GameEngineError
        self.assertTrue(issubclass(OutOfBoundsError, GameEngineError))

    def test_impassable_inherits_game_engine_error(self):
        from treasure_runner.models.exceptions import ImpassableError, GameEngineError
        self.assertTrue(issubclass(ImpassableError, GameEngineError))

    def test_no_such_room_inherits_game_engine_error(self):
        from treasure_runner.models.exceptions import NoSuchRoomError, GameEngineError
        self.assertTrue(issubclass(NoSuchRoomError, GameEngineError))

    def test_no_portal_inherits_game_engine_error(self):
        from treasure_runner.models.exceptions import NoPortalError, GameEngineError
        self.assertTrue(issubclass(NoPortalError, GameEngineError))

    def test_internal_error_inherits_game_engine_error(self):
        from treasure_runner.models.exceptions import InternalError, GameEngineError
        self.assertTrue(issubclass(InternalError, GameEngineError))

    def test_status_error_inherits_game_error(self):
        from treasure_runner.models.exceptions import StatusError, GameError
        self.assertTrue(issubclass(StatusError, GameError))

    def test_all_status_subclasses_inherit_status_error(self):
        from treasure_runner.models.exceptions import (
            StatusInvalidArgumentError, StatusNullPointerError,
            StatusNoMemoryError, StatusBoundsExceededError,
            StatusImpassableError, StatusInternalError, StatusError,
        )
        for cls in [StatusInvalidArgumentError, StatusNullPointerError,
                    StatusNoMemoryError, StatusBoundsExceededError,
                    StatusImpassableError, StatusInternalError]:
            with self.subTest(cls=cls):
                self.assertTrue(issubclass(cls, StatusError))

    def test_exceptions_are_raisable(self):
        from treasure_runner.models.exceptions import (
            GameError, ImpassableError, NoSuchRoomError, InternalError
        )
        for exc_cls in [GameError, ImpassableError, NoSuchRoomError, InternalError]:
            with self.subTest(exc=exc_cls):
                with self.assertRaises(exc_cls):
                    raise exc_cls("test")


# ============================================================
# status_to_exception
# ============================================================

class TestStatusToException(unittest.TestCase):

    def _ste(self, status_val, msg=None):
        from treasure_runner.models.exceptions import status_to_exception
        if msg:
            return status_to_exception(status_val, msg)
        return status_to_exception(status_val)

    def test_invalid_argument(self):
        from treasure_runner.bindings import Status
        from treasure_runner.models.exceptions import InvalidArgumentError
        exc = self._ste(Status.INVALID_ARGUMENT)
        self.assertIsInstance(exc, InvalidArgumentError)

    def test_bounds_exceeded(self):
        from treasure_runner.bindings import Status
        from treasure_runner.models.exceptions import OutOfBoundsError
        exc = self._ste(Status.BOUNDS_EXCEEDED)
        self.assertIsInstance(exc, OutOfBoundsError)

    def test_room_impassable(self):
        from treasure_runner.bindings import Status
        from treasure_runner.models.exceptions import ImpassableError
        exc = self._ste(Status.ROOM_IMPASSABLE)
        self.assertIsInstance(exc, ImpassableError)

    def test_no_portal(self):
        from treasure_runner.bindings import Status
        from treasure_runner.models.exceptions import NoPortalError
        exc = self._ste(Status.ROOM_NO_PORTAL)
        self.assertIsInstance(exc, NoPortalError)

    def test_no_such_room(self):
        from treasure_runner.bindings import Status
        from treasure_runner.models.exceptions import NoSuchRoomError
        exc = self._ste(Status.GE_NO_SUCH_ROOM)
        self.assertIsInstance(exc, NoSuchRoomError)

    def test_internal_error(self):
        from treasure_runner.bindings import Status
        from treasure_runner.models.exceptions import InternalError
        exc = self._ste(Status.INTERNAL_ERROR)
        self.assertIsInstance(exc, InternalError)

    def test_null_pointer_maps_to_internal(self):
        from treasure_runner.bindings import Status
        from treasure_runner.models.exceptions import InternalError
        exc = self._ste(Status.NULL_POINTER)
        self.assertIsInstance(exc, InternalError)

    def test_message_preserved(self):
        from treasure_runner.bindings import Status
        exc = self._ste(Status.INVALID_ARGUMENT, "my message")
        self.assertIn("my message", str(exc))

    def test_unknown_status_gives_game_engine_error(self):
        from treasure_runner.models.exceptions import GameEngineError
        exc = self._ste(9999)
        self.assertIsInstance(exc, GameEngineError)


# ============================================================
# status_to_status_exception
# ============================================================

class TestStatusToStatusException(unittest.TestCase):

    def _stse(self, status_val):
        from treasure_runner.models.exceptions import status_to_status_exception
        return status_to_status_exception(status_val)

    def test_invalid_argument(self):
        from treasure_runner.bindings import Status
        from treasure_runner.models.exceptions import StatusInvalidArgumentError
        self.assertIsInstance(self._stse(Status.INVALID_ARGUMENT), StatusInvalidArgumentError)

    def test_null_pointer(self):
        from treasure_runner.bindings import Status
        from treasure_runner.models.exceptions import StatusNullPointerError
        self.assertIsInstance(self._stse(Status.NULL_POINTER), StatusNullPointerError)

    def test_no_memory(self):
        from treasure_runner.bindings import Status
        from treasure_runner.models.exceptions import StatusNoMemoryError
        self.assertIsInstance(self._stse(Status.NO_MEMORY), StatusNoMemoryError)

    def test_bounds_exceeded(self):
        from treasure_runner.bindings import Status
        from treasure_runner.models.exceptions import StatusBoundsExceededError
        self.assertIsInstance(self._stse(Status.BOUNDS_EXCEEDED), StatusBoundsExceededError)

    def test_impassable(self):
        from treasure_runner.bindings import Status
        from treasure_runner.models.exceptions import StatusImpassableError
        self.assertIsInstance(self._stse(Status.ROOM_IMPASSABLE), StatusImpassableError)

    def test_internal(self):
        from treasure_runner.bindings import Status
        from treasure_runner.models.exceptions import StatusInternalError
        self.assertIsInstance(self._stse(Status.INTERNAL_ERROR), StatusInternalError)

    def test_unknown_falls_back(self):
        from treasure_runner.models.exceptions import StatusError
        self.assertIsInstance(self._stse(9999), StatusError)


# ============================================================
# Player model (mocked lib)
# ============================================================

class TestPlayer(unittest.TestCase):

    def setUp(self):
        self.patcher = patch("treasure_runner.models.player.lib")
        self.mock_lib = self.patcher.start()
        from treasure_runner.models.player import Player
        self.Player = Player
        self.ptr = ctypes.c_void_p(0xDEAD)

    def tearDown(self):
        self.patcher.stop()

    def _make(self):
        return self.Player(self.ptr)

    def test_init_stores_ptr(self):
        p = self._make()
        self.assertEqual(p._ptr, self.ptr)

    def test_get_room_delegates(self):
        self.mock_lib.player_get_room.return_value = 3
        p = self._make()
        self.assertEqual(p.get_room(), 3)
        self.mock_lib.player_get_room.assert_called_once_with(self.ptr)

    def test_get_room_returns_int(self):
        self.mock_lib.player_get_room.return_value = 0
        p = self._make()
        self.assertIsInstance(p.get_room(), int)

    def test_get_position_returns_tuple(self):
        def fake(ptr, xr, yr):
            xr._obj.value = 5
            yr._obj.value = 8
            return 0
        self.mock_lib.player_get_position.side_effect = fake
        p = self._make()
        result = p.get_position()
        self.assertIsInstance(result, tuple)
        self.assertEqual(len(result), 2)

    def test_get_position_values(self):
        def fake(ptr, xr, yr):
            xr._obj.value = 10
            yr._obj.value = 20
            return 0
        self.mock_lib.player_get_position.side_effect = fake
        p = self._make()
        x, y = p.get_position()
        self.assertEqual(x, 10)
        self.assertEqual(y, 20)

    def test_get_collected_count_zero(self):
        self.mock_lib.player_get_collected_count.return_value = 0
        p = self._make()
        self.assertEqual(p.get_collected_count(), 0)

    def test_get_collected_count_nonzero(self):
        self.mock_lib.player_get_collected_count.return_value = 5
        p = self._make()
        self.assertEqual(p.get_collected_count(), 5)
        self.mock_lib.player_get_collected_count.assert_called_once_with(self.ptr)

    def test_has_collected_true(self):
        self.mock_lib.player_has_collected_treasure.return_value = True
        p = self._make()
        self.assertTrue(p.has_collected_treasure(7))
        self.mock_lib.player_has_collected_treasure.assert_called_once_with(self.ptr, 7)

    def test_has_collected_false(self):
        self.mock_lib.player_has_collected_treasure.return_value = False
        p = self._make()
        self.assertFalse(p.has_collected_treasure(999))

    def test_get_collected_treasures_empty_on_null(self):
        self.mock_lib.player_get_collected_treasures.return_value = None
        p = self._make()
        self.assertEqual(p.get_collected_treasures(), [])

    def test_get_collected_treasures_empty_on_zero_count(self):
        def fake(ptr, count_ref):
            count_ref._obj.value = 0
            return None
        self.mock_lib.player_get_collected_treasures.side_effect = fake
        p = self._make()
        self.assertEqual(p.get_collected_treasures(), [])

    def test_get_collected_treasures_returns_list(self):
        from treasure_runner.bindings import Treasure
        t = _make_treasure(tid=3, name=b"Ruby", x=1, y=2, collected=True)
        t_ptr = ctypes.pointer(t)
        arr_type = ctypes.POINTER(Treasure) * 1
        arr = arr_type(t_ptr)

        def fake(ptr, count_ref):
            count_ref._obj.value = 1
            return arr

        self.mock_lib.player_get_collected_treasures.side_effect = fake
        p = self._make()
        result = p.get_collected_treasures()
        self.assertIsInstance(result, list)
        self.assertEqual(len(result), 1)

    def test_get_collected_treasures_dict_keys(self):
        from treasure_runner.bindings import Treasure
        t = _make_treasure(tid=1, name=b"Gold")
        t_ptr = ctypes.pointer(t)
        arr_type = ctypes.POINTER(Treasure) * 1
        arr = arr_type(t_ptr)

        def fake(ptr, count_ref):
            count_ref._obj.value = 1
            return arr

        self.mock_lib.player_get_collected_treasures.side_effect = fake
        p = self._make()
        d = p.get_collected_treasures()[0]
        for key in ["id", "name", "starting_room_id", "initial_x",
                    "initial_y", "x", "y", "collected"]:
            self.assertIn(key, d)

    def test_get_collected_treasures_name_decoded(self):
        from treasure_runner.bindings import Treasure
        t = _make_treasure(name=b"Sapphire")
        t_ptr = ctypes.pointer(t)
        arr_type = ctypes.POINTER(Treasure) * 1
        arr = arr_type(t_ptr)

        def fake(ptr, count_ref):
            count_ref._obj.value = 1
            return arr

        self.mock_lib.player_get_collected_treasures.side_effect = fake
        p = self._make()
        name = p.get_collected_treasures()[0]["name"]
        self.assertIsInstance(name, str)
        self.assertEqual(name, "Sapphire")

    def test_get_collected_treasures_null_name(self):
        from treasure_runner.bindings import Treasure
        t = _make_treasure(name=None)
        t.name = None
        t_ptr = ctypes.pointer(t)
        arr_type = ctypes.POINTER(Treasure) * 1
        arr = arr_type(t_ptr)

        def fake(ptr, count_ref):
            count_ref._obj.value = 1
            return arr

        self.mock_lib.player_get_collected_treasures.side_effect = fake
        p = self._make()
        name = p.get_collected_treasures()[0]["name"]
        self.assertEqual(name, "")

    def test_get_collected_treasures_id_value(self):
        from treasure_runner.bindings import Treasure
        t = _make_treasure(tid=42)
        t_ptr = ctypes.pointer(t)
        arr_type = ctypes.POINTER(Treasure) * 1
        arr = arr_type(t_ptr)

        def fake(ptr, count_ref):
            count_ref._obj.value = 1
            return arr

        self.mock_lib.player_get_collected_treasures.side_effect = fake
        p = self._make()
        self.assertEqual(p.get_collected_treasures()[0]["id"], 42)


# ============================================================
# GameEngine model (mocked lib)
# ============================================================

class TestLibSignatures(unittest.TestCase):

    def _lib(self):
        from treasure_runner.bindings.bindings import lib
        return lib

    def test_game_engine_create_argtypes(self):
        self.assertIsNotNone(self._lib().game_engine_create.argtypes)

    def test_game_engine_create_restype(self):
        self.assertIsNotNone(self._lib().game_engine_create.restype)

    def test_game_engine_destroy_argtypes(self):
        self.assertIsNotNone(self._lib().game_engine_destroy.argtypes)

    def test_move_player_argtypes(self):
        self.assertIsNotNone(self._lib().game_engine_move_player.argtypes)

    def test_player_get_room_argtypes(self):
        self.assertIsNotNone(self._lib().player_get_room.argtypes)

    def test_player_get_position_argtypes(self):
        self.assertIsNotNone(self._lib().player_get_position.argtypes)

    def test_player_get_collected_treasures_restype(self):
        self.assertIsNotNone(self._lib().player_get_collected_treasures.restype)

    def test_game_engine_free_string_argtypes(self):
        self.assertIsNotNone(self._lib().game_engine_free_string.argtypes)

    def test_render_current_room_argtypes(self):
        self.assertIsNotNone(self._lib().game_engine_render_current_room.argtypes)

    def test_get_room_ids_argtypes(self):
        self.assertIsNotNone(self._lib().game_engine_get_room_ids.argtypes)

    def test_game_engine_reset_argtypes(self):
        self.assertIsNotNone(self._lib().game_engine_reset.argtypes)

    def test_player_get_collected_count_argtypes(self):
        self.assertIsNotNone(self._lib().player_get_collected_count.argtypes)

    def test_player_has_collected_treasure_argtypes(self):
        self.assertIsNotNone(self._lib().player_has_collected_treasure.argtypes)

    """
Unit tests for the Treasure Runner Python layer.

Covers:
  - bindings: enums, Treasure struct, Status/Direction values, lib signatures
  - exceptions: hierarchy, status_to_exception, status_to_status_exception
  - models/player.py: all Player methods via mocked lib
  - models/game_engine.py: all GameEngine methods via mocked lib
"""

import ctypes
import unittest
from unittest.mock import MagicMock, patch, call

class TestGameEngine(unittest.TestCase):

    def setUp(self):
        self.patcher = patch("treasure_runner.models.game_engine.lib")
        self.mock_lib = self.patcher.start()

        def fake_create(path, eng_ref):
            eng_ref._obj.value = 0xCAFE
            return 0  # OK

        self.mock_lib.game_engine_create.side_effect = fake_create
        self.mock_lib.game_engine_get_player.return_value = ctypes.c_void_p(0xBEEF)

    def tearDown(self):
        self.patcher.stop()

    def _engine(self):
        from treasure_runner.models.game_engine import GameEngine
        return GameEngine("/fake/config.ini")

    # --- construction ---

    def test_create_calls_lib(self):
        self._engine()
        self.mock_lib.game_engine_create.assert_called_once()

    def test_create_encodes_path(self):
        self._engine()
        args = self.mock_lib.game_engine_create.call_args[0]
        self.assertEqual(args[0], b"/fake/config.ini")

    def test_create_failure_raises(self):
        from treasure_runner.models.exceptions import GameEngineError
        self.mock_lib.game_engine_create.side_effect = None
        self.mock_lib.game_engine_create.return_value = 1  # INVALID_ARGUMENT
        from treasure_runner.models.game_engine import GameEngine
        with self.assertRaises(GameEngineError):
            GameEngine("/bad.ini")

    def test_null_player_raises_runtime(self):
        self.mock_lib.game_engine_get_player.return_value = None
        from treasure_runner.models.game_engine import GameEngine
        with self.assertRaises(RuntimeError):
            GameEngine("/fake.ini")

    def test_player_property_is_player(self):
        from treasure_runner.models.player import Player
        eng = self._engine()
        self.assertIsInstance(eng.player, Player)

    # --- destroy ---

    def test_destroy_calls_lib(self):
        eng = self._engine()
        eng.destroy()
        self.mock_lib.game_engine_destroy.assert_called_once()

    def test_destroy_idempotent(self):
        eng = self._engine()
        eng.destroy()
        eng.destroy()
        self.assertEqual(self.mock_lib.game_engine_destroy.call_count, 1)

    def test_destroy_nulls_pointer(self):
        eng = self._engine()
        eng.destroy()
        self.assertIsNone(eng._eng)

    # --- move_player ---

    def test_move_ok_no_raise(self):
        from treasure_runner.bindings import Direction
        self.mock_lib.game_engine_move_player.return_value = 0
        eng = self._engine()
        eng.move_player(Direction.NORTH)  # no exception

    def test_move_passes_direction_int(self):
        from treasure_runner.bindings import Direction
        self.mock_lib.game_engine_move_player.return_value = 0
        eng = self._engine()
        eng.move_player(Direction.EAST)
        args = self.mock_lib.game_engine_move_player.call_args[0]
        self.assertEqual(args[1], int(Direction.EAST))

    def test_move_impassable_raises(self):
        from treasure_runner.bindings import Direction
        from treasure_runner.models.exceptions import ImpassableError
        self.mock_lib.game_engine_move_player.return_value = 6  # ROOM_IMPASSABLE
        eng = self._engine()
        with self.assertRaises(ImpassableError):
            eng.move_player(Direction.SOUTH)

    def test_move_no_such_room_raises(self):
        from treasure_runner.bindings import Direction
        from treasure_runner.models.exceptions import NoSuchRoomError
        self.mock_lib.game_engine_move_player.return_value = 9  # GE_NO_SUCH_ROOM
        eng = self._engine()
        with self.assertRaises(NoSuchRoomError):
            eng.move_player(Direction.WEST)

    def test_move_internal_error_raises(self):
        from treasure_runner.bindings import Direction
        from treasure_runner.models.exceptions import InternalError
        self.mock_lib.game_engine_move_player.return_value = 5  # INTERNAL_ERROR
        eng = self._engine()
        with self.assertRaises(InternalError):
            eng.move_player(Direction.NORTH)

    # --- render_current_room ---

    def test_render_returns_string(self):
        def fake_render(eng, str_ref):
            str_ref._obj.value = b"###\n.@.\n###\n"
            return 0
        self.mock_lib.game_engine_render_current_room.side_effect = fake_render
        eng = self._engine()
        result = eng.render_current_room()
        self.assertIsInstance(result, str)

    def test_render_content(self):
        def fake_render(eng, str_ref):
            str_ref._obj.value = b"ROOM"
            return 0
        self.mock_lib.game_engine_render_current_room.side_effect = fake_render
        eng = self._engine()
        self.assertEqual(eng.render_current_room(), "ROOM")

    def test_render_frees_string(self):
        def fake_render(eng, str_ref):
            str_ref._obj.value = b"x"
            return 0
        self.mock_lib.game_engine_render_current_room.side_effect = fake_render
        eng = self._engine()
        eng.render_current_room()
        self.mock_lib.game_engine_free_string.assert_called()

    def test_render_null_bytes_returns_empty(self):
        def fake_render(eng, str_ref):
            str_ref._obj.value = None
            return 0
        self.mock_lib.game_engine_render_current_room.side_effect = fake_render
        eng = self._engine()
        self.assertEqual(eng.render_current_room(), "")

    def test_render_error_raises(self):
        from treasure_runner.models.exceptions import GameEngineError
        self.mock_lib.game_engine_render_current_room.return_value = 5
        eng = self._engine()
        with self.assertRaises(GameEngineError):
            eng.render_current_room()

    # --- get_room_count ---

    def test_get_room_count_returns_int(self):
        def fake(eng, count_ref):
            count_ref._obj.value = 3
            return 0
        self.mock_lib.game_engine_get_room_count.side_effect = fake
        eng = self._engine()
        self.assertEqual(eng.get_room_count(), 3)

    def test_get_room_count_error_raises(self):
        from treasure_runner.models.exceptions import GameEngineError
        self.mock_lib.game_engine_get_room_count.return_value = 1
        eng = self._engine()
        with self.assertRaises(GameEngineError):
            eng.get_room_count()

    # --- get_room_dimensions ---

    def test_get_room_dimensions_returns_tuple(self):
        def fake(eng, w_ref, h_ref):
            w_ref._obj.value = 20
            h_ref._obj.value = 15
            return 0
        self.mock_lib.game_engine_get_room_dimensions.side_effect = fake
        eng = self._engine()
        w, h = eng.get_room_dimensions()
        self.assertEqual(w, 20)
        self.assertEqual(h, 15)

    def test_get_room_dimensions_is_two_tuple(self):
        def fake(eng, w_ref, h_ref):
            w_ref._obj.value = 10
            h_ref._obj.value = 10
            return 0
        self.mock_lib.game_engine_get_room_dimensions.side_effect = fake
        eng = self._engine()
        result = eng.get_room_dimensions()
        self.assertEqual(len(result), 2)

    # --- get_room_ids ---

    def test_get_room_ids_returns_list(self):
        arr = (ctypes.c_int * 2)(10, 20)
        arr_ptr = ctypes.cast(arr, ctypes.POINTER(ctypes.c_int))

        def fake(eng, ids_ref, count_ref):
            ctypes.memmove(ids_ref, ctypes.byref(arr_ptr),
                           ctypes.sizeof(ctypes.POINTER(ctypes.c_int)))
            ctypes.memmove(count_ref, ctypes.byref(ctypes.c_int(2)),
                           ctypes.sizeof(ctypes.c_int))
            return 0

        self.mock_lib.game_engine_get_room_ids.side_effect = fake
        eng = self._engine()
        result = eng.get_room_ids()
        self.assertIsInstance(result, list)
        self.assertEqual(len(result), 2)

    def test_get_room_ids_frees_array(self):
        arr = (ctypes.c_int * 1)(5)
        arr_ptr = ctypes.cast(arr, ctypes.POINTER(ctypes.c_int))

        def fake(eng, ids_ref, count_ref):
            ctypes.memmove(ids_ref, ctypes.byref(arr_ptr),
                           ctypes.sizeof(ctypes.POINTER(ctypes.c_int)))
            ctypes.memmove(count_ref, ctypes.byref(ctypes.c_int(1)),
                           ctypes.sizeof(ctypes.c_int))
            return 0

        self.mock_lib.game_engine_get_room_ids.side_effect = fake
        eng = self._engine()
        eng.get_room_ids()
        self.mock_lib.game_engine_free_string.assert_called()

    def test_get_room_ids_error_raises(self):
        from treasure_runner.models.exceptions import GameEngineError
        self.mock_lib.game_engine_get_room_ids.return_value = 5
        eng = self._engine()
        with self.assertRaises(GameEngineError):
            eng.get_room_ids()

    # --- reset ---

    def test_reset_calls_lib(self):
        self.mock_lib.game_engine_reset.return_value = 0
        eng = self._engine()
        eng.reset()
        self.mock_lib.game_engine_reset.assert_called_once()

    def test_reset_error_raises(self):
        from treasure_runner.models.exceptions import GameEngineError
        self.mock_lib.game_engine_reset.return_value = 5
        eng = self._engine()
        with self.assertRaises(GameEngineError):
            eng.reset()


# ============================================================
# Binding signatures (lib argtypes/restype)
# ============================================================

class TestLibSignatures(unittest.TestCase):

    def _lib(self):
        from treasure_runner.bindings.bindings import lib
        return lib

    def test_game_engine_create_argtypes(self):
        self.assertIsNotNone(self._lib().game_engine_create.argtypes)

    def test_game_engine_create_restype(self):
        self.assertIsNotNone(self._lib().game_engine_create.restype)

    def test_game_engine_destroy_argtypes(self):
        self.assertIsNotNone(self._lib().game_engine_destroy.argtypes)

    def test_move_player_argtypes(self):
        self.assertIsNotNone(self._lib().game_engine_move_player.argtypes)

    def test_player_get_room_argtypes(self):
        self.assertIsNotNone(self._lib().player_get_room.argtypes)

    def test_player_get_position_argtypes(self):
        self.assertIsNotNone(self._lib().player_get_position.argtypes)

    def test_player_get_collected_treasures_restype(self):
        self.assertIsNotNone(self._lib().player_get_collected_treasures.restype)

    def test_game_engine_free_string_argtypes(self):
        self.assertIsNotNone(self._lib().game_engine_free_string.argtypes)

    def test_render_current_room_argtypes(self):
        self.assertIsNotNone(self._lib().game_engine_render_current_room.argtypes)

    def test_get_room_ids_argtypes(self):
        self.assertIsNotNone(self._lib().game_engine_get_room_ids.argtypes)

    def test_game_engine_reset_argtypes(self):
        self.assertIsNotNone(self._lib().game_engine_reset.argtypes)

    def test_player_get_collected_count_argtypes(self):
        self.assertIsNotNone(self._lib().player_get_collected_count.argtypes)

    def test_player_has_collected_treasure_argtypes(self):
        self.assertIsNotNone(self._lib().player_has_collected_treasure.argtypes)



if __name__ == "__main__":
    unittest.main()
