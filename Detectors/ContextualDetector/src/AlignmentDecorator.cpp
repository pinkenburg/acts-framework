// This file is part of the Acts project.
//
// Copyright (C) 2019 Acts project team
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#include "ACTFW/ContextualDetector/AlignmentDecorator.hpp"
#include "ACTFW/Random/RandomNumberDistributions.hpp"
#include "Acts/Detector/TrackingGeometry.hpp"

FW::Contextual::AlignmentDecorator::AlignmentDecorator(
    const FW::Contextual::AlignmentDecorator::Config& cfg,
    std::unique_ptr<const Acts::Logger>               logger)
  : m_cfg(cfg), m_logger(std::move(logger))
{
}

FW::ProcessCode
FW::Contextual::AlignmentDecorator::decorate(AlgorithmContext& context)
{
  // We need to lock the Decorator
  std::lock_guard<std::mutex> alignmentLock(m_alignmentMutex);

  // In which iov batch are we?
  unsigned int iov = context.eventNumber / m_cfg.iovSize;

  if (m_cfg.randomNumberSvc != nullptr) {

    // Detect if we have a new alignment range
    if (m_iovStatus.size() == 0 or m_iovStatus.size() < iov
        or !m_iovStatus[iov]) {

      auto cios = m_iovStatus.size();
      ACTS_VERBOSE("New IOV detected at event " << context.eventNumber
                                                << ", emulate new alignment.");
      ACTS_VERBOSE(
          "New IOV identifier set to " << iov << ", curently valid: " << cios);

      for (unsigned int ic = cios; ic <= iov; ++ic) {
        m_iovStatus.push_back(false);
      }

      // Create an algorithm local random number generator
      RandomEngine rng = m_cfg.randomNumberSvc->spawnGenerator(context);
      GaussDist    gauss(0., 1.);

      // Are we in a gargabe collection event?
      for (auto& lstore : m_cfg.detectorStore) {
        for (auto& ldet : lstore) {
          // get the nominal transform
          auto& tForm = ldet->nominalTransform(context.geoContext);
          // create a new transform
          auto atForm = std::make_unique<Acts::Transform3D>(tForm);
          if (iov != 0 or not m_cfg.firstIovNominal) {
            // the shifts in x, y, z
            double tx = m_cfg.gSigmaX != 0 ? m_cfg.gSigmaX * gauss(rng) : 0.;
            double ty = m_cfg.gSigmaY != 0 ? m_cfg.gSigmaY * gauss(rng) : 0.;
            double tz = m_cfg.gSigmaZ != 0 ? m_cfg.gSigmaZ * gauss(rng) : 0.;
            // Add a translation - if there is any
            if (tx != 0. or ty != 0. or tz != 0.) {
              const auto&    tMatrix   = atForm->matrix();
              auto           colX      = tMatrix.block<3, 1>(0, 0).transpose();
              auto           colY      = tMatrix.block<3, 1>(0, 1).transpose();
              auto           colZ      = tMatrix.block<3, 1>(0, 2).transpose();
              Acts::Vector3D newCenter = tMatrix.block<3, 1>(0, 3).transpose()
                  + tx * colX + ty * colY + tz * colZ;
              atForm->translation() = newCenter;
            }
            // now modify it - rotation around local X
            if (m_cfg.aSigmaX != 0.) {
              (*atForm) *= Acts::AngleAxis3D(m_cfg.aSigmaX * gauss(rng),
                                             Acts::Vector3D::UnitX());
            }
            if (m_cfg.aSigmaY != 0.) {
              (*atForm) *= Acts::AngleAxis3D(m_cfg.aSigmaY * gauss(rng),
                                             Acts::Vector3D::UnitY());
            }
            if (m_cfg.aSigmaZ != 0.) {
              (*atForm) *= Acts::AngleAxis3D(m_cfg.aSigmaZ * gauss(rng),
                                             Acts::Vector3D::UnitZ());
            }
          }
          // put it back into the store
          ldet->addAlignedTransform(std::move(atForm), iov);
        }
      }
    }
    // book keppting
    m_iovStatus[iov] = true;
  }
  // Set the geometry context
  AlignedDetectorElement::ContextType alignedContext{iov};
  context.geoContext
      = std::make_any<AlignedDetectorElement::ContextType>(alignedContext);

  return ProcessCode::SUCCESS;
}
