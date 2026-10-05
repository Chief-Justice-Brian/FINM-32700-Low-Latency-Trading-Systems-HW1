# HW 1 — Order-Book Metrics: Results

**Brian Nguyen** · FINM 32700 Low Latency Trading Systems · Due: October 8, 2026

## Build

- Ubuntu 26.04.1 LTS (WSL), g++ 15.2.0
- `g++ -std=c++20 -O2 -Wall -Wextra hw1.cpp -o hw1 && ./hw1`

## Output

```
HW 1 - Order-Book Metrics

SELF-CHECKS
  Example: mid    = 100.0100                 PASS
  Example: spread =   0.0200                 PASS
  Example: micro  = 100.0160                 PASS
  Example: obi    =   0.6000                 PASS
  Balanced:  obi = 0, micro = mid = 100.01   PASS
  Normal Book:  accepted                     PASS
  Locked Book:  accepted, spread = 0         PASS
  Crossed Book: rejected, metrics are NaN    PASS
  Empty Book:   rejected                     PASS
  Bid-Heavy: obi > 0 and micro > mid         PASS
  Ask-Heavy: obi < 0 and micro < mid         PASS
  0 check(s) failed

SNAPSHOTS
  Locked     mid=100.0100 spread=0.0000 micro=100.0100 obi=0.0000
  Crossed    mid=nan spread=nan micro=nan obi=nan
  Balanced   mid=100.0100 spread=0.0200 micro=100.0100 obi=0.0000
  Bid-Heavy  mid=100.0100 spread=0.0200 micro=100.0180 obi=0.8000
  Ask-Heavy  mid=100.0100 spread=0.0200 micro=100.0020 obi=-0.8000

EVOLUTION
  step      bid   size      ask   size       mid  spread     micro      obi    d_mid    d_obi
  t0     100.00    500   100.02    500  100.0100  0.0200  100.0100  +0.0000        -        -
  t1     100.00    800   100.02    500  100.0100  0.0200  100.0123  +0.2308  +0.0000  +0.2308
  t2     100.00    800   100.02    150  100.0100  0.0200  100.0168  +0.6842  +0.0000  +0.4534
  t3     100.00    800   100.03    400  100.0150  0.0300  100.0200  +0.3333  +0.0050  -0.3509
  t4     100.02    300   100.03    400  100.0250  0.0100  100.0243  -0.1429  +0.0100  -0.4762
  t5     100.02    300   100.03    900  100.0250  0.0100  100.0225  -0.5000  +0.0000  -0.3571
  t6     100.02    100   100.03    900  100.0250  0.0100  100.0210  -0.8000  +0.0000  -0.3000
  t7     100.01    600   100.03    900  100.0200  0.0200  100.0180  -0.2000  -0.0050  +0.6000
```

## How Mid and OBI evolve

In the sequence that I gave, buyers push the price up from t1 to t4. After that, sellers push it part
of the way back from t5 to t7.


- **T1-T2**: Buyers join the bid and one pays the ask. Only
  sizes change, so the mid stays at 100.01, but OBI climbs from 0 to +0.68
- **T3**: The ask at 100.02 is used up and the best ask becomes
  100.03. The mid rises by half a cent. OBI falls back to +0.33.
- **T4**: A buyer posts 100.02, lifting the mid another cent to 100.025. OBI turns negative (-0.14) because
  top-of-book OBI only sees the 300 at the new best bid and no longer counts the
  800 that had been bid at 100.00.
- **T5–T6**: Sellers join the ask and the bid shrinks.
- **T7**: The bid at 100.02 is used up and the best bid falls to 100.01. The mid drops half a cent, and OBI recovers to -0.20 because
  the new bid level has 600 resting.


Observation: OBI moves first in both directions before we see changes in the mid price.
