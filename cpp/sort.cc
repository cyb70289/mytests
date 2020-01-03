/*
 * $ g++ -O3 -o sort sort.cc
 *
 * $ ./sort          sort random data with default size
 * $ ./sort s        sort pre-sort test data with default size
 * $ ./sort 123456   sort 12,3456 random data
 * $ ./sort s 88888  sort 8,8888 pre-sorted data
 *
 * $ for size in 100 1000 10000 100000 1000000 10000000; do ./sort $size; done
 */

#include <iostream>
#include <vector>
#include <algorithm>
#include <numeric>
#include <chrono>
#include <iomanip>
#include <cstdlib>

static inline void stl_sort(const std::vector<unsigned char>& v,
                            std::vector<size_t>& idx)
{
    std::stable_sort(idx.begin(), idx.end(),
            [&v](size_t i1, size_t i2) { return v[i1] < v[i2]; });
}

static inline void count_sort(const std::vector<unsigned char>& v,
                             std::vector<size_t>& idx)
{
    std::vector<unsigned long> cnt(256);

    for (auto i : v) {
        ++cnt[i];
    }

    auto save = cnt[0];
    auto prev = cnt[0] = 0;
    for (int i = 1; i < 256; ++i) {
        prev += save;
        save = cnt[i];
        cnt[i] = prev;
    }

    for (unsigned i = 0; i < idx.size(); ++i) {
        idx[cnt[v[i]]++] = i;
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
    std::cout << "sort " << n << " elements" \
              << (pre_sort ? "(sorted)" : "(random)") << "\n";

    /***************** prepare test data ********************/
    auto v = std::vector<unsigned char>(n);

    /* fill random data */
    srand(time(NULL));
    for (long i = 0; i < n; ++i) {
        v[i] = rand();
    }

    /* pre-sort if required */
    if (pre_sort) {
        std::stable_sort(v.begin(), v.end());
    }

#ifdef DEBUG_SORT
    for (auto i : v) {
        std::cout << static_cast<int>(i) << "\t";
    }
    std::cout << std::endl;
#endif

    /***************** stl stable_sort ********************/
    auto idx_stl_sort = std::vector<size_t>(n);

    /* warm up */
    std::iota(idx_stl_sort.begin(), idx_stl_sort.end(), 0);
    stl_sort(v, idx_stl_sort);
    std::iota(idx_stl_sort.begin(), idx_stl_sort.end(), 0);

    /* bench */
    auto t1 = std::chrono::high_resolution_clock::now();
    stl_sort(v, idx_stl_sort);
    auto t2 = std::chrono::high_resolution_clock::now();

    std::chrono::duration<double> elapsed_stl = t2 - t1;
    std::cout << "stl stable_sort: " << elapsed_stl.count() << " s\n";

    /***************** counting sort ********************/
    auto idx_count_sort = std::vector<size_t>(n);

    /* warm up */
    std::iota(idx_count_sort.begin(), idx_count_sort.end(), 0);
    count_sort(v, idx_count_sort);
    std::iota(idx_count_sort.begin(), idx_count_sort.end(), 0);

    /* bench */
    t1 = std::chrono::high_resolution_clock::now();
    count_sort(v, idx_count_sort);
    t2 = std::chrono::high_resolution_clock::now();

    std::chrono::duration<double> elapsed_cnt = t2 - t1;
    std::cout << "counting sort  : " << elapsed_cnt.count() << " s\n";

    /***************** compare result ********************/
    if (!std::equal(idx_stl_sort.cbegin(), idx_stl_sort.cend(),
                idx_count_sort.cbegin())) {
        std::cout << "\nERROR!!!\n";
    }

    std::cout << "speedup: " << std::setprecision(2) \
              << elapsed_stl / elapsed_cnt << "x\n\n";

#ifdef DEBUG_SORT
    for (auto i : idx_count_sort) {
        std::cout << i << "\t";
    }
    std::cout << std::endl;
#endif

    return 0;
}
