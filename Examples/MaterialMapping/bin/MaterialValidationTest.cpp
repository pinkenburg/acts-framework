///////////////////////////////////////////////////////////////////
// GenatinoRecording.cpp
///////////////////////////////////////////////////////////////////

#include "ACTFW/Extrapolation/ExtrapolationUtils.hpp"
#include "ACTFW/Framework/Sequencer.hpp"
#include "../src/MaterialValidation.hpp"

int
main()
{ 
  // set geometry building logging level
  Acts::Logging::Level surfaceLogLevel = Acts::Logging::INFO;
  Acts::Logging::Level layerLogLevel   = Acts::Logging::INFO;
  Acts::Logging::Level volumeLogLevel  = Acts::Logging::INFO;
  // set extrapolation logging level
  Acts::Logging::Level eLogLevel       = Acts::Logging::INFO;
  
  // create the tracking geometry as a shared pointer
  std::shared_ptr<const Acts::TrackingGeometry> tGeometry
      = Acts::buildGenericDetector(
          surfaceLogLevel, layerLogLevel, volumeLogLevel, 3);

  // set up the magnetic field
  std::shared_ptr<Acts::ConstantBField> magField(
      new Acts::ConstantBField{{0., 0., 2.*Acts::units::_T}});  

  // EXTRAPOLATOR - set up the extrapolator
  std::shared_ptr<Acts::IExtrapolationEngine> extrapolationEngine
      = FWE::initExtrapolator(tGeometry, magField, eLogLevel);
  
  
  
  
  
  
  
  
}
  