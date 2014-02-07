class Pickable : public Persistent {
public:
	enum PickableType {
		NONE,HEALER, LIGHTNING_BOLT, CONFUSER, FIREBALL
	};
	bool stacks;
	int stackSize;
	PickableType type;
	Pickable(bool stacks,int stacksize, PickableType type = NONE);
	bool pick(Actor *owner, Actor *wearer);
	virtual bool use(Actor *owner, Actor *wearer);
	void drop(Actor *owner, Actor *wearer);
	static Pickable *create(TCODZip &zip);
};

class Healer: public Pickable {
public: 
	float amount; //how much it heals
	Healer(float amount, bool stacks = true, int stackSize = 1, PickableType type = HEALER);
	bool use(Actor *owner, Actor *wearer);
	void load(TCODZip &zip);
	void save(TCODZip &zip);
};

class LightningBolt: public Pickable {
public: 
	float range, damage;
	LightningBolt(float range, float damage, bool stacks = true, 
		int stackSize = 1, PickableType type = LIGHTNING_BOLT);
	bool use(Actor *owner, Actor *wearer);
	void load(TCODZip &zip);
	void save(TCODZip &zip);
};

class Fireball : public LightningBolt {
public:
	float maxRange;
	Fireball(float range, float damage, float maxRange,
		bool stacks = true, int stackSize = 1, PickableType type = FIREBALL);
	bool use(Actor *owner, Actor *wearer);
	void load(TCODZip &zip);
	void save(TCODZip &zip);
};

class Confuser : public Pickable {
public:
	int nbTurns;
	float range;
	
	Confuser(int nbTurns, float range,
		bool stacks = true, int stackSize = 1, PickableType type = CONFUSER);
	bool use(Actor *owner, Actor *wearer);
	void load(TCODZip &zip);
	void save(TCODZip &zip);
};
