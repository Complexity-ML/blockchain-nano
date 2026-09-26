import unittest
from fractions import Fraction
from math import isqrt
from graduation_v3 import Q96, sqrt_at_tick, tick_at_sqrt, size, amounts

class GraduationTests(unittest.TestCase):
    def test_tick_vectors(self):
        for tick,expected in [(-887272,4295128739),(0,Q96),(887272,1461446703485210103287273052203988822378723970342)]:
            self.assertEqual(sqrt_at_tick(tick),expected)
        # Independent rational evaluation, exact ceil(sqrt(10001/10000)^tick * Q96).
        for tick in [-1000,-120,-1,1,120,1000]:
            value=Fraction(10001,10000)**tick*(1<<192)
            floor=isqrt(value.numerator//value.denominator)
            expected=floor+(floor*floor*value.denominator<value.numerator)
            self.assertEqual(sqrt_at_tick(tick),expected)
            self.assertEqual(tick_at_sqrt(expected),tick)
    def test_budget_and_rounding_both_orders(self):
        for index in [0,1]:
            for budget in [1,100,10**10]:
                L,pair=size(budget,Q96,-120,120,60,index)
                self.assertLessEqual(pair[index],budget)
                self.assertGreater(pair[1-index],0)
                # Quote-limited liquidity matches published periphery integer formula.
                a,b=sqrt_at_tick(-120),sqrt_at_tick(120)
                expected=(budget*Q96//(Q96-a) if index else budget*(Q96*b//Q96)//(b-Q96))
                self.assertEqual(L,expected)
                self.assertEqual(pair,amounts(L,Q96,a,b))
    def test_rejections(self):
        for lower,upper,spacing in [(0,120,60),(-120,0,60),(-119,120,60),(-120,120,0),(-120,120,-1),(120,-120,60)]:
            with self.assertRaises(ValueError):size(100,Q96,lower,upper,spacing,1)
        for budget in [0,-1,1<<256]:
            with self.assertRaises(ValueError):size(budget,Q96,-120,120,60,1)
        with self.assertRaises(ValueError):sqrt_at_tick(887273)

if __name__=='__main__':unittest.main()
