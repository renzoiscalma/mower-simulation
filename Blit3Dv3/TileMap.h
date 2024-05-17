#pragma once
#include <vector>
#include <string>
#include <random>
#include "Blit3D.h"
#include "Robot.h"
#include "Tile.h"

class TileMap
{
private:
	// =========== DATA MEMBERS ==============
	static int TILE_SIZE_PIXEL;
	// 2d vector that has tile classes
	std::vector<std::vector<Tile>> map2d;
	// width of the tilemap
	int width = 0;
	// height of the tilemap
	int height = 0;
	// pixel size of each tile
	// the height of the map visible in the screen
	int MAP_VIEW_HEIGHT;
	// the width of the map visible in the screen
	int MAP_VIEW_WIDTH;
	// tiles left to mow
	int tilesToMow = 0;
	// tiles mowed;
	int tilesMowed = 0;
	bool isTileInView(int x, int y, Robot* robot);
public:
	// =========== FUNCTIONS ====================
	// refer to cpp files for more detailed explanation
	TileMap(std::string filename);
	bool LoadMap(std::string filename);
	glm::vec2 toMapPosition(glm::vec2 pixelPosition);
	glm::vec2 toMapPosition(int x, int y);
	void Draw(Robot* robot);
	bool validMapPosition(glm::vec2 tileMapPosition);
	bool validMapPosition(int x, int y);
	bool hasPerimeterAdjacent(glm::vec2 tileMapPosition);
	void mowTile(int row, int col);

	// getters and setters
	Tile getTile(int row, int col) {
		return map2d[row][col];
	}

	int getTilesToMow() {
		return tilesToMow;
	}

	int getTilesMowed() {
		return tilesMowed;
	}

	int getMapViewWidth() {
		return MAP_VIEW_WIDTH;
	}

	int getMapViewHeight() {
		return MAP_VIEW_HEIGHT;
	}

	int getHeight() {
		return height;
	}

	int getWidth() {
		return width;
	}

};