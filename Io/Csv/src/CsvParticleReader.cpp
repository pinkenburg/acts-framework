// This file is part of the Acts project.
//
// Copyright (C) 2017 CERN for the benefit of the Acts project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#include "ACTFW/Io/Csv/CsvParticleReader.hpp"

#include <fstream>
#include <ios>
#include <stdexcept>
#include <string>
#include <vector>

#include <Acts/Utilities/Units.hpp>
#include <dfe/dfe_io_dsv.hpp>

#include "ACTFW/EventData/Barcode.hpp"
#include "ACTFW/EventData/SimParticle.hpp"
#include "ACTFW/Framework/WhiteBoard.hpp"
#include "ACTFW/Utilities/Paths.hpp"
#include "TrackMlData.hpp"

FW::CsvParticleReader::CsvParticleReader(
    const FW::CsvParticleReader::Config& cfg,
    Acts::Logging::Level                 level)
  : m_cfg(cfg)
  , m_eventsRange(
        determineEventFilesRange(cfg.inputDir, cfg.inputStem + ".csv"))
  , m_logger(Acts::getDefaultLogger("CsvParticleReader", level))
{
  if (m_cfg.outputParticles.empty()) {
    throw std::invalid_argument("Missing output collection");
  }
  if (m_cfg.inputStem.empty()) {
    throw std::invalid_argument("Missing input filename stem");
  }

  if (not m_cfg.inputParticleSurface.empty()) {

    if (not m_cfg.trackingGeometry) {
      throw std::invalid_argument("Missing tracking geometry");
    }

    // fill the geo id to surface map once to speed up lookups later on
    //@TODO: we need material surfaces as well as sensitive surfaces though. How
    // to do that?
    m_cfg.trackingGeometry->visitSurfaces([this](const Acts::Surface* surface) {
      this->m_surfaces[surface->geoID()] = surface;
    });
  }
}

std::string
FW::CsvParticleReader::CsvParticleReader::name() const
{
  return "CsvParticleReader";
}

std::pair<size_t, size_t>
FW::CsvParticleReader::availableEvents() const
{
  return m_eventsRange;
}

FW::ProcessCode
FW::CsvParticleReader::read(const FW::AlgorithmContext& ctx)
{
  SimParticles particles;

  auto path = perEventFilepath(
      m_cfg.inputDir, m_cfg.inputStem + ".csv", ctx.eventNumber);
  dfe::CsvNamedTupleReader<ParticleData> reader(path);
  ParticleData                           data;

  while (reader.read(data)) {
    Acts::Vector3D particlePos(data.vx * Acts::UnitConstants::mm,
                               data.vy * Acts::UnitConstants::mm,
                               data.vz * Acts::UnitConstants::mm);
    double         particleTime = data.vt * Acts::UnitConstants::ns;
    Acts::Vector3D particleMom(data.px * Acts::UnitConstants::GeV,
                               data.py * Acts::UnitConstants::GeV,
                               data.pz * Acts::UnitConstants::GeV);
    //@TODO: get mass and pdg from config?
    double mass = 0.;
    // the file is usually ordered by particle id already
    particles.emplace_hint(particles.end(),
                           particlePos,
                           particleMom,
                           mass,
                           data.q * Acts::UnitConstants::e,
                           data.particle_type,  // this is the pdg id
                           data.particle_id,
                           particleTime);
  }

  // write the truth particles to the EventStore
  ctx.eventStore.add(m_cfg.outputParticles, std::move(particles));

  if (not m_cfg.inputParticleSurface.empty()) {
    SimSurfaces surfaces;
    auto        path = perEventFilepath(
        m_cfg.inputDir, m_cfg.inputParticleSurface + ".csv", ctx.eventNumber);
    dfe::CsvNamedTupleReader<ParticleSurfaceData> reader(path);

    ParticleSurfaceData data;
    while (reader.read(data)) {
      barcode_type particle_id = data.particle_id;
      // identify surface
      Acts::GeometryID geoId;
      geoId.setVolume(data.volume_id);
      geoId.setLayer(data.layer_id);
      geoId.setSensitive(data.module_id);

      auto it = m_surfaces.find(geoId);
      if (it == m_surfaces.end() or not it->second) {
        ACTS_FATAL("Could not retrieve the surface for particle surface data"
                   << data);
        return ProcessCode::ABORT;
      }
      const Acts::Surface* surface = it->second;

      // push back the surface
      surfaces[particle_id].push_back(surface);
    }

    // write the passed surface sequence of truth particle to EventStore
    ctx.eventStore.add(m_cfg.outputParticleSurfacesMap, std::move(surfaces));
  }

  return ProcessCode::SUCCESS;
}
