// This file is part of the Acts project.
//
// Copyright (C) 2017 CERN for the benefit of the Acts project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#include "ACTFW/Io/Root/RootParticleWriter.hpp"

#include <ios>
#include <stdexcept>

#include <Acts/Utilities/Helpers.hpp>
#include <Acts/Utilities/Units.hpp>
#include <TFile.h>
#include <TTree.h>

using Acts::VectorHelpers::eta;
using Acts::VectorHelpers::perp;
using Acts::VectorHelpers::phi;

FW::RootParticleWriter::RootParticleWriter(
    const FW::RootParticleWriter::Config& cfg,
    Acts::Logging::Level                  level)
  : WriterT(cfg.collection, "RootParticleWriter", level)
  , m_cfg(cfg)
  , m_outputFile(cfg.rootFile)
{
  // An input collection name and tree name must be specified
  if (m_cfg.collection.empty()) {
    throw std::invalid_argument("Missing input collection");
  } else if (m_cfg.treeName.empty()) {
    throw std::invalid_argument("Missing tree name");
  }

  // Setup ROOT I/O
  if (m_outputFile == nullptr) {
    m_outputFile = TFile::Open(m_cfg.filePath.c_str(), m_cfg.fileMode.c_str());
    if (m_outputFile == nullptr) {
      throw std::ios_base::failure("Could not open '" + m_cfg.filePath);
    }
  }
  m_outputFile->cd();
  m_outputTree = new TTree(m_cfg.treeName.c_str(), m_cfg.treeName.c_str());
  if (m_outputTree == nullptr)
    throw std::bad_alloc();
  else {
    // I/O parameters
    m_outputTree->Branch("event_nr", &m_eventNr);
    m_outputTree->Branch("eta", &m_eta);
    m_outputTree->Branch("phi", &m_phi);
    m_outputTree->Branch("vx", &m_vx);
    m_outputTree->Branch("vy", &m_vy);
    m_outputTree->Branch("vz", &m_vz);
    m_outputTree->Branch("vt", &m_vt);
    m_outputTree->Branch("px", &m_px);
    m_outputTree->Branch("py", &m_py);
    m_outputTree->Branch("pz", &m_pz);
    m_outputTree->Branch("pt", &m_pT);
    m_outputTree->Branch("charge", &m_charge);
    m_outputTree->Branch("mass", &m_mass);
    m_outputTree->Branch("pdg", &m_pdgCode);
    m_outputTree->Branch("barcode", &m_barcode, "barcode/l");
    m_outputTree->Branch("vertex_primary", &m_vertexPrimary);
    m_outputTree->Branch("vertex_secondary", &m_vertexSecondary);
    m_outputTree->Branch("particle", &m_particle);
    m_outputTree->Branch("parent_particle", &m_parentParticle);
    m_outputTree->Branch("process", &m_process);
  }
}

FW::RootParticleWriter::~RootParticleWriter()
{
  if (m_outputFile) { m_outputFile->Close(); }
}

FW::ProcessCode
FW::RootParticleWriter::endRun()
{
  if (m_outputFile) {
    m_outputFile->cd();
    m_outputTree->Write();
    ACTS_INFO("Wrote particles to tree '" << m_cfg.treeName << "' in '"
                                          << m_cfg.filePath << "'");
  }
  return ProcessCode::SUCCESS;
}

FW::ProcessCode
FW::RootParticleWriter::writeT(const AlgorithmContext&             context,
                               const std::vector<Data::SimVertex>& vertices)
{

  if (m_outputFile == nullptr) return ProcessCode::SUCCESS;

  // Exclusive access to the tree while writing
  std::lock_guard<std::mutex> lock(m_writeMutex);

  // Get the event number
  m_eventNr = context.eventNumber;

  // loop over the process vertices
  for (auto& vertex : vertices) {
    for (auto& particle : vertex.outgoing) {
      // collect the information
      m_vx      = particle.position().x();
      m_vy      = particle.position().y();
      m_vz      = particle.position().z();
      m_vt      = particle.time() / Acts::UnitConstants::ns;
      m_eta     = eta(particle.momentum());
      m_phi     = phi(particle.momentum());
      m_px      = particle.momentum().x();
      m_py      = particle.momentum().y();
      m_pz      = particle.momentum().z();
      m_pT      = perp(particle.momentum());
      m_charge  = particle.q();
      m_mass    = particle.m();
      m_pdgCode = particle.pdg();
      // store encoded barcode
      m_barcode = particle.barcode().value();
      // store decoded barcode components
      m_vertexPrimary   = particle.barcode().vertexPrimary();
      m_vertexSecondary = particle.barcode().vertexSecondary();
      m_particle        = particle.barcode().particle();
      m_parentParticle  = particle.barcode().parentParticle();
      m_process         = particle.barcode().process();
      m_outputTree->Fill();
    }
  }

  return ProcessCode::SUCCESS;
}
