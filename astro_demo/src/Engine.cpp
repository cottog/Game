#include "main.hpp"
#include <math.h>

/* Engine::Engine() : gameStatus(STARTUP), fovRadius(3)
{
	TCODConsole::initRoot(80,50,"Astro", false);
	player = new Actor(40,25,'@', "player",TCODColor::white);
	actors.push(player);
	map = new Map(80,45);

} */

Engine::Engine(int screenWidth, int screenHeight) : gameStatus(STARTUP),
	player(NULL),map(NULL), fovRadius(3),
	screenWidth(screenWidth),screenHeight(screenHeight),level(1),turnCount(0) {
	mapWidth = 100;
	mapHeight = 100;
	TCODConsole::initRoot(screenWidth,screenHeight,"Astro", false);
	mapcon = new TCODConsole(mapWidth,mapHeight);
	gui = new Gui();
}

Engine::~Engine() {
	term();
	delete gui;
}

void Engine::term() {
	actors.clearAndDelete();
	if (map) delete map;
	gui->clear();
	engine.turnCount = 0;
}

void Engine::init() {
	player = new Actor(40,25,'@', "player",TCODColor::white);
	player->destructible = new PlayerDestructible(100, 2, "your cadaver");
	player->attacker = new Attacker(5);
	player->ai = new PlayerAi();
	player->container = new Container(26);
	actors.push(player);
	stairs = new Actor(0,0,'>', "stairs", TCODColor::white);
	stairs->blocks = false;
	actors.push(stairs);
	map = new Map(mapWidth, mapHeight);
	map->init(true);
	gui->message(TCODColor::red, 
    	"Welcome stranger! Prepare to face a horde of Orcs and Trolls");
	gameStatus = STARTUP;
}

void Engine::save() {
	if (player->destructible->isDead()) {
		TCODSystem::deleteFile("game.sav");
	} else {
		TCODZip zip;
		zip.putInt(level);
		zip.putInt(turnCount);
		//save the map first
		zip.putInt(map->width);
		zip.putInt(map->height);
		map->save(zip);
		//then the player
		player->save(zip);
		//then the stairs
		stairs->save(zip);
		//then all the other actors
		zip.putInt(actors.size() - 2);
		for (Actor **it = actors.begin(); it!=actors.end(); it++) {
			if (*it != player && *it != stairs) {
				(*it)->save(zip);
			}
		}
		//finally the message log
		gui->save(zip);
		zip.saveToFile("game.sav");
	}
}

void Engine::load(bool pause) {
	engine.gui->menu.clear();
	if (pause) {
	engine.gui->menu.addItem(Menu::NO_CHOICE, "RESUME GAME");
	}
	if (!pause) {
	engine.gui->menu.addItem(Menu::NEW_GAME, "NEW GAME");
	} else {
		engine.gui->menu.addItem(Menu::MAIN_MENU, "MAIN MENU");
	}
	
	if (pause) {
		engine.gui->menu.addItem(Menu::SAVE, "SAVE");
	}
	if(TCODSystem::fileExists("game.sav")) {
		if (pause) {
		engine.gui->menu.addItem(Menu::CONTINUE, "LOAD");
		} else {
		engine.gui->menu.addItem(Menu::CONTINUE, "CONTINUE");
		}
	}
	engine.gui->menu.addItem(Menu::EXIT,"EXIT");
	
	Menu::MenuItemCode menuItem = engine.gui->menu.pick( 
		pause ? Menu::PAUSE : Menu::MAIN);
	
	if (menuItem == Menu::EXIT || menuItem == Menu::NONE) {
		//exit or window closed
		save();
		exit(0);
	} else if (menuItem == Menu::NEW_GAME) {
		//new game 
		engine.term();
		engine.init();
	} else if (menuItem == Menu::SAVE) {
		save();
	} else if (menuItem == Menu::NO_CHOICE) {
	} else if (menuItem == Menu::MAIN_MENU) {
		save();
		TCODConsole::root->clear();
		load(false);
	}else {
		TCODZip zip;
		//continue a saved game
		engine.term();
		zip.loadFromFile("game.sav");
		level = zip.getInt();
		turnCount = zip.getInt();
		//load the map
		int width = zip.getInt();
		int height = zip.getInt();
		map = new Map(width,height);
		map->load(zip);
		//then the player
		player = new Actor(0,0,0,NULL,TCODColor::white);
		player->load(zip);
		//the stairs
		stairs = new Actor(0,0,0,NULL,TCODColor::white);
		stairs->load(zip);
		actors.push(player);
		actors.push(stairs);
		//then all other actors
		int nbActors = zip.getInt();
		while (nbActors > 0) {
			Actor *actor = new Actor(0,0,0,NULL, TCODColor::white);
			actor->load(zip);
			actors.push(actor);
			nbActors--;
		}
		//finally, the message log
		gui->load(zip);
		gui->message(TCODColor::pink,"loaded");
		gameStatus = STARTUP;
	} 
}
	
