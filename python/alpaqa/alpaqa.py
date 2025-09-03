import os
import typing


def _is_truthy(s: typing.Optional[str]):
    if s is None:
        return False
    return s.lower() not in ("", "false", "no", "off", "0")


if not typing.TYPE_CHECKING and _is_truthy(os.getenv("ALPAQA_PYTHON_DEBUG")):
    from ._alpaqa_d import *  # noqa: F403
    from ._alpaqa_d import __version__ as __c_version__
    from ._alpaqa_d.float64 import *  # noqa: F403
else:
    from ._alpaqa import *  # noqa: F403
    from ._alpaqa import __version__ as __c_version__  # noqa: F401
    from ._alpaqa.float64 import *  # noqa: F403

del _is_truthy, typing, os
