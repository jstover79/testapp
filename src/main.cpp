#include "contact_process.hpp"

#include "raylib.h"

#include <algorithm>
#include <array>
#include <charconv>
#include <cmath>
#include <cstdio>
#include <string>
#include <vector>

namespace {

constexpr Color kBackground{13, 20, 18, 255};
constexpr Color kPanel{22, 31, 27, 255};
constexpr Color kSurface{29, 41, 35, 255};
constexpr Color kBorder{54, 71, 62, 255};
constexpr Color kText{236, 241, 237, 255};
constexpr Color kMuted{151, 166, 157, 255};
constexpr Color kAccent{109, 224, 154, 255};
constexpr Color kInfected{105, 224, 150, 255};
constexpr Color kHealthy{17, 27, 23, 255};
constexpr int kPanelWidth = 350;

bool hovered(Rectangle area) { return CheckCollisionPointRec(GetMousePosition(), area); }

void label(const char* text, float x, float y, int size = 16, Color color = kMuted) {
  DrawText(text, static_cast<int>(x), static_cast<int>(y), size, color);
}

bool button(Rectangle area, const char* text, bool primary = false) {
  const bool over = hovered(area);
  DrawRectangleRounded(area, 0.18F, 8, primary ? (over ? Color{126, 236, 168, 255} : kAccent)
                                                   : (over ? Color{47, 63, 54, 255} : kSurface));
  if (!primary) DrawRectangleRoundedLinesEx(area, 0.18F, 8, 1.0F, kBorder);
  const int font_size = 17;
  const int width = MeasureText(text, font_size);
  DrawText(text, static_cast<int>(area.x + (area.width - width) / 2),
           static_cast<int>(area.y + (area.height - font_size) / 2), font_size,
           primary ? kBackground : kText);
  return over && IsMouseButtonPressed(MOUSE_LEFT_BUTTON);
}

struct TextField {
  std::string value;
  bool active{};

  bool draw(Rectangle area, bool decimal = false) {
    if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) active = hovered(area);
    bool changed = false;
    if (active) {
      int key = GetCharPressed();
      while (key > 0) {
        const bool accepted = key >= '0' && key <= '9';
        if ((accepted || (decimal && key == '.' && value.find('.') == std::string::npos)) &&
            value.size() < 9) {
          value.push_back(static_cast<char>(key));
          changed = true;
        }
        key = GetCharPressed();
      }
      if (IsKeyPressed(KEY_BACKSPACE) && !value.empty()) {
        value.pop_back();
        changed = true;
      }
      if (IsKeyPressed(KEY_ENTER)) active = false;
    }
    DrawRectangleRounded(area, 0.12F, 7, kSurface);
    DrawRectangleRoundedLinesEx(area, 0.12F, 7, active ? 2.0F : 1.0F, active ? kAccent : kBorder);
    DrawText(value.c_str(), static_cast<int>(area.x + 13), static_cast<int>(area.y + 12), 18, kText);
    if (active && (static_cast<int>(GetTime() * 2) % 2 == 0)) {
      const int cursor_x = static_cast<int>(area.x + 14) + MeasureText(value.c_str(), 18);
      DrawLine(cursor_x, static_cast<int>(area.y + 10), cursor_x, static_cast<int>(area.y + 31), kAccent);
    }
    return changed;
  }
};

double parse_double(const std::string& value, double fallback) {
  try {
    std::size_t read = 0;
    const double parsed = std::stod(value, &read);
    return read == value.size() && std::isfinite(parsed) ? parsed : fallback;
  } catch (...) {
    return fallback;
  }
}

int parse_int(const std::string& value, int fallback) {
  int result = fallback;
  const auto [end, error] = std::from_chars(value.data(), value.data() + value.size(), result);
  return error == std::errc{} && end == value.data() + value.size() ? result : fallback;
}

class GridTexture {
 public:
  ~GridTexture() {
    if (IsWindowReady()) unload();
  }

  void resize(int size) {
    unload();
    size_ = size;
    pixels_.resize(static_cast<std::size_t>(size) * size, kHealthy);
    Image image{pixels_.data(), size, size, 1, PIXELFORMAT_UNCOMPRESSED_R8G8B8A8};
    texture_ = LoadTextureFromImage(image);
    SetTextureFilter(texture_, TEXTURE_FILTER_POINT);
  }

