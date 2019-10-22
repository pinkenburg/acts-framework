// This file is part of the Acts project.
//
// Copyright (C) 2019 CERN for the benefit of the Acts project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#include "detail/FatrasEvgenBase.hpp"

#include "ACTFW/EventData/Barcode.hpp"
#include "ACTFW/Framework/RandomNumbers.hpp"
#include "ACTFW/Framework/Sequencer.hpp"
#include "ACTFW/Generators/EventGenerator.hpp"
#include "ACTFW/Options/CommonOptions.hpp"
#include "ACTFW/Options/ParticleGunOptions.hpp"
#include "ACTFW/Options/Pythia8Options.hpp"
#include "ACTFW/Plugins/Csv/CsvParticleWriter.hpp"
#include "ACTFW/Plugins/Root/RootParticleWriter.hpp"
#include "ACTFW/Utilities/Paths.hpp"

void
setupEvgenInput(boost::program_options::variables_map& vm,
                FW::Sequencer&                         sequencer,
                std::shared_ptr<FW::BarcodeSvc>        barcodeSvc,
                std::shared_ptr<FW::RandomNumbers>     randomNumberSvc)
{
  // Read the standard options
  auto logLevel = FW::Options::readLogLevel(vm);

  // Add requested event generator
  auto evgenInput = vm["evg-input-type"].template as<std::string>();
  if (evgenInput == "gun") {
    auto evgCfg          = FW::Options::readParticleGunOptions(vm);
    evgCfg.output        = "particles";
    evgCfg.randomNumbers = randomNumberSvc;
    evgCfg.barcodeSvc    = barcodeSvc;
    sequencer.addReader(std::make_shared<FW::EventGenerator>(evgCfg, logLevel));

  } else if (evgenInput == "pythia8") {
    auto evgCfg          = FW::Options::readPythia8Options(vm);
    evgCfg.output        = "particles";
    evgCfg.randomNumbers = randomNumberSvc;
    evgCfg.barcodeSvc    = barcodeSvc;
    sequencer.addReader(std::make_shared<FW::EventGenerator>(evgCfg, logLevel));

  } else {
    throw std::runtime_error("unknown event generator input: " + evgenInput);
  }

  // Output directory
  std::string outputDir = vm["output-dir"].template as<std::string>();

  // Write particles as CSV files
  if (vm["output-csv"].template as<bool>()) {
    FW::Csv::CsvParticleWriter::Config pWriterCsvConfig;
    pWriterCsvConfig.inputEvent = "particles";
    pWriterCsvConfig.outputDir  = outputDir;
    pWriterCsvConfig.outputStem = "particles";
    sequencer.addWriter(
        std::make_shared<FW::Csv::CsvParticleWriter>(pWriterCsvConfig));
  }

  // Write particles as ROOT file
  if (vm["output-root"].template as<bool>()) {
    // Write particles as ROOT TTree
    FW::Root::RootParticleWriter::Config pWriterRootConfig;
    pWriterRootConfig.collection = "particles";
    pWriterRootConfig.filePath   = FW::joinPaths(outputDir, "particles.root");
    pWriterRootConfig.treeName   = "particles";
    pWriterRootConfig.barcodeSvc = barcodeSvc;
    sequencer.addWriter(
        std::make_shared<FW::Root::RootParticleWriter>(pWriterRootConfig));
  }
}
