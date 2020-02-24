/*
 * $ g++ -O3 -o minmax-penalty minmax-penalty.cc
 *
 * $ ./minmax-penalty [count=10000000]
 *
 * $ C="1000 10000 100000 1000000 10000000"
 * $ for count in $C; do ./minmax-penalty $count; done
 */

#include <iostream>
#include <vector>
#include <algorithm>
#include <numeric>
#include <chrono>
#include <iomanip>
#include <cstdlib>

template <typename T>
static inline void stl_minmax(const std::vector<T>& array)
{
    T min = std::numeric_limits<T>::max();
    T max = std::numeric_limits<T>::min();

    for (auto v : array) {
        min = std::min(min, v);
        max = std::max(max, v);
    }

    /* just to make sure compiler won't optimize us away */
    if (min + max == 0) {
        std::cout << "lucky\n";
    }
}

template <typename T>
static inline void stl_sort(const std::vector<T>& array, std::vector<size_t>& idx) {
    std::stable_sort(idx.begin(), idx.end(),
                     [&array](size_t i1, size_t i2) { return array[i1] < array[i2]; });
}

using sort_type = long;

int main(int argc, char *argv[]) {
    long n = 10000000;

    if (argc == 2) {
        n = atol(argv[1]);
        if (n <= 0) {
            std::cout << argv[0] << " [count=10000000]\n";
            return 1;
        }
    }

    std::cout << "sort " << n << " elements\n";

    /***************** prepare test data ********************/
    auto array = std::vector<sort_type>(n);
    auto idx_sort = std::vector<size_t>(n);

    /* fill random data */
    srand(time(NULL));
    for (long i = 0; i < n; ++i) {
        array[i] = rand();
    }

    int loop_cnt = 10000000 / n;
    if (loop_cnt == 0) {
        loop_cnt = 1;
    }

    /************************ sort *************************/
    /* warm up */
    std::iota(idx_sort.begin(), idx_sort.end(), 0);
    stl_sort(array, idx_sort);

    /* bench */
    auto t1 = std::chrono::high_resolution_clock::now();
    for (int i = 0; i < loop_cnt; ++i) {
        std::iota(idx_sort.begin(), idx_sort.end(), 0);
        stl_sort(array, idx_sort);
    }
    auto t2 = std::chrono::high_resolution_clock::now();

    std::chrono::duration<double> elapsed_sort = (t2 - t1) / loop_cnt;
    std::cout << "sort: " << elapsed_sort.count() << " s\n";

    /****************** minmax + sort *********************/
    /* warm up */
    stl_minmax(array);
    stl_sort(array, idx_sort);

    /* bench */
    t1 = std::chrono::high_resolution_clock::now();
    for (int i = 0; i < loop_cnt; ++i) {
        stl_minmax(array);
        std::iota(idx_sort.begin(), idx_sort.end(), 0);
        stl_sort(array, idx_sort);
    }
    t2 = std::chrono::high_resolution_clock::now();

    std::chrono::duration<double> elapsed_minmax_sort = (t2 - t1) / loop_cnt;
    std::cout << "minmax + sort: " << elapsed_minmax_sort.count() << " s\n";

    /*************** calc minmax penalty *****************/
    double penalty = (elapsed_minmax_sort - elapsed_sort) / elapsed_sort;
    if (penalty < 0) {
        penalty = 0;
    }
    std::cout << "penalty: " << std::setprecision(2) << penalty * 100 << "%\n";

    return 0;
}
