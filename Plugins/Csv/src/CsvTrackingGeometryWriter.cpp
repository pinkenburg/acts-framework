// This file is part of the Acts project.
//
// Copyright (C) 2017 CERN for the benefit of the Acts project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#include "ACTFW/Plugins/Csv/CsvTrackingGeometryWriter.hpp"

#include <iostream>
#include <sstream>
#include <stdexcept>

#include <dfe/dfe_namedtuple.hpp>

#include <Acts/Geometry/TrackingVolume.hpp>
#include <Acts/Plugins/Digitization/CartesianSegmentation.hpp>
#include <Acts/Plugins/Digitization/DigitizationModule.hpp>
#include <Acts/Plugins/Identification/IdentifiedDetectorElement.hpp>
#include <Acts/Surfaces/Surface.hpp>

#include "ACTFW/Utilities/Paths.hpp"
#include "TrackMlData.hpp"

using namespace FW;
using namespace FW::Csv;

CsvTrackingGeometryWriter::CsvTrackingGeometryWriter(
    const CsvTrackingGeometryWriter::Config& cfg,
    Acts::Logging::Level                     lvl)
  : m_cfg(cfg)
  , m_world(nullptr)
  , m_logger(Acts::getDefaultLogger("CsvTrackingGeometryWriter", lvl))

{
  if (not m_cfg.trackingGeometry) {
    throw std::invalid_argument("Missing tracking geometry");
  }
  m_world = m_cfg.trackingGeometry->highestTrackingVolume();
  if (not m_world) {
    throw std::invalid_argument("Could not identify the world volume");
  }
}

std::string
CsvTrackingGeometryWriter::name() const
{
  return "CsvTrackingGeometryWriter";
}

namespace {
using SurfaceWriter = dfe::CsvNamedTupleWriter<SurfaceData>;

/// Write a single surface.
void
writeSurface(SurfaceWriter&               writer,
             const Acts::Surface&         surface,
             const Acts::GeometryContext& geoCtx)
{
  SurfaceData data;

  // unpack geo id
  auto geoID     = surface.geoID();
  data.volume_id = geoID.value(Acts::GeometryID::volume_mask);
  data.layer_id  = geoID.value(Acts::GeometryID::layer_mask);
  data.module_id = geoID.value(Acts::GeometryID::sensitive_mask);

  // coordinate transformation
  auto center    = surface.center(geoCtx);
  auto transform = surface.transform(geoCtx);
  // TODO units
  data.cx     = center.x();
  data.cy     = center.y();
  data.cz     = center.z();
  data.rot_xu = transform(0, 0);
  data.rot_xv = transform(0, 1);
  data.rot_xw = transform(0, 2);
  data.rot_yu = transform(1, 0);
  data.rot_yv = transform(1, 1);
  data.rot_yw = transform(1, 2);
  data.rot_zu = transform(2, 0);
  data.rot_zv = transform(2, 1);
  data.rot_zw = transform(2, 2);

  // module thickness
  if (surface.associatedDetectorElement()) {
    const auto* detElement
        = dynamic_cast<const Acts::IdentifiedDetectorElement*>(
            surface.associatedDetectorElement());
    if (detElement) { data.module_t = detElement->thickness(); }
  }

  // bounds and pitch (if available)
  const auto& bounds       = surface.bounds();
  const auto* planarBounds = dynamic_cast<const Acts::PlanarBounds*>(&bounds);
  if (planarBounds) {
    // extract limits from value store
    auto boundValues = surface.bounds().valueStore();
    if (boundValues.size() == 2) {
      data.module_minhu = boundValues[0];
      data.module_minhu = boundValues[0];
      data.module_minhu = boundValues[1];
    } else if (boundValues.size() == 3) {
      data.module_minhu = boundValues[0];
      data.module_minhu = boundValues[0];
      data.module_minhu = boundValues[1];
    }
    // get the pitch from the digitization module
    const auto* detElement
        = dynamic_cast<const Acts::IdentifiedDetectorElement*>(
            surface.associatedDetectorElement());
    if (detElement and detElement->digitizationModule()) {
      auto dModule = detElement->digitizationModule();
      // dynamic_cast to CartesianSegmentation
      const auto* cSegmentation
          = dynamic_cast<const Acts::CartesianSegmentation*>(
              &(dModule->segmentation()));
      if (cSegmentation) {
        auto pitch   = cSegmentation->pitch();
        data.pitch_u = pitch.first;
        data.pitch_u = pitch.second;
      }
    }
  }

  writer.append(data);
}

/// Write all child surfaces and descend into confined volumes.
void
writeVolume(SurfaceWriter&               writer,
            const Acts::TrackingVolume&  volume,
            const Acts::GeometryContext& geoCtx)
{
  // process all layers that are directly stored within this volume
  if (volume.confinedLayers()) {
    for (auto layer : volume.confinedLayers()->arrayObjects()) {
      // we jump navigation layers
      if (layer->layerType() == Acts::navigation) { continue; }
      // check for sensitive surfaces
      if (layer->surfaceArray()) {
        for (auto surface : layer->surfaceArray()->surfaces()) {
          if (surface) { writeSurface(writer, *surface, geoCtx); }
        }
      }
    }
  }
  // step down into hierarchy to process all child volumnes
  if (volume.confinedVolumes()) {
    for (auto confined : volume.confinedVolumes()->arrayObjects()) {
      writeVolume(writer, *confined.get(), geoCtx);
    }
  }
}
}  // namespace

ProcessCode
CsvTrackingGeometryWriter::write(const AlgorithmContext& ctx)
{
  if (not m_cfg.writePerEvent) { return ProcessCode::SUCCESS; }
  SurfaceWriter writer(
      perEventFilepath(m_cfg.outputDir, "detectors.csv", ctx.eventNumber),
      m_cfg.outputPrecision);
  writeVolume(writer, *m_world, ctx.geoContext);
  return ProcessCode::SUCCESS;
}

ProcessCode
CsvTrackingGeometryWriter::endRun()
{
  SurfaceWriter writer(joinPaths(m_cfg.outputDir, "detectors.csv"),
                       m_cfg.outputPrecision);
  writeVolume(writer, *m_world, Acts::GeometryContext());
  return ProcessCode::SUCCESS;
}
