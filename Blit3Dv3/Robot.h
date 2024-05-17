#pragma once
#include "Direction.h"
#include "Blit3D.h";
#include "CollisionType.h"
#include <random>
#include <queue>
enum class RobotState {
	MOVING,								// when robot is moving 
	MOVING_DOWN,						// state when robot is transitioning to another row
	GOING_BACK,							// state when the robot is going back to saved position (position before looking for charging)
	FOLLOWING_PATH,				// state when going around obstacle
	CHARGING,							// robot is charging
	LOOKUP_CHARGE_STN,					// robot is looking for the chargin station
	STOP,								// robot has stopped (initial state)
};

class Robot
{
private:
	// static data members
	static float SECONDS;				// seconds
	static float HOURS;					// hours in seconds
	static float MINUTES;				// minutes in seconds
	static float DISCHARGE_THRESHOLD;	// 2 hours, in seconds
	std::mt19937 rng;					// rng for choosing the angle
	// ===== DATA MEMBERS ====== /
	Sprite* sprite;									// sprite of the robot
	RobotState state = RobotState::STOP;			// state of the robot, initial value set to stop
	glm::vec2 position;								// actual position of the robot in the world map in pixcels
	glm::vec2 tileMapPosition;						// local position of the robot, relative to the visible view of the map
	glm::vec2 prevTileMapPosition;					// previous tile position
	glm::vec2 screenPosition;						// screen position of the robot
	glm::vec2 velocity = glm::vec2(0, 0);			// velocity of the robot
	glm::vec2 savedMapPosition;						// map position to resume after charging
	std::vector<glm::vec2> path;					// current path the robot is following
	float speed = 500.f;							// speed of the robot
	float size = 5.f;								// the half size of the robot (16 is the actual size)
	float battery = 100.f;							// current battery charge of the robot, initial value set to 100.f
	float batteryChargeRate = 50.f;					// charge rate
	float timePassed = 0;							// elapsed time, in HOURS
	int pathIndex = 0;								// index of the position in the current path
	int rechargeCount = 0;
	bool mapPositionSaved = false;
	Direction dir;									// direction of the robot
	Direction zigzagDir = NONE;
	Direction prevPerimeterDirection = NONE;		// current perimeter direction, used for following perimeter path
	Direction currPerimeterDirection = NONE;		// previous perimeter direction, used for following the perimeter path
	// ========= FUNCTIONS ================================== //
	// refer to implementation file for mroe details
	CollisionType collisionCheck();
	void bounce(float seconds);
	void revertPosition(float seconds);
	bool lookAheadCollision(int ticks, glm::vec2 velocity, float seconds);
	void reset();
	void move(float seconds);
	void getDirectionAlongPerimeter(float seconds);
	void getValidMoveAlongDirections(glm::vec2 tileMapPosition, bool result[]);
	void resumePreviousPosition();
	Direction getOppositeDirection(Direction direction);
	std::vector<glm::vec2> searchNextPath(glm::vec2 goalPosition);
public:
	static float directionTable[][2];
	// ========= FUNCTIONS ================================== //
	// refer to the implementation file for more details.
	Robot(int posX, int posY, Sprite* sprite,
		Direction initialDirection = DOWN);
	void Draw();									
	void Update(float seconds);	
	void Update2(float seconds);
	void start();
	void moveToDirection(Direction direction);
	void moveBelow();
	void moveOpposite();
	// getters and setters 
	glm::vec2 getPosition() {
		return position;
	}

	glm::vec2 getScreenPosition() {
		return screenPosition;
	}

	glm::vec2 getTileMapPosition() {
		return tileMapPosition;
	};

	RobotState getState() {
		return state;
	}

	float getBattery() {
		return battery;
	}

	float getTimePassed() {
		return timePassed;
	}

	int getRechargeCount() {
		return rechargeCount;
	}
};

