// Tests for the scalar Black-Scholes reference pricer.
//
// Four groups, from most specific to most general:
//   1. Textbook values   - published worked examples (Hull)
//   2. Put-call parity    - a model-free identity, checked over a grid
//   3. Bounds and shape   - no-arbitrage bounds, monotonicity, convexity, limits
//   4. Greeks             - analytic Greeks vs. central finite differences
//
// Parameter order everywhere: (S, K, r, v, T)
//   S = spot, K = strike, r = risk-free rate, v = volatility, T = years to expiry

#include <gtest/gtest.h>

#include <algorithm>
#include <cmath>
#include <vector>

#include "BlackScholes.h"
#include "Greeks.h"

namespace {

// ---------------------------------------------------------------------------
// Greek unit conventions. Set these to match YOUR implementation.
//
// The finite-difference checks compute raw derivatives (per 1.00 change in the
// input, per year of time). Each scale converts that raw derivative into your
// units. Defaults assume raw derivatives.
//
//   vega  per 1.00 vol (0.20 -> 1.20):        kVegaScale  = 1.0
//   vega  per 1 vol point (0.20 -> 0.21):     kVegaScale  = 0.01
//   theta per year:                           kThetaScale = 1.0
//   theta per calendar day:                   kThetaScale = 1.0 / 365.0
//   rho   per 1.00 rate (5% -> 105%):         kRhoScale   = 1.0
//   rho   per 1 basis point... per 1%:        kRhoScale   = 0.01
//
// Theta sign convention assumed: theta = dC/dt as calendar time passes, so it is
// NEGATIVE for a typical long call. If yours is the positive time decay, set
// kThetaSign = -1.0.
// ---------------------------------------------------------------------------
constexpr double kVegaScale = 1.0;
constexpr double kThetaScale = 1.0;
constexpr double kRhoScale = 1.0;
constexpr double kThetaSign = 1.0;

struct Inputs {
    double S, K, r, v, T;
};

// Prints the failing inputs when an assertion inside a grid loop fails.
::testing::Message Describe(const Inputs& p) {
    return ::testing::Message() << "S=" << p.S << " K=" << p.K << " r=" << p.r
                                << " v=" << p.v << " T=" << p.T;
}

double Call(const Inputs& p) { return BlackScholes_Call(p.S, p.K, p.r, p.v, p.T); }
double Put(const Inputs& p) { return BlackScholes_Put(p.S, p.K, p.r, p.v, p.T); }
Greeks CallGreeks(const Inputs& p) { return CalculateGreeksCall(p.S, p.K, p.r, p.v, p.T); }

// A grid spanning deep OTM to deep ITM, 1 day to 5 years, low to extreme vol.
std::vector<Inputs> Grid() {
    std::vector<Inputs> grid;
    for (double S : {50.0, 80.0, 95.0, 100.0, 105.0, 120.0, 200.0})
        for (double r : {0.0, 0.05})
            for (double v : {0.05, 0.20, 0.50, 1.00})
                for (double T : {1.0 / 365.0, 0.25, 1.0, 5.0})
                    grid.push_back({S, 100.0, r, v, T});
    return grid;
}

// Tolerance that is relative for large values and absolute near zero.
double Tol(double expected, double rel, double abs_floor) {
    return std::max(abs_floor, rel * std::abs(expected));
}

}  // namespace

// ===========================================================================
// 1. Textbook values
// ===========================================================================

// Hull, "Options, Futures, and Other Derivatives": S=42, K=40, r=10%, v=20%,
// T=0.5. Published answers (rounded to cents): call 4.76, put 0.81.
TEST(TextbookValues, HullCallAndPut) {
    const Inputs p{42.0, 40.0, 0.10, 0.20, 0.5};
    EXPECT_NEAR(Call(p), 4.76, 0.005);
    EXPECT_NEAR(Put(p), 0.81, 0.005);
}

