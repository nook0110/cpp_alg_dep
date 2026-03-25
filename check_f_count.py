#!/usr/bin/env python3
import sqlite3
import sys
from pathlib import Path
from sympy import symbols, sympify, expand
from sympy.parsing.sympy_parser import parse_expr
from collections import defaultdict

def check_f_polynomials(db_files):
    all_f_polys = set()
    all_f_hashes = set()
    all_f_canonical = set()
    canonical_to_polys = defaultdict(list)
    
    x, y = symbols('x y')
    
    for db_file in db_files:
        if not Path(db_file).exists():
            continue
            
        try:
            conn = sqlite3.connect(db_file)
            cursor = conn.cursor()
            
            cursor.execute("SELECT name FROM sqlite_master WHERE type='table'")
            tables = [row[0] for row in cursor.fetchall()]
            
            if 'results' not in tables:
                conn.close()
                continue
            
            cursor.execute("SELECT DISTINCT f_poly, f_hash FROM results")
            for f_poly, f_hash in cursor.fetchall():
                all_f_polys.add(f_poly)
                all_f_hashes.add(f_hash)
                
                try:
                    # Replace ^ with ** for SymPy
                    f_poly_fixed = f_poly.replace('^', '**')
                    expr = parse_expr(f_poly_fixed, local_dict={'x': x, 'y': y})
                    canonical = str(expand(expr))
                    all_f_canonical.add(canonical)
                    canonical_to_polys[canonical].append((f_poly, f_hash))
                except Exception as e:
                    print(f"Failed to parse: {f_poly} - {e}")
            
            conn.close()
        except Exception as e:
            print(f"Error processing {db_file}: {e}")
            continue
    
    print(f"Unique f_poly strings (GiNaC output): {len(all_f_polys)}")
    print(f"Unique f_hash values (custom canonical): {len(all_f_hashes)}")
    print(f"Unique f canonical (SymPy): {len(all_f_canonical)}")
    print(f"Expected: 726")
    
    print(f"\nGiNaC string difference: {len(all_f_polys) - 726} extra")

    print(f"Custom hash difference: {len(all_f_hashes) - 726} extra")
    
    print(f"SymPy canonical difference: {len(all_f_canonical) - 726} extra")
    
    # Show duplicates (first 5 only)
    print("\n" + "=" * 80)
    print("DUPLICATE POLYNOMIALS (same SymPy canonical form) - First 5")
    print("=" * 80)
    dup_count = 0
    shown = 0
    for canonical, polys in sorted(canonical_to_polys.items()):
        if len(polys) > 1:
            dup_count += 1
            if shown < 5:
                print(f"\nCanonical: {canonical}")
                for f_poly, f_hash in polys:
                    print(f"  GiNaC: {f_poly}")
                    print(f"  Hash:  {f_hash}")
                shown += 1
    
    print(f"\n... (showing 5 of {dup_count} total)")
    print(f"Total canonical forms with duplicates: {dup_count}")

if __name__ == "__main__":
    if len(sys.argv) > 1:
        db_files = sys.argv[1:]
    else:
        db_files = list(Path("data").glob("results.db.worker_*"))
    check_f_polynomials(db_files)
