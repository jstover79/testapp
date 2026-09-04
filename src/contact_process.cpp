#include "contact_process.hpp"

#include <algorithm>
#include <cmath>
#include <limits>
#include <stdexcept>

namespace harris {

ContactProcess::ContactProcess(int width, int height, double lambda,
                               std::uint64_t seed)
    : random_(seed) {
  configure(width, height, lambda);
  reset(InitialCondition::SingleSite);
}

void ContactProcess::configure(int width, int height, double lambda) {
  if (width < 2 || height < 2) {
    throw std::invalid_argument("grid dimensions must be at least 2");
  }
  if (!std::isfinite(lambda) || lambda < 0.0) {
    throw std::invalid_argument("lambda must be finite and nonnegative");
  }

  width_ = width;
  height_ = height;
  lambda_ = lambda;
  const auto population = static_cast<std::size_t>(width_) * height_;
  state_.assign(population, 0);
  infected_position_.assign(population, 0);
  infected_sites_.clear();
  time_ = 0.0;
  events_ = 0;
  pending_wait_ = -1.0;
}

void ContactProcess::reset(InitialCondition condition, double random_fraction) {
  if (random_fraction < 0.0 || random_fraction > 1.0) {
    throw std::invalid_argument("random fraction must be between zero and one");
  }

  std::fill(state_.begin(), state_.end(), 0);
  infected_sites_.clear();
  time_ = 0.0;
  events_ = 0;
  pending_wait_ = -1.0;

  if (condition == InitialCondition::SingleSite) {
    infect(static_cast<std::size_t>(height_ / 2) * width_ + width_ / 2);
  } else if (condition == InitialCondition::FullGrid) {
    for (std::size_t site = 0; site < state_.size(); ++site) infect(site);
  } else {
    std::bernoulli_distribution initially_infected(random_fraction);
    for (std::size_t site = 0; site < state_.size(); ++site) {
      if (initially_infected(random_)) infect(site);
    }
  }
}

void ContactProcess::infect(std::size_t site) {
  if (state_[site]) return;
  state_[site] = 1;
  infected_position_[site] = infected_sites_.size();
  infected_sites_.push_back(site);
}

void ContactProcess::recover(std::size_t site) {
  if (!state_[site]) return;
  const auto position = infected_position_[site];
  const auto moved_site = infected_sites_.back();
  infected_sites_[position] = moved_site;
  infected_position_[moved_site] = position;
  infected_sites_.pop_back();
  state_[site] = 0;
}

std::size_t ContactProcess::neighbor(std::size_t site, int direction) const {
  const int x = static_cast<int>(site % width_);
  const int y = static_cast<int>(site / width_);
  switch (direction) {
    case 0: return static_cast<std::size_t>(y) * width_ + (x + 1) % width_;
    case 1: return static_cast<std::size_t>(y) * width_ + (x + width_ - 1) % width_;
    case 2: return static_cast<std::size_t>((y + 1) % height_) * width_ + x;
    default: return static_cast<std::size_t>((y + height_ - 1) % height_) * width_ + x;
  }
}

bool ContactProcess::step() {
  if (infected_sites_.empty()) return false;

  if (pending_wait_ < 0.0) pending_wait_ = draw_waiting_time();
  time_ += pending_wait_;
  pending_wait_ = -1.0;
  execute_event();
  return true;
}

double ContactProcess::draw_waiting_time() {
  const double total_rate = infected_sites_.size() * (1.0 + 4.0 * lambda_);
  std::exponential_distribution<double> waiting_time(total_rate);
  return waiting_time(random_);
}

void ContactProcess::execute_event() {
  const double rate_per_site = 1.0 + 4.0 * lambda_;

  // Harris graphical construction: every occupied site recovers at rate 1 and
  // sends an infection arrow to each of its four neighbors at rate lambda.
  std::uniform_int_distribution<std::size_t> choose_site(0, infected_sites_.size() - 1);
  const auto source = infected_sites_[choose_site(random_)];
  std::uniform_real_distribution<double> choose_event(0.0, rate_per_site);
  const double event = choose_event(random_);

  if (event < 1.0 || lambda_ == 0.0) {
    recover(source);
  } else {
    const int direction = std::min(3, static_cast<int>((event - 1.0) / lambda_));
    infect(neighbor(source, direction));
  }
  ++events_;
}

std::size_t ContactProcess::advance(double duration, std::size_t event_limit) {
  if (duration <= 0.0 || event_limit == 0 || infected_sites_.empty()) return 0;
  double remaining = duration;
  std::size_t count = 0;
  while (remaining > 0.0 && count < event_limit && !infected_sites_.empty()) {
    if (pending_wait_ < 0.0) pending_wait_ = draw_waiting_time();
    if (pending_wait_ > remaining) {
      pending_wait_ -= remaining;
      time_ += remaining;
      remaining = 0.0;
    } else {
      remaining -= pending_wait_;
      time_ += pending_wait_;
      pending_wait_ = -1.0;
      execute_event();
      ++count;
    }
  }
  return count;
}

bool ContactProcess::infected(int x, int y) const {
  if (x < 0 || x >= width_ || y < 0 || y >= height_) return false;
  return state_[static_cast<std::size_t>(y) * width_ + x] != 0;
}

Snapshot ContactProcess::snapshot() const noexcept {
  return {time_, infected_sites_.size(), state_.size(), events_};
}

}  // namespace harris
