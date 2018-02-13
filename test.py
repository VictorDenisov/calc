import unittest
import subprocess

import hypothesis
import hypothesis.strategies as st
import hypothesis.extra.numpy as npst
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

farrays = npst.arrays(numpy.longdouble, 1)
expr = st.deferred(
    lambda: (
        longdouble_strategy() |
        st.tuples(st.just('('), expr) |
        st.tuples(expr, st.lists(st.tuples(st.sampled_from('+-*/'), expr)))
    )
)


@st.composite
def longdouble_strategy(draw):
    x = draw(farrays)[0]
    hypothesis.assume(numpy.isfinite(x))
    return x


def to_str(expr, wrap='{}'):
    if isinstance(expr, numpy.longdouble):
        return wrap.format(expr)
    elif expr[0] == '(':
        return '({})'.format(to_str(expr[1], wrap))
    else:
        res = to_str(expr[0], wrap)
        for op, ex in expr[1]:
            res += op + to_str(ex, wrap)
        return res
    raise TypeError(repr(expr))


def expected_result(expr, np_type, wrap=None):
    expr_str = to_str(expr, wrap if wrap else 'np_type({})')
    glob = {'__builtins__': {}, 'np_type': np_type}
    cmd = 'result = np_type({})'.format(expr_str)
    exec(cmd, glob)
    return glob['result']


@pytest.mark.parametrize('calc_type,np_type,wrap', [
    #('long_double', numpy.longdouble, 'np_type("{}")'),
    ('double', numpy.float64, None),
    ('float', numpy.float32, None),
])
@pytest.mark.parametrize('parser', PARSERS)
@hypothesis.settings(max_iterations=1, max_examples=1000)
@hypothesis.given(expr)
def test_hypo(calc_type, np_type, wrap, parser, expr):
    expr_str = to_str(expr)
    expected = expected_result(expr, np_type, wrap)
    hypothesis.assume(numpy.isfinite(expected))
    res = run_simple(calc_type, ["-p", parser, "--", expr_str])
    res_np = np_type(res)
    assert res_np == expected
