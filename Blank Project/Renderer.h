#pragma once
#include "../nclgl/OGLRenderer.h"
#include "../nclgl/Frustum.h"
#include "../nclgl/SceneNode.h"
#include "Triangle.h"

class HeightMap;
class Camera;
class Light;
class SceneNode;
class MeshAnimation;

class Renderer : public OGLRenderer	{
public:
	Renderer(Window &parent);
	 ~Renderer(void);

	 void RenderScene()				override;
	 void UpdateScene(float msec)	override;
protected:
	void BuildNodeLists(SceneNode* from);
	void SortNodeLists();
	void ClearNodeLists();
	void DrawNodes();
	void DrawNode(SceneNode * n);
	void DrawSkybox();
	void DrawHeightmap();
	void DrawWater();

	void MoveCamera();
	void RotateCamera();

	
	MeshAnimation* anim;

	SceneNode* root;
	vector<Mesh*> sceneMeshes;

	Shader* lightShader;
	Shader* skyboxShader;
	Shader* reflectShader;
	Shader* transparentShader;

	Camera* camera;
	HeightMap* heightMap;
	Light* light;

	Light* pointLights;

	GLuint terrainTex;
	GLuint texture;
	GLuint cubeMap;
	GLuint trongleTex;
	GLuint bumpmap;
	GLuint buildingBump;
	GLuint waterTex;
	GLuint bump;

	GLuint sceneDiffuse;
	GLuint sceneBump;

	Mesh* quad;
	Mesh* building;
	Mesh* trongle;
	Mesh* street;
	Mesh* monolith;
	Mesh* water;
	Mesh* glass;
	Triangle* pyramid;

	vector<SceneNode*> transparentNodeList;
	vector<SceneNode*> nodeList;

	float waterRotate;
	float waterCycle;
};
