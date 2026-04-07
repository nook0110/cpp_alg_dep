from itertools import product, combinations_with_replacement

def get_monomials(max_deg):
    """Get monomials ordered by total degree, then x-degree within each total degree."""
    monoms = []
    for total in range(max_deg + 1):
        for xdeg in range(total + 1):
            ydeg = total - xdeg
            monoms.append((xdeg, ydeg))
    return monoms

def eval_substitution(poly, monoms, sub_x_neg, sub_y_neg):
    """Apply x->-x and/or y->-y substitution."""
    new_poly = list(poly)
    for i, (xdeg, ydeg) in enumerate(monoms):
        sign = 1
        if sub_x_neg and xdeg % 2 == 1:
            sign *= -1
        if sub_y_neg and ydeg % 2 == 1:
            sign *= -1
        new_poly[i] = poly[i] * sign
    return tuple(new_poly)

def get_variants(f, g, monoms):
    """Get all up to 16 variants of (f, g)."""
    variants = set()
    for swap in [False, True]:
        for neg in [False, True]:
            for neg_x in [False, True]:
                for neg_y in [False, True]:
                    ff, gg = f, g
                    if swap:
                        ff, gg = gg, ff
                    if neg:
                        ff = tuple(-c for c in ff)
                        gg = tuple(-c for c in gg)
                    ff = eval_substitution(ff, monoms, neg_x, neg_y)
                    gg = eval_substitution(gg, monoms, neg_x, neg_y)
                    variants.add((ff, gg))
    return variants

def is_canonical(f, g, monoms):
    """Check if (f, g) is the lexicographically smallest among all variants."""
    current = (f, g)
    variants = get_variants(f, g, monoms)
    return current == min(variants)

def is_valid(poly, monoms):
    """Not all-zero and not constant."""
    if all(c == 0 for c in poly):
        return False
    # constant: only the (0,0) monomial can be nonzero
    for i, (xdeg, ydeg) in enumerate(monoms):
        if (xdeg > 0 or ydeg > 0) and poly[i] != 0:
            return True
    return False  # only constant term nonzero

def count_canonical_pairs(max_deg, coeff_range):
    monoms = get_monomials(max_deg)
    n = len(monoms)
    coeffs = list(coeff_range)
    
    # Generate all valid polynomials
    valid_polys = []
    for poly in product(coeffs, repeat=n):
        if is_valid(poly, monoms):
            valid_polys.append(poly)
    
    print(f"Number of valid polynomials: {len(valid_polys)}")
    
    count = 0
    total_pairs = 0
    for i, f in enumerate(valid_polys):
        for j, g in enumerate(valid_polys):
            total_pairs += 1
            if is_canonical(f, g, monoms):
                count += 1
    
    print(f"Total pairs checked: {total_pairs}")
    return count

# Run for max_deg=1, coefficients in {-1, 0, 1}
result = count_canonical_pairs(2, {-1, 0, 1})
print(f"Canonical pairs (max_deg=1): {result}")
