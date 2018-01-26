// This file is part of the ACTS project.
//
// Copyright (C) 2017 ACTS project team
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#ifndef ACTFW_CSV_PLUGINS_TRACKINGGEOMETRYWRITER_H
#define ACTFW_CSV_PLUGINS_TRACKINGGEOMETRYWRITER_H

#include <mutex>

#include <fstream>
#include <iostream>
#include "ACTFW/Framework/IService.hpp"
#include "ACTFW/Framework/ProcessCode.hpp"
#include "ACTFW/Plugins/Csv/CsvSurfaceWriter.hpp"
#include "ACTS/Detector/TrackingGeometry.hpp"
#include "ACTS/Surfaces/Surface.hpp"
#include "ACTS/Utilities/Logger.hpp"

namespace Acts {
class TrackingVolume;
}
namespace FW {
class ISurfaceWriter;
}

namespace FW {
namespace Csv {

/// @class CsvTrackingGeometryWriter
///
/// An Csv writer for the geometry
/// It delegates the writing of surfaces to the surface writers
class CsvTrackingGeometryWriter : public FW::IWriterT<Acts::TrackingGeometry>
{
public:
  // @class Config
  //
  // The nested config class
  class Config
  {
  public:
    /// the default logger
    std::shared_ptr<const Acts::Logger> logger;
    /// the name of the writer
    std::string name = "";
    /// surfaceWriters
    std::shared_ptr<CsvSurfaceWriter> surfaceWriter = nullptr;
    std::string                       filePrefix    = "";
    std::string                       layerPrefix   = "";

    /// Constructor for the nested config class
    /// @param lname is the name of the writer
    /// @lvl is the screen output logging level
    Config(const std::string&   lname = "CsvTrackingGeometryWriter",
           Acts::Logging::Level lvl   = Acts::Logging::INFO)
      : logger(Acts::getDefaultLogger(lname, lvl))
      , name(lname)
      , surfaceWriter()
    {
    }
  };

  /// Constructor
  /// @param cfg is the configuration class
  CsvTrackingGeometryWriter(const Config& cfg);

  /// Framework name() method
  /// @return the name of the tool
  std::string
  name() const final override;

  /// The write interface
  /// @param tGeometry is the geometry to be written out
  /// @return ProcessCode to indicate success/failure
  FW::ProcessCode
  write(const Acts::TrackingGeometry& tGeometry) final override;

private:
  Config m_cfg;  ///< the config class

  /// process this volume
  /// @param tVolume the volume to be processed
  void
  write(const Acts::TrackingVolume& tVolume);

  /// Private access to the logging instance
  const Acts::Logger&
  logger() const
  {
    return *m_cfg.logger;
  }
};
} // end of namespace Csv
} // end of namesapce FW

#endif  // ACTFW_CSV_PLUGINS_TRACKINGGEOMETRYWRITER_H