void Engine::update() {
	if (gameStatus == STARTUP) map->computeFov();
	gameStatus = IDLE;
	TCODSystem::checkForEvent(TCOD_EVENT_KEY_PRESS, &lastKey, NULL); //delete the mouse stuff to remove mouse look (change &mouse to NULL)
	if (lastKey.vk == TCODK_ESCAPE) {
		//save();  why save automatically every time escape is called?
		load(true);
	} 
	player->update();
	if (gameStatus == NEW_TURN){
		engine.turnCount++;
		for (Actor **iterator = actors.begin(); iterator != actors.end(); iterator++) {
			Actor *actor = *iterator;
			if ( actor != player) {
				actor->update();
			}
		}
	}
}

float Engine::distance(int x1, int x2, int y1, int y2) {
	int dx = x1 - x2;
	int dy = y1 - y2;
	return sqrt(dx*dx+dy*dy);
}

void Engine::render()
{
	TCODConsole::root->clear();
	mapcon->clear();

	//draw the map
	map->render();
	
	//draw the actors
	for (Actor **iterator=actors.begin(); iterator != actors.end(); iterator++) {
		Actor *actor = *iterator;
		if ( actor != player) {
			if (map->isInFov(actor->x,actor->y)) {
				actor->render();
			} else if (map->isExplored(actor->x,actor->y)) {
				TCODColor tmpcol = actor->col;
				actor->col = TCODColor::lightGrey;
				actor->render();
				actor->col = tmpcol;
			}
		}
	}
	player->render();
	//show the player's stats
	
	gui->render();
	
	int mapx1 = 0, mapy1 = 0, mapy2 = 0, mapx2 = 0;
	
	mapx1 = player->x - ((screenWidth -22)/2);
	mapy1 = player->y - ((screenHeight -12)/2);
	mapx2 = player->x + ((screenWidth -22)/2);
	mapy2 = player->y + ((screenHeight -12)/2);
	
	if (mapx1 < 0) {
		mapx2 += (0-mapx1);
		mapx1 = 0;
	}	
	if (mapy1 < 0) { 
		mapy2 += (0-mapy1);
		mapy1 = 0;
	}
	if (mapx2 > 100) {
		mapx1 += (100-mapx2);
		mapx2 = 100;
	}
	if (mapy2 > 100) {
		mapy1 += (100-mapy2);
		mapy2 = 100;
	}
	//if (mapx2 > TCODConsole::root->getWidth() - 22) mapx2 = TCODConsole::root->getWidth() - 22;
	if (mapy2 > TCODConsole::root->getHeight() - 12) mapy2 = TCODConsole::root->getHeight() - 12;
	
	TCODConsole::blit(mapcon, mapx1, mapy1, mapx2, mapy2, 
		TCODConsole::root, 22, 0);
	
	//the comment below is the old gui code
	/* TCODConsole::root->print(1, screenHeight-2, "HP: %d/%d", 
	(int)player->destructible->hp,(int)player->destructible->maxHp); */
}

void Engine::nextLevel() {
	level++;
	
	gui->message(TCODColor::lightViolet, "Sitting at the top of the stairs, you take a brief moment to rest...");
	player->destructible->heal(player->destructible->maxHp/2);
	gui->message(TCODColor::red,"Gathering your courage, you rush down the dungeon stairs, mindful that greater dangers may lurk below...");
	
	delete map;
	//delete all actors but player and stairs
	for(Actor **it = actors.begin(); it != actors.end(); it++) {
		if (*it != player && *it != stairs) {
			delete *it;
			it = actors.remove(it);
		}
	}
	//create a new map
	map = new Map(mapWidth,mapHeight);
	map->init(true);
	gameStatus = STARTUP;
	save();
}

