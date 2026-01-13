"""Brute force runner with checkpointing and multiprocessing."""

from concurrent.futures import ProcessPoolExecutor, as_completed
from threading import Lock
import time

from .config import Config
from .generator import PolynomialGenerator
from .dependency_finder import DependencyFinder
from .divisibility import DivisibilityChecker
from .cache import ResultCache
from .state import BruteForceState


def process_pair_worker(args):
    """
    Worker function for processing a single pair (must be top-level for pickling).
    
    Args:
        args: Tuple of (f, g, config)
        
    Returns:
        Tuple of (f, g, q, divisibility, was_trivial, was_cached)
    """
    f, g, config = args
    
    # Create process-local instances
    cache = ResultCache(config.cache_file)
    
    try:
        # Check cache first
        cached = cache.get_result(f, g)
        if cached:
            return (f, g, None, {}, False, True)
        
        # Create instances for this process
        finder = DependencyFinder(config)
        checker = DivisibilityChecker()
        
        # Find dependency (returns tuple: q, was_trivial)
        q, was_trivial = finder.find_dependency(f, g)
        
        # Check divisibility if dependency found
        divisibility = {}
        if q:
            divisibility = checker.check_conditions(q, f, g)
        
        return (f, g, q, divisibility, was_trivial, False)
    finally:
        cache.close()


class BruteForceRunner:
    """Run brute force search with state persistence and multiprocessing."""
    
    def __init__(self, config: Config):
        """
        Initialize brute force runner.
        
        Args:
            config: Configuration object
        """
        self.config = config
        self.generator = PolynomialGenerator(config)
        self.state = BruteForceState.load(config.state_file)
        
        # Thread-safe lock for state updates
        self.state_lock = Lock()
        self.cache_lock = Lock()
        self.checkpoint_counter = 0
    
    def run(self):
        """Run brute force search with parallel processing and checkpointing."""
        print("Starting brute force search...")
        print(f"Configuration:")
        print(f"  Max degree f: {self.config.max_degree_f}")
        print(f"  Max degree g: {self.config.max_degree_g}")
        print(f"  Max degree q: {self.config.max_degree_q}")
        print(f"  Coefficient range: [{self.config.coeff_min}, {self.config.coeff_max}]")
        print(f"  Strategy: {self.config.enum_strategy}")
        print(f"  Workers: {self.config.num_workers}")
        print()
        
        if self.state.total_pairs_checked > 0:
            print("Resuming from previous state:")
            print(self.state.get_summary())
            print()
        
        try:
            # Collect pairs into batches for parallel processing
            pairs_batch = []
            
            for f, g in self.generator.generate_pairs():
                pairs_batch.append((f, g))
                
                # Process batch when it reaches batch_size
                if len(pairs_batch) >= self.config.batch_size:
                    self._process_batch(pairs_batch)
                    pairs_batch = []
            
            # Process remaining pairs
            if pairs_batch:
                self._process_batch(pairs_batch)
        
        except KeyboardInterrupt:
            print("\n\nInterrupted by user. Saving state...")
            self.state.save(self.config.state_file)
            print("State saved. You can resume later with --resume flag.")
        
        finally:
            # Final save
            self.state.save(self.config.state_file)
            
            # Print final statistics
            print("\n" + "="*60)
            print("FINAL STATISTICS")
            print("="*60)
            print(self.state.get_summary())
            
            # Get statistics with temporary cache connection
            cache = ResultCache(self.config.cache_file)
            try:
                stats = cache.get_statistics()
                print(f"\nDetailed Statistics:")
                print(f"  Total pairs checked: {stats['total']}")
                print(f"  Dependencies found: {stats['with_dependency']}")
                print(f"    - Trivial (rejected): {stats['trivial_rejected']}")
                print(f"    - Non-trivial (kept): {stats['nontrivial_found']}")
                print(f"  No dependency found: {stats['no_dependency']}")
                print(f"\nDivisibility Results (for non-trivial dependencies):")
                print(f"  ∂q/∂f : ∂q/∂x only: {stats['df_divisible_only']}")
                print(f"  ∂q/∂g : ∂q/∂x only: {stats['dg_divisible_only']}")
                print(f"  Both conditions satisfied: {stats['both_divisible']}")
            finally:
                cache.close()
    
    def _process_batch(self, pairs_batch):
        """
        Process a batch of pairs in parallel using multiple processes.
        
        Args:
            pairs_batch: List of (f, g) tuples
        """
        # Prepare arguments for worker processes
        worker_args = [(f, g, self.config) for f, g in pairs_batch]
        
        # Use ProcessPoolExecutor for true parallelism
        with ProcessPoolExecutor(max_workers=self.config.num_workers) as executor:
            # Submit all pairs for processing
            future_to_pair = {
                executor.submit(process_pair_worker, args): args[:2]
                for args in worker_args
            }
            
            # Process results as they complete
            for future in as_completed(future_to_pair):
                f, g = future_to_pair[future]
                
                try:
                    f_result, g_result, q, divisibility, was_trivial, was_cached = future.result()
                    
                    if was_cached:
                        print(f"[CACHED] f={f}, g={g}")
                    else:
                        print(f"[CHECKED] f={f}, g={g}")
                        
                        if was_trivial:
                            print(f"  Found dependency but REJECTED as trivial (only linear x)")
                        elif q:
                            print(f"  Found q={q}")
                            print(f"  ∂q/∂f : ∂q/∂x = {divisibility['df_divisible']}")
                            print(f"  ∂q/∂g : ∂q/∂x = {divisibility['dg_divisible']}")
                        else:
                            print(f"  No dependency found")
                        
                        # Save result with thread-local cache connection
                        with self.cache_lock:
                            cache = ResultCache(self.config.cache_file)
                            try:
                                cache.save_result(f, g, q, divisibility, was_trivial)
                            finally:
                                cache.close()
                        
                        # Update state (thread-safe)
                        with self.state_lock:
                            self.state.update_progress(0, 0, q is not None)
                            self.checkpoint_counter += 1
                            
                            # Checkpoint
                            if self.checkpoint_counter >= self.config.checkpoint_interval:
                                self.state.save(self.config.state_file)
                                self.checkpoint_counter = 0
                                print(f"[CHECKPOINT] {self.state.total_pairs_checked} pairs checked")
                                print()
                
                except Exception as e:
                    print(f"[ERROR] Failed to process f={f}, g={g}: {e}")