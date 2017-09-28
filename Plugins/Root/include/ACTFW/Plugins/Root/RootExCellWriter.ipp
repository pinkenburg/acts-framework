// This file is part of the ACTS project.
//
// Copyright (C) 2017 ACTS project team
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#include <ios>
#include <stdexcept>

template <class T>
FW::ProcessCode
FW::Root::RootExCellWriter<T>::writeT(
    const FW::AlgorithmContext&                    ctx,
    const std::vector<Acts::ExtrapolationCell<T>>& ecells)
{
  // exclusive access to the tree
  std::lock_guard<std::mutex> lock(m_writeMutex);

  // loop over all the extrapolation cells
  for (auto& eCell : ecells) {
    // the event paramters
    auto sMomentum = eCell.startParameters->momentum();
    m_eta          = sMomentum.eta();
    m_phi          = sMomentum.phi();
    m_materialX0   = eCell.materialX0;
    m_materialL0   = eCell.materialL0;

    // Displacement, momentum difference and energy loss due to material effects
    if (eCell.endParameters) {
    Acts::Vector3D firstPosition = eCell.startParameters->position();
    Acts::Vector3D lastPosition = eCell.endParameters->position();
    Acts::Vector3D lastMomentum = eCell.endParameters->momentum();
    
    m_r0 = firstPosition.perp();
    m_r1 = lastPosition.perp();
    m_pt0 = sMomentum.perp();
    m_pt1 = lastMomentum.perp();
    m_dx = fabs(firstPosition.x() - lastPosition.x());
    m_dy = fabs(firstPosition.y() - lastPosition.y());
    m_dz = fabs(firstPosition.z() - lastPosition.z());
    m_dr = fabs(firstPosition.perp() - lastPosition.perp());
    m_dPx = fabs(sMomentum.x() - lastMomentum.x());
    m_dPy = fabs(sMomentum.y() - lastMomentum.y());
    m_dPz = fabs(sMomentum.z() - lastMomentum.z());
    m_dPt = fabs(sMomentum.perp() - lastMomentum.perp());
    }

    // clear the vectors & reserve
    // - for main step information
    m_s_positionX.clear();
    m_s_positionY.clear();
    m_s_positionZ.clear();
    m_s_positionR.clear();
    m_s_volumeID.clear();
    m_s_layerID.clear();
    m_s_surfaceID.clear();
    m_s_positionX.reserve(MAXSTEPS);
    m_s_positionY.reserve(MAXSTEPS);
    m_s_positionZ.reserve(MAXSTEPS);
    m_s_positionR.reserve(MAXSTEPS);
    m_s_volumeID.reserve(MAXSTEPS);
    m_s_layerID.reserve(MAXSTEPS);
    m_s_surfaceID.reserve(MAXSTEPS);

    // - for the sensitive
    if (m_cfg.writeSensitive) {
      m_s_sensitive.clear();
      m_s_localposition0.clear();
      m_s_localposition1.clear();
      m_s_sensitive.reserve(MAXSTEPS);
      m_s_localposition0.reserve(MAXSTEPS);
      m_s_localposition1.reserve(MAXSTEPS);
    }
    // - for the material
    if (m_cfg.writeMaterial) {
      m_s_material.clear();
      m_s_materialX0.clear();
      m_s_materialL0.clear();
      m_s_material.reserve(MAXSTEPS);
      m_s_materialX0.reserve(MAXSTEPS);
      m_s_materialL0.reserve(MAXSTEPS);
    }
    // - for the boundary
    if (m_cfg.writeBoundary) {
      m_s_boundary.clear();
      m_s_boundary.reserve(MAXSTEPS);
    }
    // the number of sensitive hits per event
    m_hits = 0;
    // loop over extrapolation steps
    size_t parameterFlag = false;
    for (auto& es : eCell.extrapolationSteps) {
      if (es.parameters) {
          parameterFlag = true;
        /// step parameters
        const T& pars = (*es.parameters);
        /// type information
        int material = es.configuration.checkMode(
            Acts::ExtrapolationMode::CollectMaterial);
        int boundary = es.configuration.checkMode(
            Acts::ExtrapolationMode::CollectBoundary);
        int sensitive = es.configuration.checkMode(
            Acts::ExtrapolationMode::CollectSensitive);
        int passive = es.configuration.checkMode(
            Acts::ExtrapolationMode::CollectPassive);

        /// check the layer, surface, volume ID
        geo_id_value volumeID = pars.referenceSurface().geoID().value(
            Acts::GeometryID::volume_mask);
        geo_id_value layerID = pars.referenceSurface().geoID().value(
            Acts::GeometryID::layer_mask);
        geo_id_value surfaceID = pars.referenceSurface().geoID().value(
            Acts::GeometryID::sensitive_mask);
        ///
        if ((m_cfg.writeSensitive && sensitive)
            || (m_cfg.writeBoundary && boundary)
            || (m_cfg.writeMaterial && material)
            || (m_cfg.writePassive && passive)) {

          // the material steps
          if (m_cfg.writeMaterial) {
            // the material is being written out
            double materialStepX0 = 0.;
            double materialStepL0 = 0.;
            if (es.material) {
              // assign the material
              materialStepX0
                  = es.materialScaling * es.material->thicknessInX0();
              materialStepX0
                  = es.materialScaling * es.material->thicknessInL0();
            }
            m_s_materialX0.push_back(materialStepX0);
            m_s_materialX0.push_back(materialStepL0);
          }

          /// goblal position information
          m_s_positionX.push_back(pars.position().x());
          m_s_positionY.push_back(pars.position().y());
          m_s_positionZ.push_back(pars.position().z());
          m_s_positionR.push_back(pars.position().perp());

          /// local position information - only makes sense for sensitive really
          if (m_cfg.writeSensitive) {
            m_s_localposition0.push_back(pars.parameters()[Acts::eLOC_X]);
            m_s_localposition1.push_back(pars.parameters()[Acts::eLOC_Y]);
          }
          /// volume, layer and surface ID
          m_s_volumeID.push_back(volumeID);
          m_s_layerID.push_back(layerID);
          m_s_surfaceID.push_back(surfaceID);
          /// indicate what hit you have
          m_s_material.push_back(material);
          m_s_boundary.push_back(boundary);
          m_s_sensitive.push_back(sensitive);
        }
        if (sensitive) m_hits++;
      }
    }
    m_outputTree->Fill();
  }

  // return scuess
  return FW::ProcessCode::SUCCESS;
}

