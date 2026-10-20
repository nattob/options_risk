

struct Greeks
{
    double delta;
    double gamma;
    double theta;
    double vega;
    double rho;
};

Greeks CalculateGreeksCall(double S, double K, double r, double v, double T, const double change);