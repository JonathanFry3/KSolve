# Wilson Interval for binomial ratio calculator
from scipy.stats import norm
from numpy import sqrt
def bound(p_hat, z, n):
    numerator = p_hat + z*z/(2*n) + z*sqrt((p_hat*(1.-p_hat)/n + z*z/(4.*n*n)))
    denominator = 1.+z*z/n
    return numerator/denominator
print("Alpha: ")
alpha = float(input())
print("Number of successes: ")
ones = float(input())
print("Number of failures: ")
zeroes = float(input())

n = ones+zeroes
p_hat = ones/n
lbound = bound(p_hat, norm.ppf(alpha/2.), n)
ubound = bound(p_hat, norm.ppf(1.-alpha/2.),n)
print("p hat", p_hat)
center = (lbound+ubound)/2.
print("(",lbound,",",ubound,")")
print("or", center, "+-", ubound-center)
