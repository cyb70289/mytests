/*
 * $ g++ -O3 -o minmax minmax.cc
 *
 * $ ./minmax         find minmax from random data with default size
 * $ ./minmax s       find minmax from pre-sort test data with default size
 * $ ./minmax 123456  find minmax from 12,3456 random data
 * $ ./mimmax s 88888 find minmax from 8,8888 pre-sorted data
 */

#include <iostream>
#include <vector>
#include <algorithm>
#include <numeric>
#include <chrono>
#include <iomanip>
#include <cstdlib>

static inline void stl_minmax(int v, int& min, int &max)
{
    min = std::min(v, min);
    max = std::max(v, max);
}

/* Much slower than separated min,max due to branches */
static inline void opt_minmax(int v, int& min, int& max)
{
    if (v < min) {
        min = v;
    } else if (v > max) {
        max = v;
    }
}

int main(int argc, char *argv[])
{
    int pre_sort = 0;
    long n = 10000000;

    if (argc >= 2) {
        if (argv[1][0] == 's') {
           pre_sort = 1;
        }
        if (pre_sort+2 == argc) {
            long m = atol(argv[pre_sort+1]);
            if (m > 0) {
                n = m;
            } else {
                std::cout << "wrong element count, use default\n";
            }
        }
    }
    std::cout << "find min,max in " << n << " elements" \
              << (pre_sort ? "(sorted)" : "(random)") << "\n";

    /***************** prepare test data ********************/
    auto values = std::vector<int>(n);

    /* fill random data */
    std::cerr << "generating test data...";
    srand(time(NULL));
    for (long i = 0; i < n; ++i) {
        values[i] = rand();
    }
    std::cout << "done\n";

    /* pre-sort if required */
    if (pre_sort) {
        std::cerr << "sorting elements... ";
        std::sort(values.begin(), values.end());
        std::cout << "done\n";
    }

    /***************** std::min, std::max  ********************/
    int min1, max1;

    /* warm up */
    min1 = std::numeric_limits<int>::max();
    max1 = std::numeric_limits<int>::min();
    for (auto v : values) {
        stl_minmax(v, min1, max1);
    }

    /* bench */
    min1 = std::numeric_limits<int>::max();
    max1 = std::numeric_limits<int>::min();
    auto t1 = std::chrono::high_resolution_clock::now();
    for (auto v : values) {
        stl_minmax(v, min1, max1);
    }
    auto t2 = std::chrono::high_resolution_clock::now();

    std::chrono::duration<double> elapsed_stl = t2 - t1;
    std::cout << "stl minmax: " << elapsed_stl.count() << " s\n";

    /***************** optimized minmax ********************/
    int min2, max2;

    /* warm up */
    min2 = max2 = values[0];
    for (auto v : values) {
        opt_minmax(v, min2, max2);
    }

    /* bench */
    min2 = max2 = values[0];
    t1 = std::chrono::high_resolution_clock::now();
    for (auto v : values) {
        opt_minmax(v, min2, max2);
    }
    t2 = std::chrono::high_resolution_clock::now();

    std::chrono::duration<double> elapsed_opt = t2 - t1;
    std::cout << "opt minmax: " << elapsed_opt.count() << " s\n";

    /***************** compare result ********************/
    if (min1 != min2 || max1 != max2) {
        std::cout << "\nERROR!!!\n";
    }

    std::cout << "speedup: " << std::setprecision(2) \
              << elapsed_stl / elapsed_opt << "x\n\n";

    return 0;
}
