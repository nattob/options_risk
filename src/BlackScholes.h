#pragma once

double BlackScholes_Call( double S, double K, double r, double v, double T );
double BlackScholes_Put( double S, double K, double r, double v, double T );

double BlackScholes_NormCDF( double x );
double BlackScholes_d1( double S, double K, double r, double v, double T );