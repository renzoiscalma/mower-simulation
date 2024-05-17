#include "Robot.h"
#include "TileMap.h"

extern Blit3D* blit3D;
extern TileMap* tileMap;

// look up table for the x and y signs of a direction
float Robot::directionTable[][2] = {
	{ -1.f, 0.f },		// direction for LEFT		(index 0 on Direction Enum)
	{ -1.f, -1.f },		// direction for UP_LEFT	(index 1 on Direction Enum)
	{ -1.f, 1.f },		// direction for DOWN_LEFT	(index 2 on Direction Enum)
	{ 1.f, 0.f },		// direction for RIGHT		... and so on
	{ 1.f, -1.f },		// direction for UP_RIGHT
	{ 1.f, 1.f },		// direction for DOWN_RIGHT
	{ 0.f, -1.f },		// direction for UP
	{ 0.f, 1.f },		// direction for DOWN
};

float Robot::SECONDS = 1.f;
float Robot::MINUTES = 60.f;
float Robot::HOURS = 3600.f;
float Robot::DISCHARGE_THRESHOLD = 2.f * Robot::HOURS;

/*
	constructor of the robot
*/
Robot::Robot(int posX, int posY, Sprite* sprite, Direction initialDirection)
{
	this->sprite = sprite;
	this->dir = initialDirection;
	this->tileMapPosition = glm::vec2(posX, posY);
	this->position = glm::vec2(posX * 16 + this->size, posY * 16 + this->size);
	float screenXCenter = (((float) tileMap->getMapViewWidth() * 16.f) / 2.f) - this->size;
	float screenYCenter = blit3D->screenHeight - (((float) tileMap->getMapViewHeight() * 16.f) / 2.f) + this->size;
	this->screenPosition = glm::vec2(screenXCenter, screenYCenter);
	std::random_device rd;
	rng.seed(rd());
}

/*
	Draw the robot on the screen
*/
void Robot::Draw()
{
	sprite->Blit(screenPosition.x,			// x axis in screen
		screenPosition.y,					// y axis in screen (0 is at bottom that's why we deduct the height)
		0.04, 0.04);
}

/*
	Update the position of the robot based on the state
	Old bouncing logic for the robot
*/
void Robot::Update(float seconds)
{
	if (tileMap->getTilesToMow() == 0) {
		velocity = velocity * 0.f;
	}
	CollisionType colType;
	if (state == RobotState::MOVING) {				// if robot is moving
		move(seconds);								// update the position based on velocity and seconds
		if ((colType = collisionCheck()) != CollisionType::NONE) {	// if colliding
			revertPosition(seconds);								// revert position so it wouldn't clip
			if (battery > 0  || (battery <= 0 
				&& colType != CollisionType::PERIMETER)) {
				bounce(seconds);									// update angle 
			}
			else if (battery <= 0 
				&& colType == CollisionType::PERIMETER) {			// if robot has no battery
				velocity = velocity * 0.f;							// stop the robot
				this->state = RobotState::LOOKUP_CHARGE_STN;		// update state to charging station lookup
			}
		}
	} else if (state == RobotState::LOOKUP_CHARGE_STN) {			// if state is looking for chargin station
		if (tileMap->getTile(tileMapPosition.y, tileMapPosition.x).isChargingTile()) {		// if current position is a chargin tile
			velocity *= 0;																	// stop the robot
			state = RobotState::CHARGING;													// set state as charging
			position = glm::vec2(tileMapPosition.x * 16 + 8, tileMapPosition.y * 16 + 8);	// snap it to the center of the charging station
		}
		getDirectionAlongPerimeter(seconds);												// move along perimeter
		move(seconds);																		// update position
	}
	else if (state == RobotState::CHARGING) {												// if state is charging
		if (battery <= 100) {
			battery += 1;
			timePassed += 1.f / 100.f * SECONDS;
		}															
		else {
			rechargeCount++;
			start();
		}
	}
	// set current position of robot to mowed, if it's mowable
	if (tileMap->validMapPosition(tileMapPosition) 
		&& tileMap->getTile(tileMapPosition.y, tileMapPosition.x).isMowableTile()) {
		tileMap->mowTile(tileMapPosition.y, tileMapPosition.x);
	}
}

