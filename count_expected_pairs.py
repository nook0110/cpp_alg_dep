#!/usr/bin/env python3
"""Count expected number of polynomial pairs after filtering."""

def should_skip_poly(coeffs):
    """Check if polynomial should be skipped."""
    # All zero
    if all(c == 0 for c in coeffs):
        return True
    # All coefficients for x and y terms are zero (constant only)
    if all(coeffs[i] == 0 for i in range(1, len(coeffs))):
        return True
    return False

# For degree 2: monomials are (0,0), (1,0), (0,1), (2,0), (1,1), (0,2)
# That's: 1, x, y, x^2, xy, y^2
num_monomials = 6
coeff_values = [-1, 0, 1]

# Generate all possible coefficient combinations
from itertools import product

valid_f = []
valid_g = []

for coeffs in product(coeff_values, repeat=num_monomials):
    if not should_skip_poly(coeffs):
        valid_f.append(coeffs)
        valid_g.append(coeffs)

print(f"Valid f polynomials: {len(valid_f)}")
print(f"Valid g polynomials: {len(valid_g)}")
print(f"Total pairs: {len(valid_f) * len(valid_g)}")
print(f"Expected in database: {len(valid_f) * len(valid_g)}")
