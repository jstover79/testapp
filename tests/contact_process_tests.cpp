#include "contact_process.hpp"

#include <cmath>
#include <cstdlib>
#include <iostream>
#include <stdexcept>

namespace {

void require(bool condition, const char* message) {
  if (!condition) {
    std::cerr << "FAIL: " << message << '\n';
    std::exit(EXIT_FAILURE);
  }
}

}  // namespace

int main() {
  using harris::ContactProcess;
  using harris::InitialCondition;

  ContactProcess process(32, 24, 1.65, 42);
  process.reset(InitialCondition::SingleSite);
  require(process.snapshot().infected == 1, "single-site initial condition");
  require(process.infected(16, 12), "single site is centered");

  process.reset(InitialCondition::FullGrid);
  require(process.snapshot().infected == 32U * 24U, "full-grid initial condition");
  require(process.step(), "an occupied process can advance");
  require(process.snapshot().events == 1, "events are counted");
  require(process.snapshot().time > 0.0, "continuous time advances");

  process.configure(32, 32, 0.0);
  process.reset(InitialCondition::FullGrid);
  const auto short_events = process.advance(0.000001, 100);
  require(short_events == 0, "advance does not apply a future event early");
  require(std::abs(process.snapshot().time - 0.000001) < 1e-12,
          "advance follows the requested observation time");

  process.configure(8, 8, 0.0);
  process.reset(InitialCondition::FullGrid);
  for (int i = 0; i < 64; ++i) require(process.step(), "recovery event exists");
  require(process.snapshot().infected == 0, "lambda zero process becomes extinct");
  require(!process.step(), "extinct process cannot advance");

  process.configure(100, 100, 1.0);
  process.reset(InitialCondition::Random, 0.3);
  const auto random_count = process.snapshot().infected;
  require(random_count > 2500 && random_count < 3500, "random density is plausible");

  bool rejected = false;
  try {
    process.configure(1, 20, 1.0);
  } catch (const std::invalid_argument&) {
    rejected = true;
  }
  require(rejected, "invalid dimensions are rejected");

  std::cout << "All Harris contact process tests passed.\n";
  return EXIT_SUCCESS;
}
