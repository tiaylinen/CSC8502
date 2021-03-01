#pragma once
#include "..\nclgl\SceneNode.h"
class Triangle : public SceneNode
{
public:
	Triangle(Mesh* m);
	~Triangle(void) {};
	void Update(float dt) override;
};
