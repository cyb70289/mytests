/*
 * $ g++ -O3 -o mode-range mode-range.cc
 *
 * $ ./mode-range [range=1000] [count=10000000]
 *
 * $ V="100 300 1000 3000 10000 30000 100000 300000 1000000 3000000 10000000"
 * $ for range in $V; do for count in $V; do ./mode-range $range $count; done; done
 */

#include <iostream>
#include <vector>
#include <unordered_map>
#include <algorithm>
#include <numeric>
#include <chrono>
#include <iomanip>
#include <cstdlib>
#include <cassert>

template <typename T>
static std::pair<T, int64_t> stl_mode(const std::vector<T>& array) {
    std::unordered_map<T, int64_t> value_counts{};
    for (const auto value : array) {
        ++value_counts[value];
    }

    T mode = std::numeric_limits<T>::min();
    int64_t count = 0;
    for (const auto& value_count : value_counts) {
        auto this_value = value_count.first;
        auto this_count = value_count.second;
        if (this_count > count || (this_count == count && this_value < mode)) {
            count = this_count;
            mode = this_value;
        }
    }

    return std::make_pair(mode, count);
}

template <typename T>
static std::pair<T, int64_t> count_mode(const std::vector<T>& array) {
    T min = std::numeric_limits<T>::max();
    T max = std::numeric_limits<T>::min();
    for (const auto value : array) {
        min = std::min(min, value);
        max = std::max(max, value);
    }

    std::vector<int64_t> value_counts(max - min + 1);
    for (const auto value : array) {
        ++value_counts[value - min];
    }

    T mode = std::numeric_limits<T>::min();
    int64_t count = 0;
    for (int64_t i = 0; i < max - min + 1; ++i) {
        T this_value = static_cast<T>(min + i);
        int64_t this_count = value_counts[i];
        if (this_count > count || (this_count == count && this_value < mode)) {
            count = this_count;
            mode = this_value;
        }
    }

    return std::make_pair(mode, count);
}

using mode_type = long;

int main(int argc, char *argv[]) {
    long range = 1000, n = 10000000;
    mode_type min, max;

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
    std::cout << "\nmode " << n << " elements, range = " << range
              << " [" << min << ", " << max << "]\n";

    /***************** prepare test data ********************/
    auto array = std::vector<mode_type>(n);

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

    /***************** unordered_map ********************/
    std::pair<mode_type, int64_t> stl_mode_ret{};

    int loops_stl = 10000000 / n;
    if (loops_stl == 0) {
        loops_stl = 1;
    }

    /* warm up */
    stl_mode_ret = stl_mode(array);

    /* bench */
    auto t1 = std::chrono::high_resolution_clock::now();
    for (int i = 0; i < loops_stl; ++i) {
        auto ret = stl_mode(array);
        stl_mode_ret.first |= ret.first;
        stl_mode_ret.second |= ret.second;
    }
    auto t2 = std::chrono::high_resolution_clock::now();

    std::chrono::duration<double> elapsed_stl = (t2 - t1)  / loops_stl;
    std::cout << "stable_mode: " << elapsed_stl.count() << " s\n";

    /***************** counting mode ********************/
    std::pair<mode_type, int64_t> count_mode_ret{};

    /* 4x more loops than stl mode */
    int loops_cnt = 40000000 / n;

    /* warm up */
    t1 = std::chrono::high_resolution_clock::now();
    count_mode_ret = count_mode(array);
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
        auto ret = count_mode(array);
        count_mode_ret.first |= ret.first;
        count_mode_ret.second |= ret.second;
    }
    t2 = std::chrono::high_resolution_clock::now();

    elapsed_cnt = (t2 - t1) / loops_cnt;
    std::cout << "counting mode  : " << elapsed_cnt.count() << " s\n";

    /***************** compare result ********************/
    if (stl_mode_ret != count_mode_ret) {
        std::cout << "\nERROR!!!\n";
    }

    std::cout << "speedup: " << std::setprecision(2) \
              << elapsed_stl / elapsed_cnt << "x\n";

    /* csv format: count,range,speedup_ratio */
    std::cerr << n << ',' << range << ','
              << std::setprecision(2) << elapsed_stl / elapsed_cnt << '\n';

    return 0;
}