  void update(const harris::ContactProcess& process) {
    if (size_ != process.width()) resize(process.width());
    for (int y = 0; y < size_; ++y) {
      for (int x = 0; x < size_; ++x) {
        pixels_[static_cast<std::size_t>(y) * size_ + x] = process.infected(x, y) ? kInfected : kHealthy;
      }
    }
    UpdateTexture(texture_, pixels_.data());
  }

  void draw(Rectangle destination) const {
    DrawTexturePro(texture_, Rectangle{0, 0, static_cast<float>(size_), static_cast<float>(size_)},
                   destination, Vector2{0, 0}, 0.0F, WHITE);
    DrawRectangleLinesEx(destination, 1.0F, kBorder);
  }

 private:
  void unload() {
    if (texture_.id != 0) UnloadTexture(texture_);
    texture_ = {};
  }
  int size_{};
  Texture2D texture_{};
  std::vector<Color> pixels_;
};

}  // namespace

int main() {
  SetConfigFlags(FLAG_WINDOW_RESIZABLE | FLAG_VSYNC_HINT | FLAG_MSAA_4X_HINT);
  InitWindow(1240, 800, "Harris Lab — 2D Contact Process");
  SetWindowMinSize(980, 680);
  SetTargetFPS(60);

  TextField lambda_field{"1.65"};
  TextField size_field{"128"};
  TextField density_field{"25"};
  std::array<const char*, 3> initial_names{"One infected site", "Full grid", "Random density"};
  std::array<harris::InitialCondition, 3> initial_values{harris::InitialCondition::SingleSite,
                                                        harris::InitialCondition::FullGrid,
                                                        harris::InitialCondition::Random};
  int initial_index = 0;
  bool dropdown_open = false;
  bool running = false;
  bool configuration_dirty = true;
  double speed = 1.0;
  std::string notice = "Ready — configure the lattice and press Start.";
  harris::ContactProcess process(128, 128, 1.65);
  GridTexture grid;
  grid.update(process);
  double last_texture_update = 0.0;

  auto reset = [&] {
    const double lambda = parse_double(lambda_field.value, -1.0);
    const int size = parse_int(size_field.value, -1);
    const double density = parse_double(density_field.value, -1.0);
    if (lambda < 0.0 || lambda > 100.0) {
      notice = "Lambda must be between 0 and 100.";
      return false;
    }
    if (size < 8 || size > 2048) {
      notice = "Grid size must be between 8 and 2048.";
      return false;
    }
    if (initial_index == 2 && (density < 0.0 || density > 100.0)) {
      notice = "Random density must be from 0% to 100%.";
      return false;
    }
    process.configure(size, size, lambda);
    process.reset(initial_values[initial_index], density / 100.0);
    grid.update(process);
    notice = "New realization initialized.";
    configuration_dirty = false;
    return true;
  };

  while (!WindowShouldClose()) {
    const float dt = GetFrameTime();
    std::size_t frame_events = 0;
    if (running) {
      frame_events = process.advance(dt * speed, 250000);
      if (process.snapshot().infected == 0) {
        running = false;
        notice = "The process reached the absorbing state (extinction).";
      } else if (frame_events == 250000) {
        notice = "Display event cap reached — reduce speed for real-time pacing.";
      }
      if (GetTime() - last_texture_update > 1.0 / 30.0) {
        grid.update(process);
        last_texture_update = GetTime();
      }
    }

    BeginDrawing();
    ClearBackground(kBackground);
    const int window_width = GetScreenWidth();
    const int window_height = GetScreenHeight();
    DrawRectangle(0, 0, kPanelWidth, window_height, kPanel);
    DrawLine(kPanelWidth, 0, kPanelWidth, window_height, kBorder);

    DrawCircle(34, 34, 13, kAccent);
    DrawCircle(34, 34, 5, kPanel);
    DrawText("HARRIS LAB", 57, 23, 22, kText);
    label("2D CONTACT PROCESS", 57, 48, 10, kMuted);
    DrawLine(24, 76, kPanelWidth - 24, 76, kBorder);

    label("INFECTION RATE", 24, 99, 11, kMuted);
    DrawText("Lambda (per neighbor)", 24, 119, 16, kText);
    configuration_dirty |= lambda_field.draw(Rectangle{24, 145, 302, 44}, true);
    label("Recovery rate is normalized to 1.", 24, 195, 12, kMuted);

    label("LATTICE", 24, 231, 11, kMuted);
    DrawText("Square grid size", 24, 251, 16, kText);
    configuration_dirty |= size_field.draw(Rectangle{24, 277, 302, 44});
    label("Recommended 32–256. Above 1000 may be slow.", 24, 327, 11, kMuted);

    label("INITIAL CONDITION", 24, 360, 11, kMuted);
    const Rectangle dropdown{24, 383, 302, 44};
    DrawRectangleRounded(dropdown, 0.12F, 7, kSurface);
    DrawRectangleRoundedLinesEx(dropdown, 0.12F, 7, 1, dropdown_open ? kAccent : kBorder);
    DrawText(initial_names[initial_index], 37, 396, 16, kText);
    DrawText(dropdown_open ? "^" : "v", 301, 396, 16, kMuted);
    if (hovered(dropdown) && IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) dropdown_open = !dropdown_open;

    if (initial_index == 2) {
      DrawText("Initially infected (%)", 24, 442, 16, kText);
      configuration_dirty |= density_field.draw(Rectangle{211, 432, 115, 42}, true);
    }

    const float speed_y = initial_index == 2 ? 500.0F : 450.0F;
    label("OBSERVATION SPEED", 24, speed_y, 11, kMuted);
    char speed_text[32];
    std::snprintf(speed_text, sizeof(speed_text), "%.1fx", speed);
    DrawText(speed_text, 276, static_cast<int>(speed_y - 2), 16, kAccent);
    const Rectangle slider{24, speed_y + 30, 302, 24};
    DrawRectangleRounded(Rectangle{slider.x, slider.y + 9, slider.width, 6}, 1, 4, kBorder);
    const float slider_t = static_cast<float>((std::log10(speed) + 1.0) / 3.0);
    DrawRectangleRounded(Rectangle{slider.x, slider.y + 9, slider.width * slider_t, 6}, 1, 4, kAccent);
    DrawCircle(static_cast<int>(slider.x + slider.width * slider_t), static_cast<int>(slider.y + 12), 9, kText);
    if (hovered(slider) && IsMouseButtonDown(MOUSE_LEFT_BUTTON)) {
      const double position = std::clamp((GetMouseX() - slider.x) / slider.width, 0.0F, 1.0F);
      speed = std::pow(10.0, -1.0 + 3.0 * position);
    }
    label("0.1x", 24, speed_y + 59, 10, kMuted);
    label("100x", 296, speed_y + 59, 10, kMuted);

    const float action_y = speed_y + 90;
    if (button(Rectangle{24, action_y, 194, 48}, running ? "Pause" : "Start", true)) {
      if (running) {
        running = false;
        notice = "Paused.";
      } else {
        if (configuration_dirty || process.snapshot().infected == 0) reset();
        running = process.snapshot().infected > 0;
        if (running) notice = "Simulation running.";
      }
    }
    if (button(Rectangle{228, action_y, 98, 48}, "Reset")) {
      running = false;
      reset();
    }

    const auto snapshot = process.snapshot();
    char status[128];
    std::snprintf(status, sizeof(status), "t = %.3f    infected = %zu / %zu", snapshot.time,
                  snapshot.infected, snapshot.population);
    label(status, 24, action_y + 65, 12, kMuted);

    const float available_width = static_cast<float>(window_width - kPanelWidth - 70);
    const float available_height = static_cast<float>(window_height - 145);
    const float canvas_size = std::max(100.0F, std::min(available_width, available_height));
    const Rectangle canvas{kPanelWidth + 35 + (available_width - canvas_size) / 2,
                           94 + (available_height - canvas_size) / 2, canvas_size, canvas_size};
    DrawText("LATTICE VIEW", kPanelWidth + 35, 28, 12, kMuted);
    DrawText(running ? "LIVE" : "PAUSED", window_width - 94, 26, 12, running ? kAccent : kMuted);
    grid.draw(canvas);
    DrawText(notice.c_str(), kPanelWidth + 35, window_height - 38, 13, kMuted);

    if (dropdown_open) {
      for (int i = 0; i < 3; ++i) {
        const Rectangle option{24, 428.0F + i * 42.0F, 302, 42};
        DrawRectangleRec(option, hovered(option) ? Color{47, 63, 54, 255} : kSurface);
        DrawText(initial_names[i], 37, static_cast<int>(option.y + 12), 15, i == initial_index ? kAccent : kText);
        if (hovered(option) && IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
          initial_index = i;
          configuration_dirty = true;
          dropdown_open = false;
        }
      }
      DrawRectangleLinesEx(Rectangle{24, 428, 302, 126}, 1, kBorder);
    }

    DrawText("Periodic boundary conditions", window_width - 205, window_height - 36, 11, kMuted);
    EndDrawing();
  }

  CloseWindow();
  return 0;
}
