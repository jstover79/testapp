#pragma once

#include <cstddef>
#include <cstdint>
#include <random>
#include <vector>

namespace harris {

enum class InitialCondition { SingleSite, FullGrid, Random };

struct Snapshot {
  double time{};
  std::size_t infected{};
  std::size_t population{};
  std::uint64_t events{};
};

class ContactProcess {
 public:
  ContactProcess(int width, int height, double lambda,
                 std::uint64_t seed = std::random_device{}());

  void configure(int width, int height, double lambda);
  void reset(InitialCondition condition, double random_fraction = 0.25);
  bool step();
  std::size_t advance(double duration, std::size_t event_limit);

  [[nodiscard]] bool infected(int x, int y) const;
  [[nodiscard]] int width() const noexcept { return width_; }
  [[nodiscard]] int height() const noexcept { return height_; }
  [[nodiscard]] double lambda() const noexcept { return lambda_; }
  [[nodiscard]] Snapshot snapshot() const noexcept;

 private:
  void infect(std::size_t site);
  void recover(std::size_t site);
  void execute_event();
  [[nodiscard]] double draw_waiting_time();
  [[nodiscard]] std::size_t neighbor(std::size_t site, int direction) const;

  int width_{};
  int height_{};
  double lambda_{};
  double time_{};
  std::uint64_t events_{};
  double pending_wait_{-1.0};
  std::vector<std::uint8_t> state_;
  std::vector<std::size_t> infected_sites_;
  std::vector<std::size_t> infected_position_;
  std::mt19937_64 random_;
};

}  // namespace harris
