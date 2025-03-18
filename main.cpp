#include <iostream>
#include <vector>
#include <cmath>
#include <random>
#include <chrono>
#include <algorithm>
using namespace std;

// Trade structure for each strategy's open trade
struct Trade {
    int strategyType;    // 1: Straddle, 2: Strangle, 3: Bull Spread, 4: Bear Spread, 5: Butterfly Spread
    int entryTick;
    int exitTick;
    double entryPrice;
    double exitPrice;
    // For options legs, we use strikes computed at entry.
    double strike1, strike2, strike3; 
    int volume;
    double payoff;
    bool open;
};

// -------------------------
// Option Payoff Functions
// (Assuming zero premiums for simplicity)
// -------------------------
double straddlePayoff(double S, double K) {
    double call = max(S - K, 0.0);
    double put  = max(K - S, 0.0);
    return call + put;
}

double stranglePayoff(double S, double K1, double K2) {
    double put  = max(K1 - S, 0.0);
    double call = max(S - K2, 0.0);
    return put + call;
}

double bullSpreadPayoff(double S, double K1, double K2) {
    double longCall  = max(S - K1, 0.0);
    double shortCall = max(S - K2, 0.0);
    return longCall - shortCall;
}

double bearSpreadPayoff(double S, double K1, double K2) {
    double longPut  = max(K1 - S, 0.0);
    double shortPut = max(S - K2, 0.0);
    return longPut - shortPut;
}

double butterflySpreadPayoff(double S, double K1, double K2, double K3) {
    double longCall1  = max(S - K1, 0.0);
    double shortCalls = 2.0 * max(S - K2, 0.0);
    double longCall2  = max(S - K3, 0.0);
    return longCall1 - shortCalls + longCall2;
}

// -------------------------
// Indicator functions: Moving Average and Volatility
// -------------------------
double computeMA(const vector<double>& prices, int currentTick, int window) {
    if (currentTick < window - 1) return prices[currentTick];
    double sum = 0;
    for (int i = currentTick - window + 1; i <= currentTick; i++) {
        sum += prices[i];
    }
    return sum / window;
}

double computeVolatility(const vector<double>& prices, int currentTick, int window) {
    if (currentTick < window) return 0.0;
    vector<double> returns;
    for (int i = currentTick - window + 1; i <= currentTick; i++) {
        if (i == 0) continue;
        double r = log(prices[i] / prices[i - 1]);
        returns.push_back(r);
    }
    double mean = 0;
    for (double r : returns) {
        mean += r;
    }
    mean /= returns.size();
    double variance = 0;
    for (double r : returns) {
        variance += (r - mean) * (r - mean);
    }
    variance /= returns.size();
    return sqrt(variance);
}