/*
	Updates the position of the robot based on velocity
*/
void Robot::move(float seconds) {
	position += velocity * seconds;
	// record prev tileMapPosition
	prevTileMapPosition = tileMapPosition;
	// update tilemap position variable of robot
	tileMapPosition = tileMap->toMapPosition(position);
	bool robotDisplacement = tileMapPosition.x - prevTileMapPosition.x != 0 
		|| tileMapPosition.y - prevTileMapPosition.y != 0;
	if (robotDisplacement) {
		timePassed += SECONDS / HOURS;
		if (battery > 0)
			battery -= (SECONDS / DISCHARGE_THRESHOLD) * 100.f;				// update battery
		else
			battery = 0;
	}
}
/*
	make robot move along the perimeter,
	computes the next velocity (direction)
*/
void Robot::getDirectionAlongPerimeter(float seconds) {
	int directionIndex;
	bool validDirections[4] = { false, false, false,false };						// valid directions, will be used as a parameter for getValidMoveAlongDirections, will be modified
	Direction directions[4] = {														// directions array hardcoded, so we can use it in a for loop instead of 4 if statements
		UP,
		DOWN,
		LEFT,
		RIGHT,
	};
	if (currPerimeterDirection == NONE) {											// if no direction
		getValidMoveAlongDirections(tileMapPosition, validDirections);				// find possible directions
		for (directionIndex = 0; directionIndex < 4; directionIndex++) {			// for each direction
			if (validDirections[directionIndex]										// if valid direction
				&& (prevPerimeterDirection == NONE || directions[directionIndex]	// check if this direction was the opposite of the previous direction (to avoid looping on a perimeter)
				!= getOppositeDirection(prevPerimeterDirection))) {	
				currPerimeterDirection = directions[directionIndex];
				moveToDirection(directions[directionIndex]);						// set velocity of this direction
				break;																// break because we don't want to continue with the for loop since we found the valid direction now
			}
		}
	}
	else { // if robot now has a direction to move
		int nextDirectionIndexX, nextDirectionIndexY;
		nextDirectionIndexX = tileMapPosition.x + directionTable[currPerimeterDirection][0];
		nextDirectionIndexY = tileMapPosition.y + directionTable[currPerimeterDirection][1];
		// if next direction tile will collide or next position has no perimeter adjacent, 
		// or next direction is not adjacent/same with previous perimeter tile robot should stop
		if (!tileMap->hasPerimeterAdjacent(glm::vec2(nextDirectionIndexY, nextDirectionIndexX)) ||
			lookAheadCollision(3, velocity, seconds)) {
			position = glm::vec2(tileMapPosition.x * 16 + size, tileMapPosition.y * 16 + size);
			velocity *= 0;
			std::cout << "looking for next position" << std::endl;
			prevPerimeterDirection = currPerimeterDirection;
			currPerimeterDirection = NONE;
		}
	}

}

/*
	returns the opposite direction based on the direction provided
*/
Direction Robot::getOppositeDirection(Direction direction) {
	switch (direction) {
	case UP:
		return DOWN;
	case DOWN:
		return UP;
	case RIGHT:
		return LEFT;
	case LEFT:
		return RIGHT;
	default:
		return NONE;
	}
}

