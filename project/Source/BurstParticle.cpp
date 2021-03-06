#include "BurstParticle.h"

BurstParticle::BurstParticle(vec3 position, vec3 velocity, vec3 colour, float duration, int texture, quat rotation) : mPosition(position), mVelocity(velocity), mColour(colour), mDuration(duration), mTexture(texture), mRotation(rotation) {

}

void BurstParticle::draw(GLenum* renderingMode, ShaderManager* shaderProgram)
{
	mat4 matrix = translate(mat4(1.0f), mPosition) * toMat4(mRotation) * scale(mat4(1.0f), vec3(BURST_SCALE)) * scale(mat4(1.0f), vec3(1.0f, 1.0f, 0.1f));
	shaderProgram->setMat4("worldMatrix", matrix);
	shaderProgram->setVec3("colour", mColour);
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, mTexture);
	glDrawArrays(GL_TRIANGLES, 0, 36);
}

void BurstParticle::update(vector<ScheduledEvent>* eventQueue, double dt)
{
	mDuration -= dt;
	mVelocity.y -= dt * GRAVITY;
	mPosition += mVelocity * (float)dt;

	mRotation = mRotation * angleAxis(mVelocity.x * (float)dt, vec3(1.0f, 0.0f, 0.0f));
	mRotation = mRotation * angleAxis(mVelocity.y * (float)dt, vec3(0.0f, 1.0f, 0.0f));
	mRotation = mRotation * angleAxis(mVelocity.z * (float)dt, vec3(0.0f, 0.0f, 1.0f));
	mRotation = normalize(mRotation);
}

bool BurstParticle::isDead()
{
	return mDuration <= 0;
}
