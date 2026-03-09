#!/usr/bin/env python3

import sqlite3
import glob
import sys
import argparse
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
            SUM(CASE WHEN q_poly IS NULL THEN 1 ELSE 0 END) as no_dependency,
            SUM(CASE WHEN needs_review = 1 THEN 1 ELSE 0 END) as needs_review
        FROM results
    """
    
    cursor.execute(query)
    row = cursor.fetchone()
    
    detailed_query = """
        SELECT
            CASE WHEN q_poly IS NULL THEN 0 ELSE 1 END as has_dep,
            COALESCE(is_trivial, 0) as is_trivial,
            COALESCE(df_divisible, 0) as df_div,
            COALESCE(dg_divisible, 0) as dg_div,
            COALESCE(needs_review, 0) as needs_rev,
            COUNT(*) as count
        FROM results
        GROUP BY has_dep, is_trivial, df_div, dg_div, needs_rev
        ORDER BY has_dep DESC, is_trivial ASC, df_div DESC, dg_div DESC, needs_rev DESC
    """
    
    cursor.execute(detailed_query)
    detailed_rows = cursor.fetchall()
    
    conn.close()
    
    detailed = []
    for drow in detailed_rows:
        has_dependency = drow[0] != 0
        is_trivial = drow[1] != 0
        df_divisible = drow[2] != 0
        dg_divisible = drow[3] != 0
        needs_review = drow[4] != 0
        count = drow[5]
        
        detailed.append({
            'has_dependency': has_dependency,
            'is_nontrivial': has_dependency and not is_trivial,
            'df_divisible': df_divisible,
            'dg_divisible': dg_divisible,
            'both_divisible': df_divisible and dg_divisible,
            'needs_review': needs_review,
            'count': count
        })
    
    return {
        'total': row[0] or 0,
        'with_dependency': row[1] or 0,
        'trivial_rejected': row[2] or 0,
        'nontrivial_found': row[3] or 0,
        'df_divisible_only': row[4] or 0,
        'dg_divisible_only': row[5] or 0,
        'both_divisible': row[6] or 0,
        'no_dependency': row[7] or 0,
        'needs_review': row[8] or 0,
        'detailed': detailed
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
        'no_dependency': 0,
        'needs_review': 0,
        'detailed': {}
    }
    
    for stats in stats_list:
        for key in ['total', 'with_dependency', 'trivial_rejected', 'nontrivial_found',
                    'df_divisible_only', 'dg_divisible_only', 'both_divisible', 'no_dependency', 'needs_review']:
            merged[key] += stats[key]
        
        for detail in stats.get('detailed', []):
            key = (detail['has_dependency'], detail['is_nontrivial'],
                   detail['df_divisible'], detail['dg_divisible'], detail['both_divisible'], detail['needs_review'])
            if key not in merged['detailed']:
                merged['detailed'][key] = 0
            merged['detailed'][key] += detail['count']
    
    detailed_list = []
    for key, count in merged['detailed'].items():
        detailed_list.append({
            'has_dependency': key[0],
            'is_nontrivial': key[1],
            'df_divisible': key[2],
            'dg_divisible': key[3],
            'both_divisible': key[4],
            'needs_review': key[5],
            'count': count
        })
    
    detailed_list.sort(key=lambda x: (not x['has_dependency'], x['is_nontrivial'],
                                       not x['df_divisible'], not x['dg_divisible']))
    merged['detailed'] = detailed_list
    
    return merged

def query_polynomials(db_files, has_dep=None, is_nontrivial=None, df_div=None, dg_div=None, both_div=None, needs_rev=None, limit=None):
    results = []
    
    for db_file in db_files:
        if not db_file.endswith('.db') and not 'worker_' in db_file:
            continue
            
        try:
            conn = sqlite3.connect(db_file)
            cursor = conn.cursor()
            
            query = "SELECT f_poly, g_poly, q_poly, is_trivial, df_divisible, dg_divisible, both_divisible, needs_review FROM results WHERE 1=1"
            params = []
            
            if has_dep is not None:
                if has_dep:
                    query += " AND q_poly IS NOT NULL"
                else:
                    query += " AND q_poly IS NULL"
            
            if is_nontrivial is not None:
                if is_nontrivial:
                    query += " AND q_poly IS NOT NULL AND is_trivial = 0"
                else:
                    query += " AND (q_poly IS NULL OR is_trivial = 1)"
            
            if df_div is not None:
                query += " AND df_divisible = ?"
                params.append(1 if df_div else 0)
            
            if dg_div is not None:
                query += " AND dg_divisible = ?"
                params.append(1 if dg_div else 0)
            
            if both_div is not None:
                query += " AND both_divisible = ?"
                params.append(1 if both_div else 0)
            
            if needs_rev is not None:
                query += " AND needs_review = ?"
                params.append(1 if needs_rev else 0)
            
            if limit:
                query += f" LIMIT {limit}"
            
            cursor.execute(query, params)
            rows = cursor.fetchall()
            
            for row in rows:
                results.append({
                    'f': row[0],
                    'g': row[1],
                    'q': row[2],
                    'is_trivial': row[3],
                    'df_divisible': row[4],
                    'dg_divisible': row[5],
                    'both_divisible': row[6],
                    'needs_review': row[7] if len(row) > 7 else False
                })
            
            conn.close()
        except Exception as e:
            pass
    
    return results

def main():
    parser = argparse.ArgumentParser(
        description='Check statistics from polynomial dependency databases',
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog="""
Examples:
  # Show statistics from all worker databases
  python check_stats.py data/results.db.worker_*
  
  # Show polynomials with dependency and df_divisible
  python check_stats.py data/*.db --show --has-dep --df-div
  
  # Show first 10 non-trivial polynomials with both divisible
  python check_stats.py data/*.db --show --nontrivial --both-div --limit 10
  
  # Show polynomials without dependency
  python check_stats.py data/*.db --show --no-dep --limit 5
        """)
    
    parser.add_argument('files', nargs='+', help='Database files or patterns')
    parser.add_argument('--show', action='store_true', help='Show actual polynomials')
    parser.add_argument('--has-dep', action='store_true', help='Filter: has dependency')
    parser.add_argument('--no-dep', action='store_true', help='Filter: no dependency')
    parser.add_argument('--nontrivial', action='store_true', help='Filter: non-trivial (x^2/x*u/x*v)')
    parser.add_argument('--trivial', action='store_true', help='Filter: trivial')
    parser.add_argument('--df-div', action='store_true', help='Filter: dq/df : dq/dx')
    parser.add_argument('--dg-div', action='store_true', help='Filter: dq/dg : dq/dx')
    parser.add_argument('--both-div', action='store_true', help='Filter: both divisible')
    parser.add_argument('--needs-review', action='store_true', help='Filter: needs review (0/0 derivatives)')
    parser.add_argument('--limit', type=int, help='Limit number of results')
    
    args = parser.parse_args()
    
    db_files = []
    for arg in args.files:
        if '*' in arg or '?' in arg:
            db_files.extend(glob.glob(arg))
        else:
            if Path(arg).exists():
                db_files.append(arg)
    
    if not db_files:
        print("No database files found")
        sys.exit(1)
    
    if args.show:
        has_dep = True if args.has_dep else (False if args.no_dep else None)
        is_nontrivial = True if args.nontrivial else (False if args.trivial else None)
        df_div = True if args.df_div else None
        dg_div = True if args.dg_div else None
        both_div = True if args.both_div else None
        needs_rev = True if args.needs_review else None
        
        results = query_polynomials(db_files, has_dep, is_nontrivial, df_div, dg_div, both_div, needs_rev, args.limit)
        
        print(f"Found {len(results)} polynomial(s) matching criteria")
        print()
        
        for i, result in enumerate(results, 1):
            print(f"Result #{i}:")
            print(f"  f(x,y) = {result['f']}")
            print(f"  g(x,y) = {result['g']}")
            if result['q']:
                print(f"  q(x,u,v) = {result['q']}")
                print(f"  Trivial: {'Yes' if result['is_trivial'] else 'No'}")
                print(f"  dq/df : dq/dx: {'Yes' if result['df_divisible'] else 'No'}")
                print(f"  dq/dg : dq/dx: {'Yes' if result['dg_divisible'] else 'No'}")
                print(f"  Both divisible: {'Yes' if result['both_divisible'] else 'No'}")
                if result.get('needs_review'):
                    print(f"  ⚠ NEEDS REVIEW (all derivatives zero)")
            else:
                print(f"  No dependency found")
            print()
        
        return
    
    print(f"Found {len(db_files)} database file(s)")
    print()
    
    all_stats = []
    for db_file in sorted(db_files):
        try:
            stats = get_db_stats(db_file)
            all_stats.append((Path(db_file).name, stats))
        except Exception as e:
            if 'not a database' not in str(e):
                print(f"Error reading {db_file}: {e}")
    
    if len(all_stats) > 1:
        merged = merge_stats([s for _, s in all_stats])
        stats = merged
        title = "MERGED STATISTICS FROM ALL DATABASES"
    elif len(all_stats) == 1:
        _, stats = all_stats[0]
        title = "STATISTICS"
    else:
        return
    
    print("=" * 100)
    print(title)
    print("=" * 100)
    print()
    print(f"Total pairs checked: {stats['total']}")
    print()
    
    print("+--------------+-----------------+-----------------+-----------------+-------------+---------+-----------+")
    print("| Dependency   | Non-trivial     | dq/df : dq/dx   | dq/dg : dq/dx   | Both        |   Count | Percent   |")
    print("| Found        | (x^2/x*u/x*v)   |                 |                 | Divisible   |         |           |")
    print("+==============+=================+=================+=================+=============+=========+===========+")
    
    for row in stats['detailed']:
        dep_status = "+" if row['has_dependency'] else "-"
        nontrivial_status = "+" if row['is_nontrivial'] else "-"
        df_status = "+" if row['df_divisible'] else "-"
        dg_status = "+" if row['dg_divisible'] else "-"
        both_status = "+" if row['both_divisible'] else "-"
        
        pct = (100.0 * row['count'] / stats['total']) if stats['total'] > 0 else 0.0
        
        print(f"| {dep_status:<12} | {nontrivial_status:<15} | {df_status:<15} | {dg_status:<15} | {both_status:<11} | {row['count']:>7} | {pct:>8.2f}% |")
        print("+--------------+-----------------+-----------------+-----------------+-------------+---------+-----------+")
    
    print()
    print(f"Cases needing review (0/0 derivatives): {stats.get('needs_review', 0)}")
    print()
    print("Legend:")
    print("  + = Passed")
    print("  - = Failed")
    print("  ⚠ = Needs Review")
    print()
    print("Note: Non-trivial means x appears as x^2 or x*u or x*v (not just linear like 4x-5)")

if __name__ == "__main__":
    main()