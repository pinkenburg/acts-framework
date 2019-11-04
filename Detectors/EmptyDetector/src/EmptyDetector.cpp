// This file is part of the Acts project.
//
// Copyright (C) 2019 CERN for the benefit of the Acts project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#include "ACTFW/EmptyDetector/EmptyDetector.hpp"
#include "ACTFW/EmptyDetector/EmptyDetectorOptions.hpp"

#include <boost/program_options.hpp>
#include "ACTFW/Framework/IContextDecorator.hpp"
#include "Acts/Geometry/CylinderVolumeBounds.hpp"
#include "Acts/Geometry/TrackingGeometry.hpp"
#include "Acts/Geometry/TrackingVolume.hpp"
#include "Acts/Utilities/Units.hpp"

void
EmptyDetector::addOptions(
    boost::program_options::options_description& opt) const
{
  FW::Options::addEmptyGeometryOptions(opt);
}

auto
EmptyDetector::finalize(
    const boost::program_options::variables_map& vm,
    std::shared_ptr<const Acts::IMaterialDecorator> /*unused*/)
    -> std::pair<TrackingGeometryPtr, ContextDecorators>
{

  using namespace Acts::UnitLiterals;

  // Create an empty cylinder with the chosen radius / halflength
  double r  = vm["geo-empty-radius"].as<double>() * 1_m;
  double hz = vm["geo-empty-halfLength"].as<double>() * 1_m;

  // The cylinder volume bounds
  auto cvBounds = std::make_shared<Acts::CylinderVolumeBounds>(r, hz);

  // Create the world volume
  auto worldVolume = Acts::TrackingVolume::create(
      nullptr, cvBounds, nullptr, "EmptyCylinder");

  // Create the tracking geometry
  auto tgGeometry = std::make_shared<Acts::TrackingGeometry>(
      std::move(worldVolume), nullptr);

  /// Empty decorators
  ContextDecorators eContextDeocrators = {};

  // And return the pair with empty decorators
  return std::make_pair<TrackingGeometryPtr, ContextDecorators>(
      std::move(tgGeometry), std::move(eContextDeocrators));
}