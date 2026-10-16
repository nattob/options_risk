#include <iostream>
#include <iomanip>
#include "BlackScholes.h"

int main() 
{
    // Parameter parameters
    double S = 100.0;   // Underlying Asset Price (Spot)
    double K = 100.0;   // Strike Price
    double r = 0.05;    // Risk-free interest rate (5%)
    double v = 0.20;    // Volatility (20%)
    double T = 1.0;     // Time to maturity (1 year)

    // Calculate option prices
    double call_price = BlackScholes_Call(S, K, r, v, T);
    double put_price = BlackScholes_Put(S, K, r, v, T);

    // Output results
    std::cout << std::fixed << std::setprecision(5);
    std::cout << "--- Black-Scholes Option Pricing ---" << std::endl;
    std::cout << "Underlying Price (S): " << S << std::endl;
    std::cout << "Strike Price (K)    : " << K << std::endl;
    std::cout << "Risk-Free Rate (r)  : " << r << std::endl;
    std::cout << "Volatility (v)      : " << v << std::endl;
    std::cout << "Maturity (T)        : " << T << " year(s)" << std::endl;
    std::cout << "------------------------------------" << std::endl;
    std::cout << "Call Option Price   : " << call_price << std::endl; // Expected ~10.45058
    std::cout << "Put Option Price    : " << put_price << std::endl;  // Expected ~5.57352

    return 0;
}