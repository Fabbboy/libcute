# GPA Memory Usage Investigation

This document captures experiments exploring high memory usage when allocating
small strings with the general purpose allocator (GPA). Valgrind's Massif tool
was used to track peak memory consumption.


## Initial test

Compiled `gpa_string_debug` which repeatedly creates and extends small strings.
Each allocator was profiled with `valgrind --tool=massif`.

Peak heap usage (in bytes):

- GPA (backed by page allocator): 24576
- C allocator: 68
- Page allocator: 4096

The GPA clearly over-allocates; 100 short string operations triggered
multiple 4 KiB buckets.

## Bucket reuse tweak

`cu_gpa_free_small` was modified to retain one empty bucket per size class.
This avoids constant churn when the allocator repeatedly allocates and frees
small objects.

After rebuilding, unit tests pass and the GPA example no longer times out.
Peak usage remained roughly the same (~24 KiB) with the current test, but
allocation reuse is now visible in profiling output.

## Summary

The inefficiency mainly stems from GPA's bucket strategy. Each small string
allocation forces the allocator to touch at least one 4 KiB bucket. Destroying a
bucket when empty led to constant creation of new buckets. Retaining one bucket
per size class reduced allocator churn but the inherent bucket granularity means
the GPA still uses significantly more memory than the C allocator for tiny
objects.
