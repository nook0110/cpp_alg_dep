"""Command-line interface for polynomial dependency checker."""

import argparse
import sys

from .config import Config
from .brute_force import BruteForceRunner
from .manual_check import ManualChecker
from .cache import ResultCache


def main():
    """Main CLI entry point."""
    parser = argparse.ArgumentParser(
        description="Polynomial Algebraic Dependency Checker",
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog="""
Examples:
  # Run brute force search
  python -m src.cli brute
  
  # Run with custom parameters
  python -m src.cli brute --max-degree-f 3 --max-degree-g 3 --max-degree-q 4
  
  # Resume from checkpoint
  python -m src.cli brute --resume
  
  # Check specific polynomial pair
  python -m src.cli check "x^2 + y^2" "x*y"
  
  # Show statistics
  python -m src.cli stats
  
  # Query results
  python -m src.cli query --both-divisible
        """
    )
    
    subparsers = parser.add_subparsers(dest='mode', help='Operation mode')
    
    # Brute force mode
    brute_parser = subparsers.add_parser('brute', help='Run brute force search')
    brute_parser.add_argument('--max-degree-f', type=int, default=2,
                             help='Maximum degree for polynomial f (default: 2)')
    brute_parser.add_argument('--max-degree-g', type=int, default=2,
                             help='Maximum degree for polynomial g (default: 2)')
    brute_parser.add_argument('--max-degree-q', type=int, default=3,
                             help='Maximum degree for dependency q (default: 3)')
    brute_parser.add_argument('--coeff-min', type=int, default=-2,
                             help='Minimum coefficient value (default: -2)')
    brute_parser.add_argument('--coeff-max', type=int, default=2,
                             help='Maximum coefficient value (default: 2)')
    brute_parser.add_argument('--strategy', 
                             choices=['lexicographic', 'degree_first'],
                             default='lexicographic',
                             help='Enumeration strategy (default: lexicographic)')
    brute_parser.add_argument('--checkpoint-interval', type=int, default=10,
                             help='Save state every N pairs (default: 10)')
    brute_parser.add_argument('--workers', type=int, default=4,
                             help='Number of parallel workers (default: 4)')
    brute_parser.add_argument('--resume', action='store_true',
                             help='Resume from last checkpoint')
    
    # Manual check mode
    manual_parser = subparsers.add_parser('check', 
                                         help='Check specific polynomial pair')
    manual_parser.add_argument('f', type=str, 
                              help='First polynomial (e.g., "x^2 + y^2")')
    manual_parser.add_argument('g', type=str,
                              help='Second polynomial (e.g., "x*y")')
    manual_parser.add_argument('--max-degree-q', type=int, default=3,
                              help='Maximum degree for dependency q (default: 3)')
    manual_parser.add_argument('--coeff-min', type=int, default=-2,
                              help='Minimum coefficient value (default: -2)')
    manual_parser.add_argument('--coeff-max', type=int, default=2,
                              help='Maximum coefficient value (default: 2)')
    
    # Statistics mode
    stats_parser = subparsers.add_parser('stats', help='Show statistics')
    
    # Query mode
    query_parser = subparsers.add_parser('query', help='Query results')
    query_parser.add_argument('--both-divisible', action='store_true',
                             help='Show only pairs where both conditions hold')
    query_parser.add_argument('--limit', type=int, default=20,
                             help='Maximum number of results to show (default: 20)')
    
    args = parser.parse_args()
    
    if not args.mode:
        parser.print_help()
        sys.exit(1)
    
    try:
        if args.mode == 'brute':
            config = Config(
                max_degree_f=args.max_degree_f,
                max_degree_g=args.max_degree_g,
                max_degree_q=args.max_degree_q,
                coeff_min=args.coeff_min,
                coeff_max=args.coeff_max,
                enum_strategy=args.strategy,
                checkpoint_interval=args.checkpoint_interval,
                num_workers=args.workers
            )
            runner = BruteForceRunner(config)
            runner.run()
        
        elif args.mode == 'check':
            config = Config(
                max_degree_q=args.max_degree_q,
                coeff_min=args.coeff_min,
                coeff_max=args.coeff_max
            )
            checker = ManualChecker(config)
            checker.check_pair(args.f, args.g)
        
        elif args.mode == 'stats':
            cache = ResultCache('data/results.db')
            stats = cache.get_statistics()
            
            print("="*60)
            print("DETAILED STATISTICS")
            print("="*60)
            print(f"\nTotal pairs checked: {stats['total']}")
            print(f"\nDependency Finding:")
            print(f"  Dependencies found: {stats['with_dependency']}")
            print(f"    - Trivial (rejected): {stats['trivial_rejected']}")
            print(f"    - Non-trivial (kept): {stats['nontrivial_found']}")
            print(f"  No dependency found: {stats['no_dependency']}")
            
            print(f"\nDivisibility Results (for non-trivial dependencies):")
            print(f"  ∂q/∂f : ∂q/∂x only: {stats['df_divisible_only']}")
            print(f"  ∂q/∂g : ∂q/∂x only: {stats['dg_divisible_only']}")
            print(f"  Both conditions satisfied: {stats['both_divisible']}")
            
            if stats['total'] > 0:
                print(f"\nPercentages:")
                dep_pct = (stats['with_dependency'] / stats['total']) * 100
                trivial_pct = (stats['trivial_rejected'] / stats['total']) * 100
                nontrivial_pct = (stats['nontrivial_found'] / stats['total']) * 100
                no_dep_pct = (stats['no_dependency'] / stats['total']) * 100
                print(f"  Any dependency found: {dep_pct:.1f}%")
                print(f"  Trivial (rejected): {trivial_pct:.1f}%")
                print(f"  Non-trivial (kept): {nontrivial_pct:.1f}%")
                print(f"  No dependency: {no_dep_pct:.1f}%")
                
                if stats['nontrivial_found'] > 0:
                    both_of_nontrivial = (stats['both_divisible'] / stats['nontrivial_found']) * 100
                    print(f"  Both conditions (of non-trivial): {both_of_nontrivial:.1f}%")
            
            cache.close()
        
        elif args.mode == 'query':
            cache = ResultCache('data/results.db')
            results = cache.query_results(both_divisible=args.both_divisible)
            
            print("="*60)
            print("QUERY RESULTS")
            print("="*60)
            
            if args.both_divisible:
                print("Showing pairs where both divisibility conditions hold")
            else:
                print("Showing all results")
            
            print(f"Limit: {args.limit}")
            print()
            
            if not results:
                print("No results found.")
            else:
                for i, result in enumerate(results[:args.limit], 1):
                    print(f"Result #{i}")
                    print(f"  f = {result['f_poly']}")
                    print(f"  g = {result['g_poly']}")
                    
                    if result['q_poly']:
                        print(f"  q = {result['q_poly']}")
                        print(f"  ∂q/∂u : ∂q/∂x = {bool(result['df_divisible'])}")
                        print(f"  ∂q/∂v : ∂q/∂x = {bool(result['dg_divisible'])}")
                    else:
                        print(f"  No dependency found")
                    
                    print(f"  Timestamp: {result['timestamp']}")
                    print()
                
                if len(results) > args.limit:
                    print(f"... and {len(results) - args.limit} more results")
            
            cache.close()
    
    except KeyboardInterrupt:
        print("\n\nInterrupted by user.")
        sys.exit(1)
    except Exception as e:
        print(f"\nError: {e}", file=sys.stderr)
        import traceback
        traceback.print_exc()
        sys.exit(1)


if __name__ == '__main__':
    main()