// -------------------------
// Main Simulation
// -------------------------
int main(){
    // Simulation parameters
    const int totalTicks = 10000;   // total simulation steps (HFT style)
    const double S0 = 100.0;        // initial underlying price
    const double mu = 0.0001;       // drift per tick
    const double sigma = 0.01;      // volatility per tick
    const double dt = 1.0;          // time step
    const int holdPeriod = 10;      // holding period (in ticks) for each trade

    // Strategy-specific parameters
    const double delta = 0.05; // 5% offset for strikes
    // Indicator windows (in ticks)
    const int shortWindow = 5;
    const int longWindow  = 20;
    const int volWindow   = 5;
    // Volatility thresholds (arbitrary values for demonstration)
    const double volThresholdHigh       = 0.01;   // for straddle entry
    const double volThresholdLow        = 0.005;  // for straddle exit
    const double volThresholdHighStrangle = 0.012; // for strangle entry
    const double volThresholdLowStrangle  = 0.007; // for strangle exit

    // Cumulative PnL per strategy (indices 1 to 5)
    double cumulativePnL[6] = {0, 0, 0, 0, 0, 0};

    // Active trade record for each strategy (only one open trade per strategy)
    Trade activeTrades[6];
    for (int i = 1; i <= 5; i++) {
        activeTrades[i].open = false;
    }

    vector<double> prices;
    prices.reserve(totalTicks);
    prices.push_back(S0);

    // Set up random number generator for GBM simulation
    unsigned seed = chrono::system_clock::now().time_since_epoch().count();
    default_random_engine generator(seed);
    normal_distribution<double> distribution(0.0, 1.0);

    // Main simulation loop
    for (int t = 1; t < totalTicks; t++) {
        // ----- Simulate underlying price using GBM -----
        double Z = distribution(generator);
        double S_prev = prices.back();
        double S_new = S_prev * exp((mu - 0.5 * sigma * sigma) * dt + sigma * sqrt(dt) * Z);
        prices.push_back(S_new);

        // ----- Compute indicators (if enough data) -----
        double shortMA   = computeMA(prices, t, shortWindow);
        double longMA    = computeMA(prices, t, longWindow);
        double volatility = computeVolatility(prices, t, volWindow);

        // ----- Generate alpha signals for each strategy ----- 
        // +1 means "enter" (or hold long), -1 means "exit"
        int alphaStraddle  = 0;
        int alphaStrangle  = 0;
        int alphaBull      = 0;
        int alphaBear      = 0;
        int alphaButterfly = 0;

        // Straddle: long if high volatility, exit if low
        if (volatility > volThresholdHigh)
            alphaStraddle = +1;
        else if (volatility < volThresholdLow)
            alphaStraddle = -1;

        // Strangle: similar but with its own thresholds
        if (volatility > volThresholdHighStrangle)
            alphaStrangle = +1;
        else if (volatility < volThresholdLowStrangle)
            alphaStrangle = -1;

        // Bull Spread (calls): if short MA > long MA, expect upward movement
        if (shortMA > longMA)
            alphaBull = +1;
        else
            alphaBull = -1;

        // Bear Spread (puts): if short MA < long MA, expect downward movement
        if (shortMA < longMA)
            alphaBear = +1;
        else
            alphaBear = -1;

        // Butterfly Spread (calls): profits from low volatility
        if (volatility < volThresholdLow)
            alphaButterfly = +1;
        else
            alphaButterfly = -1;

        // ----- Execute trades for each strategy -----

        // Strategy 1: Straddle
        if (!activeTrades[1].open && alphaStraddle == +1) {
            // Open a new straddle trade
            activeTrades[1].open = true;
            activeTrades[1].strategyType = 1;
            activeTrades[1].entryTick = t;
            activeTrades[1].entryPrice = S_new;
            // For a straddle, we use the entry price as the strike.
            activeTrades[1].strike1 = S_new;
            activeTrades[1].volume = 10;
        } else if (activeTrades[1].open) {
            // Close if holding period met or exit signal triggered
            if ((t - activeTrades[1].entryTick >= holdPeriod) || (alphaStraddle == -1)) {
                activeTrades[1].exitTick = t;
                activeTrades[1].exitPrice = S_new;
                double payoff = straddlePayoff(S_new, activeTrades[1].strike1);
                activeTrades[1].payoff = payoff * activeTrades[1].volume;
                cumulativePnL[1] += activeTrades[1].payoff;
                activeTrades[1].open = false;
            }
        }

        // Strategy 2: Strangle
        if (!activeTrades[2].open && alphaStrangle == +1) {
            activeTrades[2].open = true;
            activeTrades[2].strategyType = 2;
            activeTrades[2].entryTick = t;
            activeTrades[2].entryPrice = S_new;
            // For a strangle, use lower and higher strikes around the entry price.
            activeTrades[2].strike1 = S_new * (1 - delta);
            activeTrades[2].strike2 = S_new * (1 + delta);
            activeTrades[2].volume = 10;
        } else if (activeTrades[2].open) {
            if ((t - activeTrades[2].entryTick >= holdPeriod) || (alphaStrangle == -1)) {
                activeTrades[2].exitTick = t;
                activeTrades[2].exitPrice = S_new;
                double payoff = stranglePayoff(S_new, activeTrades[2].strike1, activeTrades[2].strike2);
                activeTrades[2].payoff = payoff * activeTrades[2].volume;
                cumulativePnL[2] += activeTrades[2].payoff;
                activeTrades[2].open = false;
            }
        }

        // Strategy 3: Bull Spread (calls)
        if (!activeTrades[3].open && alphaBull == +1) {
            activeTrades[3].open = true;
            activeTrades[3].strategyType = 3;
            activeTrades[3].entryTick = t;
            activeTrades[3].entryPrice = S_new;
            // For a bull spread, choose strikes below and above the entry price.
            activeTrades[3].strike1 = S_new * (1 - delta); // long call
            activeTrades[3].strike2 = S_new * (1 + delta); // short call
            activeTrades[3].volume = 10;
        } else if (activeTrades[3].open) {
            if ((t - activeTrades[3].entryTick >= holdPeriod) || (alphaBull == -1)) {
                activeTrades[3].exitTick = t;
                activeTrades[3].exitPrice = S_new;
                double payoff = bullSpreadPayoff(S_new, activeTrades[3].strike1, activeTrades[3].strike2);
                activeTrades[3].payoff = payoff * activeTrades[3].volume;
                cumulativePnL[3] += activeTrades[3].payoff;
                activeTrades[3].open = false;
            }
        }

        // Strategy 4: Bear Spread (puts)
        if (!activeTrades[4].open && alphaBear == +1) {
            activeTrades[4].open = true;
            activeTrades[4].strategyType = 4;
            activeTrades[4].entryTick = t;
            activeTrades[4].entryPrice = S_new;
            // For a bear spread, use a higher strike for the long put and a lower strike for the short put.
            activeTrades[4].strike1 = S_new * (1 + delta); // long put strike
            activeTrades[4].strike2 = S_new * (1 - delta); // short put strike
            activeTrades[4].volume = 10;
        } else if (activeTrades[4].open) {
            if ((t - activeTrades[4].entryTick >= holdPeriod) || (alphaBear == -1)) {
                activeTrades[4].exitTick = t;
                activeTrades[4].exitPrice = S_new;
                double payoff = bearSpreadPayoff(S_new, activeTrades[4].strike1, activeTrades[4].strike2);
                activeTrades[4].payoff = payoff * activeTrades[4].volume;
                cumulativePnL[4] += activeTrades[4].payoff;
                activeTrades[4].open = false;
            }
        }

        // Strategy 5: Butterfly Spread (calls)
        if (!activeTrades[5].open && alphaButterfly == +1) {
            activeTrades[5].open = true;
            activeTrades[5].strategyType = 5;
            activeTrades[5].entryTick = t;
            activeTrades[5].entryPrice = S_new;
            // For a butterfly spread, use three strikes:
            activeTrades[5].strike1 = S_new * (1 - delta);
            activeTrades[5].strike2 = S_new; 
            activeTrades[5].strike3 = S_new * (1 + delta);
            activeTrades[5].volume = 10;
        } else if (activeTrades[5].open) {
            if ((t - activeTrades[5].entryTick >= holdPeriod) || (alphaButterfly == -1)) {
                activeTrades[5].exitTick = t;
                activeTrades[5].exitPrice = S_new;
                double payoff = butterflySpreadPayoff(S_new, activeTrades[5].strike1, activeTrades[5].strike2, activeTrades[5].strike3);
                activeTrades[5].payoff = payoff * activeTrades[5].volume;
                cumulativePnL[5] += activeTrades[5].payoff;
                activeTrades[5].open = false;
            }
        }
    } // end simulation loop

    // ----- Final Reporting -----
    double totalPnL = 0;
    cout << "Cumulative PnL per Strategy:" << endl;
    for (int i = 1; i <= 5; i++) {
        cout << "  Strategy " << i << ": " << cumulativePnL[i] << endl;
        totalPnL += cumulativePnL[i];
    }
    cout << "Total PnL: " << totalPnL << endl;

    return 0;
}
