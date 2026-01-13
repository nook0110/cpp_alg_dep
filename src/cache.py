"""Result caching with SQLite database."""

import sqlite3
import os
from typing import Optional, Dict, List

from .polynomial import poly_hash


class ResultCache:
    """Cache results for polynomial pairs using SQLite."""
    
    def __init__(self, db_path: str):
        """
        Initialize result cache.
        
        Args:
            db_path: Path to SQLite database file
        """
        # Create directory if it doesn't exist
        os.makedirs(os.path.dirname(db_path), exist_ok=True)
        
        self.db_path = db_path
        self.conn = sqlite3.connect(db_path)
        self.conn.row_factory = sqlite3.Row  # Enable column access by name
        self._create_tables()
    
    def _create_tables(self):
        """Create database tables if they don't exist."""
        self.conn.execute("""
            CREATE TABLE IF NOT EXISTS results (
                id INTEGER PRIMARY KEY AUTOINCREMENT,
                f_poly TEXT NOT NULL,
                g_poly TEXT NOT NULL,
                f_hash TEXT NOT NULL,
                g_hash TEXT NOT NULL,
                q_poly TEXT,
                is_trivial INTEGER DEFAULT 0,
                df_divisible INTEGER,
                dg_divisible INTEGER,
                both_divisible INTEGER,
                timestamp DATETIME DEFAULT CURRENT_TIMESTAMP,
                UNIQUE(f_hash, g_hash)
            )
        """)
        
        # Migration: Add is_trivial column if it doesn't exist
        try:
            cursor = self.conn.execute("PRAGMA table_info(results)")
            columns = [row[1] for row in cursor.fetchall()]
            if 'is_trivial' not in columns:
                self.conn.execute("ALTER TABLE results ADD COLUMN is_trivial INTEGER DEFAULT 0")
                self.conn.commit()
        except Exception:
            # If migration fails, table might not exist yet or already has column
            pass
        
        self.conn.execute("""
            CREATE INDEX IF NOT EXISTS idx_hashes 
            ON results(f_hash, g_hash)
        """)
        
        self.conn.execute("""
            CREATE INDEX IF NOT EXISTS idx_both_divisible 
            ON results(both_divisible)
        """)
        
        self.conn.commit()
    
    def get_result(self, f, g) -> Optional[Dict]:
        """
        Check if result exists for this polynomial pair.
        
        Args:
            f: First polynomial (SymPy expression)
            g: Second polynomial (SymPy expression)
            
        Returns:
            Dictionary with result data if found, None otherwise
        """
        f_hash_val = poly_hash(f)
        g_hash_val = poly_hash(g)
        
        cursor = self.conn.execute(
            "SELECT * FROM results WHERE f_hash = ? AND g_hash = ?",
            (f_hash_val, g_hash_val)
        )
        
        row = cursor.fetchone()
        if row:
            return dict(row)
        return None
    
    def save_result(self, f, g, q: Optional, divisibility: Dict[str, bool], is_trivial: bool = False):
        """
        Save result for polynomial pair.
        
        Args:
            f: First polynomial (SymPy expression)
            g: Second polynomial (SymPy expression)
            q: Dependency polynomial (SymPy expression) or None
            divisibility: Dictionary with divisibility results
            is_trivial: Whether the dependency was rejected as trivial
        """
        f_str = str(f)
        g_str = str(g)
        f_hash_val = poly_hash(f)
        g_hash_val = poly_hash(g)
        q_str = str(q) if q is not None else None
        
        self.conn.execute("""
            INSERT OR REPLACE INTO results
            (f_poly, g_poly, f_hash, g_hash, q_poly, is_trivial, df_divisible, dg_divisible, both_divisible)
            VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?)
        """, (
            f_str, g_str,
            f_hash_val, g_hash_val,
            q_str,
            1 if is_trivial else 0,
            1 if divisibility.get('df_divisible') else 0,
            1 if divisibility.get('dg_divisible') else 0,
            1 if divisibility.get('both_divisible') else 0
        ))
        
        self.conn.commit()
    
    def get_statistics(self) -> Dict:
        """
        Get summary statistics.
        
        Returns:
            Dictionary with statistics including:
            - total: total pairs checked
            - with_dependency: pairs where dependency was found
            - trivial_rejected: dependencies rejected as trivial
            - nontrivial_found: non-trivial dependencies found
            - df_divisible_only: only ∂q/∂f : ∂q/∂x holds
            - dg_divisible_only: only ∂q/∂g : ∂q/∂x holds
            - both_divisible: both divisibility conditions hold
            - no_dependency: no dependency found at all
        """
        cursor = self.conn.execute("""
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
        """)
        
        row = cursor.fetchone()
        return {
            'total': row['total'],
            'with_dependency': row['with_dependency'] or 0,
            'trivial_rejected': row['trivial_rejected'] or 0,
            'nontrivial_found': row['nontrivial_found'] or 0,
            'df_divisible_only': row['df_divisible_only'] or 0,
            'dg_divisible_only': row['dg_divisible_only'] or 0,
            'both_divisible': row['both_divisible'] or 0,
            'no_dependency': row['no_dependency'] or 0
        }
    
    def query_results(self, both_divisible: bool = False) -> List[Dict]:
        """
        Query results from database.
        
        Args:
            both_divisible: If True, only return results where both conditions hold
            
        Returns:
            List of result dictionaries
        """
        if both_divisible:
            cursor = self.conn.execute("""
                SELECT * FROM results 
                WHERE both_divisible = 1
                ORDER BY timestamp DESC
            """)
        else:
            cursor = self.conn.execute("""
                SELECT * FROM results 
                ORDER BY timestamp DESC
            """)
        
        return [dict(row) for row in cursor.fetchall()]
    
    def close(self):
        """Close database connection."""
        self.conn.close()
    
    def __enter__(self):
        """Context manager entry."""
        return self
    
    def __exit__(self, exc_type, exc_val, exc_tb):
        """Context manager exit."""
        self.close()