// This file is part of the ACTS project.
//
// Copyright (C) 2018 ACTS project team
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#include "ACTFW/Plugins/HepMC/HepMC3Vertex.hpp"
#include "ACTFW/Plugins/HepMC/HepMC3Particle.hpp"

std::unique_ptr<Acts::ProcessVertex>
FW::SimulatedVertex<HepMC::GenVertex>::processVertex(
    const HepMC::GenVertex* vertex)
{
  const std::vector<HepMC::GenParticlePtr> genParticlesIn
      = vertex->particles_in();
  std::vector<Acts::ParticleProperties> actsParticlesIn;
  // Translate all incoming particles
  for (auto& genParticle : genParticlesIn)
    actsParticlesIn.push_back(
        *(FW::SimParticle::particleProperties<HepMC::GenParticle>(
            &*genParticle)));

  const std::vector<HepMC::GenParticlePtr> genParticlesOut
      = vertex->particles_out();
  std::vector<Acts::ParticleProperties> actsParticlesOut;
  // Translate all outgoing particles
  for (auto& genParticle : genParticlesOut)
    actsParticlesOut.push_back(
        *(FW::SimParticle::particleProperties<HepMC::GenParticle>(
            &*genParticle)));

  // Create Acts vertex
  return std::make_unique<Acts::ProcessVertex>(Acts::ProcessVertex(
      {vertex->position().x(), vertex->position().y(), vertex->position().z()},
      vertex->position().t(),
      0,  // TODO: what does process_type?
      actsParticlesIn,
      actsParticlesOut));
}

bool
FW::SimulatedVertex<HepMC::GenVertex>::inEvent(const HepMC::GenVertex* vertex)
{
  return vertex->in_event();
}

int
FW::SimulatedVertex<HepMC::GenVertex>::id(const HepMC::GenVertex* vertex)
{
  return vertex->id();
}

std::vector<Acts::ParticleProperties>
FW::SimulatedVertex<HepMC::GenVertex>::particlesIn(
    const HepMC::GenVertex* vertex)
{
  const std::vector<HepMC::GenParticlePtr> genParticles
      = vertex->particles_in();
  std::vector<Acts::ParticleProperties> actsParticles;
  // Translate all particles
  for(auto& genParticle : genParticles)
		actsParticles.push_back(*(SimParticle::particleProperties<HepMC::GenParticle>(&*genParticle)));
  return actsParticles;
}

std::vector<Acts::ParticleProperties>
FW::SimulatedVertex<HepMC::GenVertex>::particlesOut(
    const HepMC::GenVertex* vertex)
{
  const std::vector<HepMC::GenParticlePtr> genParticles
      = vertex->particles_out();
  std::vector<Acts::ParticleProperties> actsParticles;
  // Translate all particles
  for(auto& genParticle : genParticles)
		actsParticles.push_back(*(SimParticle::particleProperties<HepMC::GenParticle>(&*genParticle)));
  return actsParticles;
}

Acts::Vector3D
FW::SimulatedVertex<HepMC::GenVertex>::position(const HepMC::GenVertex* vertex)
{
  Acts::Vector3D vec;
  vec(0) = vertex->position().x();
  vec(1) = vertex->position().y();
  vec(2) = vertex->position().z();
  return vec;
}

double
FW::SimulatedVertex<HepMC::GenVertex>::time(const HepMC::GenVertex* vertex)
{
  return vertex->position().t();
}

void
FW::SimulatedVertex<HepMC::GenVertex>::addParticleIn(
    HepMC::GenVertex*         vertex,
    Acts::ParticleProperties* particle)
{
	// TODO: genParticleToActs could become part of HepMC3Vertex
	// Extract momentum and energy from Acts particle for HepMC::FourVector
	Acts::Vector3D mom    = particle->momentum();
	double         energy = sqrt(particle->mass() * particle->mass()
                       + particle->momentum().dot(particle->momentum()));
	const HepMC::FourVector vec(mom(0), mom(1), mom(2), energy);
	// Create HepMC::GenParticle
	HepMC::GenParticle genParticle(vec, particle->pdgID());
	genParticle.set_generated_mass(particle->mass());
  
	// Add particle to the vertex
	vertex->add_particle_in(HepMC::SmartPointer<HepMC::GenParticle>(&genParticle));
}

void
FW::SimulatedVertex<HepMC::GenVertex>::addParticleOut(
    HepMC::GenVertex*         vertex,
    Acts::ParticleProperties* particle)
{
	// Extract momentum and energy from Acts particle for HepMC::FourVector
	Acts::Vector3D mom    = particle->momentum();
	double         energy = sqrt(particle->mass() * particle->mass()
                       + particle->momentum().dot(particle->momentum()));
	const HepMC::FourVector vec(mom(0), mom(1), mom(2), energy);
	// Create HepMC::GenParticle
	HepMC::GenParticle genParticle(vec, particle->pdgID());
	genParticle.set_generated_mass(particle->mass());
  
	// Add particle to the vertex
	vertex->add_particle_out(HepMC::SmartPointer<HepMC::GenParticle>(&genParticle));
}

void
FW::SimulatedVertex<HepMC::GenVertex>::removeParticleIn(
    HepMC::GenVertex*         vertex,
    Acts::ParticleProperties* particle)
{
	// TODO: Search for the particle could be moved to private method
	const std::vector<HepMC::GenParticlePtr> genParticles
      = vertex->particles_in();
	const barcode_type id = particle->barcode();
	// Search HepMC::GenParticle with the same id as the Acts particle
	for (auto& genParticle : genParticles)
	    if (genParticle->id() == id) {
	      // Remove particle if found
	      vertex->remove_particle_in(genParticle);
	      break;
	    }
}

void
FW::SimulatedVertex<HepMC::GenVertex>::removeParticleOut(
    HepMC::GenVertex*         vertex,
    Acts::ParticleProperties* particle)
{
	const std::vector<HepMC::GenParticlePtr> genParticles
      = vertex->particles_out();
	const barcode_type id = particle->barcode();
	// Search HepMC::GenParticle with the same id as the Acts particle
	for (auto& genParticle : genParticles)
	    if (genParticle->id() == id) {
	      // Remove particle if found
	      vertex->remove_particle_out(genParticle);
	      break;
	    }
}


