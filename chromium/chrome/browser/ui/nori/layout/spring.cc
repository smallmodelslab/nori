// Copyright 2026 The Nori Authors
// You can use, redistribute, and/or modify this source code under
// the terms of the GPL-3.0 license that can be found in the LICENSE file.

#include "chrome/browser/ui/nori/layout/spring.h"

#include <cmath>

namespace nori {

namespace {
constexpr double kVelocityStep = 0.001;
}  // namespace

Spring::Spring(double from,
               double to,
               double initial_velocity,
               SpringParams params)
    : to_(to),
      x0_(from - to),
      v0_(initial_velocity),
      beta_(params.damping_ratio * std::sqrt(params.stiffness)),
      omega0_(std::sqrt(params.stiffness)),
      params_(params) {}

double Spring::OffsetAt(double t) const {
  const double envelope = std::exp(-beta_ * t);
  if (std::abs(beta_ - omega0_) < 1e-9) {
    return envelope * (x0_ + (beta_ * x0_ + v0_) * t);
  }
  if (beta_ < omega0_) {
    const double omega1 = std::sqrt(omega0_ * omega0_ - beta_ * beta_);
    return envelope * (x0_ * std::cos(omega1 * t) +
                       (beta_ * x0_ + v0_) / omega1 * std::sin(omega1 * t));
  }
  const double omega2 = std::sqrt(beta_ * beta_ - omega0_ * omega0_);
  return envelope * (x0_ * std::cosh(omega2 * t) +
                     (beta_ * x0_ + v0_) / omega2 * std::sinh(omega2 * t));
}

double Spring::ValueAt(base::TimeDelta elapsed) const {
  return to_ + OffsetAt(elapsed.InSecondsF());
}

double Spring::VelocityAt(base::TimeDelta elapsed) const {
  const double t = elapsed.InSecondsF();
  return (OffsetAt(t + kVelocityStep) - OffsetAt(t)) / kVelocityStep;
}

bool Spring::IsDoneAt(base::TimeDelta elapsed) const {
  return std::abs(OffsetAt(elapsed.InSecondsF())) < params_.epsilon &&
         std::abs(VelocityAt(elapsed)) < params_.epsilon * 60;
}

void Motion::AnimateTo(double target, base::TimeTicks now, SpringParams params) {
  if (target == target_) {
    return;
  }
  const double velocity =
      IsAnimatingAt(now) ? spring_->VelocityAt(now - start_) : 0;
  spring_.emplace(ValueAt(now), target, velocity, params);
  start_ = now;
  target_ = target;
}

void Motion::JumpTo(double value) {
  spring_.reset();
  target_ = value;
}

void Motion::Shift(double delta) {
  target_ += delta;
  if (spring_) {
    spring_->Shift(delta);
  }
}

double Motion::ValueAt(base::TimeTicks now) const {
  return IsAnimatingAt(now) ? spring_->ValueAt(now - start_) : target_;
}

bool Motion::IsAnimatingAt(base::TimeTicks now) const {
  return spring_ && !spring_->IsDoneAt(now - start_);
}

}  // namespace nori
