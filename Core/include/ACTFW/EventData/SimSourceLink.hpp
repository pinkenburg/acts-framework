// This file is part of the Acts project.
//
// Copyright (C) 2016-2019 CERN for the benefit of the Acts project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#pragma once

#include <stdexcept>
#include <string>

#include <Acts/EventData/Measurement.hpp>
#include <Acts/EventData/MeasurementHelpers.hpp>
#include <Acts/EventData/SourceLinkConcept.hpp>

#include "ACTFW/EventData/SimHit.hpp"
#include "ACTFW/EventData/SimParticle.hpp"

namespace FW {
namespace Data {

  /// Source link class for simulation in the acts-framework
  class SimSourceLink
  {
  public:
    // must be default_constructible to satisfy SourceLinkConcept
    SimSourceLink() = default;
    SimSourceLink(const SimHit*     truthHit,
                  size_t            dim,
                  Acts::BoundVector values,
                  Acts::BoundMatrix cov)
      : m_truthHit(truthHit), m_dim(dim), m_values(values), m_cov(cov)
    {
    }
    SimSourceLink(SimSourceLink&&)      = default;
    SimSourceLink(const SimSourceLink&) = default;
    SimSourceLink&
    operator=(SimSourceLink&&)
        = default;
    SimSourceLink&
    operator=(const SimSourceLink&)
        = default;

    bool
    operator==(const SimSourceLink& rhs) const
    {
      return m_truthHit == rhs.m_truthHit;
    }

    const SimHit&
    truthHit() const
    {
      return *m_truthHit;
    }

    const Acts::Surface&
    referenceSurface() const
    {
      return *m_truthHit->surface;
    }

    Acts::FittableMeasurement<SimSourceLink> operator*() const
    {
      if (m_dim == 0) {
        throw std::runtime_error("Cannot create dim 0 measurement");
      } else if (m_dim == 1) {
        return Acts::Measurement<SimSourceLink, Acts::ParDef::eLOC_0>{
            m_truthHit->surface->getSharedPtr(),
            *this,
            m_cov.topLeftCorner<1, 1>(),
            m_values[0]};
      } else if (m_dim == 2) {
        return Acts::Measurement<SimSourceLink,
                                 Acts::ParDef::eLOC_0,
                                 Acts::ParDef::eLOC_1>{
            m_truthHit->surface->getSharedPtr(),
            *this,
            m_cov.topLeftCorner<2, 2>(),
            m_values[0],
            m_values[1]};
      } else {
        throw std::runtime_error("Dim " + std::to_string(m_dim)
                                 + " currently not supported.");
      }
    }

  private:
    const SimHit*     m_truthHit = nullptr;
    size_t            m_dim      = 0u;
    Acts::BoundVector m_values;
    Acts::BoundMatrix m_cov;
  };

  static_assert(Acts::SourceLinkConcept<SimSourceLink>,
                "SimSourceLink does not fulfill SourceLinkConcept.");

}  // namespace Data
}  // end of namespace FW