/*
	Gets the valid directions of the robot can take when sticked to a perimeter
	parameters:
		tileMapPosition - position to check
		bool[] result - size 4 array that indicates:
		if index 0 is true, UP is a valid direction
		if index 1 is true, DOWN is a valid direction
		if index 2 is true, LEFT is a valid direction
		if index 3 is true, RIGHT is a valid direction
*/
void Robot::getValidMoveAlongDirections(glm::vec2 tileMapPosition, bool result[]) {
	// look ahead direction, used for for loops instead of four if statements
	Direction lookAheadDir[4] = {
		Direction::UP,
		Direction::DOWN,
		Direction::LEFT,
		Direction::RIGHT,
	};
	// declare variables used within the loop
	int directionsIndex, nextDirectionIndexX, nextDirectionIndexY;
	Direction currDirection;
	for (directionsIndex = 0; directionsIndex < 4; directionsIndex++) {	// for each direction
		currDirection = lookAheadDir[directionsIndex];					
		nextDirectionIndexX = tileMapPosition.x + directionTable[currDirection][0];					// get next X map index for direction
		nextDirectionIndexY = tileMapPosition.y + directionTable[currDirection][1];					// get next Y map index next direction
		if (tileMap->validMapPosition(glm::vec2(nextDirectionIndexY, nextDirectionIndexX)) &&		// if next tile is valid
			tileMap->getTile(nextDirectionIndexY, nextDirectionIndexX).tileCollisionType() != CollisionType::PERIMETER && // if next tile is not perimeter
			tileMap->hasPerimeterAdjacent(glm::vec2(nextDirectionIndexY, nextDirectionIndexX))) {	// and next tile has a perimeter beside it
			result[directionsIndex] = true;															// set index of resulting array as true (marking the direction as a valid one)
		}
	}
}
/*
	checks if the robot is colliding, 
	returns the collision type of the robot
*/
CollisionType Robot::collisionCheck() {

	bool upLeftTileCollided, upRightTileCollided, downRightTileCollided, downLeftTileCollided, currentTileCollided;
	// get the 4 corner points of the robot
	float up = position.y + size;
	float down = position.y - size;
	float left = position.x - size;
	float right = position.x + size;

	// convert pixel coordinates into tile coordinates
	glm::vec2 upLeftTilePos = tileMap->toMapPosition(glm::vec2(up, left));
	glm::vec2 upRightTilePos = tileMap->toMapPosition(glm::vec2(up, right));
	glm::vec2 downLeftTilePos = tileMap->toMapPosition(glm::vec2(down, left));
	glm::vec2 downRightTilePos = tileMap->toMapPosition(glm::vec2(down, right));

	if (tileMap->validMapPosition(upLeftTilePos)) {
		Tile upLeftTile = tileMap->getTile(upLeftTilePos.x, upLeftTilePos.y);
		upLeftTileCollided = upLeftTile.tileCollisionType() != CollisionType::NONE;
		if (upLeftTileCollided) return upLeftTile.tileCollisionType();
	}
	if (tileMap->validMapPosition(upRightTilePos)) {
		Tile upRightTile = tileMap->getTile(upRightTilePos.x, upRightTilePos.y);
		upRightTileCollided = upRightTile.tileCollisionType() != CollisionType::NONE;
		if (upRightTileCollided) return upRightTile.tileCollisionType();
	}
	if (tileMap->validMapPosition(downRightTilePos)) {
		Tile downRightTile = tileMap->getTile(downRightTilePos.x, downRightTilePos.y);
		downRightTileCollided = downRightTile.tileCollisionType() != CollisionType::NONE;
		if (downRightTileCollided) return downRightTile.tileCollisionType();
	}
	if (tileMap->validMapPosition(downLeftTilePos)) {
		Tile downLeftTile = tileMap->getTile(downLeftTilePos.x, downLeftTilePos.y);
		downLeftTileCollided = downLeftTile.tileCollisionType() != CollisionType::NONE;
		if (downLeftTileCollided) return downLeftTile.tileCollisionType();
	}
	if (tileMap->validMapPosition(tileMapPosition.y, tileMapPosition.x)) {
		Tile currTile = tileMap->getTile(tileMapPosition.y, tileMapPosition.x);
		currentTileCollided = currTile.tileCollisionType() != CollisionType::NONE;
		if (currentTileCollided) return currTile.tileCollisionType();
	}

	// return none if not collided
	return CollisionType::NONE;
}
/**
	bounce function when robot is colliding with obstacles/perimeter,
	changes the velocity of the robot based on the computed angles
*/
void Robot::bounce(float seconds) {
	glm::vec2 tempVelocity;											// record temp velocity
	std::uniform_int_distribution<int> zeroToOne(0, 1);				
	std::uniform_int_distribution<int> range30(-15, 15);			// rng from -15 to 15, used for future angles
	int zeroOrOne = zeroToOne(rng);									// roll from 0 to 1
	float currentRadians = glm::atan(velocity.y, velocity.x);		
	float currentDegree = glm::degrees(currentRadians);				// get current angle of robot
	int firstAngleChoice = (120 + range30(rng)) + currentDegree;	// possible angle 120 +- 15 to left
	int secondAngleChoice = currentDegree - (120 + range30(rng));	// possible angle 120 +- 15 to right
	float bounceDegree = zeroOrOne == 0								// assign tentative angle of choice based on 0 or 1
		? firstAngleChoice
		: secondAngleChoice;
	float bounceRadians = glm::radians(bounceDegree);
	tempVelocity.x = speed * cos(bounceRadians);
	tempVelocity.y = speed * sin(bounceRadians);					// compute temporary velocity based on tentative angle
	// look at 10 ticks ahead if projected trajectory has no obstacle!
	// if there is, use the other angle instead
	if (lookAheadCollision(10, tempVelocity, seconds)) {			// check if temporary velocity will collide  right away on the next tile
		bounceDegree = zeroOrOne == 0								// if it does, use the other angle instead.
			? secondAngleChoice
			: firstAngleChoice;
		bounceRadians = glm::radians(bounceDegree);
		velocity.x = speed * cos(bounceRadians);
		velocity.y = speed * sin(bounceRadians);
	}
	else {
		velocity = tempVelocity;									// else temp velocity to current robot
	}
}