template <class T>
FW::Root::RootExCellWriter<T>::RootExCellWriter(
    const FW::Root::RootExCellWriter<T>::Config& cfg,
    Acts::Logging::Level                         level)
  : FW::WriterT<std::vector<Acts::ExtrapolationCell<T>>>(cfg.collection,
                                                         "RootExCellWriter",
                                                         level)
  , m_cfg(cfg)
  , m_outputFile(nullptr)
  , m_outputTree(nullptr)
{
  // Validate the configuration
  if (m_cfg.collection.empty()) {
    throw std::invalid_argument("Missing input collection");
  } else if (m_cfg.treeName.empty()) {
    throw std::invalid_argument("Missing tree name");
  }

  // Setup ROOT I/O
  m_outputFile = TFile::Open(m_cfg.filePath.c_str(), m_cfg.fileMode.c_str());
  if (!m_outputFile) {
    throw std::ios_base::failure("Could not open '" + m_cfg.filePath);
  }
  m_outputFile->cd();
  m_outputTree
      = new TTree(m_cfg.treeName.c_str(), "TTree from RootPlanarClusterWriter");
  if (!m_outputTree) throw std::bad_alloc();

  // Initial parameters
  m_outputTree->Branch("eta", &m_eta);
  m_outputTree->Branch("phi", &m_phi);

  // Output the step information
  m_outputTree->Branch("step_x", &m_s_positionX);
  m_outputTree->Branch("step_y", &m_s_positionY);
  m_outputTree->Branch("step_z", &m_s_positionZ);
  m_outputTree->Branch("step_r", &m_s_positionR);

  // Identification
  m_outputTree->Branch("volumeID", &m_s_volumeID);
  m_outputTree->Branch("layerID", &m_s_layerID);
  m_outputTree->Branch("surfaceID", &m_s_surfaceID);

  // Material section
  if (m_cfg.writeMaterial) {
    m_outputTree->Branch("material_X0", &m_materialX0);
    m_outputTree->Branch("material_L0", &m_materialL0);
    m_outputTree->Branch("step_material_X0", &m_s_materialX0);
    m_outputTree->Branch("step_material_L0", &m_s_materialL0);
    m_outputTree->Branch("material", &m_s_material);
  }

  // Sensitive section
  if (m_cfg.writeSensitive) {
    m_outputTree->Branch("sensitive", &m_s_sensitive);
    m_outputTree->Branch("step_l0", &m_s_localposition0);
    m_outputTree->Branch("step_l1", &m_s_localposition1);
  }

  // Boundary section
  if (m_cfg.writeBoundary) m_outputTree->Branch("boundary", &m_s_boundary);

  // Number of sensitive hits
  m_outputTree->Branch("hits", &m_hits);
    
  // compare displacement due to material effects
  m_outputTree->Branch("r0", &m_r0);
  m_outputTree->Branch("r1", &m_r1);
    
  m_outputTree->Branch("pt0", &m_pt0);
  m_outputTree->Branch("pt1", &m_pt1);
    
  m_outputTree->Branch("dx", &m_dx);
  m_outputTree->Branch("dy", &m_dy);
  m_outputTree->Branch("dz", &m_dz);
  m_outputTree->Branch("dr", &m_dr);
  // energy loss due to material effects
  m_outputTree->Branch("dPx", &m_dPx);
  m_outputTree->Branch("dPy", &m_dPy);
  m_outputTree->Branch("dPz", &m_dPz);
  m_outputTree->Branch("dPt", &m_dPt);

  return FW::ProcessCode::SUCCESS;

}

template <class T>
FW::ProcessCode
FW::Root::RootExCellWriter<T>::endRun()
{
  m_outputFile->cd();
  m_outputTree->Write();
  // ACTS_INFO("Wrote particles to tree '" << m_cfg.treeName << "' in '"
  //                                      << m_cfg.filePath << "'");
  return ProcessCode::SUCCESS;
}
