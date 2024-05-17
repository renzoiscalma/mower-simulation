#pragma once
#include "Blit3D.h"
#include "Robot.h"
class Tile
{
private:
	// static data members
	static int chargingTileList[];			// types of tiles that are charging tile 
	static int perimeterTileList[];			// types of tiles that are perimeters
	// ====== DATA MEMBERS =========
	int foregroundTileNum;					// type of foregroundTile 
	int backgroundTileNum;					// type of backgroundTile
public:
	// ============ FUNCTIONS ==========
	CollisionType tileCollisionType();
	bool isChargingTile();
	bool isMowableTile();
	void mow();

	// getters/setters
	int getBackgroundTile() {
		return backgroundTileNum;
	}

	void setBackgroundTile(int tileNum) {
		backgroundTileNum = tileNum;
	}

	int getForegroundTile() {
		return foregroundTileNum;
	}

	void setForegroundTile(int tileNum) {
		foregroundTileNum = tileNum;
	}
};
