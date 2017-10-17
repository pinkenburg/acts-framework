// This file is part of the ACTS project.
//
// Copyright (C) 2017 ACTS project team
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

//
//  layerMaterial.cpp
//
//
//  Created by Julia Hrdinka on 18/08/16.
//
//

#include <algorithm>
#include <iostream>
#include <vector>
#include "TFile.h"
#include "TH1F.h"
#include "TObject.h"
#include "TROOT.h"
#include "TTree.h"

// This root script generates material histograms and maps of a layer.
// It is foreseen to be used for output generated by the RootMaterialWriter or
// the RootMaterialStepWriter. For the indicated Layer it produces the
// histograms and maps of the material properties along the local coordiantes of
// the layer.
// It also writes the assigned and real global positions into histograms, if
// given.

void
layerMaterial(std::string inFile,
              std::string outFile,
              int         binsLoc1,
              int         binsLoc2,
              int         binsZ,
              float       minGlobZ,
              float       maxGlobZ,
              int         binsR,
              float       minGlobR,
              float       maxGlobR,
              std::string layerName)
{
  std::cout << "Opening file: " << inFile << std::endl;
  TFile  inputFile(inFile.c_str());
  TTree* layer = (TTree*)inputFile.Get(layerName.c_str());
  std::cout << "Reading tree: " << layerName << std::endl;

  std::vector<float>* loc0  = new std::vector<float>;
  std::vector<float>* loc1  = new std::vector<float>;
  std::vector<float>* A     = new std::vector<float>;
  std::vector<float>* Z     = new std::vector<float>;
  std::vector<float>* x0    = new std::vector<float>;
  std::vector<float>* l0    = new std::vector<float>;
  std::vector<float>* d     = new std::vector<float>;
  std::vector<float>* rho   = new std::vector<float>;
  std::vector<float>* dInX0 = new std::vector<float>;
  std::vector<float>* dInL0 = new std::vector<float>;
  std::vector<float>* t     = new std::vector<float>;

  std::vector<float>* globX = new std::vector<float>;
  std::vector<float>* globY = new std::vector<float>;
  std::vector<float>* globZ = new std::vector<float>;
  std::vector<float>* globR = new std::vector<float>;

  std::vector<float>* assignedGlobX = new std::vector<float>;
  std::vector<float>* assignedGlobY = new std::vector<float>;
  std::vector<float>* assignedGlobZ = new std::vector<float>;
  std::vector<float>* assignedGlobR = new std::vector<float>;

  layer->SetBranchAddress("loc0", &loc0);
  layer->SetBranchAddress("loc1", &loc1);
  layer->SetBranchAddress("A", &A);
  layer->SetBranchAddress("Z", &Z);
  layer->SetBranchAddress("x0", &x0);
  layer->SetBranchAddress("l0", &l0);
  layer->SetBranchAddress("thickness", &d);
  layer->SetBranchAddress("rho", &rho);
  layer->SetBranchAddress("tInX0", &dInX0);
  layer->SetBranchAddress("tInL0", &dInL0);
  layer->SetBranchAddress("thickness", &t);

  if (layer->FindBranch("globX") && layer->FindBranch("globY")
      && layer->FindBranch("globZ")
      && layer->FindBranch("globR")) {
    layer->SetBranchAddress("globX", &globX);
    layer->SetBranchAddress("globY", &globY);
    layer->SetBranchAddress("globZ", &globZ);
    layer->SetBranchAddress("globR", &globR);
  }

  if (layer->FindBranch("assignedGlobX") && layer->FindBranch("assignedGlobY")
      && layer->FindBranch("assignedGlobZ")
      && layer->FindBranch("assignedGlobR")) {
    layer->SetBranchAddress("assignedGlobX", &assignedGlobX);
    layer->SetBranchAddress("assignedGlobY", &assignedGlobY);
    layer->SetBranchAddress("assignedGlobZ", &assignedGlobZ);
    layer->SetBranchAddress("assignedGlobR", &assignedGlobR);
  }
  layer->GetEntry(0);

  // get minima and maxima for different layers
  auto  minmax0 = std::minmax_element(loc0->begin(), loc0->end());
  float min0    = *minmax0.first;
  float max0    = *minmax0.second;

  auto  minmax1 = std::minmax_element(loc1->begin(), loc1->end());
  float min1    = *minmax1.first;
  float max1    = *minmax1.second;

  inputFile.Close();
  std::cout << "Creating new output file: " << outFile << " and writing "
                                                          "material maps"
            << std::endl;
  TFile       outputFile(outFile.c_str(), "update");
  TDirectory* dir = outputFile.mkdir(layerName.c_str());
  dir->cd();
  // thickness in X0
  TProfile* dInX0_loc1
      = new TProfile("dInX0_loc1", "dInX0_loc1", binsLoc1, min1, max1);
  TProfile* dInX0_loc0
      = new TProfile("dInX0_loc0", "dInX0_loc0", binsLoc1, min0, max0);
  TProfile2D* dInX0_map = new TProfile2D(
      "dInX0", "dInX0", binsLoc1, min1, max1, binsLoc1, min0, max0);
  // thickness in L0
  TProfile* dInL0_loc1
      = new TProfile("dInL0_loc1", "dInL0_loc1", binsLoc2, min1, max1);
  TProfile* dInL0_loc0
      = new TProfile("dInL0_loc0", "dInL0_loc0", binsLoc1, min0, max0);
  TProfile2D* dInL0_map = new TProfile2D(
      "dInL0", "dInL0", binsLoc2, min1, max1, binsLoc1, min0, max0);
  // A
  TProfile*   A_loc1 = new TProfile("A_loc1", "A_loc1", binsLoc2, min1, max1);
  TProfile*   A_loc0 = new TProfile("A_loc0", "A_loc0", binsLoc1, min0, max0);
  TProfile2D* A_map
      = new TProfile2D("A", "A", binsLoc2, min1, max1, binsLoc1, min0, max0);
  // Z
  TProfile*   Z_loc1 = new TProfile("Z_loc1", "Z_loc1", binsLoc2, min1, max1);
  TProfile*   Z_loc0 = new TProfile("Z_loc0", "Z_loc0", binsLoc1, min0, max0);
  TProfile2D* Z_map
      = new TProfile2D("Z", "Z", binsLoc2, min1, max1, binsLoc1, min0, max0);
  // Rho
  TProfile* rho_loc1
      = new TProfile("rho_loc1", "rho_loc1", binsLoc2, min1, max1);
  TProfile* rho_loc0
      = new TProfile("rho_loc0", "rho_loc0", binsLoc1, min0, max0);
  TProfile2D* rho_map = new TProfile2D(
      "rho", "rho", binsLoc2, min1, max1, binsLoc1, min0, max0);
  // x0
  TProfile* x0_loc1 = new TProfile("x0_loc1", "x0_loc1", binsLoc2, min1, max1);
  TProfile* x0_loc0 = new TProfile("x0_loc0", "x0_loc0", binsLoc1, min0, max0);
  TProfile2D* x0_map
      = new TProfile2D("x0", "x0", binsLoc2, min1, max1, binsLoc1, min0, max0);
  // l0
  TProfile* l0_loc1 = new TProfile("l0_loc1", "l0_loc1", binsLoc2, min1, max1);
  TProfile* l0_loc0 = new TProfile("l0_loc0", "l0_loc0", binsLoc1, min0, max0);
  TProfile2D* l0_map
      = new TProfile2D("l0", "l0", binsLoc2, min1, max1, binsLoc1, min0, max0);

  // thickness
  TProfile*   t_loc1 = new TProfile("t_loc1", "t_loc1", binsLoc2, min1, max1);
  TProfile*   t_loc0 = new TProfile("t_loc0", "t_loc0", binsLoc1, min0, max0);
  TProfile2D* t_map
      = new TProfile2D("t", "t", binsLoc2, min1, max1, binsLoc1, min0, max0);

  // global
  TH2F* glob_r_z = new TH2F(
      "r_z", "r_z", binsZ, minGlobZ, maxGlobZ, binsR, minGlobR, maxGlobR);
  TH2F* assigned_r_z = new TH2F("r_z_assigned",
                                "r_z_assigned",
                                binsZ,
                                minGlobZ,
                                maxGlobZ,
                                binsR,
                                minGlobR,
                                maxGlobR);

  TH2F* glob_x_y = new TH2F(
      "x_y", "x_y", binsR, -maxGlobR, maxGlobR, binsR, -maxGlobR, maxGlobR);
  TH2F* assigned_x_y = new TH2F("x_y_assigned",
                                "x_y_assigned",
                                binsR,
                                -maxGlobR,
                                maxGlobR,
                                binsR,
                                -maxGlobR,
                                maxGlobR);

  size_t nEntries = loc1->size();
  for (int i = 0; i < nEntries; i++) {
    // A
    A_loc1->Fill(loc1->at(i), A->at(i));
    A_loc0->Fill(loc0->at(i), A->at(i));
    A_map->Fill(loc1->at(i), loc0->at(i), A->at(i));
    // Z
    Z_loc1->Fill(loc1->at(i), Z->at(i));
    Z_loc0->Fill(loc0->at(i), Z->at(i));
    Z_map->Fill(loc1->at(i), loc0->at(i), Z->at(i));
    // x0
    x0_loc1->Fill(loc1->at(i), x0->at(i));
    x0_loc0->Fill(loc0->at(i), x0->at(i));
    x0_map->Fill(loc1->at(i), loc0->at(i), x0->at(i));
    // l0
    l0_loc1->Fill(loc1->at(i), l0->at(i));
    l0_loc0->Fill(loc0->at(i), l0->at(i));
    l0_map->Fill(loc1->at(i), loc0->at(i), l0->at(i));
    // rho
    rho_loc1->Fill(loc1->at(i), rho->at(i));
    rho_loc0->Fill(loc0->at(i), rho->at(i));
    rho_map->Fill(loc1->at(i), loc0->at(i), rho->at(i));
    // thickness in X0
    dInX0_loc1->Fill(loc1->at(i), dInX0->at(i));
    dInX0_loc0->Fill(loc0->at(i), dInX0->at(i));
    dInX0_map->Fill(loc1->at(i), loc0->at(i), dInX0->at(i));
    // thickness in L0
    dInL0_loc1->Fill(loc1->at(i), dInL0->at(i));
    dInL0_loc0->Fill(loc0->at(i), dInL0->at(i));
    dInL0_map->Fill(loc1->at(i), loc0->at(i), dInL0->at(i));
    // thickness
    t_loc1->Fill(loc1->at(i), t->at(i));
    t_loc0->Fill(loc0->at(i), t->at(i));
    t_map->Fill(loc1->at(i), loc0->at(i), t->at(i));

    // fill global r/z
    if (globZ->size() && globR->size())
      glob_r_z->Fill(globZ->at(i), globR->at(i));

    if (assignedGlobZ->size() && assignedGlobR->size())
      assigned_r_z->Fill(assignedGlobZ->at(i), assignedGlobR->at(i));

    // fill global x/y
    if (globX->size() && globY->size())
      glob_x_y->Fill(globX->at(i), globY->at(i));

    if (assignedGlobX->size() && assignedGlobY->size())
      assigned_x_y->Fill(assignedGlobX->at(i), assignedGlobY->at(i));
  }

  gStyle->SetOptStat(0);
  // A
  A_loc1->Write();
  A_loc0->Write();
  A_map->Write();
  delete A_loc1;
  delete A_loc0;
  delete A_map;
  // Z
  Z_loc1->Write();
  Z_loc0->Write();
  Z_map->Write();
  delete Z_loc1;
  delete Z_loc0;
  delete Z_map;
  // x0
  x0_loc1->Write();
  x0_loc0->Write();
  x0_map->Write();
  delete x0_loc1;
  delete x0_loc0;
  delete x0_map;
  // l0
  l0_loc1->Write();
  l0_loc0->Write();
  l0_map->Write();
  delete l0_loc1;
  delete l0_loc0;
  delete l0_map;
  // rho
  rho_loc1->Write();
  rho_loc0->Write();
  rho_map->Write();
  delete rho_loc1;
  delete rho_loc0;
  delete rho_map;
  // thickness in X0
  dInX0_loc1->Write();
  dInX0_loc0->Write();
  dInX0_map->Write();
  delete dInX0_loc1;
  delete dInX0_loc0;
  delete dInX0_map;
  // thickness in L0
  dInL0_loc1->Write();
  dInL0_loc0->Write();
  dInL0_map->Write();
  delete dInL0_loc1;
  delete dInL0_loc0;
  delete dInL0_map;
  // thickness
  t_loc1->Write();
  t_loc0->Write();
  t_map->Write();

  delete loc0;
  delete loc1;
  delete A;
  delete Z;
  delete x0;
  delete l0;
  delete d;
  delete rho;
  delete dInX0;
  delete dInL0;
  delete t;

  glob_r_z->Write();
  assigned_r_z->Write();
  glob_x_y->Write();
  assigned_x_y->Write();
  delete glob_r_z;
  delete assigned_r_z;
  delete glob_x_y;
  delete assigned_x_y;
  outputFile.Close();
}