// Hull's Greeks example: S=49, K=50, r=5%, v=20%, T=20 weeks (0.3846 years).
// Published: price 2.40, delta 0.522, gamma 0.066, vega 12.1 (per 1.00 vol),
// theta -4.31 (per year), rho 8.91 (per 1.00 rate).
TEST(TextbookValues, HullGreeks) {
    const Inputs p{49.0, 50.0, 0.05, 0.20, 0.3846};
    EXPECT_NEAR(Call(p), 2.40, 0.005);

    const Greeks g = CallGreeks(p);
    EXPECT_NEAR(g.delta, 0.522, 0.0005);
    EXPECT_NEAR(g.gamma, 0.066, 0.0005);
    EXPECT_NEAR(g.vega, 12.1 * kVegaScale, 0.05 * kVegaScale);
    EXPECT_NEAR(g.theta, kThetaSign * -4.31 * kThetaScale, 0.005 * kThetaScale);
    EXPECT_NEAR(g.rho, 8.91 * kRhoScale, 0.005 * kRhoScale);
}

// ===========================================================================
// 2. Put-call parity: C - P = S - K e^(-rT), for every input.
// ===========================================================================

TEST(PutCallParity, HoldsOverGrid) {
    for (const Inputs& p : Grid()) {
        SCOPED_TRACE(Describe(p));
        const double lhs = Call(p) - Put(p);
        const double rhs = p.S - p.K * std::exp(-p.r * p.T);
        EXPECT_NEAR(lhs, rhs, 1e-10 * p.S);
    }
}

// ===========================================================================
// 3. Bounds, shape, and limits
// ===========================================================================

// No-arbitrage bounds:
//   max(S - K e^(-rT), 0) <= C <= S
//   max(K e^(-rT) - S, 0) <= P <= K e^(-rT)
TEST(Bounds, CallAndPutWithinNoArbitrageBounds) {
    for (const Inputs& p : Grid()) {
        SCOPED_TRACE(Describe(p));
        const double df = std::exp(-p.r * p.T);
        const double eps = 1e-12 * p.S;  // rounding slack only

        const double c = Call(p);
        EXPECT_GE(c, std::max(p.S - p.K * df, 0.0) - eps);
        EXPECT_LE(c, p.S + eps);

        const double put = Put(p);
        EXPECT_GE(put, std::max(p.K * df - p.S, 0.0) - eps);
        EXPECT_LE(put, p.K * df + eps);
    }
}

// Call falls and put rises as strike rises; both rise with volatility;
// call rises and put falls with spot.
TEST(Shape, Monotonicity) {
    for (const Inputs& p : Grid()) {
        SCOPED_TRACE(Describe(p));
        const double eps = 1e-12 * p.S;

        Inputs higher_k = p;
        higher_k.K += 1.0;
        EXPECT_LE(Call(higher_k), Call(p) + eps);
        EXPECT_GE(Put(higher_k), Put(p) - eps);

        Inputs higher_v = p;
        higher_v.v += 0.01;
        EXPECT_GE(Call(higher_v), Call(p) - eps);
        EXPECT_GE(Put(higher_v), Put(p) - eps);

        Inputs higher_s = p;
        higher_s.S += 1.0;
        EXPECT_GE(Call(higher_s), Call(p) - eps);
        EXPECT_LE(Put(higher_s), Put(p) + eps);
    }
}

// Option prices are convex in strike: C(K-h) - 2C(K) + C(K+h) >= 0.
// (A violation would be a free butterfly spread.)
TEST(Shape, ConvexInStrike) {
    for (const Inputs& p : Grid()) {
        SCOPED_TRACE(Describe(p));
        const double h = 1.0;
        Inputs lo = p, hi = p;
        lo.K -= h;
        hi.K += h;
        const double eps = 1e-10 * p.S;
        EXPECT_GE(Call(lo) - 2.0 * Call(p) + Call(hi), -eps);
        EXPECT_GE(Put(lo) - 2.0 * Put(p) + Put(hi), -eps);
    }
}

// With almost no volatility the option is worth its discounted intrinsic value.
TEST(Limits, ZeroVolatilityGivesDiscountedIntrinsic) {
    for (double S : {80.0, 120.0}) {  // avoid S == K e^(-rT), where the limit is a kink
        const Inputs p{S, 100.0, 0.05, 1e-6, 1.0};
        SCOPED_TRACE(Describe(p));
        const double df = std::exp(-p.r * p.T);
        EXPECT_NEAR(Call(p), std::max(p.S - p.K * df, 0.0), 1e-6);
        EXPECT_NEAR(Put(p), std::max(p.K * df - p.S, 0.0), 1e-6);
    }
}

