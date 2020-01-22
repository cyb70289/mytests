/*
 * $ g++ -O3 -o sort-range sort-range.cc
 *
 * $ ./sort-range [range=1000] [count=10000000]
 *
 * $ V="100 300 1000 3000 10000 30000 100000 300000 1000000 3000000 10000000"
 * $ for range in $V; do for count in $V; do ./sort-range $range $count; done; done
 */

#include <iostream>
#include <vector>
#include <algorithm>
#include <numeric>
#include <chrono>
#include <iomanip>
#include <cstdlib>
#include <cassert>

template <typename T>
static void stl_sort(const std::vector<T>& array, std::vector<size_t>& idx) {
    std::stable_sort(idx.begin(), idx.end(),
                     [&array](size_t i1, size_t i2) { return array[i1] < array[i2]; });
}

template <typename T>
static void count_sort(const std::vector<T>& array, std::vector<size_t>& idx) {
    T min = std::numeric_limits<T>::max();
    T max = std::numeric_limits<T>::min();

    for (auto v : array) {
        min = std::min(min, v);
        max = std::max(max, v);
    }

    const size_t value_range = max - min + 1;

    /* first slot for prefix sum */
    std::vector<unsigned long> cnt(1 + value_range);

    for (auto v : array) {
        ++cnt[v-min+1];
    }

    for (size_t i = 1; i <= value_range; ++i) {
        cnt[i] += cnt[i-1];
    }

    for (size_t i = 0; i < idx.size(); ++i) {
        idx[cnt[array[i]-min]++] = i;
    }
}

using sort_type = long;

int main(int argc, char *argv[]) {
    long range = 1000, n = 10000000;
    sort_type min, max;

    if (argc >= 2) {
        range = atol(argv[1]);
        if (argc == 3) {
            n = atol(argv[2]);
        }
    }
    if (range <= 0 || n <= 0) {
        std::cout << argv[0] << " [range=200] [count=10000000]\n";
        return 1;
    }

    min = -(range / 2);
    max = range + min;
    std::cout << "\nsort " << n << " elements, range = " << range
              << " [" << min << ", " << max << "]\n";

    /***************** prepare test data ********************/
    auto array = std::vector<sort_type>(n);

    /* fill random data */
    srand(time(NULL));
    for (long i = 0; i < n; ++i) {
        array[i] = rand() % (range + 1) + min;
    }

    /* make sure at least one min and max value */
    int i1, i2;
    do {
        i1 = rand() % n;
        i2 = rand() % n;

        array[i1] = min;
        array[i2] = max;
    } while (i1 == i2);

    /***************** stl stable_sort ********************/
    auto idx_stl_sort = std::vector<size_t>(n);

    int loops_stl = 10000000 / n;
    if (loops_stl == 0) {
        loops_stl = 1;
    }

    /* warm up */
    std::iota(idx_stl_sort.begin(), idx_stl_sort.end(), 0);
    stl_sort(array, idx_stl_sort);

    /* bench */
    auto t1 = std::chrono::high_resolution_clock::now();
    for (int i = 0; i < loops_stl; ++i) {
        std::iota(idx_stl_sort.begin(), idx_stl_sort.end(), 0);
        stl_sort(array, idx_stl_sort);
    }
    auto t2 = std::chrono::high_resolution_clock::now();

    std::chrono::duration<double> elapsed_stl = (t2 - t1)  / loops_stl;
    std::cout << "stable_sort: " << elapsed_stl.count() << " s\n";

    /***************** counting sort ********************/
    auto idx_count_sort = std::vector<size_t>(n);

    /* 10x more loops than stl sort */
    int loops_cnt = 100000000 / n;

    /* warm up */
    t1 = std::chrono::high_resolution_clock::now();
    count_sort(array, idx_count_sort);
    t2 = std::chrono::high_resolution_clock::now();

    /* adjust loops for extreme cases (short array with large value range) */
    std::chrono::duration<double> elapsed_cnt = t2 - t1;
    if (loops_cnt > 0.5 / elapsed_cnt.count()) {
        loops_cnt = 0.5 / elapsed_cnt.count();
    }
    if (loops_cnt == 0) {
        loops_cnt = 1;
    }

    /* bench */
    t1 = std::chrono::high_resolution_clock::now();
    for (int i = 0; i < loops_cnt; ++i) {
        count_sort(array, idx_count_sort);
    }
    t2 = std::chrono::high_resolution_clock::now();

    elapsed_cnt = (t2 - t1) / loops_cnt;
    std::cout << "counting sort  : " << elapsed_cnt.count() << " s\n";

    /***************** compare result ********************/
    if (!std::equal(idx_stl_sort.cbegin(), idx_stl_sort.cend(),
                    idx_count_sort.cbegin())) {
        std::cout << "\nERROR!!!\n";
    }

    std::cout << "speedup: " << std::setprecision(2) \
              << elapsed_stl / elapsed_cnt << "x\n";

    /* csv format: count,range,speedup_ratio */
    std::cerr << n << ',' << range << ','
              << std::setprecision(2) << elapsed_stl / elapsed_cnt << '\n';

    return 0;
}
