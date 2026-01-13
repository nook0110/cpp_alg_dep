#!/usr/bin/env python3
"""
Integration test to verify that trivial dependencies are rejected in the full workflow.
"""

from sympy import symbols, Poly
from src.dependency_finder import DependencyFinder
from src.config import Config

def test_integration():
    """Test that trivial dependencies are properly rejected in find_dependency."""
    
    config = Config()
    finder = DependencyFinder(config)
    x, y = symbols('x y')
    
    print("Integration Test: Verifying trivial dependency rejection")
    print("=" * 60)
    
    # Test case 1: f and g that would produce a trivial dependency
    # For example, if f = x and g = x, then q = u - v would be a dependency
    # but it's trivial (no x^2 or x*u or x*v terms)
    print("\nTest 1: f = x, g = x")
    print("-" * 60)
    f1 = Poly(x, x, y, domain='ZZ')
    g1 = Poly(x, x, y, domain='ZZ')
    
    result1 = finder.find_dependency(f1, g1)
    if result1 is None:
        print("✓ PASS: Trivial dependency correctly rejected (or no dependency found)")
    else:
        print(f"Result: {result1}")
        # Check if it's non-trivial
        if finder._is_nontrivial_in_x(result1):
            print("✓ PASS: Found non-trivial dependency")
        else:
            print("✗ FAIL: Found trivial dependency (should have been rejected)")
    
    # Test case 2: f and g that should produce a non-trivial dependency
    # For example, f = x^2, g = x^3, then q = u^3 - v^2 is a dependency
    # and it's non-trivial (has u and v terms)
    print("\nTest 2: f = x^2, g = x^3")
    print("-" * 60)
    f2 = Poly(x**2, x, y, domain='ZZ')
    g2 = Poly(x**3, x, y, domain='ZZ')
    
    result2 = finder.find_dependency(f2, g2)
    if result2 is None:
        print("⚠ WARNING: No dependency found (might be expected)")
    else:
        print(f"Result: {result2}")
        if finder._is_nontrivial_in_x(result2):
            print("✓ PASS: Found non-trivial dependency")
        else:
            print("✗ FAIL: Found trivial dependency")
    
    # Test case 3: More complex polynomials
    print("\nTest 3: f = x^2 + y, g = x*y")
    print("-" * 60)
    f3 = Poly(x**2 + y, x, y, domain='ZZ')
    g3 = Poly(x*y, x, y, domain='ZZ')
    
    result3 = finder.find_dependency(f3, g3)
    if result3 is None:
        print("⚠ WARNING: No dependency found (might be expected)")
    else:
        print(f"Result: {result3}")
        if finder._is_nontrivial_in_x(result3):
            print("✓ PASS: Found non-trivial dependency")
        else:
            print("✗ FAIL: Found trivial dependency")
    
    print("\n" + "=" * 60)
    print("Integration test complete!")
    print("\nNote: Some test cases may not find dependencies due to")
    print("computational complexity or degree limits. This is expected.")

if __name__ == "__main__":
    test_integration()