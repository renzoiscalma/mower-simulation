#include "Tile.h";

int Tile::perimeterTileList[] = {
	26, 28, 29, 59,
	57, 85, 84, 93,
	63, 34, 33, 3
};

int Tile::chargingTileList[] = {
	279, 281, 339, 341
};

/*
	checks if the tile object is a collision type
	returns CollisionType::PERIMETER if the tile is a perimeterand OBSTACLE if it's an obstacle
*/
CollisionType Tile::tileCollisionType() {
	// check first if foreground is -1, if it is return NONE
	if (foregroundTileNum == -1)
		return CollisionType::NONE;
	unsigned int perimeterTileIndex;
	// scans the perimeter tile list if the current tile is a perimeter tile
	for (perimeterTileIndex = 0; perimeterTileIndex < 12; perimeterTileIndex++) {
		if (foregroundTileNum == perimeterTileList[perimeterTileIndex])
		{
			return CollisionType::PERIMETER;
		}
	}
	// by default if it's not -1, it's an obstacle
	return CollisionType::OBSTACLE;
}

// checks if the Tile is a charging tile
bool Tile::isChargingTile() {
	unsigned int chargingTileIndex;
	// scans the chargingTile list if the current tile is a charging tile
	for (chargingTileIndex = 0; chargingTileIndex < 4; chargingTileIndex++) {
		if (backgroundTileNum == chargingTileList[chargingTileIndex])
			return true;
	};
	return false;
}
/*
	Checks if the this tile is mowable
*/
bool Tile::isMowableTile() {
	return backgroundTileNum == 7 && foregroundTileNum == -1;
}

void Tile::mow() {
	backgroundTileNum = 11;	// turn tile into mowed
}