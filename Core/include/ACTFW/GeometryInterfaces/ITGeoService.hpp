// This file is part of the ACTS project.
//
// Copyright (C) 2017 ACTS project team
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

///////////////////////////////////////////////////////////////////
// ITGeoService.hpp
///////////////////////////////////////////////////////////////////

#ifndef GEOMETRYINTERFACES_ITGEOSERVICE_H
#define GEOMETRYINTERFACES_ITGEOSERVICE_H

#include "ACTFW/Framework/IService.hpp"
#include "ACTFW/Framework/ProcessCode.hpp"

class TGeoNode;

namespace FW {
    
    
    /// @class ITGeoService
    ///
    /// The ITGeoService is the interface to return the TGeoGeometry.
    ///
    /// @TODO solve problem with double inheritance
    
    class ITGeoService {//: public IService {

    public:
        /// Virtual destructor
        virtual ~ITGeoService() = default;
        /// Access to the TGeo geometry
        /// @return The world TGeoNode (physical volume)
        virtual TGeoNode* tgeoGeometry() = 0;
        
    };
}
#endif // GEOMETRYINTERFACES_ITGEOSERVICE_H