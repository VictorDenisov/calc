import unittest
import subprocess

import hypothesis
import hypothesis.strategies as st

PARSERS = [ "compute"
          , "ast_iter"
          , "ast_rec"
          , "gccjit"
          , "libjit"
          , "llvm"
          , "elf"
          , "dl"
          ]
FLOAT_CALC_TYPES = [ "long_double"
                   , "double"
                   , "float"
                   ]
INT_CALC_TYPES = [ "long_int"
                 , "int"
                 , "char"
                 ]
CALC_TYPES = INT_CALC_TYPES + FLOAT_CALC_TYPES

expr = st.deferred(
    lambda: (
        st.floats(allow_nan=False, allow_infinity=False) |
        st.tuples(st.just('('), expr) |
        st.tuples(expr, st.lists(st.tuples(st.sampled_from('+-*/'), expr)))
    )
)


def to_str(expr):
    if isinstance(expr, float):
        return str(expr)
    elif expr[0] == '(':
        return '({})'.format(to_str(expr[1]))
    else:
        res = to_str(expr[0])
        for op, ex in expr[1]:
            res += op + to_str(ex)
        return res
    raise TypeError(repr(expr))

def expected_result(expr_str):
    glob = {'__builtins__': {}}
    exec('result = ' + expr_str, glob)
    return glob['result']


class TestCalc(unittest.TestCase):
    def run_simple(self, calc_type, params):
        p = subprocess.run(["./main." + calc_type] + params, stdout=subprocess.PIPE)
        self.assertEqual(p.returncode, 0)
        return p.stdout.decode('ascii').strip()

    def test_simple(self):
        for calc_type in CALC_TYPES:
            for parser in PARSERS:
                with self.subTest(calc_type=calc_type, parser=parser):
                    res = self.run_simple(calc_type, ["-p", parser, "2"])
                    self.assertEqual(res, "2")

    def test_round(self):
        for calc_type in INT_CALC_TYPES:
            for parser in PARSERS:
                with self.subTest(calc_type=calc_type, parser=parser):
                    res = self.run_simple(calc_type, ["-p", parser, "2.1"])
                    self.assertEqual(res, "2")

    @hypothesis.given(st.sampled_from(FLOAT_CALC_TYPES), st.sampled_from(PARSERS), expr)
    def test_hypo(self, calc_type, parser, expr):
        expr_str = to_str(expr)
        try:
            expected = expected_result(expr_str)
        except ZeroDivisionError:
            hypothesis.assume(False)
        res = self.run_simple(calc_type, ["-p", parser, "--", expr_str])
        self.assertEqual(float(res), expected)

if __name__ == '__main__':
    unittest.main()
