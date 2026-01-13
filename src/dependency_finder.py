"""Find algebraic dependencies q(x, f, g) = 0 using resultants."""

from itertools import product
from typing import Optional
from sympy import symbols, resultant, Poly, gcd, simplify, expand, factor
from sympy.abc import x, y, u, v

from .config import Config
from .polynomial import substitute, is_zero


class DependencyFinder:
    """Find polynomial q such that q(x, f, g) = 0 using resultant method."""
    
    def __init__(self, config: Config):
        """
        Initialize dependency finder.
        
        Args:
            config: Configuration object
        """
        self.config = config
        self.x, self.y, self.u, self.v = x, y, u, v
    
    def _is_nontrivial_in_x(self, q) -> bool:
        """
        Check if x appears non-trivially in q.
        
        Non-trivial means:
        - x appears with degree >= 2 (like x^2, x^3, etc.), OR
        - x is multiplied with u or v (like x*u, x*v, x*u*v, etc.)
        
        Trivial (rejected):
        - Only linear in x without u,v: like 4*x - 5
        
        Args:
            q: Polynomial in Q[x,u,v]
            
        Returns:
            True if x appears non-trivially, False otherwise
        """
        from sympy import degree, Poly
        
        try:
            # Convert to polynomial
            poly = Poly(q, self.x, self.u, self.v)
            
            # Check all monomials
            for monom, coeff in poly.terms():
                x_deg, u_deg, v_deg = monom
                
                # Skip constant and pure u,v terms
                if x_deg == 0:
                    continue
                
                # x appears with degree >= 2: non-trivial
                if x_deg >= 2:
                    return True
                
                # x appears with degree 1
                if x_deg == 1:
                    # If multiplied with u or v: non-trivial
                    if u_deg > 0 or v_deg > 0:
                        return True
            
            # Only linear x terms without u,v: trivial
            return False
            
        except:
            # If we can't analyze, be conservative and accept it
            return True
    
    def find_dependency(self, f, g):
        """
        Find q(x, u, v) such that q(x, f(x,y), g(x,y)) = 0.
        
        Uses resultant method to eliminate y and find algebraic relation.
        
        Algorithm:
        1. Consider u - f(x,y) = 0 and v - g(x,y) = 0
        2. Compute resultant with respect to y to eliminate y
        3. This gives a polynomial in x, u, v
        
        Args:
            f: First polynomial in Z[x,y]
            g: Second polynomial in Z[x,y]
            
        Returns:
            Tuple (q, was_trivial) where:
            - q: Polynomial q in Q[x,u,v] if found, None otherwise
            - was_trivial: True if a dependency was found but rejected as trivial
        """
        # Try resultant method first
        q = self._try_resultant(f, g)
        if q is not None:
            if self._is_nontrivial_in_x(q):
                return q, False
            else:
                # Found dependency but it's trivial
                return None, True
        
        # Try Gröbner basis as backup
        q = self._try_groebner(f, g)
        if q is not None:
            if self._is_nontrivial_in_x(q):
                return q, False
            else:
                # Found dependency but it's trivial
                return None, True
        
        # Fallback to brute force
        for q in self._generate_candidates():
            if self._is_dependency(q, f, g):
                if self._is_nontrivial_in_x(q):
                    return q, False
                else:
                    # Found dependency but it's trivial
                    return None, True
        
        # No dependency found at all
        return None, False
    
    def _try_resultant(self, f, g) -> Optional:
        """
        Use resultant to find dependency by eliminating y.
        
        Given f(x,y) and g(x,y), we want to find q(x,u,v) such that
        q(x, f(x,y), g(x,y)) = 0.
        
        Method:
        - Consider p1 = u - f(x,y) and p2 = v - g(x,y)
        - Compute resultant Res_y(p1, p2) to eliminate y
        - This gives a polynomial in x, u, v
        
        Args:
            f: First polynomial
            g: Second polynomial
            
        Returns:
            Dependency found via resultant, or None
        """
        try:
            # Create polynomials u - f(x,y) and v - g(x,y)
            p1 = self.u - f
            p2 = self.v - g
            
            # Compute resultant with respect to y
            # This eliminates y and gives polynomial in x, u, v
            res = resultant(p1, p2, self.y)
            
            # Simplify and factor
            res = simplify(expand(res))
            
            # Check if it's non-trivial and valid
            if res != 0 and self.y not in res.free_symbols:
                # Verify it's a valid dependency
                if self._is_dependency(res, f, g):
                    return res
            
            return None
        except Exception as e:
            # Resultant computation can fail
            return None
    
    def _try_groebner(self, f, g) -> Optional:
        """
        Use Gröbner basis as backup method.
        
        Args:
            f: First polynomial
            g: Second polynomial
            
        Returns:
            Dependency found via Gröbner basis, or None
        """
        try:
            from sympy import groebner
            
            # Create ideal generators: u - f(x,y), v - g(x,y)
            ideal_gens = [self.u - f, self.v - g]
            
            # Compute Gröbner basis with elimination order
            # Order: y > x > u > v (eliminate y first)
            gb = groebner(ideal_gens, self.y, self.x, self.u, self.v, order='lex')
            
            # Look for polynomial in Q[x,u,v] (no y)
            for poly in gb:
                if self.y not in poly.free_symbols and poly != 0:
                    # Verify it's a valid dependency
                    if self._is_dependency(poly, f, g):
                        return poly
            
            return None
        except Exception:
            return None
    
    def _generate_candidates(self):
        """
        Generate candidate q polynomials in Q[x,u,v].
        
        Yields:
            SymPy expressions representing candidate q polynomials
        """
        max_deg = self.config.max_degree_q
        
        # Generate all monomials x^i * u^j * v^k where i+j+k <= max_deg
        monomials = []
        for total_deg in range(max_deg + 1):
            for i in range(total_deg + 1):
                for j in range(total_deg - i + 1):
                    k = total_deg - i - j
                    monomials.append((i, j, k))
        
        # Generate coefficient combinations
        coeff_range = range(self.config.coeff_min, self.config.coeff_max + 1)
        
        for coeffs in product(coeff_range, repeat=len(monomials)):
            # Skip all-zero polynomial
            if all(c == 0 for c in coeffs):
                continue
            
            # Build polynomial q(x, u, v)
            q = sum(c * (self.x**i) * (self.u**j) * (self.v**k)
                   for c, (i, j, k) in zip(coeffs, monomials))
            
            yield q
    
    def _is_dependency(self, q, f, g) -> bool:
        """
        Check if q(x, f(x,y), g(x,y)) = 0.
        
        Args:
            q: Candidate polynomial in Q[x,u,v]
            f: First polynomial in Z[x,y]
            g: Second polynomial in Z[x,y]
            
        Returns:
            True if q is a dependency, False otherwise
        """
        # Substitute u -> f(x,y) and v -> g(x,y)
        result = substitute(q, self.u, f)
        result = substitute(result, self.v, g)
        
        # Check if result is identically zero
        return is_zero(result)