// Brian Nguyen - FINM32700 Low Latency Trading Systems
// hw1.cpp - HW 1 "Order-book metrics"
//
// Ubuntu WSL:
//     g++ -std=c++20 -O2 -Wall -Wextra hw1.cpp -o hw1 && ./hw1
// Windows PowerShell:
//     g++ -std=c++20 -O2 -Wall -Wextra hw1.cpp -o hw1.exe; ./hw1.exe

#include <cmath>
#include <cstdio>
#include <limits>
#include <vector>

/// struct representing one top-of-book snapshot: the best (highest) bid and the best (lowest) ask,
/// each with the size resting there.
struct TopOfBookSnapshot {
    double bid_px, bid_sz;
    double ask_px, ask_sz;
};

struct Metrics {
    double mid, spread, micro, obi;
};

// Guard: a crossed book (ask < bid) or an empty top of book is bad data.
// A locked book (ask == bid) is allowed: its spread is 0.
bool is_valid(const TopOfBookSnapshot& s) {
    // return !(s.ask_px < s.bid_px || s.bid_sz + s.ask_sz <= 0.0);
    return s.ask_px >= s.bid_px && s.bid_sz + s.ask_sz > 0.0;
}

Metrics compute(const TopOfBookSnapshot& s) {

    if (!is_valid(s)) {
        const double nan = std::numeric_limits<double>::quiet_NaN();

        return {nan, nan, nan, nan};
    }
    double mid = (s.bid_px + s.ask_px) / 2.0;

    double spread = s.ask_px - s.bid_px;

    double micro = (s.ask_px * s.bid_sz + s.bid_px * s.ask_sz) / (s.bid_sz + s.ask_sz);

    double obi = (s.bid_sz - s.ask_sz) / (s.bid_sz + s.ask_sz);

    return Metrics{.mid = mid, .spread = spread, .micro = micro, .obi = obi};
}

void show(const char* label, const TopOfBookSnapshot& s) {
    Metrics m = compute(s);
    std::printf("  %-10s mid=%.4f spread=%.4f micro=%.4f obi=%.4f\n", label, m.mid, m.spread,
                m.micro, m.obi);
}

// Buyers push the price up from t1 to t4
// Sellers push it part of the way back from t5 to t7
const std::vector<TopOfBookSnapshot> sequence = {
    // bid_px  bid_sz  ask_px  ask_sz
    {100.00, 500, 100.02, 500}, // t0 (Balanced Book)
    {100.00, 800, 100.02, 500}, // t1 (Increase in Buyers)
    {100.00, 800, 100.02, 150}, // t2 (A buyer is willing to pay @ 100.02)
    {100.00, 800, 100.03, 400}, // t3 (Ask Level is used up)
    {100.02, 300, 100.03, 400}, // t4 (A buyer posts a better price)
    {100.02, 300, 100.03, 900}, // t5 (More Sellers Joining)
    {100.02, 100, 100.03, 900}, // t6 (Buyers leave/cancelled, trades filled)
    {100.01, 600, 100.03, 900}, // t7 (Bid Level is used up)
};

void print_evolution(const std::vector<TopOfBookSnapshot>& seq) {
    std::printf("  %-4s %8s %6s %8s %6s %9s %7s %9s %8s %8s %8s\n", "step", "bid", "size", "ask",
                "size", "mid", "spread", "micro", "obi", "d_mid", "d_obi");

    Metrics prev{};
    for (std::size_t i = 0; i < seq.size(); i++) {
        const TopOfBookSnapshot& s = seq[i];
        const Metrics m = compute(s);

        std::printf("  t%-3zu %8.2f %6.0f %8.2f %6.0f %9.4f %7.4f %9.4f %+8.4f", i, s.bid_px,
                    s.bid_sz, s.ask_px, s.ask_sz, m.mid, m.spread, m.micro, m.obi);

        // change since the previous step; the first row has nothing to compare with
        if (i == 0) {
            std::printf(" %8s %8s\n", "-", "-");
        } else {
            std::printf(" %+8.4f %+8.4f\n", m.mid - prev.mid, m.obi - prev.obi);
        }
        prev = m;
    }
}

static int failed = 0;

static void check(const char* scenario, bool assertion) {
    std::printf("  %-42s %s\n", scenario, assertion ? "PASS" : "FAIL");
    if (!assertion) {
        failed++;
    }
}

static bool approx(double a, double b) {
    return std::fabs(a - b) < 1e-9;
}

static void run_checks() {
    // Example: 800 lots bid, 200 offered
    const Metrics example = compute({100.00, 800, 100.02, 200});
    check("Example: mid    = 100.0100", approx(example.mid, 100.01));
    check("Example: spread =   0.0200", approx(example.spread, 0.02));
    check("Example: micro  = 100.0160", approx(example.micro, 100.016));
    check("Example: obi    =   0.6000", approx(example.obi, 0.6));

    // 1. Balanced Case: OBI = 0 and Micro == Mid
    const Metrics mBal = compute({100.00, 500, 100.02, 500});
    check("Balanced:  obi = 0, micro = mid = 100.01",
          approx(mBal.obi, 0.0) && approx(mBal.micro, 100.01) && approx(mBal.mid, 100.01));

    // 2. Guard: Locked gives spread 0, crossed gives nan
    const TopOfBookSnapshot normal = {100.00, 500, 100.02, 500};
    const TopOfBookSnapshot locked{100.01, 500, 100.01, 500};
    const TopOfBookSnapshot crossed{100.02, 500, 100.00, 500};

    check("Normal Book:  accepted", is_valid(normal));
    check("Locked Book:  accepted, spread = 0",
          is_valid(locked) && approx(compute(locked).spread, 0.0));
    check("Crossed Book: rejected, metrics are NaN",
          !is_valid(crossed) && std::isnan(compute(crossed).mid));
    check("Empty Book:   rejected", !is_valid({100.00, 0, 100.02, 0}));

    // 3. Microprice should lean the way obi says
    const Metrics bid = compute({100.00, 900, 100.02, 100});
    check("Bid-Heavy: obi > 0 and micro > mid", bid.obi > 0.0 && bid.micro > bid.mid);

    const Metrics ask = compute({100.00, 100, 100.02, 900});
    check("Ask-Heavy: obi < 0 and micro < mid", ask.obi < 0.0 && ask.micro < ask.mid);
}

// ===========================================================================
//  main
// ===========================================================================

int main() {
    std::printf("HW 1 - Order-Book Metrics\n");

    std::printf("\nSELF-CHECKS\n");
    run_checks();
    std::printf("  %d check(s) failed\n", failed);

    std::printf("\nSNAPSHOTS\n");
    show("Locked", {100.01, 500, 100.01, 500});
    show("Crossed", {100.02, 500, 100.00, 500});
    show("Balanced", {100.00, 500, 100.02, 500});
    show("Bid-Heavy", {100.00, 900, 100.02, 100});
    show("Ask-Heavy", {100.00, 100, 100.02, 900});

    std::printf("\nEVOLUTION\n");
    print_evolution(sequence);

    return 0;
}
