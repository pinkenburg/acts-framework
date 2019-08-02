// This file is part of the Acts project.
//
// Copyright (C) 2019 Acts project team
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#include "Acts/Plugins/DD4hep/ActsExtension.hpp"
#include "Acts/Plugins/DD4hep/IActsExtension.hpp"
#include "DD4hep/DetFactoryHelper.h"

using namespace std;
using namespace dd4hep;

/**
 Constructor for a cylindrical barrel volume, possibly containing layers and the
 layers possibly containing modules.
 */

static Ref_t
create_element(Detector& lcdd, xml_h xml, SensitiveDetector sens)
{
  xml_det_t x_det      = xml;
  string    barrelName = x_det.nameStr();
  // Make DetElement
  DetElement barrelDetector(barrelName, x_det.id());
  // add Extension to Detlement for the RecoGeometry
  Acts::ActsExtension::Config volConfig;
  volConfig.isBarrel             = true;
  Acts::ActsExtension* detvolume = new Acts::ActsExtension(volConfig);
  barrelDetector.addExtension<Acts::IActsExtension>(detvolume);

  // Make Volume
  dd4hep::xml::Dimension x_det_dim(x_det.dimensions());
  Tube   barrelShape(x_det_dim.rmin(), x_det_dim.rmax(), x_det_dim.dz());
  Volume barrelVolume(
      barrelName, barrelShape, lcdd.air());  // air at the moment change later
  barrelVolume.setVisAttributes(lcdd, x_det.visStr());

  unsigned int layerNum = 0;

  // Loop over the layers and build them
  for (xml_coll_t j(xml, _U(layer)); j; ++j, ++layerNum) {
    // Get the layer xml configuration
    xml_comp_t x_layer = j;
    double     rmin    = x_layer.rmin();
    double     rmax    = x_layer.rmax();
    // Create Volume for Layer
    string layerName = barrelName + _toString((int)layerNum, "layer%d");
    Volume layerVolume(layerName,
                       Tube(rmin, rmax, x_layer.dz()),
                       lcdd.material(x_layer.materialStr()));
    DetElement layerElement(barrelDetector, layerName, layerNum);
    // Visualization
    layerVolume.setVisAttributes(lcdd, x_layer.visStr());

    // Place the support cylinder
    if (x_layer.hasChild(_U(support))) {
      xml_comp_t x_support = x_layer.child(_U(support));

      // Create the volume of the support structure
      Volume supportVolume(
          "SupportStructure",
          Tube(x_support.rmin(), x_support.rmax(), x_support.dz()),
          lcdd.material(x_support.materialStr()));
      supportVolume.setVisAttributes(lcdd, x_support.visStr());
      // Place the support structure
      PlacedVolume placedSupport = layerVolume.placeVolume(supportVolume);
    }

    // Construct the volume
    if (x_layer.hasChild(_U(module))) {

      xml_comp_t x_module = x_layer.child(_U(module));
      // create the module volume and its corresponing component volumes first
      Volume moduleVolume(
          "module",
          Box(0.5 * x_module.dx(), 0.5 * x_module.dy(), 0.5 * x_module.dz()),
          lcdd.material(x_module.materialStr()));
      // Visualization
      moduleVolume.setVisAttributes(lcdd, x_module.visStr());

      xml_comp_t   x_mod_placement = x_module.child(_U(parameters));
      unsigned int nphi            = x_mod_placement.nphi();
      double       phi0            = x_mod_placement.phi0();
      double       phiTilt         = x_mod_placement.phi_tilt();
      double       r               = x_mod_placement.r();
      double       deltaPhi        = 2 * M_PI / nphi;

      // Place the components inside the module
      unsigned int compNum = 0;

      for (xml_coll_t c(x_module, _U(component)); c; ++c, ++compNum) {
        xml_comp_t x_comp = c;
        // Component volume
        string componentName = _toString((int)compNum, "component%d");
        Volume componentVolume(
            componentName,
            Box(0.5 * x_comp.dx(), 0.5 * x_comp.dy(), 0.5 * x_comp.dz()),
            lcdd.material(x_comp.materialStr()));
        // Visualization
        componentVolume.setVisAttributes(lcdd, x_comp.visStr());
        // Place Module Box Volumes in layer
        PlacedVolume placedComponent = moduleVolume.placeVolume(
            componentVolume,
            Position(x_comp.x_offset(), x_comp.y_offset(), x_comp.z_offset()));
      }

      // Add cooling pipe
      if (x_module.hasChild(_U(tubs))) {
        xml_comp_t x_tubs = x_module.child(_U(tubs));
        Volume     pipeVolume("CoolingPipe",
                          Tube(x_tubs.rmin(), x_tubs.rmax(), x_tubs.length()),
                          lcdd.material(x_tubs.materialStr()));
        pipeVolume.setVisAttributes(lcdd, x_tubs.visStr());
        // Place the cooling pipe into the module
        PlacedVolume placedPipe = moduleVolume.placeVolume(
            pipeVolume,
            Transform3D(RotationX(0.5 * M_PI) * RotationY(0.5 * M_PI),
                        Position(x_tubs.x_offset(),
                                 x_tubs.y_offset(),
                                 x_tubs.z_offset())));
        // placedPipe.addPhysVolID("component", comp_num);
        // comp_num++; //<- check if we need this
      }

      // Add mount
      if (x_module.hasChild(_U(anchor))) {
        xml_comp_t x_trd = x_module.child(_U(anchor));
        // create the two shapes first
        Trapezoid mountShape(
            x_trd.x1(), x_trd.x2(), x_trd.length(), x_trd.length(), x_trd.dz());

        Volume mountVolume(
            "ModuleMount", mountShape, lcdd.material(x_trd.materialStr()));

        // Place the mount onto the module
        PlacedVolume placedMount
            = moduleVolume.placeVolume(mountVolume,
                                       Transform3D(RotationZ(0.5 * M_PI),
                                                   Position(x_trd.x_offset(),
                                                            x_trd.y_offset(),
                                                            x_trd.z_offset())));
        // placedMount.addPhysVolID("component", comp_num);
        // comp_num++; //<- check if we need this
      }

      // Add cable
      if (x_module.hasChild(_U(box))) {
        xml_comp_t x_cab = x_module.child(_U(box));
        Volume     cableVolume(
            "Cable",
            Box(0.5 * x_cab.dx(), 0.5 * x_cab.dy(), 0.5 * x_cab.dz()),
            lcdd.material(x_cab.materialStr()));
        // Visualization
        cableVolume.setVisAttributes(lcdd, x_cab.visStr());
        // Place Module Box Volumes in layer
        PlacedVolume placedCable
            = moduleVolume.placeVolume(cableVolume,
                                       Transform3D(RotationX(x_cab.alpha()),
                                                   Position(x_cab.x_offset(),
                                                            x_cab.y_offset(),
                                                            x_cab.z_offset())));
      }

      // Place the modules
      for (int iphi = 0; iphi < nphi; ++iphi) {

        double phi = phi0 + iphi * deltaPhi;

        string   moduleName = layerName + _toString((int)iphi, "module%d");
        Position trans(r * cos(phi), r * sin(phi), 0.);
        // create detector element
        DetElement moduleElement(layerElement, moduleName, iphi);

        // Place Module Box Volumes in layer
        PlacedVolume placedModule = layerVolume.placeVolume(
            moduleVolume,
            Transform3D(RotationY(0.5 * M_PI) * RotationX(-phi - phiTilt),
                        trans));
        placedModule.addPhysVolID("module", iphi);
        // assign module DetElement to the placed module volume
        moduleElement.setPlacement(placedModule);
      }
    }

    // Configure the ACTS extension
    Acts::ActsExtension::Config layerConfig;
    layerConfig.isLayer = true;
    ///@todo re-enable material mapping
    // layConfig.materialBins1         = 100;
    // layConfig.materialBins2         = 100;
    // layConfig.layerMaterialPosition = Acts::LayerMaterialPos::inner;
    Acts::ActsExtension* layerExtension = new Acts::ActsExtension(layerConfig);
    layerElement.addExtension<Acts::IActsExtension>(layerExtension);
    // Place layer volume
    PlacedVolume placedLayer = barrelVolume.placeVolume(layerVolume);
    placedLayer.addPhysVolID("layer", layerNum);
    // Assign layer DetElement to layer volume
    layerElement.setPlacement(placedLayer);
  }

  // Place Volume
  Volume       motherVolume = lcdd.pickMotherVolume(barrelDetector);
  PlacedVolume placedTube   = motherVolume.placeVolume(barrelVolume);
  placedTube.addPhysVolID("barrel", barrelDetector.id());
  barrelDetector.setPlacement(placedTube);

  return barrelDetector;
}

DECLARE_DETELEMENT(DemonstratorBarrel, create_element)