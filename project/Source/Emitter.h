#pragma once

#include <list>
#include <vector>

#include <glm/glm.hpp>  // GLM is an optimized math library with syntax to similar to OpenGL Shading Language
#include <glm/gtc/matrix_transform.hpp> // include this to create transformation matrices
#include <glm/common.hpp>

#include "Constants.h"
#include "BurstParticle.h"
#include "FlameParticle.h"

using namespace glm;
using namespace std;

class Emitter {
public:
	Emitter(int VAO);
	void EmitBurst(vec3 position, int particleCount, float force, int texture);
	void EmitFlame(vec3 position, int particleCount, float force, int texture);
	void Update(float dt);
	void Draw(ShaderManager shaderManager);

private:
	vector<BurstParticle> burstParticles;
	vector<FlameParticle> flameParticles;
	int particleVAO;
};