void Engine::sendToBack(Actor *actor) {
	actors.remove(actor);
	actors.insertBefore(actor,0);
}

Actor *Engine::getClosestMonster(int x, int y, float range) const {
	Actor *closest = NULL;
	float bestDistance = 1E6f;
	
	for (Actor **iterator = actors.begin(); iterator != actors.end(); iterator++) {
		Actor *actor = *iterator;
		if (actor != player && actor->destructible 
			&& !actor->destructible->isDead()) {
			float distance = actor->getDistance(x,y);
			if (distance < bestDistance && (distance <= range || range ==0.0f)) {
				bestDistance = distance;
				closest = actor;
			}
		}
	}
	return closest;
}

bool Engine::pickATile(int *x, int *y, float maxRange, float AOE) {
	while (!TCODConsole::isWindowClosed()) {
		int dx = 0, dy = 0;
		render();
		TCODSystem::checkForEvent(TCOD_EVENT_KEY_PRESS,&lastKey,NULL);
		switch (lastKey.vk) {
		case TCODK_UP: dy = -1; break;
		case TCODK_DOWN: dy = 1; break;
		case TCODK_LEFT: dx =-1; break;
		case TCODK_RIGHT: dx = 1; break;
		case TCODK_ENTER: 
		{
			if ((player->getDistance(*x,*y) > maxRange && maxRange != 0)) {
				gui->message(TCODColor::pink,"this tile is out of range!");
			} else {
				return true;
			}
		}
		case TCODK_ESCAPE: return false;
		default: break;
		}
		*x += dx;
		*y += dy;
		
		if (*x > 100) *x = 100;
		if (*x < 0) *x = 0;
		if (*y > 100) *y = 100;
		if (*y < 0) *y = 0;
		
		for (int i = 0; i < map->height; i++) {
			for (int j = 0; j < map->width; j++) {
				if ( distance(*x,j,*y,i) <= AOE ) {
					if ( distance(*x,player->x,*y,player->y) >= maxRange && maxRange != 0) {
						mapcon->setCharBackground(j,i,TCODColor::desaturatedPink);
					} else {
						mapcon->setCharBackground(j,i,TCODColor::pink);
					}
				}
			}
		}
		
	int mapx1 = 0, mapy1 = 0, mapy2 = 0, mapx2 = 0;
	
	mapx1 = *x - ((screenWidth -22)/2);
	mapy1 = *y - ((screenHeight -12)/2);
	mapx2 = *x + ((screenWidth -22)/2);
	mapy2 = *y + ((screenHeight -12)/2);
	
if (mapx1 < 0) {
		mapx2 += (0-mapx1);
		mapx1 = 0;
	}	
	if (mapy1 < 0) { 
		mapy2 += (0-mapy1);
		mapy1 = 0;
	}
	if (mapx2 > 100) {
		mapx1 += (100-mapx2);
		mapx2 = 100;
	}
	if (mapy2 > 100) {
		mapy1 += (100-mapy2);
		mapy2 = 100;
	}
	//if (mapx2 > TCODConsole::root->getWidth() - 22) mapx2 = TCODConsole::root->getWidth() - 14;
	if (mapy2 > TCODConsole::root->getHeight() - 12) mapy2 = TCODConsole::root->getHeight() - 12;
	 
	TCODConsole::blit(mapcon, mapx1, mapy1, mapx2, mapy2, 
		TCODConsole::root, 22, 0);
		
	TCODConsole::flush();
		
	} 
	return false;
}

Actor *Engine::getActor(int x, int y) const {
	for (Actor **it = actors.begin(); it != actors.end(); it++) {
		Actor *actor = *it;
		if (actor->x ==x && actor->y==y && actor->destructible &&
			!actor->destructible->isDead()) {
			return actor;
		}
	}
	return NULL;
}

Actor *Engine::getAnyActor(int x, int y) const {
	for (Actor **it = actors.begin(); it != actors.end(); it++) {
		Actor *actor = *it;
		if (actor->x ==x && actor->y == y) {
			return actor;
		}
	}
	return NULL;
}

void Engine::win() {
	gui->message(TCODColor::darkRed,"You win!");
	gameStatus=Engine::VICTORY;
}