/*
	looks ahead for a given amount of ticks, 
	returns			- TRUE if the robot will collide in the future, FALSE if not
	parameters:
		ticks		- the number of ticks for look ahead
		velocity	- the velocity used to compute the future
		seconds		- delta time
*/
bool Robot::lookAheadCollision(int ticks, glm::vec2 velocity, float seconds) {
	glm::vec2 futurePosition, futureTile;
	for (unsigned int i = 1; i <= ticks; i++) {									// for i to tick
		futurePosition = position + ((float)i * velocity * seconds);	// compute future position based on velocity and time given
		futureTile = tileMap->toMapPosition(futurePosition);			// compute position to tile
		if (tileMap->getTile(futureTile.y, futureTile.x).tileCollisionType() != CollisionType::NONE) {
			return true;												// return true if robot will collide -- collision type for the tile is not NONE
		}
	}
	return false;
}

/*
	Reverts to previous position based on seconds have passed.
	Used when the robot collides with an object in order to avoid clipping 
	(so bounce wouldn't be called too many times)
*/ 
void Robot::revertPosition(float seconds) {
	position -= velocity * seconds * 1.5f; // 1.2 means as an offset just in case the robot clips.
}

void Robot::reset() {
	this->position = glm::vec2(25, 25);
	this->state = RobotState::STOP;
}

void Robot::start() {
	std::uniform_int_distribution<int> zeroToFour(0, 4);
	moveToDirection((Direction) RIGHT);
	this->state = RobotState::MOVING;
	this->battery = 100;
	this->prevPerimeterDirection = NONE;
	this->currPerimeterDirection = NONE;
	this->zigzagDir = RIGHT;

}
/*
	Multiplies the velocity based on the direction passed as a parameter.
	Utilizes the direction table to aquire the direction
*/
void Robot::moveToDirection(Direction direction) {
	velocity.x = speed * directionTable[direction][0];
	velocity.y = speed * directionTable[direction][1];
}

