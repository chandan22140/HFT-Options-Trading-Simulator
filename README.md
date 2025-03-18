# HFT Options Trading Simulator

This project is a high-frequency trading (HFT) simulator for options trading strategies implemented in C++. It simulates the trading of five popular options strategies using a geometric Brownian motion (GBM) model for underlying asset price generation, dynamic alpha signal generation, and real-time payoff calculation.

## Strategies Implemented

- **Straddle:**  
  Long call and put at the same strike price. Profits when the underlying asset makes large moves in either direction.

- **Strangle:**  
  Long out-of-the-money call and put with different strikes. Designed to benefit from significant price movements.

- **Bull Spread (using calls):**  
  Long a call at a lower strike and short a call at a higher strike. Profits from moderate upward moves.

- **Bear Spread (using puts):**  
  Long a put at a higher strike and short a put at a lower strike. Profits from moderate downward moves.

- **Butterfly Spread (using calls):**  
  Combines long and short calls at three strikes to profit from low volatility when the price remains near a target level.

## Key Features

- **Underlying Price Simulation:**  
  The underlying asset price is generated using a geometric Brownian motion (GBM) model, a standard approach for simulating stock prices.

- **Alpha Signal Generation:**  
  The simulator computes simple indicators such as short-term and long-term moving averages and a volatility estimate to generate alpha signals (+1 to buy/enter, -1 to sell/exit) for each strategy based on market conditions.

- **Trade Execution:**  
  At each time step, trades are executed based on the alpha signals. Each trade is held for a fixed number of ticks or closed early if an exit signal is generated.

- **Profit & Loss (PnL) Tracking:**  
  The simulator calculates the payoff for each closed trade and tracks the cumulative PnL for each strategy over the simulation horizon.

## Getting Started

### Prerequisites

- A C++ compiler that supports C++11 (or later).
- (Optional) CMake for building the project.
- Standard C++ libraries: `<iostream>`, `<vector>`, `<cmath>`, `<random>`, `<chrono>`, `<algorithm>`.

### Building the Project

#### Using g++ Directly

```bash
g++ -std=c++11 -O2 -o hft_simulator main.cpp
```

#### Using CMake

1. Create a `CMakeLists.txt` file (an example is provided in the repository).
2. Run the following commands:

   ```bash
   mkdir build && cd build
   cmake ..
   make
   ```

### Running the Simulator

After building the project, run the executable:

```bash
./hft_simulator
```

The simulation will execute a pre-defined number of ticks (simulating an HFT environment), and at the end, it will display the cumulative profit/loss for each strategy.

## Configuration

You can modify simulation parameters such as:
- Total simulation ticks (e.g., 10,000 ticks)
- Initial underlying price, drift, and volatility for the GBM model
- Indicator window sizes and thresholds for alpha signal generation
- Holding period for trades
- Trading volume (number of contracts)

These parameters are defined in the source code, allowing you to experiment with different market conditions and trading behaviors.

## Contributing

Contributions, improvements, and suggestions are welcome. Please open an issue or submit a pull request if you have ideas to enhance the simulator.

## License

This project is licensed under the MIT License. See the [LICENSE](LICENSE) file for details.

## Disclaimer

This simulator is intended for educational and research purposes only. It is not designed for real trading, and trading options involves significant risk.

