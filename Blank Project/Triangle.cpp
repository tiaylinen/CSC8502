#include "Triangle.h"

Triangle::Triangle(Mesh* m) {
	SetMesh(m);

	SceneNode* body = new SceneNode(m, Vector4(1, 0, 0, 1)); // Red !
	body->SetModelScale(Vector3(10, 15, 5));
	body->SetTransform(Matrix4::Translation(Vector3(0, 35, 0)));
	body->SetBoundingRadius(15.0f);
}

void Triangle::Update(float dt) {
	transform = transform * Matrix4::Rotation(30.0f * dt, Vector3(0, 1, 0));
	SceneNode::Update(dt);
}