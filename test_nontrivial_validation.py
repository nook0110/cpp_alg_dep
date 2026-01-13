#!/usr/bin/env python3
"""
Test script to verify that the nontrivial x validation works correctly.
"""

from sympy import symbols, Poly
from src.dependency_finder import DependencyFinder
from src.config import Config

def test_nontrivial_validation():
    """Test the _is_nontrivial_in_x validation function."""
    
    config = Config()
    finder = DependencyFinder(config)
    x, u, v = symbols('x u v')
    
    print("Testing nontrivial x validation...")
    print("=" * 60)
    
    # Test cases that should be REJECTED (trivial)
    trivial_cases = [
        ("4*x - 5", "Linear x with constant"),
        ("2*x + 3", "Linear x with constant"),
        ("x", "Just x"),
        ("-7*x + 1", "Negative linear x"),
    ]
    
    print("\n1. TRIVIAL cases (should be REJECTED):")
    print("-" * 60)
    for expr_str, description in trivial_cases:
        q = Poly(expr_str, x, u, v, domain='ZZ')
        result = finder._is_nontrivial_in_x(q)
        status = "✓ PASS" if not result else "✗ FAIL"
        print(f"{status}: {expr_str:20s} - {description}")
        if result:
            print(f"      ERROR: Should be rejected but was accepted!")
    
    # Test cases that should be ACCEPTED (non-trivial)
    nontrivial_cases = [
        ("x**2", "x squared"),
        ("x**3", "x cubed"),
        ("x*u", "x times u"),
        ("x*v", "x times v"),
        ("x*u*v", "x times u times v"),
        ("x**2 + u", "x squared plus u"),
        ("x*u + v", "x*u plus v"),
        ("2*x**2 - 3*x*u + v", "Mixed with x^2 and x*u"),
        ("x**2*u + x*v**2", "x^2*u plus x*v^2"),
    ]
    
    print("\n2. NON-TRIVIAL cases (should be ACCEPTED):")
    print("-" * 60)
    for expr_str, description in nontrivial_cases:
        q = Poly(expr_str, x, u, v, domain='ZZ')
        result = finder._is_nontrivial_in_x(q)
        status = "✓ PASS" if result else "✗ FAIL"
        print(f"{status}: {expr_str:20s} - {description}")
        if not result:
            print(f"      ERROR: Should be accepted but was rejected!")
    
    # Edge cases
    edge_cases = [
        ("u + v", "No x at all", False),
        ("u**2 + v**2", "No x at all", False),
        ("x**2 + 4*x - 5", "Has x^2 (non-trivial) and linear x", True),
    ]
    
    print("\n3. EDGE cases:")
    print("-" * 60)
    for expr_str, description, expected in edge_cases:
        q = Poly(expr_str, x, u, v, domain='ZZ')
        result = finder._is_nontrivial_in_x(q)
        status = "✓ PASS" if result == expected else "✗ FAIL"
        expected_str = "ACCEPT" if expected else "REJECT"
        actual_str = "ACCEPT" if result else "REJECT"
        print(f"{status}: {expr_str:20s} - {description}")
        print(f"      Expected: {expected_str}, Got: {actual_str}")
    
    print("\n" + "=" * 60)
    print("Test complete!")

if __name__ == "__main__":
    test_nontrivial_validation()