// At expiry the option is worth its intrinsic value.
TEST(Limits, NearExpiryGivesIntrinsic) {
    for (double S : {80.0, 100.0, 120.0}) {
        const Inputs p{S, 100.0, 0.05, 0.20, 1e-10};
        SCOPED_TRACE(Describe(p));
        EXPECT_NEAR(Call(p), std::max(p.S - p.K, 0.0), 1e-3);
        EXPECT_NEAR(Put(p), std::max(p.K - p.S, 0.0), 1e-3);
    }
}

// ===========================================================================
// 4. Greeks
// ===========================================================================

// Basic sign and range facts for a long call.
TEST(Greeks, SignsAndRanges) {
    for (const Inputs& p : Grid()) {
        SCOPED_TRACE(Describe(p));
        const Greeks g = CallGreeks(p);
        EXPECT_GE(g.delta, 0.0);
        EXPECT_LE(g.delta, 1.0);
        EXPECT_GE(g.gamma, 0.0);
        EXPECT_GE(g.vega * kVegaScale, 0.0);  // scale > 0, so this is a sign check
        EXPECT_GE(g.rho * kRhoScale, 0.0);
    }
}

// Each analytic Greek must match the slope of the price, computed by bumping one
// input up and down and repricing (central differences, error O(h^2)).
//
// Step sizes balance two errors:
//   - too large, and the bump straddles the curve's bend, so the slope is coarse
//   - too small, and subtracting two nearly equal prices loses precision to
//     rounding (the rounding error is ~1e-16 * price, divided by h or h^2)
//
// The spot bump scales with S * v * sqrt(T): the width of the price curve's bend.
// Near expiry with low vol that width is tiny (about 0.26 for a 1-day, 5%-vol
// option on 100), so a bump proportional to S alone would be far too coarse.
// Gamma divides by h^2, which amplifies rounding, so it uses a larger step.
TEST(Greeks, MatchFiniteDifferences) {
    for (const Inputs& p : Grid()) {
        SCOPED_TRACE(Describe(p));
        const Greeks g = CallGreeks(p);
        const double spot_scale = p.S * p.v * std::sqrt(p.T);

        // Delta and gamma: bump S.
        {
            const double h1 = 1e-4 * spot_scale;
            Inputs up = p, dn = p;
            up.S += h1;
            dn.S -= h1;
            const double fd_delta = (Call(up) - Call(dn)) / (2.0 * h1);
            EXPECT_NEAR(g.delta, fd_delta, Tol(fd_delta, 1e-6, 1e-8)) << "delta";

            const double h2 = 1e-2 * spot_scale;
            up = p;
            dn = p;
            up.S += h2;
            dn.S -= h2;
            const double fd_gamma = (Call(up) - 2.0 * Call(p) + Call(dn)) / (h2 * h2);
            EXPECT_NEAR(g.gamma, fd_gamma, Tol(fd_gamma, 1e-3, 1e-7)) << "gamma";
        }

        // Vega: bump v.
        {
            const double h = 1e-5;  // absolute vol step; a relative one is too small at low vol
            Inputs up = p, dn = p;
            up.v += h;
            dn.v -= h;
            const double fd = (Call(up) - Call(dn)) / (2.0 * h) * kVegaScale;
            EXPECT_NEAR(g.vega, fd, Tol(fd, 1e-5, 1e-8)) << "vega";
        }

        // Rho: bump r (absolute step, since r can be zero).
        {
            const double h = 1e-6;
            Inputs up = p, dn = p;
            up.r += h;
            dn.r -= h;
            const double fd = (Call(up) - Call(dn)) / (2.0 * h) * kRhoScale;
            EXPECT_NEAR(g.rho, fd, Tol(fd, 1e-6, 1e-7)) << "rho";
        }

        // Theta: as calendar time passes, T shrinks, so dC/dt = -dC/dT.
        {
            const double h = 1e-4 * p.T;
            Inputs up = p, dn = p;
            up.T += h;
            dn.T -= h;
            const double dC_dT = (Call(up) - Call(dn)) / (2.0 * h);
            const double fd = kThetaSign * -dC_dT * kThetaScale;
            EXPECT_NEAR(g.theta, fd, Tol(fd, 1e-5, 1e-7)) << "theta";
        }
    }
}
