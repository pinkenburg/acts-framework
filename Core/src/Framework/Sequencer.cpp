// This file is part of the Acts project.
//
// Copyright (C) 2017 Acts project team
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#include "ACTFW/Framework/Sequencer.hpp"

#include <algorithm>
#include <cstdlib>
#include <exception>

#include <TROOT.h>
#include <tbb/tbb.h>

#include "ACTFW/Framework/ProcessCode.hpp"
#include "ACTFW/Framework/WhiteBoard.hpp"

FW::Sequencer::Sequencer(const Sequencer::Config&            cfg,
                         std::unique_ptr<const Acts::Logger> logger)
  : m_cfg(cfg), m_logger(std::move(logger))
{
  // automatically determine the number of concurrent threads to use
  if (m_cfg.numThreads < 0) {
    const char* numThreadsEnv = getenv("ACTSFW_NUM_THREADS");
    if (numThreadsEnv) {
      m_cfg.numThreads = std::stoi(numThreadsEnv);
    } else {
      m_cfg.numThreads = tbb::task_scheduler_init::default_num_threads();
    }
  }
  ROOT::EnableThreadSafety();
}

void
FW::Sequencer::addService(std::shared_ptr<IService> service)
{
  if (not service) {
    throw std::invalid_argument("Can not add empty/NULL service");
  }
  m_services.push_back(std::move(service));
  ACTS_INFO("Added service '" << m_services.back()->name() << "'");
}

void
FW::Sequencer::addContextDecorator(std::shared_ptr<IContextDecorator> decorator)
{
  if (not decorator) {
    throw std::invalid_argument("Can not add empty/NULL context decorator");
  }
  m_decorators.push_back(std::move(decorator));
  ACTS_INFO("Added context decarator '" << m_decorators.back()->name() << "'");
}

void
FW::Sequencer::addReader(std::shared_ptr<IReader> reader)
{
  if (not reader) {
    throw std::invalid_argument("Can not add empty/NULL reader");
  }
  m_readers.push_back(std::move(reader));
  ACTS_INFO("Added reader '" << m_readers.back()->name() << "'");
}

void
FW::Sequencer::addAlgorithm(std::shared_ptr<IAlgorithm> algorithm)
{
  if (not algorithm) {
    throw std::invalid_argument("Can not add empty/NULL algorithm");
  }
  m_algorithms.push_back(std::move(algorithm));
  ACTS_INFO("Added algorithm '" << m_algorithms.back()->name() << "'");
}

void
FW::Sequencer::addWriter(std::shared_ptr<IWriter> writer)
{
  if (not writer) {
    throw std::invalid_argument("Can not add empty/NULL writer");
  }
  m_writers.push_back(std::move(writer));
  ACTS_INFO("Added writer '" << m_writers.back()->name() << "'");
}

int
FW::Sequencer::run(boost::optional<size_t> events, size_t skip)
{
  ACTS_INFO("Starting event loop with " << m_cfg.numThreads << " threads");
  ACTS_INFO("  " << m_services.size() << " services");
  ACTS_INFO("  " << m_decorators.size() << " context decorators");
  ACTS_INFO("  " << m_readers.size() << " readers");
  ACTS_INFO("  " << m_algorithms.size() << " algorithms");
  ACTS_INFO("  " << m_writers.size() << " writers");

  // The number of events to be processed
  size_t numEvents = 0;

  // There are two possibilities how the event loop can be steered
  // 1) By the number of given events
  // 2) By the number of events given by the readers
  // Calulate minimum and maximum of events to be read in
  auto min = std::min_element(
      m_readers.begin(), m_readers.end(), [](const auto& a, const auto& b) {
        return (a->numEvents() < b->numEvents());
      });
  // Check if number of events is given by the reader(s)
  if (min == m_readers.end()) {
    // 1) In case there are no readers, no event should be skipped
    if (skip != 0) {
      ACTS_ERROR(
          "Number of skipped events given although no readers present. Abort");
      return EXIT_FAILURE;
    }
    // Number of events is not given by readers, in this case the parameter
    // 'events' must be specified - Abort, if this is not the case
    if (!events) {
      ACTS_ERROR("Number of events not specified. Abort");
      return EXIT_FAILURE;
    }
    // 'events' is specified, set 'numEvents'
    numEvents = *events;
  } else {
    // 2) Number of events given by reader(s)
    numEvents = ((*min)->numEvents());
    // Check if the number of skipped events is smaller then the overall number
    // if events
    if (skip > numEvents) {
      ACTS_ERROR("Number of events to be skipped > than total number of "
                 "events. Abort");
      return EXIT_FAILURE;
    }
    // The total number of events is the maximum number of events minus the
    // number of skipped evebts
    numEvents -= skip;
    // Check if user wants to process less events than given by the reader
    if (events && (*events) < numEvents) {
      numEvents = *events;
    }
  }

  // Execute the event loop
  tbb::task_scheduler_init init(m_cfg.numThreads);
  tbb::parallel_for(
      tbb::blocked_range<size_t>(skip, numEvents + skip),
      [&](const tbb::blocked_range<size_t>& r) {
        for (size_t event = r.begin(); event != r.end(); ++event) {
          ACTS_INFO("start event " << event);

          // Setup the event and algorithm context
          WhiteBoard eventStore(Acts::getDefaultLogger(
              "EventStore#" + std::to_string(event), m_cfg.eventStoreLogLevel));

          // This makes sure that each algorithm can have it's own
          // view of the algorithm context, e.g. for random number
          // initialization
          //
          // If we ever wanted to run algorithms in parallel, this needs to be
          // changed
          // to Algorithm context copies
          size_t ialg = 0;

          // Create the Algorithm context
          //
          // If we ever wanted to run algorithms in parallel, this needs to be
          // changed
          // to Algorithm context copies
          AlgorithmContext context(ialg++, event, eventStore);

          /// Decorate the context
          for (auto& cdr : m_decorators) {
            if (cdr->decorate(context) != ProcessCode::SUCCESS) {
              throw std::runtime_error("Failed to decorate event context");
            }
          }
          // Read everything in
          for (auto& rdr : m_readers) {
            if (rdr->read(++context) != ProcessCode::SUCCESS) {
              throw std::runtime_error("Failed to read input data");
            }
          }
          // Process all algorithms
          for (auto& alg : m_algorithms) {
            if (alg->execute(++context) != ProcessCode::SUCCESS) {
              throw std::runtime_error("Failed to process event data");
            }
          }
          // Write out results
          for (auto& wrt : m_writers) {
            if (wrt->write(++context) != ProcessCode::SUCCESS) {
              throw std::runtime_error("Failed to write output data");
            }
          }

          ACTS_INFO("event " << event << " done");
        }
      });

  // Call endRun() for writers and services
  for (auto& wrt : m_writers) {
    if (wrt->endRun() != ProcessCode::SUCCESS) {
      return EXIT_FAILURE;
    }
  }
  for (auto& svc : m_services) {
    if (svc->endRun() != ProcessCode::SUCCESS) {
      return EXIT_FAILURE;
    }
  }
  return EXIT_SUCCESS;
}
