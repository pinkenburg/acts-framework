// This file is part of the ACTS project.
//
// Copyright (C) 2017 ACTS project team
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#ifndef ACTFW_OPTIONS_DD4HEPDETECTOROPTIONS_HPP
#define ACTFW_OPTIONS_DD4HEPDETECTOROPTIONS_HPP

#include <cstdlib>
#include <iostream>
#include <utility>
#include "ACTFW/Plugins/DD4hep/GeometryService.hpp"
#include "ACTFW/Utilities/Options.hpp"
#include "ACTS/Utilities/Units.hpp"

namespace po = boost::program_options;

namespace au = Acts::units;

namespace FW {

namespace Options {

  /// the particle gun options, the are prefixes with gp
  template <class AOPT>
  void
  addDD4hepOptions(AOPT& opt)
  {
    opt.add_options()(
        "dd4hep-input",
        po::value<std::string>()->default_value(
            "file:Detectors/DD4hepDetector/compact/FCChhTrackerTkLayout.xml"),
        "The location of the input DD4hep file, use 'file:foo.xml'")(
        "dd4hep-envelopeR",
        po::value<double>()->default_value(0.),
        "The envelop cover in R for DD4hep volumes.")(
        "dd4hep-envelopeZ",
        po::value<double>()->default_value(0.),
        "The envelop cover in z for DD4hep volumes.")(
        "dd4hep-digitizationmodules",
        po::value<bool>()->default_value(false),
        "The envelop cover in z for DD4hep volumes.");
  }

  /// read the particle gun options and return a Config file
  template <class AMAP>
  FW::DD4hep::GeometryService::Config
  readDD4hepConfig(const AMAP& vm)
  {
    // DETECTOR configuration:
    // --------------------------------------------------------------------------------
    FW::DD4hep::GeometryService::Config gsConfig("GeometryService");
    gsConfig.xmlFileName = vm["dd4hep-input"].template as<std::string>();
    gsConfig.bTypePhi    = Acts::equidistant;
    gsConfig.bTypeR      = Acts::arbitrary;
    gsConfig.bTypeZ      = Acts::equidistant;
    gsConfig.envelopeR   = vm["dd4hep-envelopeR"].template as<double>();
    gsConfig.envelopeZ   = vm["dd4hep-envelopeZ"].template as<double>();
    gsConfig.buildDigitizationModules
        = vm["dd4hep-digitizationmodules"].template as<bool>();
    return gsConfig;
  }
}
}

#endif  // ACTFW_OPTIONS_DD4HEPDETECTOROPTIONS_HPP
