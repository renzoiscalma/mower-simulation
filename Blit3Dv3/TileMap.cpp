#include "TileMap.h"
#include <fstream>
#include <iostream>
#include "CollisionType.h"
extern int MAP_VIEW_SIZE;
extern std::vector<Sprite*> tileSpriteList;
extern Blit3D* blit3D;

int TileMap::TILE_SIZE_PIXEL = 16;
/*
* Constructor for this class, loads the map with the filename passed
*/
TileMap::TileMap(std::string filename)
{
	LoadMap(filename);
}

/*
	parses the file found with the filename
	returns true if it successfuly loads the file,
	exits the program if it doesn't
*/
bool TileMap::LoadMap(std::string filename)
{
	// load file
	std::ifstream mapFile;
	mapFile.open(filename);

	if (!mapFile.is_open()) {
		std::cout << "Can't open map file!" << std::endl;
		exit(-1);
	}

	std::string currLine;
	mapFile >> width;
	mapFile >> height;
	MAP_VIEW_HEIGHT = blit3D->screenHeight / 16;
	MAP_VIEW_WIDTH = blit3D->screenWidth / 16;
	// initialize map2d with height and width sizes
	map2d = std::vector<std::vector<Tile>>(
		height,
		std::vector<Tile>(width));

	// load background tiles
	unsigned int i, j, tileNum;
	for (i = 0; i < height; i++) {
		for (j = 0; j < width; j++) {
			Tile t;
			mapFile >> tileNum;
			t.setBackgroundTile(tileNum);
			map2d[i][j] = t;
		}
	}

	// load foreground tiles
	for (i = 0; i < height; i++) {
		for (j = 0; j < width; j++) {
			mapFile >> tileNum;
			map2d[i][j].setForegroundTile(tileNum);
		}
	}

	// analyze tiles and count mowable tiles
	for (i = 0; i < height; i++) {
		for (j = 0; j < width; j++) {
			if (map2d[i][j].isMowableTile()) {
				tilesToMow++;
			}
		}
	}
	mapFile.close();
	return true;
}

/*
	converts x and y to map position
	NOTE: this is not relative to the current screen
	parameter: 
		- takes a position in pixels and converts it into tileMap position
*/ 
glm::vec2 TileMap::toMapPosition(int x, int y)
{
	return glm::vec2((x / 16), (y / 16));
}
/*
	Helper function that converts glmvec2 position to glmvec2 tilemapposition
	NOTE: this is not relative to the current screen
	parameter:
		- takes a position in pixels and converts it into tileMap position
*/
glm::vec2 TileMap::toMapPosition(glm::vec2 pixelPosition)
{
	return toMapPosition(pixelPosition.x, pixelPosition.y);
}

/*
	Draws the map that is only visible in the screen, since the robot is always at the center, 
	we just pan the map around the center point at the same rate the robot is moving. (so it's much more efficient)

	parameters: 
		- robot: the robot since its tile map draw is relative to robot's position
*/
void TileMap::Draw(Robot* robot) {
	unsigned int row, col;
	int rowMin, colMin,
		bgTileIdx, fgTileIdx, 
		tileIndex_X, tileIndex_Y;
	float tileScreenPosition_X, tileScreenPosition_Y;
	rowMin = robot->getTileMapPosition().y - (MAP_VIEW_HEIGHT + 2) / 2.f;
	colMin = robot->getTileMapPosition().x - (MAP_VIEW_WIDTH + 2) / 2.f;

	// render tiles only visible to the screen MAP_VIEW_HEIGHT * MAP_VIEW_WIDTH
	// refer to TileMap Constructor for this
	for (row = 0; row < MAP_VIEW_HEIGHT + 2; row++) {
		for (col = 0; col < MAP_VIEW_WIDTH + 3; col++) {
			// get tile indeces
			tileIndex_Y = row + rowMin;
			tileIndex_X = col + colMin;
			// get tile posistion in screen
			// tile position - robot position (pixels) + robot screen position (which is always center)
			tileScreenPosition_X = (tileIndex_X * 16) - (robot->getPosition().x) + robot->getScreenPosition().x;
			tileScreenPosition_Y = blit3D->screenHeight - ((tileIndex_Y * 16) - (robot->getPosition().y) + robot->getScreenPosition().y);

			// if out of bounds, render an ocean tile
			if (!validMapPosition(tileIndex_X, tileIndex_Y))
			{
				tileSpriteList[210]->Blit(tileScreenPosition_X, tileScreenPosition_Y);

			} else { // else if not out of bounds
				// get background and foreground tiles
				bgTileIdx = map2d[tileIndex_Y][tileIndex_X].getBackgroundTile();
				fgTileIdx = map2d[tileIndex_Y][tileIndex_X].getForegroundTile();
				// draw the tile
				tileSpriteList[bgTileIdx]->Blit(tileScreenPosition_X, tileScreenPosition_Y);
				if (fgTileIdx != -1) {
					// draw fg tile in the screen if a valid tile
					tileSpriteList[fgTileIdx]->Blit(tileScreenPosition_X, tileScreenPosition_Y);
				}
			}
		}
	}
}

/*
	checks if the tileMapPosition passed is valid in the loaded map,
	returns true if it is a valid map position, else false
*/
bool TileMap::validMapPosition(glm::vec2 tileMapPosition) {
	return validMapPosition(tileMapPosition.x, tileMapPosition.y);
}

/*
	checks if int x and int y are valid values in the map
	returns true if it is a valid map position, else false
*/
bool TileMap::validMapPosition(int x, int y) {
	return (x >= 0 && y >= 0 && x < width && y < height);
}

/*
	checks if the adjacent tiles of the given tileMapPosition has adjacent perimeter tiles
	parameters: tileMapPosition - the position of the map to check
	returns true if tileMapPosition has an adjacent perimeter
*/
bool TileMap::hasPerimeterAdjacent(glm::vec2 tileMapPosition) {
	Direction lookAheadDir[8] = {
		Direction::LEFT,
		Direction::RIGHT,
		Direction::UP,
		Direction::DOWN,
		Direction::DOWN_LEFT,
		Direction::DOWN_RIGHT,
		Direction::UP_LEFT,
		Direction::UP_RIGHT,
	};
	int directionsIndex, nextDirectionIndexX, nextDirectionIndexY;
	Direction currDirection;
	for (directionsIndex = 0; directionsIndex < 8; directionsIndex++) {	// for each directions
		currDirection = lookAheadDir[directionsIndex];					// get the direction's x and y values
		nextDirectionIndexX = tileMapPosition.x + Robot::directionTable[currDirection][0];	// get the next tile position based on the direction
		nextDirectionIndexY = tileMapPosition.y + Robot::directionTable[currDirection][1];	
		if (validMapPosition(nextDirectionIndexX, nextDirectionIndexY) &&					// if next tile is valid map position 
			map2d[nextDirectionIndexX][nextDirectionIndexY].tileCollisionType() == CollisionType::PERIMETER) { // and tile is a perimeter
			return true;	// return true if perimeter
		}
	}
	return false;			// return false, means each direction hasn't found a perimeter beside a tile
}

void TileMap::mowTile(int row, int col) {
	tilesToMow--;
	tilesMowed++;
	map2d[row][col].mow();
}