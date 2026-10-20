#include <cmath>

// Standard normal cumulative distribution function (CDF)
double BlackScholes_NormCDF( double x )
{
    return 0.5 * std::erfc( -x * std::sqrt( 0.5 ) );
}

// Function to calculate d1
double BlackScholes_d1( double S, double K, double r, double v, double T )
{
    return ( std::log( S / K ) + ( r + 0.5 * v * v ) * T ) / ( v * std::sqrt( T ) );
}

// European
double BlackScholes_Call( double S, double K, double r, double v, double T )
{
    double d1 = BlackScholes_d1( S, K, r, v, T );
    double d2 = d1 - v * std::sqrt( T );

    return S * BlackScholes_NormCDF( d1 ) - K * std::exp( -r * T ) * BlackScholes_NormCDF( d2 );
}

double BlackScholes_Put( double S, double K, double r, double v, double T )
{
    double d1 = BlackScholes_d1( S, K, r, v, T );
    double d2 = d1 - v * std::sqrt( T );
    return K * std::exp( -r * T ) * BlackScholes_NormCDF( -d2 ) - S * BlackScholes_NormCDF( -d1 );
}