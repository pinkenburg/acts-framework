// This file is part of the ACTS project.
//
// Copyright (C) 2017 ACTS project team
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

//  IPlanarClusterWriter.h
//  ACTS-Development
//
//  Created by Andreas Salzburger on 23/05/16.
//
//
#ifndef ACTFW_WRITERS_IEventDataWriterTT_H
#define ACTFW_WRITERS_IEventDataWriterTT_H

#include "ACTFW/Framework/IService.hpp"
#include "ACTFW/EventData/DataContainers.hpp"
#include "ACTS/Utilities/GeometryID.hpp"

namespace FW {

/// @class IEventDataWriterT
///
/// Interface class for writing EventData are ordered
/// in DataContainers
///
template <class T> class IEventDataWriterT : public IService
{
public:
  /// The write interface
  /// @param dd is the detector data in the dedicated container
  /// @return is a ProcessCode indicating success/failure
  virtual ProcessCode
  write(const DetectorData<geo_id_value, T>& dd) = 0;
};

}

#endif  // ACTFW_WRITERS_IEventDataWriterTT_H
