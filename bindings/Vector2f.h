#pragma once

namespace DAT {

class Vector2f_t {
  public:
    Vector2f_t() = default;
    Vector2f_t(float x, float y)
        : x_(x), y_(y) {}

    float x() const { return x_; }
    float y() const { return y_; }

  private:
    float x_{0.0F};
    float y_{0.0F};
};

}  // namespace DAT
