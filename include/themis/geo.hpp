#pragma once
#include <algorithm>
#include <cmath>
#include <numbers>

namespace themis {

// Great-circle distance in km between two points given in degrees.
inline double haversine_km(double lat1, double lon1, double lat2, double lon2) {
  const double to_rad = std::numbers::pi / 180.0;
  const double phi1 = lat1 * to_rad;
  const double phi2 = lat2 * to_rad;
  const double dphi = (lat2 - lat1) * to_rad;
  const double dlambda = (lon2 - lon1) * to_rad;

  const double a = std::sin(dphi / 2) * std::sin(dphi / 2) +
                   std::cos(phi1) * std::cos(phi2) * std::sin(dlambda / 2) *
                       std::sin(dlambda / 2);
  return 2 * 6371.0 * std::asin(std::min(1.0, std::sqrt(a)));
}
} // namespace themis
