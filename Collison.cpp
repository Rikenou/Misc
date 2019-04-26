#include "stdafx.h"
#include "Collision.h"
#include "SceneManager.h"
#include <limits>
#include "GameObject.h"

using namespace HARIKEN;

Collision::Collision()
{
	//This class is intended to be inherited. To make sure it's inherited properly, type has been set to -1 to make sure it has been given a proper type through the inherited creator.
	type = -1;

}


Collision::~Collision()
{
}

void Collision::onCreate(GameObject * Owner)
{
	//Stores pointer to scene manager singleton to save fetch time if it needs to be called in the future
	sceneManager = SceneManager::GetInstance();
	//Attached collider to the game object that owns it
	owner = Owner;
	//Make collider position relative to its owner rather than world space
	pos = owner->pos + posOffset;

	for (size_t i = 0; i < pointOffsetFromPos.size(); i++) {
		//Makes a collision mesh out of the points given to it
		points.push_back(pointOffsetFromPos[i] + pos);

	}

}

void Collision::update()
{

	pos = owner->pos + posOffset;

}

void HARIKEN::Collision::check()
{

	Scene* currentScene = sceneManager->currentScene;
	
	std::vector<Collision*>possibleCollisions;
	//Scene is in charge of sweeping game objects into lists of possible collisions, we then get the list that corresponds to this particular collider
	for (size_t i = 0; i < currentScene->sweepedColliders.size(); i++)
		if (!currentScene->sweepedColliders[i].empty()) {
			if (currentScene->sweepedColliders[i].size() > 1 && currentScene->sweepedColliders[i][0] == this) {

				possibleCollisions = currentScene->sweepedColliders[i];

				break;

			}
		}
	//Update the points in the mesh
	for (size_t i = 0; i < pointOffsetFromPos.size(); i++) {

		points[i] = pointOffsetFromPos[i] + pos;

	}
	//Make sure there aren't any residual objects left in the owner's list of collided objects
	owner->collidedObjects.clear();
	triggered = false;
	//Use the collision detection function to check if the collider has indeed collided with any objects in the sweeped list and return a list of real collisions to its owner
	for (size_t i = 1; i < possibleCollisions.size(); i++)
		if (hasCollided(possibleCollisions[i]))
			owner->collidedObjects.push_back(possibleCollisions[i]->owner);

}

