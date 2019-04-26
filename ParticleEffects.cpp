//Author Jonathan Leung
#include "stdafx.h"
#include "ParticleEffects.h"
#include "Scene.h"

using namespace HARIKEN;

ParticleEffects::ParticleEffects()
{
	//Create buffers in GPU to store the particles
	glGenBuffers(1, &ssbo);
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, ssbo);
	glBufferData(GL_SHADER_STORAGE_BUFFER, sizeof(Particle) * MAX_PARTICLES, NULL, GL_STATIC_DRAW);

	GLint bufMask = GL_MAP_WRITE_BIT | GL_MAP_INVALIDATE_BUFFER_BIT;

	ssbo_particle_dataLoc = static_cast<Particle*>(glMapBufferRange(GL_SHADER_STORAGE_BUFFER, 0, MAX_PARTICLES * sizeof(Particle), bufMask));

	glUnmapBuffer(GL_SHADER_STORAGE_BUFFER);

}


ParticleEffects::~ParticleEffects()
{
	//Delete all particles created by this particle effect
	for (auto p : emittedParticles)
		delete p;

}

void HARIKEN::ParticleEffects::init(GameObject * owner_)
{
	//Link the game object that owns this effect
	owner = owner_;

}

void HARIKEN::ParticleEffects::afterUpdate(glm::vec3 pos, float deltaTime_)
{
	//Called after inherited update
	deltaTime = deltaTime_;

	if (first) {
		first = false;
		timer = frequency;
	}

	timer += deltaTime_;
	//Checks to see if the effect is supposed to be playing
	if (timer >= frequency && isPlaying) {

		timer = 0.0f;
		//See if this effect is a point spawner or shaped spawner
		if (!lineDrawn)
			spawnParticle(pos);

		else {

			for (int i = 0; i < static_cast<int>(linePoints.size()); i++)
				spawnParticle(linePoints[i]);

		}

	}
	//If the amount of particles exceed the amount specified, kills the earliest particle in the list
	if (static_cast<int>(emittedParticles.size()) - maxPart > 0)
		for (int i = 0; i < static_cast<int>(emittedParticles.size()) - maxPart; i++)
			emittedParticles[i]->kill = true;

	s = pos;
	//Update all emitted particles
	for (size_t i = 0; i < emittedParticles.size(); i++) {

		emittedParticles[i]->update(this, deltaTime);

	}
	//If the particle is dead, reset it and put it in the recycle queue to be reused
	for (size_t i = 0; i < emittedParticles.size(); i++)
		if (emittedParticles[i]->isDead()) {

			emittedParticles[i]->reset(properties, pos);
			resetParticles.push(emittedParticles[i]);
			emittedParticles.erase(emittedParticles.begin() + i);

		}

	emittedParticles.shrink_to_fit();
	//Add this effect to the owner's render queue
	if (owner->particleEffRenderQueue.empty())
		owner->particleEffRenderQueue.push_back(this);

	else {

		bool closest = true;
		//See if any other effects has a higher render priority on the queue
		for (size_t i = 0; i < owner->particleEffRenderQueue.size(); i++) {

			if (s.z <= owner->particleEffRenderQueue[i]->layer) {
				owner->particleEffRenderQueue.insert(owner->particleEffRenderQueue.begin() + static_cast<int>(i), this);
				closest = false;
				break;
			}

		}

		if (closest)
			owner->particleEffRenderQueue.push_back(this);

	}

}

void HARIKEN::ParticleEffects::render()
{
	//Renders all the particles queued to be rendered
	while (!renderQueue.empty()) {

		renderQueue[0]->render();
		renderQueue.erase(renderQueue.begin());

	}

}

void HARIKEN::ParticleEffects::play()
{
	isPlaying = true;
}

void HARIKEN::ParticleEffects::stop()
{
	isPlaying = false;
}

particleProperties * HARIKEN::ParticleEffects::getParticleProperties()
{
	return &properties;
}

void HARIKEN::ParticleEffects::setPos(glm::vec3 pos_)
{

	s = pos_;

}

glm::vec3 HARIKEN::ParticleEffects::getPos()
{
	return s;
}
//Set the spawn rate of particles
void HARIKEN::ParticleEffects::setFreq(float frequency_)
{

	frequency = frequency_;

}
//Set the maximum particles allowed
void HARIKEN::ParticleEffects::setMaxParticles(int maxNum)
{

	if (maxNum > MAX_PARTICLES)
		maxPart = MAX_PARTICLES;

	maxPart = maxNum;

}
//Draws spawn points for shaped spawning
void HARIKEN::ParticleEffects::drawLine(std::vector<glm::vec2> lineKeyPoint, int density)
{

	if (lineKeyPoint.size() <= 0)
		return;

	int total = std::max(std::round(static_cast<float>(static_cast<float>(density) / lineKeyPoint.size())), static_cast<float>(lineKeyPoint.size()));

	std::vector<int> positions;
	positions.push_back(0);
	for (int i = 1; i < static_cast<int>(lineKeyPoint.size()); i++) {
		positions.push_back(i * total);
	}

	for (int i = 1; i < static_cast<int>(positions.size()); i++) {

		glm::vec3 increment = glm::vec3(lineKeyPoint[i].x - lineKeyPoint[i - 1].x, lineKeyPoint[i].y - lineKeyPoint[i - 1].y, 0.0f) / static_cast<float>(total);

		for (int j = 0; j < total; j++) {

			linePoints.push_back(glm::vec3(lineKeyPoint[i - 1].x, lineKeyPoint[i - 1].y, 0.0f) + increment * static_cast<float>(j));

		}

	}

	linePoints.push_back(glm::vec3(lineKeyPoint[static_cast<int>(lineKeyPoint.size()) - 1].x, lineKeyPoint[static_cast<int>(lineKeyPoint.size()) - 1].y, 0.0f));

	lineDrawn = true;

	lineDownIndex = 0;
	lineUpIndex = static_cast<int>(linePoints.size()) - 1;
	lineMidDownIndex = static_cast<int>(linePoints.size()) / 4;
	lineMidUpIndex = static_cast<int>(linePoints.size()) / 4 * 3;

}
//Spawn particles
void HARIKEN::ParticleEffects::spawnParticle(glm::vec3 pos)
{
	//Check if there are any dead particles available to be reused, if there isn't then create a new particle
	if (resetParticles.empty())
		emittedParticles.push_back(new Particle(properties, pos));
	else {
		resetParticles.front()->reset(properties, pos);
		emittedParticles.push_back(resetParticles.front());
		resetParticles.pop();
	}

}