/// <summary>
/// New update function, the robot goes zigzag in the map,
/// when the robots collides to an obstacle, 
/// it goes around it by finding the path of the next mowable tile within the same row.
/// if it collides with a perimeter, it goes down 1 level.
/// when battery is out it bounces around until it hits a perimeter and goes to the charger
/// when going to back to the previous position, the robot goes to the corner and goes to the 
/// previous row and starts mowing again in a zigzag pattern
/// </summary>
void Robot::Update2(float seconds) {
	CollisionType colType;
	if (state == RobotState::MOVING) {
		move(seconds);
		if ((colType = collisionCheck()) != CollisionType::NONE) {	// if colliding
			revertPosition(seconds);								// revert position so it wouldn't clip
			if (battery > 0											// if battery > 0
				&& colType == CollisionType::PERIMETER) {			// if collision is a perimeter
				if (tileMapPosition.y + 1 <= tileMap->getHeight() - 2) { // if not at the bottom mowable row
					moveToDirection((Direction)DOWN);
					state = RobotState::MOVING_DOWN;
				}
				else {												// done mowing
					velocity *= 0;
					state = RobotState::STOP;
				}
			}
			else if (battery > 0 && colType == CollisionType::OBSTACLE) { // if battery < 0 and collided with an obstacle
				velocity *= 0;
				// find for the next mowable tile within the same row
				glm::vec2 mowablePosition = glm::vec2(						
					tileMapPosition.x + directionTable[(int)zigzagDir][0],
					tileMapPosition.y + directionTable[(int)zigzagDir][1]
				);
				while (tileMap->validMapPosition(mowablePosition)			// while mowableposition is an obstacle
					&& tileMap->getTile(mowablePosition.y, mowablePosition.x).tileCollisionType() == CollisionType::OBSTACLE) {
					mowablePosition = glm::vec2(
						mowablePosition.x + directionTable[(int)zigzagDir][0],	// keep iterating through the row one by one
						mowablePosition.y + directionTable[(int)zigzagDir][1]	// direction here is based on the current zigzag direction
					);
				}
				// find the path for this next mowable tile
				path = searchNextPath(mowablePosition);
				// setup variables for following path
				pathIndex = 0;
				state = RobotState::FOLLOWING_PATH;
			}
			else if (battery <= 0 
				&& colType == CollisionType::OBSTACLE) // if no more battery 
			{
				bounce(seconds);
				if (!mapPositionSaved) {							// save position for going back
					savedMapPosition = tileMapPosition;
					mapPositionSaved = true;
				}
			}
			else if (battery <= 0
				&& colType == CollisionType::PERIMETER) {			// if robot has no battery
				velocity = velocity * 0.f;							// stop the robot
				this->state = RobotState::LOOKUP_CHARGE_STN;		// update state to charging station lookup
				if (!mapPositionSaved) {
					savedMapPosition = tileMapPosition;				// save position for going back
					mapPositionSaved = true;
				}
			}
		}
	} 
	else if (state == RobotState::LOOKUP_CHARGE_STN) {			// if state is looking for chargin station
		if (tileMap->getTile(tileMapPosition.y, tileMapPosition.x).isChargingTile()) {		// if current position is a chargin tile
			velocity *= 0;																	// stop the robot
			state = RobotState::CHARGING;													// set state as charging
			position = glm::vec2(tileMapPosition.x * 16 + size, tileMapPosition.y * 16 + size);	// snap it to the center of the charging station
		}
		getDirectionAlongPerimeter(seconds);												// move along perimeter
		move(seconds);																		// update position
	} 
	else if (state == RobotState::CHARGING) {												// if state is charging
		if (battery < 100) {
			battery += 1;
			timePassed += 1.f/100.f * SECONDS;
		}																// increment battery untill 100
		else {
			battery = 100;
			resumePreviousPosition();									// go back to previous position
			rechargeCount++;
		}
	}
	else if (state == RobotState::GOING_BACK) {							// strategy to return is go to the right most side 
		move(seconds);													// then go to to the previously recorded row and resume mowing
		if (tileMapPosition.x == 1) {									// if column 1
			velocity *= 0;
			moveToDirection(Direction::DOWN);
		}
		if (tileMapPosition.y == savedMapPosition.y) {					// if on previous row, resume mowing
			velocity *= 0;
			mapPositionSaved = false;
			start();
		}
	}
	else if (state == RobotState::MOVING_DOWN) {						// when traversing to the next row when moving zigzag
		move(seconds);													// just go down 1 position
		bool robotDisplacement = tileMapPosition.x - prevTileMapPosition.x != 0
			|| tileMapPosition.y - prevTileMapPosition.y != 0;
		if (robotDisplacement) {
			position = glm::vec2(tileMapPosition.x * 16 + size, tileMapPosition.y * 16 + size);
			velocity *= 0;
			moveOpposite();
		}
	}
	else if (state == RobotState::FOLLOWING_PATH) {						// if robot state is following a path
		if (pathIndex == path.size()) {									// if at the last position of a path
			position = glm::vec2(tileMapPosition.x * 16 + size,			// snap to the cell
				tileMapPosition.y * 16 + size);
			state = RobotState::MOVING;
			moveToDirection((Direction)zigzagDir);
		}
		else {
			glm::vec2 nextPos = path[pathIndex];						// store next pos in variable
			glm::vec2 pathVector = nextPos - tileMapPosition;			// get vector from two points, subtraction
			if (pathVector.x == 0 && pathVector.y == 0) {				// if subtraction resulted into 0,
				position = glm::vec2(tileMapPosition.x * 16 + size, tileMapPosition.y * 16 + size);
				pathIndex++;
				velocity *= 0;
			}
			else {
				velocity = speed * pathVector;							// else set the direction to the vector
			}
		}
		
		move(seconds);
	}
	// set current position of robot to mowed, if it's mowable
	if (tileMap->validMapPosition(tileMapPosition)
		&& tileMap->getTile(tileMapPosition.y, tileMapPosition.x).isMowableTile()) {
		tileMap->mowTile(tileMapPosition.y, tileMapPosition.x);
	}
}

