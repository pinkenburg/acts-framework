//
//  ReadEvgenExample.cpp
//  ACTFW
//
//  Created by Andreas Salzburger on 11/05/16.
//

#include <memory>
#include "ACTFW/Framework/Algorithm.hpp"
#include "ACTFW/Framework/Sequencer.hpp"
#include "ACTFW/Framework/WhiteBoard.hpp"
#include "ACTFW/Random/RandomNumbersSvc.hpp"
#include "ACTFW/Root/ParticlePropertiesWriter.hpp"
#include "ACTFW/RootPythia8/ParticleGenerator.hpp"
#include "ACTFW/ReadEvgen/ReadEvgenAlgorithm.hpp"

// the main hello world executable
int
main(int argc, char* argv[])
{
  size_t nEvents = 500;

  // creating the data stores
  auto detectorStore = std::make_shared<FW::WhiteBoard>(
      Acts::getDefaultLogger("DetectorStore", Acts::Logging::INFO));

  // create a pythia generator for the hard scatter
  // process: HardQCD for the moment
  FWRootPythia8::ParticleGenerator::Config hsPythiaConfig;
  hsPythiaConfig.pdgBeam0       = 2212;
  hsPythiaConfig.pdgBeam1       = 2212;
  hsPythiaConfig.cmsEnergy      = 14000.;
  hsPythiaConfig.processStrings = {{"HardQCD:all = on"}};
  auto hsPythiaGenerator = std::make_shared<FWRootPythia8::ParticleGenerator>(
      hsPythiaConfig,
      Acts::getDefaultLogger("HardScatterParticleGenerator",
                             Acts::Logging::VERBOSE));

  // create a pythia generator for the pile-up
  // MinBias with SD, DD and ND
  FWRootPythia8::ParticleGenerator::Config puPythiaConfig;
  puPythiaConfig.pdgBeam0       = 2212;
  puPythiaConfig.pdgBeam1       = 2212;
  puPythiaConfig.cmsEnergy      = 14000.;
  puPythiaConfig.processStrings = {{"SoftQCD:all = on"}};

  auto puPythiaGenerator = std::make_shared<FWRootPythia8::ParticleGenerator>(
      puPythiaConfig,
      Acts::getDefaultLogger("PileUpParticleGenerator",
                             Acts::Logging::VERBOSE));

  // random number generation
  // Create the random number engine
  FW::RandomNumbersSvc::Config pileupNumbersCfg;
  pileupNumbersCfg.poisson_parameter = 200;
  std::shared_ptr<FW::RandomNumbersSvc> pileupNumbers(
      new FW::RandomNumbersSvc(pileupNumbersCfg));

  FW::RandomNumbersSvc::Config pileupVertexTCfg;
  pileupVertexTCfg.gauss_parameters = {{0., 0.015}};
  std::shared_ptr<FW::RandomNumbersSvc> pileupVertexT(
      new FW::RandomNumbersSvc(pileupVertexTCfg));

  FW::RandomNumbersSvc::Config pileupVertexZCfg;
  pileupVertexZCfg.gauss_parameters = {{0., 5.5}};
  std::shared_ptr<FW::RandomNumbersSvc> pileupVertexZ(
      new FW::RandomNumbersSvc(pileupVertexZCfg));

  // Write ROOT TTree
  FWRoot::ParticlePropertiesWriter::Config particleWriterConfig;
  particleWriterConfig.fileName = "$PWD/SimulationParticles.root";
  particleWriterConfig.treeName = "SimulationParticles.root";
  auto particleWriter = std::make_shared<FWRoot::ParticlePropertiesWriter>(
      particleWriterConfig);

  // get the read-in algorithm
  FWE::ReadEvgenAlgorithm::Config readEvgenCfg;

  readEvgenCfg.particleCollectionName = "SimulationParticles";
  /// the hard scatter reader
  readEvgenCfg.hardscatterParticleReader = hsPythiaGenerator;
  /// the pileup reader
  readEvgenCfg.pileupParticleReader = puPythiaGenerator;
  /// the number of pileup events
  readEvgenCfg.pileupType          = FW::Distribution::poisson;
  readEvgenCfg.pileupRandomNumbers = pileupNumbers;
  readEvgenCfg.pileupVertexDistT   = pileupVertexT;
  readEvgenCfg.pileupVertexDistZ   = pileupVertexZ;
  /// the job WhiteBoard
  readEvgenCfg.jBoard = detectorStore;
  /// set the particle writer
  readEvgenCfg.particleWriter = particleWriter;
  /// create the read Algorithm
  auto readEvgen = std::make_shared<FWE::ReadEvgenAlgorithm>(
      readEvgenCfg,
      Acts::getDefaultLogger("ReadEvgenAlgorithm", Acts::Logging::INFO));

  // create the config object for the sequencer
  FW::Sequencer::Config seqConfig;
  // now create the sequencer
  FW::Sequencer sequencer(seqConfig);
  sequencer.addServices(
      {particleWriter, pileupNumbers, pileupVertexT, pileupVertexZ});
  sequencer.addIOAlgorithms({readEvgen});
  sequencer.appendEventAlgorithms({});

  // initialize loop
  sequencer.initializeEventLoop();
  // run the loop
  sequencer.processEventLoop(nEvents);
  // finalize loop
  sequencer.finalizeEventLoop();
}
