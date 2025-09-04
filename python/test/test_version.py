import alpaqa as pa


def test_version():
    py_v = pa.__version__.split("+", 1)
    cpp_v = pa.__c_version__.split("+", 1)
    assert py_v[0] == cpp_v[0]
    assert len(cpp_v) == 1 or py_v == cpp_v
