/*
Delta - reprsents the change in price of on option resulting from the change in price of the underlying asset.
    --> Ranges from 0 to 1
    --> At-the-money call (put) options have a delta close to 0.5 (-0.5), which will increase (decrease) 
        towards 1 (-1) as the option gets deeper in the money, and closer to 0 as it gets further out of the money.
    --> Delta also gives an indication of the likelihood of an option expiring in the money; an option with 
        a delta of 0.7 has a roughly 70% chance of expiring in the money.
Gamma - the change in the delta of the option in response to a change in price of the underlying  asset.
    --> second derivitave of the Delta
    --> The higher the value of gamma, the more sensitive the option’s delta to price changes in the 
        underlying asset. 
    --> Gamma is highest for options that are at the money and with shorter time to expiry, 
        and decreases as the option becomes deeper in or out of the money.
Theta - the time decay of an option contract and specifices the "time value" the option loses everyday.
    --> theta is higher for at-the-money options, as these have more time value built in the premium 
    than in- or out-of-the-money options with the same underlying asset and expiration date
Vega - the rate of change in the price of the option due to a change in implied volitilaty
    --> implied volatility is the expected volitilaty of the underlying asset from now until the option expires
    --> This is positive for the value of options, which is why option prices increase when the volatility 
        of the underlying asset rises.
Rho - the change in price of an option relative to a change in the interest rate
    --> an increase in interest rates increases the value of call options (opposite for puts)
*/

#include "BlackScholes.h"
#include "Greeks.h"

Greeks CalculateGreeksCall(double S, double K, double r, double v, double T, const double change)
{

    double price = BlackScholes_Call(S, K, r, v, T);
    double priceUp = BlackScholes_Call(S + change, K, r, v, T);
    double priceDown = BlackScholes_Call(S - change, K, r, v, T);
    double priceT = BlackScholes_Call(S, K, r, v, T - change / 365);
    double priceV = BlackScholes_Call(S, K, r, v + change, T);
    double priceR = BlackScholes_Call(S, K, r + change, v, T);

    Greeks greeks;
    greeks.delta = (priceUp - priceDown) / (2 * change);
    greeks.gamma = (priceUp - 2 * price + priceDown) / (change * change);
    greeks.theta = -(priceT - price) / (change / 365);
    greeks.vega = (priceV - price) / change;
    greeks.rho = (priceR - price) / change;

    return greeks;
}