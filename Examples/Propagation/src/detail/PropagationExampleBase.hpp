// This file is part of the Acts project.
//
// Copyright (C) 2017 Acts project team
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#pragma once

#include <memory>
#include <boost/program_options.hpp>
#include "ACTFW/Common/CommonOptions.hpp"
#include "ACTFW/Common/GeometryOptions.hpp"
#include "ACTFW/Common/MaterialOptions.hpp"
#include "ACTFW/Common/OutputOptions.hpp"
#include "ACTFW/Framework/Sequencer.hpp"
#include "ACTFW/Plugins/BField/BFieldOptions.hpp"
#include "ACTFW/Plugins/BField/ScalableBField.hpp"
#include "ACTFW/Plugins/Obj/ObjPropagationStepsWriter.hpp"
#include "ACTFW/Plugins/Root/RootPropagationStepsWriter.hpp"
#include "ACTFW/Propagation/PropagationAlgorithm.hpp"
#include "ACTFW/Propagation/PropagationOptions.hpp"
#include "ACTFW/Random/RandomNumbersOptions.hpp"
#include "ACTFW/Random/RandomNumbersSvc.hpp"
#include "ACTFW/Utilities/Paths.hpp"
#include "Acts/Detector/TrackingGeometry.hpp"
#include "Acts/Extrapolator/Navigator.hpp"
#include "Acts/MagneticField/ConstantBField.hpp"
#include "Acts/MagneticField/InterpolatedBFieldMap.hpp"
#include "Acts/MagneticField/SharedBField.hpp"
#include "Acts/Material/IMaterialDecorator.hpp"
#include "Acts/Propagator/EigenStepper.hpp"
#include "Acts/Propagator/Propagator.hpp"
#include "Acts/Propagator/StraightLineStepper.hpp"

/// @brief Propgation setup
///
/// @tparam sequencer_t Type of the sequencer of the framework
/// @tparam bfield_t Type of the magnetic field
///
/// @param sequencer The framework sequencer, Propgation algorithm to be added
/// @param bfield The bfield object needed for the Stepper & propagagor
/// @param vm The program options for the log file
/// @param randomNumberSvc The framework random number engine
/// @param tGeometry The TrackingGeometry object
///
/// @return a process code
template <typename sequencer_t, typename bfield_t>
FW::ProcessCode
setupPropagation(sequencer_t&                                  sequencer,
                 bfield_t                                      bfield,
                 po::variables_map&                            vm,
                 std::shared_ptr<FW::RandomNumbersSvc>         randomNumberSvc,
                 std::shared_ptr<const Acts::TrackingGeometry> tGeometry)
{
  // Get the log level
  auto logLevel = FW::Options::readLogLevel(vm);

  // Get a Navigator
  Acts::Navigator navigator(tGeometry);

  // Resolve the bfield map template and create the propgator
  using Stepper    = Acts::EigenStepper<bfield_t>;
  using Propagator = Acts::Propagator<Stepper, Acts::Navigator>;
  Stepper    stepper(std::move(bfield));
  Propagator propagator(std::move(stepper), std::move(navigator));

  // Read the propagation config and create the algorithms
  auto pAlgConfig = FW::Options::readPropagationConfig(vm, propagator);
  pAlgConfig.randomNumberSvc = randomNumberSvc;
  sequencer.addAlgorithm(std::make_shared<FW::PropagationAlgorithm<Propagator>>(
      pAlgConfig, logLevel));

  return FW::ProcessCode::SUCCESS;
}

/// @brief Straight Line Propgation setup
///
/// @tparam sequencer_t Type of the sequencer of the framework
///
/// @param sequencer The framework sequencer, Propgation algorithm to be added
/// @param vm The program options for the log file
/// @param randomNumberSvc The framework random number engine
/// @param tGeometry The TrackingGeometry object
///
/// @return a process code
template <typename sequencer_t>
FW::ProcessCode
setupStraightLinePropgation(
    sequencer_t&                                  sequencer,
    boost::program_options::variables_map&        vm,
    std::shared_ptr<FW::RandomNumbersSvc>         randomNumberSvc,
    std::shared_ptr<const Acts::TrackingGeometry> tGeometry)
{
  // Get the log level
  auto logLevel = FW::Options::readLogLevel(vm);

  // Get a Navigator
  Acts::Navigator navigator(tGeometry);

  // Straight line stepper
  using SlStepper  = Acts::StraightLineStepper;
  using Propagator = Acts::Propagator<SlStepper, Acts::Navigator>;
  // Make stepper and propagator
  SlStepper  stepper;
  Propagator propagator(std::move(stepper), std::move(navigator));

  // Read the propagation config and create the algorithms
  auto pAlgConfig = FW::Options::readPropagationConfig(vm, propagator);
  pAlgConfig.randomNumberSvc = randomNumberSvc;
  sequencer.addAlgorithm(std::make_shared<FW::PropagationAlgorithm<Propagator>>(
      pAlgConfig, logLevel));

  return FW::ProcessCode::SUCCESS;
}

