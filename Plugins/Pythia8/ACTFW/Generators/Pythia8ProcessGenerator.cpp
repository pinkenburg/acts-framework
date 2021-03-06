// This file is part of the Acts project.
//
// Copyright (C) 2017-2019 CERN for the benefit of the Acts project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#include "ACTFW/Generators/Pythia8ProcessGenerator.hpp"

#include <algorithm>
#include <iterator>
#include <random>

namespace {
struct FrameworkRndmEngine : public Pythia8::RndmEngine
{
  FW::RandomEngine& rng;

  FrameworkRndmEngine(FW::RandomEngine& rng_) : rng(rng_) {}
  double
  flat()
  {
    return std::uniform_real_distribution<double>(0.0, 1.0)(rng);
  }
};
}  // namespace

std::function<std::vector<FW::Data::SimVertex>(FW::RandomEngine&)>
FW::Pythia8Generator::makeFunction(const FW::Pythia8Generator::Config& cfg,
                                   Acts::Logging::Level                lvl)
{
  auto gen = std::make_shared<Pythia8Generator>(cfg, lvl);
  return [=](RandomEngine& rng) { return (*gen)(rng); };
}

FW::Pythia8Generator::Pythia8Generator(const FW::Pythia8Generator::Config& cfg,
                                       Acts::Logging::Level                lvl)
  : m_cfg(cfg)
  , m_logger(Acts::getDefaultLogger("Pythia8Generator", lvl))
  , m_pythia8("", false)
{
  // disable all output by default but allow reenable via config
  m_pythia8.settings.flag("Print:quiet", true);
  for (const auto& str : m_cfg.settings) {
    ACTS_VERBOSE("use Pythia8 setting '" << str << "'");
    m_pythia8.readString(str.c_str());
  }
  m_pythia8.settings.mode("Beams:idA", m_cfg.pdgBeam0);
  m_pythia8.settings.mode("Beams:idB", m_cfg.pdgBeam1);
  m_pythia8.settings.mode("Beams:frameType", 1);
  m_pythia8.settings.parm("Beams:eCM", m_cfg.cmsEnergy / Acts::units::_GeV);
  m_pythia8.init();
}

std::vector<FW::Data::SimVertex>
FW::Pythia8Generator::operator()(FW::RandomEngine& rng)
{
  using namespace Acts::UnitLiterals;
  using namespace Data;

  // first process vertex is the primary one at origin with time=0
  std::vector<SimVertex> vertices = {
      SimVertex({0.0, 0.0, 0.0}),
  };

  // pythia8 is not thread safe and generation needs to be protected
  std::lock_guard<std::mutex> lock(m_pythia8Mutex);
  // use per-thread random engine also in pythia
  FrameworkRndmEngine rndmEngine(rng);
  m_pythia8.rndm.rndmEnginePtr(&rndmEngine);
  m_pythia8.next();

  // convert generated final state particles into internal format
  for (size_t ip = 0; ip < m_pythia8.event.size(); ++ip) {
    const auto& particle = m_pythia8.event[ip];

    // ignore beam particles
    if (particle.statusHepMC() == 4) { continue; }
    // only interested in final, visible particles
    if (not particle.isFinal()) { continue; }
    if (not particle.isVisible()) { continue; }

    // extract particle type
    const auto mass   = particle.m0() * 1_GeV;
    const auto charge = particle.charge() * 1_e;
    const auto pdg    = particle.id();
    // extract particle kinematic state
    const auto pos  = Acts::Vector3D(particle.xProd() * 1_mm,
                                    particle.yProd() * 1_mm,
                                    particle.zProd() * 1_mm);
    const auto time = particle.tProd() * 1_mm;  // units mm/c, and we use c=1
    const auto mom  = Acts::Vector3D(
        particle.px() * 1_GeV, particle.py() * 1_GeV, particle.pz() * 1_GeV);

    // identify secondary vertex
    std::vector<SimVertex>::iterator secondaryVertex;
    if (not particle.hasVertex()) {
      // w/o defined vertex, must belong to the first (primary) process vertex
      secondaryVertex = vertices.begin();
    } else {
      // either add to existing secondary vertex if exists or create new one
      // TODO can we do this w/o the manual search and position/time check?
      secondaryVertex = std::find_if(
          vertices.begin(), vertices.end(), [=](const SimVertex& vertex) {
            return (vertex.position == pos) and (vertex.time == time);
          });
      if (secondaryVertex == vertices.end()) {
        // no matching secondary vertex exists -> create new one
        SimVertex tmp(pos, {}, {}, 0u, time);
        vertices.push_back(std::move(tmp));
        secondaryVertex = std::prev(vertices.end());

        ACTS_VERBOSE("created new secondary vertex " << pos.transpose());
      }
    }

    // encode event structure information into barcode
    Barcode barcode;
    // NOTE: the primary vertex identifier in the barcode is set in the event
    //      generator and does not need to be set here in the process generator.
    barcode.setVertexSecondary(
        std::distance(vertices.begin(), secondaryVertex));
    // ensure particle identifier is non-zero
    barcode.setParticle(1u + secondaryVertex->outgoing.size());
    secondaryVertex->outgoing.emplace_back(
        pos, mom, mass, charge, pdg, barcode, time);
  }
  return vertices;
}