bool Collision::hasCollided(Collision * other)
{
	//GJK algorithm, using Minkowski difference, it turns the point of collision into the origin (0, 0)
	int simVertAmount = 0;
	glm::vec2 direction;
	std::vector<glm::vec2> simplex;
	glm::vec2 A;
	glm::vec2 B;
	glm::vec2 C;
	glm::vec2 A0;
	glm::vec2 AB;
	glm::vec2 AC;

	while (true) {

		switch (simVertAmount) {
			//start simplex with point furthest away from possible collision
		case 0:
			direction = pos - other->pos;
			simplex.push_back(support(direction, this, other));
			simVertAmount = 1;
			direction = -simplex[0];
			break;
			//Add the point furthest away from the first point in the opposite direction, if the dot product of the new point and the direction of search is less than 0 it has not collided
		case 1:
			simplex.push_back(support(direction, this, other));
			simVertAmount = 2;
			if (glm::dot(simplex[1], direction) < 0)
				return false;
			break;
			//Add the third point furthest away perpendicular from the other two points in the roughly the direction of possible collision. If the dot product of the new point and the direction of search is less than 0 it has not collided
		case 2:
			A = simplex[1];
			B = simplex[0];
			AB = B - A;
			direction = glm::vec2(-AB.y, AB.x);
			if (glm::dot(direction, -A) < 0)
				direction = -direction;
			simplex.push_back(support(direction, this, other));
			simVertAmount = 3;
			if (glm::dot(simplex[2], direction) < 0)
				return false;
			break;
			//If the last point has added to the simplex makes a triangle that contains the origin
		case 3:
			A = simplex[2];
			B = simplex[1];
			C = simplex[0];
			AB = B - A;
			AC = C - A;
			A0 = -A;
			glm::vec2 ABNorm = glm::vec2(-AB.y, AB.x);
			if (glm::dot(ABNorm, AC) > 0)
				ABNorm = -ABNorm;
			glm::vec2 ACNorm = glm::vec2(-AC.y, AC.x);
			if (glm::dot(ACNorm, AB) > 0)
				ACNorm = -ACNorm;
			if (glm::dot(ABNorm, A0) > 0) { //Check if origin is in the direction of the normal of the 2nd and 3rd point of the simplex
				direction = ABNorm;
				simplex.clear(); // Remove the the 1st point of the simplex and add in the new found point
				simplex.push_back(B);
				simplex.push_back(A);
				simplex.push_back(support(direction, this, other));
				if (glm::dot(simplex[2], direction) < 0)
					return false;
				break;
			}
			if (glm::dot(ACNorm, A0) > 0) { //Checks if the origin is the direction of the normal of the 1st and 3rd point
				direction = ACNorm;
				simplex.clear(); // Remove the 2nd point of the simplex and add in the new found point
				simplex.push_back(C);
				simplex.push_back(A);
				simplex.push_back(support(direction, this, other));
				if (glm::dot(simplex[2], direction) < 0)
					return false;
				break;
			}
			if (glm::dot(-ABNorm, A0) >= 0 && glm::dot(-ACNorm, A0) >= 0) { //Checks if origin is within the bounds of the simplex
				owner->collidedObjects.push_back(other->owner);
				if (isTrigger) // If the object is a trigger, do not resolve collision
					return false;
				else if (other->isTrigger)
					return false;
				else { // If object is kinematic, resolve collision but do not provide collision normals
					if (!owner->isKinematic()) {

						EPA(simplex, other);

					}
					return true;
				}
			}

			return false;

		}

	}

	return false;

}
// Support function for finding points in a shape generated by the minkowski difference
glm::vec2 HARIKEN::Collision::support(glm::vec2 direction, Collision * collider, Collision * otherCollider)
{

	glm::vec2 maxPoint;
	glm::vec2 maxPointOther;
	//Special case for circular colliders as there are no edge points
	if (collider->type == CIRCLE && otherCollider->type == CIRCLE)
		maxPoint = collider->pos + glm::normalize(direction) * collider->r;
	else {

		maxPoint = collider->points[0];
		float maxDot = glm::dot(collider->points[0], direction);

		for (size_t i = 1; i < collider->points.size(); i++) {

			float check = glm::dot(collider->points[i], direction);
			if (check > maxDot) {
				maxPoint = collider->points[i];
				maxDot = check;
			}

		}

	}
	//Special case for circular colliders as there are no edge points
	if (collider->type == CIRCLE && otherCollider->type == CIRCLE)
		maxPointOther = otherCollider->pos + glm::normalize(-direction) * otherCollider->r;
	else {

		maxPointOther = otherCollider->points[0];
		float maxDot = glm::dot(otherCollider->points[0], -direction);

		for (size_t i = 1; i < otherCollider->points.size(); i++) {

			float check = glm::dot(otherCollider->points[i], -direction);
			if (check > maxDot) {
				maxPointOther = otherCollider->points[i];
				maxDot = check;
			}

		}

	}

	return maxPoint - maxPointOther;

}
//Expanding prototype algorithm currently unhelpful other than providing a check, fast moving objects or long and thin objects cause colliding objects to pop out in unintended places
void HARIKEN::Collision::EPA(std::vector<glm::vec2> simplex, Collision* other)
{
	glm::vec2 prevNorm = glm::vec2(std::numeric_limits<float>::infinity(), std::numeric_limits<float>::infinity());
	glm::vec2 prevPrevNorm = glm::vec2(std::numeric_limits<float>::infinity(), std::numeric_limits<float>::infinity());

	while (true) { // Find the edge closest to the origin, currently under development, I believe there is room for improvement

		glm::vec2 closestEdge = simplex[0] - simplex[simplex.size() - 1];
		glm::vec2 A = simplex[0];
		glm::vec2 B = simplex[simplex.size() - 1];

		float mindist = glm::dot(A, B);

		float index = 0;

		for (size_t i = 1; i < simplex.size(); i++) { // Obtains the edge closest to the origin within the simplex

			glm::vec2 edge = simplex[i] - simplex[i - 1];

			glm::vec2 A = simplex[i];
			glm::vec2 B = simplex[i - 1];

			float h = glm::dot(A, B);

			if (h < mindist) {
				mindist = h;
				closestEdge = edge;
				index = i - 1;
			}

		}

		glm::vec2 closestEdgeNormal = glm::vec2(closestEdge.y, -closestEdge.x);

		if (glm::dot(closestEdgeNormal, simplex[index]) < 0)
			closestEdgeNormal = -closestEdgeNormal;

		if (closestEdgeNormal.x == 0 && closestEdgeNormal.y == 0) { //Odd chance that the origin is on the edge, in which case this is the closest edge

			closestEdgeNormal = glm::normalize(pos - other->pos);

			owner->collisionNormal += closestEdgeNormal;
			return;

		}
		// If a closer edge could not be found twice, the current edge is determined to be the closest
		glm::vec2 check = closestEdgeNormal - prevNorm;
		glm::vec2 check2 = closestEdgeNormal - prevPrevNorm;

		glm::vec2 newVert = support(closestEdgeNormal, this, other);

		if (std::pow(check.x, 2) < 0.01 && std::pow(check.y, 2) < 0.01) {

			owner->collisionNormal += closestEdgeNormal;
			return;

		}

		prevPrevNorm = prevNorm;
		prevNorm = closestEdgeNormal;

		std::vector<glm::vec2>newSimplex;
		//Grow the simplex
		for (size_t i = 0; i < simplex.size(); i++) {

			newSimplex.push_back(simplex[i]);

			if (i == index)
				newSimplex.push_back(newVert);

		}

		simplex = newSimplex;

	}

}