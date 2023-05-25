# Class for binomial sample calculations
class Binomial:
    def __init__(self,yes,no):
        self.y = yes
        self.n = no
        self.tot = yes+no
        self.p_hat = float(self.y)/self.tot
    # Return a Wilson confidence intereval in the form of
    # its center and half-width.
    def wilson_ci(self,alpha):
        from scipy.stats import norm
        from numpy import sqrt
        def bound(z):
            ph = self.p_hat
            n = self.tot
            numerator = ph + z*z/(2*n) + z*sqrt((ph*(1.-ph)/n + z*z/(4.*n*n)))
            denominator = 1.+z*z/n
            return numerator/denominator
        n = self.tot
        l = bound(norm.ppf(alpha/2.))
        u = bound(norm.ppf(1.-alpha/2.))
        c = (l+u)/2.
        return [c, u-c]