/// The Propagation example
///
/// @tparam options_setup_t are the callable example options
/// @tparam geometry_setup_t Type of the geometry getter struct
///
/// @param argc the number of argumetns of the call
/// @param argv the argument list
/// @param optionsSetup is a callable options struct
/// @param geometrySetup is a callable geometry getter
///
template <typename options_setup_t, typename geometry_setup_t>
int
propagationExample(int               argc,
                   char*             argv[],
                   options_setup_t&  optionsSetup,
                   geometry_setup_t& geometrySetup)
{

  // Create the config object for the sequencer
  FW::Sequencer::Config seqConfig;
  // Now create the sequencer
  FW::Sequencer sequencer(seqConfig);

  // Declare the supported program options.
  po::options_description desc("Allowed options");
  // Add the common options
  FW::Options::addCommonOptions<po::options_description>(desc);
  // Add the geometry options
  FW::Options::addGeometryOptions<po::options_description>(desc);
  // Add the material options
  FW::Options::addMaterialOptions<po::options_description>(desc);
  // Add the bfield options
  FW::Options::addBFieldOptions<po::options_description>(desc);
  // Add the random number options
  FW::Options::addRandomNumbersOptions<po::options_description>(desc);
  // Add the fatras options
  FW::Options::addPropagationOptions<po::options_description>(desc);
  // Add the output options
  FW::Options::addOutputOptions<po::options_description>(desc);

  // Add specific options for this geometry
  optionsSetup(desc);
  auto vm = FW::Options::parse(desc, argc, argv);
  if (vm.empty()) {
    return EXIT_FAILURE;
  }

  // Now read the standard options
  auto logLevel = FW::Options::readLogLevel<po::variables_map>(vm);
  auto nEvents  = FW::Options::readNumberOfEvents<po::variables_map>(vm);

  // Material loading from external source
  auto mDecorator = FW::Options::readMaterialDecorator<po::variables_map>(vm);

  auto geometry          = geometrySetup(vm, mDecorator);
  auto tGeometry         = geometry.first;
  auto contextDecorators = geometry.second;

  // Add it to the sequencer
  for (auto cdr : contextDecorators) {
    sequencer.addContextDecorator(cdr);
  }

  // Create the random number engine
  auto randomNumberSvcCfg = FW::Options::readRandomNumbersConfig(vm);
  auto randomNumberSvc
      = std::make_shared<FW::RandomNumbersSvc>(randomNumberSvcCfg);
  // Add it to the sequencer
  sequencer.addService(randomNumberSvc);

  // Create BField service
  auto bField  = FW::Options::readBField(vm);
  auto field2D = std::get<std::shared_ptr<InterpolatedBFieldMap2D>>(bField);
  auto field3D = std::get<std::shared_ptr<InterpolatedBFieldMap3D>>(bField);

  if (vm["prop-stepper"].template as<int>() == 0) {
    // Straight line stepper was chosen
    setupStraightLinePropgation(sequencer, vm, randomNumberSvc, tGeometry);
  } else if (field2D) {
    // Define the interpolated b-field: 2D
    using BField = Acts::SharedBField<InterpolatedBFieldMap2D>;
    BField fieldMap(field2D);
    setupPropagation(sequencer, fieldMap, vm, randomNumberSvc, tGeometry);
  } else if (field3D) {
    // Define the interpolated b-field: 3D
    using BField = Acts::SharedBField<InterpolatedBFieldMap3D>;
    BField fieldMap(field3D);
    setupPropagation(sequencer, fieldMap, vm, randomNumberSvc, tGeometry);
  } else if (vm["bf-context-scalable"].template as<bool>()) {
    using SField = FW::BField::ScalableBField;
    SField fieldMap(*std::get<std::shared_ptr<SField>>(bField));
    setupPropagation(sequencer, fieldMap, vm, randomNumberSvc, tGeometry);
  } else {
    // Create the constant  field
    using CField = Acts::ConstantBField;
    CField fieldMap(*std::get<std::shared_ptr<CField>>(bField));
    setupPropagation(sequencer, fieldMap, vm, randomNumberSvc, tGeometry);
  }

  // ---------------------------------------------------------------------------------
  // Output directory
  std::string outputDir    = vm["output-dir"].template as<std::string>();
  auto        psCollection = vm["prop-step-collection"].as<std::string>();

  if (vm["output-root"].template as<bool>()) {
    // Write the propagation steps as ROOT TTree
    FW::Root::RootPropagationStepsWriter::Config pstepWriterRootConfig;
    pstepWriterRootConfig.collection = psCollection;
    pstepWriterRootConfig.filePath
        = FW::joinPaths(outputDir, psCollection + ".root");
    sequencer.addWriter(std::make_shared<FW::Root::RootPropagationStepsWriter>(
        pstepWriterRootConfig));
  }

  if (vm["output-obj"].template as<bool>()) {

    using PropagationSteps = Acts::detail::Step;
    using ObjPropagationStepsWriter
        = FW::Obj::ObjPropagationStepsWriter<PropagationSteps>;

    // Write the propagation steps as Obj TTree
    ObjPropagationStepsWriter::Config pstepWriterObjConfig;
    pstepWriterObjConfig.collection = psCollection;
    pstepWriterObjConfig.outputDir  = outputDir;
    sequencer.addWriter(
        std::make_shared<ObjPropagationStepsWriter>(pstepWriterObjConfig));
  }

  return sequencer.run();
}
