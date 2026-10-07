// Copyright 2026 The Nori Authors
// You can use, redistribute, and/or modify this source code under
// the terms of the GPL-3.0 license that can be found in the LICENSE file.

#ifndef CHROME_BROWSER_UI_NORI_LAYOUT_SPRING_H_
#define CHROME_BROWSER_UI_NORI_LAYOUT_SPRING_H_

#include "base/time/time.h"

namespace nori {

struct SpringParams {
  double stiffness = 800;
  double damping_ratio = 1;
  // Pixels from the target below which the motion is finished.
  double epsilon = 0.25;
};

// A damped spring with unit mass, solved in closed form, as in niri.
class Spring {
 public:
  Spring(double from, double to, double initial_velocity, SpringParams params);

  double ValueAt(base::TimeDelta elapsed) const;
  // Pixels per second.
  double VelocityAt(base::TimeDelta elapsed) const;
  bool IsDoneAt(base::TimeDelta elapsed) const;

  double to() const { return to_; }

 private:
  // Displacement from the target.
  double OffsetAt(double seconds) const;

  double to_;
  double x0_;
  double v0_;
  double beta_;
  double omega0_;
  SpringParams params_;
};

}  // namespace nori

#endif  // CHROME_BROWSER_UI_NORI_LAYOUT_SPRING_H_
