#include <iostream>
#include <sstream>
#include <vector>

#include <boost/accumulators/accumulators.hpp>
#include <boost/accumulators/statistics/p_square_quantile.hpp>
#include <boost/accumulators/statistics/extended_p_square_quantile.hpp>
#include <boost/accumulators/statistics/stats.hpp>

namespace acc = boost::accumulators;

double quantile_boost(const std::vector<double>& values, double prob) {
    using accumulator_type =
        acc::accumulator_set<double, acc::stats<acc::tag::p_square_quantile>>;

    accumulator_type acc_q(acc::quantile_probability = prob);
    for (auto v : values) {
        acc_q(v);
    }
    return acc::p_square_quantile(acc_q);
}

std::vector<double> quantile_boost_extend(const std::vector<double>& values,
                                          const std::vector<double>& probs) {
    using accumulator_type = acc::accumulator_set<
        double, acc::stats<acc::tag::extended_p_square_quantile(acc::quadratic)>>;

    accumulator_type acc_q(acc::extended_p_square_probabilities = probs);
    for (auto v : values) {
        acc_q(v);
    }

    std::vector<double> q(probs.size());
    //XXX: q = acc::quantile(acc_q);
    for (size_t i = 0; i < q.size(); ++i) {
      q[i] = acc::quantile(acc_q, acc::quantile_probability = probs[i]);
    }
    return q;
}

int main(void) {
    const std::vector<double> values{
        0.02, 0.5, 0.74, 3.39, 0.83, 22.37, 10.15, 15.43, 38.62, 15.92, 34.60,
        10.28, 1.47, 0.40, 0.05, 11.39, 0.27, 0.42, 0.09, 11.37};
    const std::vector<double> probs{0.01, 0.1, 0.5, 0.9, 0.99};

    std::cout << "boost p-square:" << std::endl;
    for (size_t i = 0; i < probs.size(); ++i) {
        std::cout << quantile_boost(values, probs[i]) << " ";
    }
    std::cout << std::endl << std::endl;

    std::cout << "boost extended p-square:" << std::endl;
    std::vector<double> quantiles = quantile_boost_extend(values, probs);
    for (auto v : quantiles) {
        std::cout << v << " ";
    }
    std::cout << std::endl << std::endl;

    return 0;
}
