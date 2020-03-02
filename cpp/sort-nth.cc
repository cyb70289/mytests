/*
 * $ g++ -O3 -o sort-nth sort-nth.cc
 *
 * $ ./sort-nth [count=10000000]
 */

#include <iostream>
#include <vector>
#include <algorithm>
#include <numeric>
#include <chrono>
#include <iomanip>
#include <cstdlib>

static void sort(const std::vector<long>& array, std::vector<int>& idx) {
    std::sort(idx.begin(), idx.end(),
              [&array](int i1, int i2) { return array[i1] < array[i2]; });
}

static void stable_sort(const std::vector<long>& array, std::vector<int>& idx) {
    std::stable_sort(idx.begin(), idx.end(),
                     [&array](int i1, int i2) { return array[i1] < array[i2]; });
}

static void partial_sort(const std::vector<long>& array, int n, std::vector<int>& idx) {
    std::partial_sort(idx.begin(), idx.begin() + n, idx.end(),
                      [&array](int i1, int i2) { return array[i1] < array[i2]; });
}

static void nth(const std::vector<long>& array, int n, std::vector<int>& idx) {
    std::nth_element(idx.begin(), idx.begin() + n, idx.end(),
                     [&array](int i1, int i2) { return array[i1] < array[i2]; });
}

int main(int argc, char *argv[]) {
    long n = 10000000;

    if (argc == 2) {
        n = atol(argv[1]);
        if (n <= 0) {
            std::cout << "count must be positive integer\n";
            return 1;
        }
    }

    /* position for partial_sort and nth_element */
    const int p = n / 3;

    int loops = 10000000 / n;
    if (loops == 0) {
        loops = 1;
    }

    std::cout << "sort/partial_sort/nth_element on " << n << " elements\n";

    /***************** prepare test data ********************/
    auto array = std::vector<long>(n);

    /* fill random data */
    srand(time(NULL));
    for (long i = 0; i < n; ++i) {
        array[i] = rand();
    }

    /********************* std::sort() **********************/
    auto idx_sort = std::vector<int>(n);

    /* warm up */
    std::iota(idx_sort.begin(), idx_sort.end(), 0);
    sort(array, idx_sort);

    /* bench */
    auto t1 = std::chrono::high_resolution_clock::now();
    for (int i = 0; i < loops; ++i) {
        std::iota(idx_sort.begin(), idx_sort.end(), 0);
        sort(array, idx_sort);
    }
    auto t2 = std::chrono::high_resolution_clock::now();

    std::chrono::duration<double> elapsed = (t2 - t1)  / loops;
    std::cout << "std::sort(): " << elapsed.count() << " s\n";

    /****************** std::stable_sort() *******************/
    auto idx_stable_sort = std::vector<int>(n);

    /* warm up */
    std::iota(idx_stable_sort.begin(), idx_stable_sort.end(), 0);
    stable_sort(array, idx_stable_sort);

    /* bench */
    t1 = std::chrono::high_resolution_clock::now();
    for (int i = 0; i < loops; ++i) {
        std::iota(idx_stable_sort.begin(), idx_stable_sort.end(), 0);
        stable_sort(array, idx_stable_sort);
    }
    t2 = std::chrono::high_resolution_clock::now();

    elapsed = (t2 - t1)  / loops;
    std::cout << "std::stable_sort(): " << elapsed.count() << " s\n";

    /***************** std::partial_sort() *******************/
    auto idx_partial_sort = std::vector<int>(n);

    /* warm up */
    std::iota(idx_partial_sort.begin(), idx_partial_sort.end(), 0);
    partial_sort(array, p, idx_partial_sort);

    /* bench */
    t1 = std::chrono::high_resolution_clock::now();
    for (int i = 0; i < loops; ++i) {
        std::iota(idx_partial_sort.begin(), idx_partial_sort.end(), 0);
        partial_sort(array, p, idx_partial_sort);
    }
    t2 = std::chrono::high_resolution_clock::now();

    elapsed = (t2 - t1)  / loops;
    std::cout << "std::partial_sort(" << p << "): " << elapsed.count() << " s\n";

    /********************* std::nth() **********************/
    auto idx_nth = std::vector<int>(n);

    /* warm up */
    std::iota(idx_nth.begin(), idx_nth.end(), 0);
    nth(array, p, idx_nth);

    /* bench */
    t1 = std::chrono::high_resolution_clock::now();
    for (int i = 0; i < loops; ++i) {
        std::iota(idx_nth.begin(), idx_nth.end(), 0);
        nth(array, p, idx_nth);
    }
    t2 = std::chrono::high_resolution_clock::now();

    elapsed = (t2 - t1)  / loops;
    std::cout << "std::nth(" << p << "): " << elapsed.count() << " s\n";

    return 0;
}
