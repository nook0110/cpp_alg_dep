#!/usr/bin/env python3

import sqlite3
import glob
import sys
from pathlib import Path

def get_db_stats(db_path):
    conn = sqlite3.connect(db_path)
    cursor = conn.cursor()
    
    query = """
        SELECT
            COUNT(*) as total,
            SUM(CASE WHEN q_poly IS NOT NULL THEN 1 ELSE 0 END) as with_dependency,
            SUM(CASE WHEN is_trivial = 1 THEN 1 ELSE 0 END) as trivial_rejected,
            SUM(CASE WHEN q_poly IS NOT NULL AND is_trivial = 0 THEN 1 ELSE 0 END) as nontrivial_found,
            SUM(CASE WHEN df_divisible = 1 AND dg_divisible = 0 THEN 1 ELSE 0 END) as df_divisible_only,
            SUM(CASE WHEN df_divisible = 0 AND dg_divisible = 1 THEN 1 ELSE 0 END) as dg_divisible_only,
            SUM(CASE WHEN both_divisible = 1 THEN 1 ELSE 0 END) as both_divisible,
            SUM(CASE WHEN q_poly IS NULL THEN 1 ELSE 0 END) as no_dependency
        FROM results
    """
    
    cursor.execute(query)
    row = cursor.fetchone()
    conn.close()
    
    return {
        'total': row[0] or 0,
        'with_dependency': row[1] or 0,
        'trivial_rejected': row[2] or 0,
        'nontrivial_found': row[3] or 0,
        'df_divisible_only': row[4] or 0,
        'dg_divisible_only': row[5] or 0,
        'both_divisible': row[6] or 0,
        'no_dependency': row[7] or 0
    }

def merge_stats(stats_list):
    merged = {
        'total': 0,
        'with_dependency': 0,
        'trivial_rejected': 0,
        'nontrivial_found': 0,
        'df_divisible_only': 0,
        'dg_divisible_only': 0,
        'both_divisible': 0,
        'no_dependency': 0
    }
    
    for stats in stats_list:
        for key in merged:
            merged[key] += stats[key]
    
    return merged

def main():
    if len(sys.argv) < 2:
        print("Usage: python check_stats.py <db_files_or_pattern>")
        print("Examples:")
        print("  python check_stats.py 'build/cache.db*'")
        print("  python check_stats.py data/results.db.worker_*")
        print("  python check_stats.py file1.db file2.db file3.db")
        sys.exit(1)
    
    db_files = []
    for arg in sys.argv[1:]:
        if '*' in arg or '?' in arg:
            db_files.extend(glob.glob(arg))
        else:
            if Path(arg).exists():
                db_files.append(arg)
    
    if not db_files:
        print(f"No database files found")
        sys.exit(1)
    
    print(f"Found {len(db_files)} database file(s):")
    for db_file in sorted(db_files):
        print(f"  - {db_file}")
    print()
    
    all_stats = []
    for db_file in sorted(db_files):
        try:
            stats = get_db_stats(db_file)
            all_stats.append(stats)
            print(f"Stats from {Path(db_file).name}:")
            print(f"  Total pairs: {stats['total']}")
            print(f"  With dependency: {stats['with_dependency']}")
            print(f"  No dependency: {stats['no_dependency']}")
            print()
        except Exception as e:
            print(f"Error reading {db_file}: {e}")
            print()
    
    if len(all_stats) > 1:
        merged = merge_stats(all_stats)
        print("=" * 60)
        print("MERGED STATISTICS FROM ALL DATABASES")
        print("=" * 60)
        print(f"Total pairs checked: {merged['total']}")
        print(f"Dependencies found: {merged['with_dependency']}")
        print(f"  - Trivial (rejected): {merged['trivial_rejected']}")
        print(f"  - Non-trivial (kept): {merged['nontrivial_found']}")
        print(f"No dependency found: {merged['no_dependency']}")
        print()
        print("Divisibility Results (for non-trivial dependencies):")
        print(f"  dq/df : dq/dx only: {merged['df_divisible_only']}")
        print(f"  dq/dg : dq/dx only: {merged['dg_divisible_only']}")
        print(f"  Both conditions satisfied: {merged['both_divisible']}")

if __name__ == "__main__":
    main()