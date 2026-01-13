#!/usr/bin/env python3
"""
Test script to verify statistics tracking works correctly.
"""

import os
import tempfile
from sympy import symbols, Poly
from src.config import Config
from src.dependency_finder import DependencyFinder
from src.divisibility import DivisibilityChecker
from src.cache import ResultCache

def test_statistics():
    """Test that statistics are properly tracked."""
    
    # Create temporary database
    with tempfile.NamedTemporaryFile(mode='w', suffix='.db', delete=False) as f:
        db_path = f.name
    
    try:
        config = Config()
        config.cache_file = db_path
        
        finder = DependencyFinder(config)
        checker = DivisibilityChecker()
        cache = ResultCache(db_path)
        
        x, y = symbols('x y')
        
        print("Testing statistics tracking...")
        print("=" * 60)
        
        # Test case 1: No dependency
        print("\n1. Testing pair with no dependency: f=x, g=y")
        f1 = Poly(x, x, y, domain='ZZ')
        g1 = Poly(y, x, y, domain='ZZ')
        q1, trivial1 = finder.find_dependency(f1, g1)
        cache.save_result(f1, g1, q1, {}, is_trivial=trivial1)
        print(f"   Result: q={q1}, trivial={trivial1}")
        
        # Test case 2: Trivial dependency (if we can construct one)
        print("\n2. Testing pair that might produce trivial dependency: f=x, g=x")
        f2 = Poly(x, x, y, domain='ZZ')
        g2 = Poly(x, x, y, domain='ZZ')
        q2, trivial2 = finder.find_dependency(f2, g2)
        cache.save_result(f2, g2, q2, {}, is_trivial=trivial2)
        print(f"   Result: q={q2}, trivial={trivial2}")
        
        # Test case 3: Non-trivial dependency
        print("\n3. Testing pair with non-trivial dependency: f=x^2, g=x^3")
        f3 = Poly(x**2, x, y, domain='ZZ')
        g3 = Poly(x**3, x, y, domain='ZZ')
        q3, trivial3 = finder.find_dependency(f3, g3)
        
        divisibility3 = {}
        if q3:
            divisibility3 = checker.check_conditions(q3, f3, g3)
        
        cache.save_result(f3, g3, q3, divisibility3, is_trivial=trivial3)
        print(f"   Result: q={q3}, trivial={trivial3}")
        if q3:
            print(f"   Divisibility: {divisibility3}")
        
        # Get statistics
        print("\n" + "=" * 60)
        print("STATISTICS")
        print("=" * 60)
        stats = cache.get_statistics()
        
        print(f"Total pairs checked: {stats['total']}")
        print(f"Dependencies found: {stats['with_dependency']}")
        print(f"  - Trivial (rejected): {stats['trivial_rejected']}")
        print(f"  - Non-trivial (kept): {stats['nontrivial_found']}")
        print(f"No dependency found: {stats['no_dependency']}")
        print(f"\nDivisibility Results:")
        print(f"  ∂q/∂f : ∂q/∂x only: {stats['df_divisible_only']}")
        print(f"  ∂q/∂g : ∂q/∂x only: {stats['dg_divisible_only']}")
        print(f"  Both conditions satisfied: {stats['both_divisible']}")
        
        cache.close()
        
        print("\n" + "=" * 60)
        print("✓ Statistics tracking test complete!")
        
    finally:
        # Clean up
        if os.path.exists(db_path):
            os.remove(db_path)

if __name__ == "__main__":
    test_statistics()