#!/usr/bin/env python3

import sqlite3
import glob
import sys
import argparse
from pathlib import Path


def merge_dbs(db_files):
    conn = sqlite3.connect(':memory:')
    cursor = conn.cursor()
    cursor.execute('''
        CREATE TABLE results (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            f_poly TEXT NOT NULL,
            g_poly TEXT NOT NULL,
            q_poly TEXT,
            q_min TEXT,
            error TEXT,
            UNIQUE(f_poly, g_poly)
        )
    ''')

    for db_file in db_files:
        try:
            wconn = sqlite3.connect(db_file)
            wcursor = wconn.cursor()
            wcursor.execute("PRAGMA table_info(results)")
            cols = {row[1] for row in wcursor.fetchall()}
            q_min_col = 'q_min' if 'q_min' in cols else 'NULL'
            error_col = 'error' if 'error' in cols else 'NULL'
            wcursor.execute(f'SELECT f_poly, g_poly, q_poly, {q_min_col}, {error_col} FROM results')
            rows = wcursor.fetchall()
            wconn.close()
            cursor.executemany(
                'INSERT OR IGNORE INTO results (f_poly, g_poly, q_poly, q_min, error) VALUES (?, ?, ?, ?, ?)',
                rows
            )
        except Exception:
            pass

    conn.commit()
    return conn


def get_stats(conn):
    cursor = conn.cursor()
    cursor.execute('''
        SELECT
            COUNT(*) as total,
            SUM(CASE WHEN q_poly IS NOT NULL THEN 1 ELSE 0 END) as with_q,
            SUM(CASE WHEN q_min IS NOT NULL THEN 1 ELSE 0 END) as with_q_min,
            SUM(CASE WHEN error IS NOT NULL THEN 1 ELSE 0 END) as with_error,
            SUM(CASE WHEN q_poly IS NULL THEN 1 ELSE 0 END) as no_q
        FROM results
    ''')
    row = cursor.fetchone()
    return {
        'total': row[0] or 0,
        'with_q': row[1] or 0,
        'with_q_min': row[2] or 0,
        'with_error': row[3] or 0,
        'no_q': row[4] or 0,
    }


def query_results(conn, has_q=None, has_q_min=None, has_error=None, limit=0):
    sql = 'SELECT f_poly, g_poly, q_poly, q_min, error FROM results WHERE 1=1'
    if has_q is True:
        sql += ' AND q_poly IS NOT NULL'
    elif has_q is False:
        sql += ' AND q_poly IS NULL'
    if has_q_min is True:
        sql += ' AND q_min IS NOT NULL'
    elif has_q_min is False:
        sql += ' AND q_min IS NULL'
    if has_error is True:
        sql += ' AND error IS NOT NULL'
    elif has_error is False:
        sql += ' AND error IS NULL'
    if limit > 0:
        sql += f' LIMIT {limit}'

    cursor = conn.cursor()
    cursor.execute(sql)
    return [
        {'f': r[0], 'g': r[1], 'q': r[2], 'q_min': r[3], 'error': r[4]}
        for r in cursor.fetchall()
    ]


def main():
    parser = argparse.ArgumentParser(description='Check statistics from polynomial dependency databases')
    parser.add_argument('files', nargs='+', help='Database files or glob patterns')
    parser.add_argument('--show', action='store_true', help='Show actual polynomials')
    parser.add_argument('--has-q', action='store_true', help='Filter: has dependency q')
    parser.add_argument('--no-q', action='store_true', help='Filter: no dependency q')
    parser.add_argument('--has-q-min', action='store_true', help='Filter: has extracted q_min')
    parser.add_argument('--has-error', action='store_true', help='Filter: has error')
    parser.add_argument('--limit', type=int, default=0, help='Limit number of results shown')

    args = parser.parse_args()

    db_files = []
    for arg in args.files:
        if '*' in arg or '?' in arg:
            db_files.extend(glob.glob(arg))
        elif Path(arg).exists():
            db_files.append(arg)

    if not db_files:
        print("No database files found")
        sys.exit(1)

    conn = merge_dbs(db_files)

    if args.show:
        has_q = True if args.has_q else (False if args.no_q else None)
        has_q_min = True if args.has_q_min else None
        has_error = True if args.has_error else None
        results = query_results(conn, has_q=has_q, has_q_min=has_q_min, has_error=has_error, limit=args.limit)

        print(f"Found {len(results)} result(s)\n")
        for i, r in enumerate(results, 1):
            print(f"Result #{i}:")
            print(f"  f(x,y)   = {r['f']}")
            print(f"  g(x,y)   = {r['g']}")
            if r['q']:
                print(f"  q        = {r['q']}")
            if r['q_min']:
                print(f"  q_min    = {r['q_min']}")
            if r['error']:
                print(f"  ⚠ error  = {r['error']}")
            print()
        return

    stats = get_stats(conn)
    print(f"Found {len(db_files)} database file(s)\n")
    print(f"Total saved:        {stats['total']}")
    print(f"With q:             {stats['with_q']}")
    print(f"With q_min:         {stats['with_q_min']}")
    print(f"With error:         {stats['with_error']}")
    print(f"No q (no dep):      {stats['no_q']}")


if __name__ == "__main__":
    main()
