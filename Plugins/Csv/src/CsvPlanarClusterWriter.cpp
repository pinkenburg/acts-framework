// This file is part of the Acts project.
//
// Copyright (C) 2017 CERN for the benefit of the Acts project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#include <stdexcept>

#include <Acts/Plugins/Digitization/PlanarModuleCluster.hpp>
#include <Acts/Utilities/Units.hpp>
#include <dfe/dfe_io_dsv.hpp>

#include "ACTFW/EventData/DataContainers.hpp"
#include "ACTFW/EventData/SimIdentifier.hpp"
#include "ACTFW/EventData/SimParticle.hpp"
#include "ACTFW/EventData/SimVertex.hpp"
#include "ACTFW/Framework/WhiteBoard.hpp"
#include "ACTFW/Plugins/Csv/CsvPlanarClusterWriter.hpp"
#include "ACTFW/Utilities/Paths.hpp"
#include "TrackMlData.hpp"

FW::Csv::CsvPlanarClusterWriter::CsvPlanarClusterWriter(
    const FW::Csv::CsvPlanarClusterWriter::Config& cfg,
    Acts::Logging::Level                           level)
  : Base(cfg.inputClusters, "CsvPlanarClusterWriter", level), m_cfg(cfg)
{
}

FW::ProcessCode
FW::Csv::CsvPlanarClusterWriter::writeT(
    const AlgorithmContext&                                          context,
    const FW::DetectorData<geo_id_value, Acts::PlanarModuleCluster>& clusters)
{
  // open per-event file for all components
  std::string pathHits
      = perEventFilepath(m_cfg.outputDir, "hits.csv", context.eventNumber);
  std::string pathCells
      = perEventFilepath(m_cfg.outputDir, "cells.csv", context.eventNumber);
  std::string pathTruth
      = perEventFilepath(m_cfg.outputDir, "truth.csv", context.eventNumber);

  dfe::CsvNamedTupleWriter<HitData> writerHits(pathHits, m_cfg.outputPrecision);
  dfe::CsvNamedTupleWriter<CellData>  writerCells(pathCells,
                                                 m_cfg.outputPrecision);
  dfe::CsvNamedTupleWriter<TruthData> writerTruth(pathTruth,
                                                  m_cfg.outputPrecision);

  HitData   hit;
  CellData  cell;
  TruthData truth;
  // will be reused as hit counter
  hit.hit_id = 0;

  for (auto& volumeData : clusters) {
    for (auto& layerData : volumeData.second) {
      for (auto& moduleData : layerData.second) {
        for (auto& cluster : moduleData.second) {
          // local cluster information
          const auto&    parameters = cluster.parameters();
          Acts::Vector2D localPos(parameters[Acts::ParDef::eLOC_0],
                                  parameters[Acts::ParDef::eLOC_1]);
          Acts::Vector3D globalFakeMom(1, 1, 1);
          Acts::Vector3D globalPos(0, 0, 0);
          // transform local into global position information
          cluster.referenceSurface().localToGlobal(
              context.geoContext, localPos, globalFakeMom, globalPos);

          // write global hit information
          hit.x         = globalPos.x() / Acts::UnitConstants::mm;
          hit.y         = globalPos.y() / Acts::UnitConstants::mm;
          hit.z         = globalPos.z() / Acts::UnitConstants::mm;
          hit.t         = 0 / Acts::UnitConstants::ns;  // TODO
          hit.volume_id = volumeData.first;
          hit.layer_id  = layerData.first;
          hit.module_id = moduleData.first;
          writerHits.append(hit);

          // write local cell information
          cell.hit_id = hit.hit_id;
          for (auto& c : cluster.digitizationCells()) {
            cell.ch0       = c.channel0;
            cell.ch1       = c.channel1;
            cell.timestamp = 0;  // TODO
            cell.value     = c.data;
            writerCells.append(cell);
          }

          // write hit-particle truth association
          // each hit can have multiple particles, e.g. in a dense environment
          truth.hit_id = hit.hit_id;
          for (auto& p : cluster.sourceLink().truthParticles()) {
            truth.particle_id = p->barcode();
            truth.tx          = p->position().x() / Acts::UnitConstants::mm;
            truth.ty          = p->position().y() / Acts::UnitConstants::mm;
            truth.tz          = p->position().z() / Acts::UnitConstants::mm;
            truth.tt          = 0 / Acts::UnitConstants::ns;  // TODO
            truth.tpx         = p->momentum().x() / Acts::UnitConstants::GeV;
            truth.tpy         = p->momentum().y() / Acts::UnitConstants::GeV;
            truth.tpz         = p->momentum().z() / Acts::UnitConstants::GeV;
            writerTruth.append(truth);
          }

          // increase hit id for next iteration
          hit.hit_id += 1;
        }
      }
    }
  }

  return FW::ProcessCode::SUCCESS;
}
