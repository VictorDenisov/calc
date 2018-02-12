import unittest
import subprocess

import hypothesis
import hypothesis.strategies as st
import numpy
import pytest

# Ignore all floating point errors, we'll handle Inf and Nan results instead
numpy.seterr(all='ignore')
PARSERS = [ "compute"
          , "ast_iter"
          , "ast_rec"
          , "gccjit"
          , "libjit"
          , "llvm"
          , "elf"
          , "dl"
          ]
FLOAT_CALC_TYPES = [
    "long_double",
    "double",
    "float",
]
INT_CALC_TYPES = [ "long_int"
                 , "int"
                 , "char"
                 ]
CALC_TYPES = INT_CALC_TYPES + FLOAT_CALC_TYPES


def run_simple(calc_type, params):
    p = subprocess.run(["./main." + calc_type] + params, stdout=subprocess.PIPE)
    assert p.returncode == 0
    return p.stdout.decode('ascii').strip()


@pytest.mark.parametrize('calc_type', CALC_TYPES)
@pytest.mark.parametrize('parser', PARSERS)
def test_simple(calc_type, parser):
    res = run_simple(calc_type, ["-p", parser, "2"])
    assert res == "2"

@pytest.mark.parametrize('calc_type', INT_CALC_TYPES)
@pytest.mark.parametrize('parser', PARSERS)
def test_round(calc_type, parser):
    res = run_simple(calc_type, ["-p", parser, "2.1"])
    assert res == "2"

expr = st.deferred(
    lambda: (
        st.floats(allow_nan=False, allow_infinity=False) |
        st.tuples(st.just('('), expr) |
        st.tuples(expr, st.lists(st.tuples(st.sampled_from('+-*/'), expr)))
    )
)


def to_str(expr, wrap=None):
    if isinstance(expr, float):
        if not wrap:
            return str(expr)
        else:
            return wrap.format(expr)
    elif expr[0] == '(':
        return '({})'.format(to_str(expr[1], wrap))
    else:
        res = to_str(expr[0], wrap)
        for op, ex in expr[1]:
            res += op + to_str(ex, wrap)
        return res
    raise TypeError(repr(expr))


def expected_result(expr, np_type):
    expr_str = to_str(expr, 'np_type({})')
    glob = {'__builtins__': {}, 'np_type': np_type}
    exec('result = np_type({})'.format(expr_str), glob)
    return glob['result']


@pytest.mark.parametrize('calc_type,np_type', [
    #('long_double', numpy.longdouble),
    ('double', numpy.float64),
    ('float', numpy.float32),
])
@pytest.mark.parametrize('parser', PARSERS)
@hypothesis.settings(max_iterations=100, max_examples=100)
@hypothesis.given(expr)
def test_hypo(calc_type, np_type, parser, expr):
    expr_str = to_str(expr)
    expected = expected_result(expr, np_type)
    hypothesis.assume(numpy.isfinite(expected))
    res = run_simple(calc_type, ["-p", parser, "--", expr_str])
    assert np_type(res) == expected