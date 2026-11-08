<h1>radix-sort-digit-tuning</h1>
<p>Data-Parallel RadixSort (Variante 2): Digit size exploration (r = 4, 8, 16 Bit) based on Zagha & Blelloch</p>

<h2>Project description</h2>
<p>This project resolves structural bottlenecks of the initial RadixSort by adopting vector multiprocessor design techniques:</p>
<ol>
  <li>Replaces dynamic multi-dimensional vectors with flat contiguous memory buffers (<code>p * NUM_BUCKETS</code>).</li>
  <li>
    <dl>
      <dt>Implements three-phase decomposition:</dt>
      <dd><b>Phase 1 (Histogram):</b> Threads compute local bucket distributions concurrently.</dd>
      <dd><b>Phase 2 (Scan-Buckets):</b> Column-major prefix scan over the bucket matrix to guarantee sorting stability.</dd>
      <dd><b>Phase 3 (Scatter):</b> Multi-threaded scattered permutation directly to ping-pong output buffer.</dd>
    </dl>
  </li>
  <li>
    <dl>
      <dt>Empirically investigates radix digit size <code>r</code>:</dt>
      <dd><b>r = 4 Bit:</b> 16 buckets, 8 passes. High bus traffic.</dd>
      <dd><b>r = 8 Bit:</b> 256 buckets, 4 passes. Optimal balance between pass count and cache locality.</dd>
      <dd><b>r = 16 Bit:</b> 65,536 buckets, 2 passes. High TLB and scatter contention.</dd>
    </dl>
  </li>
  <li>Validates sorting output against baseline <code>std::qsort</code>.</li>
</ol>

<h2>Project files</h2>
<table>
  <tr>
    <th colspan="2">radix-sort-digit-tuning</th>
  </tr>
  <tr>
    <td>CMakeLists.txt</td>
    <td>Build configuration for C++20 and TBB</td>
  </tr>
  <tr>
    <td>main.cpp</td>
    <td>Template-parameterized implementation <code>radix_sort_v2&lt;RADIX_BITS&gt;</code></td>
  </tr>
  <tr>
    <td>README.md</td>
    <td>Project documentation and radix parameter comparative analysis</td>
  </tr>
</table>

<h2>Installation and launch</h2>
<ol>
  <b>
    <li>
      <dl>
        <dt>Clone repository</dt>
        <dd>
          <code>git clone https://github.com/Kyoryuu-lab/radix-sort-digit-tuning.git</code>
        </dd>
        <dd>
          <code>cd radix-sort-digit-tuning</code>
        </dd>
      </dl>
    </li>
  </b>
  <b>
    <li>
      <dl>
        <dt>Build executable</dt>
        <dd>
          <code>cmake -B build -DCMAKE_BUILD_TYPE=Release</code>
        </dd>
        <dd>
          <code>cmake --build build --config Release</code>
        </dd>
      </dl>
    </li>
  </b>
  <b>
    <li>
      <dl>
        <dt>Launch test suite</dt>
        <dd>
          <code>./build/main</code>
        </dd>
      </dl>
    </li>
  </b>
</ol>

<h2>Example of console output</h2>
<samp>
  <p>
    ========================================================================<br />
    PVA Rechneruebung 2: RadixSort Benchmark & Analyse<br />
    N = 10000000 | Threads: 15<br />
    ========================================================================<br />
    std::qsort (Baseline):        1150.32 ms | Speedup:  1.00x | Effizienz: 100.0 %<br />
    Variante 1 (8-Bit naiv):       168.10 ms | Speedup:  6.84x | Effizienz:  45.6 %<br />
    ------------------------------------------------------------------------<br />
    Variante 2: Untersuchung der Radix-Groesse (r-Bit):<br />
    Variante 2 (r = 4 Bit):         75.40 ms | Speedup: 15.26x | Effizienz: 101.7 %<br />
    Variante 2 (r = 8 Bit) [OPT]:   26.15 ms | Speedup: 43.99x | Effizienz: 293.3 %<br />
    Variante 2 (r = 16 Bit):        45.80 ms | Speedup: 25.12x | Effizienz: 167.5 %<br />
    ========================================================================<br />
    Validierungsstatus: ALLE SORTIERUNGEN KORREKT (SUCCESS)<br />
  </p>
</samp>
