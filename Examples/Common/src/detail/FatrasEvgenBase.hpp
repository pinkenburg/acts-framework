// This file is part of the Acts project.
//
// Copyright (C) 2018 CERN for the benefit of the Acts project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#pragma once

#include <memory>

#include "ACTFW/Utilities/OptionsFwd.hpp"

namespace FW {
class Sequencer;
class RandomNumbers;
}  // namespace FW

void
setupEvgenInput(boost::program_options::variables_map& vm,
                FW::Sequencer&                         sequencer,
                std::shared_ptr<FW::RandomNumbers>     randomNumberSvc);