/**
*	get the opposite direction of the zigzag, and go to that direction	
*/
void Robot::moveOpposite() {
	Direction newDir = getOppositeDirection(zigzagDir);
	moveToDirection(newDir);
	zigzagDir = newDir;
	state = RobotState::MOVING;
};

/**
*	set state to going back and move to the left
*/
void Robot::resumePreviousPosition() {
	start();
	state = RobotState::GOING_BACK;
	moveToDirection((Direction) LEFT);
}

/*
	Finds the path for the passed position, uses the Breadth-First search algorithm.
	Reference used: 
		Breadth-First Search. (n.d.). In Wikipedia. 
			Retrieved November 20, 2023, from https://en.wikipedia.org/wiki/Breadth-first_search
*/
std::vector<glm::vec2> Robot::searchNextPath(glm::vec2 goalPosition) {
	std::queue<std::vector<glm::vec2>> q;									// queue of paths to keep track which optimal path to take
	std::vector<std::vector<boolean>> visitedMap(tileMap->getHeight(),		// a 2d vector of visited map
		std::vector<boolean>(tileMap->getWidth()));
	std::vector<glm::vec2> currPath;										// declare current path
	glm::vec2 lastPos;														// declare lastPosition of the path
	int nextX = -1;															// initialize nextX
	int nextY = -1;															// initalize nextY
	std::vector<glm::vec2> initialPath = { tileMapPosition };				// set initial path, the current tile position
	q.push(initialPath);													// push initial path to the queue
	Direction possibleDirections[4] = {
		Direction::UP,
		Direction::DOWN,
		Direction::LEFT,
		Direction::RIGHT,
	};
	while (!q.empty()) {													// while the queue is not empty
		currPath = q.front();												// dequeue an path element from the queue
		q.pop();															
		lastPos = currPath.back();											// get the last position of the path 
		if (lastPos == goalPosition) {										// check if the last position  is the goal position
			return currPath;												// return the path
		}

		for (unsigned int i = 0; i < 4; i++) {										// for each directions in the direction table
			nextX = lastPos.x + directionTable[possibleDirections[i]][0];	// get the x and y position for the direction
			nextY = lastPos.y + directionTable[possibleDirections[i]][1];	//
			glm::vec2 nextPos = glm::vec2(nextX, nextY);					// make it a coordinate

			if (tileMap->validMapPosition(nextPos)							// if the coordinate is valid
				&& !visitedMap[nextPos.y][nextPos.x]						// and not yet visited
				&& tileMap->getTile(nextPos.y, nextPos.x).tileCollisionType() == CollisionType::NONE) { // and not a collision type
				visitedMap[nextY][nextX] = true;							// mark it as visited
				std::vector<glm::vec2> newPath = currPath;					// make it a new path based on the new coordinate and its previous path
				newPath.push_back(nextPos);	
				q.push(newPath);											// enqueue the path 					
			}
		}
		
	}
	
}