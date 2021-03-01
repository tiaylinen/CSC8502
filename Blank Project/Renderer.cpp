#include "Renderer.h"
#include "../nclgl/camera.h"
#include "../nclgl/HeightMap.h"
#include <algorithm>
#include <time.h> 

Renderer::Renderer(Window &parent) : OGLRenderer(parent)	{
	heightMap = new HeightMap(TEXTUREDIR "noise.png");
	Vector3 heightmapSize = heightMap->GetHeightmapSize();
	camera = new Camera(0.0f, 0.0f, heightmapSize * Vector3(0.5f, 5.0f, 0.5f));

	Vector3 dimensions = heightMap->GetHeightmapSize();
	camera->SetPosition(dimensions * Vector3(0.5, 1.2, 0.5));

	lightShader = new Shader("BumpVertex.glsl", "BumpFragment.glsl");
	skyboxShader = new Shader("skyboxVertex.glsl", "skyboxFragment.glsl");
	reflectShader = new Shader("reflectVertex.glsl", "reflectFragment.glsl");

	if (!lightShader->LoadSuccess() || !skyboxShader->LoadSuccess() || !reflectShader->LoadSuccess()) { return; }


	texture = SOIL_load_OGL_texture(TEXTUREDIR "facade.png", SOIL_LOAD_AUTO, SOIL_CREATE_NEW_ID, 0);
	buildingBump = SOIL_load_OGL_texture(TEXTUREDIR "NormalMap.png", SOIL_LOAD_AUTO, SOIL_CREATE_NEW_ID, SOIL_FLAG_MIPMAPS);
	terrainTex = SOIL_load_OGL_texture(TEXTUREDIR "ground.png", SOIL_LOAD_AUTO, SOIL_CREATE_NEW_ID, SOIL_FLAG_MIPMAPS);
	bumpmap = SOIL_load_OGL_texture(TEXTUREDIR "normal.JPG", SOIL_LOAD_AUTO, SOIL_CREATE_NEW_ID, SOIL_FLAG_MIPMAPS);
	trongleTex = SOIL_load_OGL_texture(TEXTUREDIR "trongletex.png", SOIL_LOAD_AUTO, SOIL_CREATE_NEW_ID, 0);
	waterTex = SOIL_load_OGL_texture(TEXTUREDIR "water.TGA", SOIL_LOAD_AUTO, SOIL_CREATE_NEW_ID, SOIL_FLAG_MIPMAPS);
	cubeMap = SOIL_load_OGL_cubemap(TEXTUREDIR "corona_lf.png", TEXTUREDIR "corona_rt.png",
									TEXTUREDIR "corona_up.png", TEXTUREDIR "corona_dn.png", 
									TEXTUREDIR "corona_ft.png", TEXTUREDIR "corona_bk.png", SOIL_LOAD_RGB, SOIL_CREATE_NEW_ID, 0);

	if (!terrainTex || !texture || !cubeMap || !trongleTex) { return; }
	SetTextureRepeating(terrainTex, true);
	SetTextureRepeating(bumpmap, true);
	SetTextureRepeating(texture, true);
	SetTextureRepeating(trongleTex, true);
	SetTextureRepeating(waterTex, true);

	quad = Mesh::GenerateQuad();
	building = Mesh::LoadFromMeshFile("singlebuilding.msh");
	trongle = Mesh::LoadFromMeshFile("trongle.msh");
	monolith = Mesh::LoadFromMeshFile("monolith.msh");
	water = Mesh::LoadFromMeshFile("OffsetCubeY.msh");
	glass = Mesh::GenerateQuad();

	light = new Light[4];

	root = new SceneNode();
	srand(time(NULL));
	for (int i = 0; i < 20; i++) {
		for (int j = 0; j < 20; j++) {
			float height = std::rand() % 100 + 30;
			SceneNode* s = new SceneNode();
			s->SetColour(Vector4(1.0f, 1.0f, 1.0f, 0.5f));
			s->SetTransform(Matrix4::Translation(Vector3(120.0f * j, -height, 120.0f * i)));
			s->SetModelScale(Vector3(std::rand() % 20 + 30, height, std::rand() % 20 + 30));
			s->SetBoundingRadius(100.0f);
			s->SetMesh(building);
			s->SetTexture(texture);
			root->AddChild(s);
		}
	}

	for (int i = 1; i < 10; i++) {
		pyramid = new Triangle(trongle);
		pyramid->SetColour(Vector4(1.0f, 1.0f, 1.0f, 0.5f));
		if (i == 3 || i == 6 || i == 9) {
			pyramid->SetTransform(Matrix4::Translation(Vector3(1500.0f, 300.0f, 200.0f * i)));
		} else if (i == 2 || i == 5 || i == 8) {
			pyramid->SetTransform(Matrix4::Translation(Vector3(750.0f, 300.0f, 200.0f * i)));
		} else {
			pyramid->SetTransform(Matrix4::Translation(Vector3(0.0f, 300.0f, 200.0f * i)));
		}
		pyramid->SetModelScale(Vector3(50.0f, 50.0f, 50.0f));
		pyramid->SetBoundingRadius(100.0f);
		pyramid->SetMesh(trongle);
		pyramid->SetTexture(trongleTex);
		root->AddChild(pyramid);
	}
	
	SceneNode* m = new SceneNode();
	m->SetColour(Vector4(1.0f, 1.0f, 1.0f, 1.0f));
	m->SetTransform(Matrix4::Translation(Vector3(3300.0f, 100.0f, 3300.0f)));
	m->SetModelScale(Vector3(20.0f, 20.0f, 20.0f));
	m->SetBoundingRadius(100.0f);
	m->SetMesh(monolith);
	m->SetTexture(trongleTex);
	root->AddChild(m);

	projMatrix = Matrix4::Perspective(1.0f, 15000.0f, (float)width / (float)height, 45.0f);

	glEnable(GL_DEPTH_TEST);
	glEnable(GL_CULL_FACE);
	glCullFace(GL_BACK);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glEnable(GL_TEXTURE_CUBE_MAP_SEAMLESS);

	waterRotate = 0.0f;
	waterCycle = 0.0f;

	init = true;}



Renderer::~Renderer(void)	{
	delete camera;
	delete lightShader;
	delete skyboxShader;
	delete heightMap;
	delete[] light;
	delete quad;
	glDeleteTextures(1, &texture);
}

void Renderer::UpdateScene(float dt) {
	camera->UpdateCamera(dt);
	viewMatrix = camera->BuildViewMatrix();
	root->Update(dt);
	waterRotate += dt * 2.0f; 
	waterCycle += dt * 0.25f;
	MoveCamera();
	RotateCamera();
}

void Renderer::BuildNodeLists(SceneNode* from) {
	Vector3 dir = from->GetWorldTransform().GetPositionVector() - camera->GetPosition();
	from->SetCameraDistance(Vector3::Dot(dir, dir));
	if (from->GetColour().w < 1.0f) { transparentNodeList.push_back(from); }
	else { nodeList.push_back(from); }

	for (vector <SceneNode*>::const_iterator i = from->GetChildIteratorStart(); i != from->GetChildIteratorEnd(); ++i) { BuildNodeLists((*i)); }
}

void Renderer::SortNodeLists() {
	std::sort(transparentNodeList.rbegin(), transparentNodeList.rend(), SceneNode::CompareByCameraDistance);
	std::sort(nodeList.begin(), nodeList.end(), SceneNode::CompareByCameraDistance);
}

void Renderer::DrawNodes() {
	for (const auto& i : nodeList) { DrawNode(i); }
	for (const auto& i : transparentNodeList) { DrawNode(i); }
}

void Renderer::DrawNode(SceneNode* n) {
	if (n->GetMesh()) {
		BindShader(lightShader);
		Matrix4 model = n->GetWorldTransform() * Matrix4::Scale(n->GetModelScale());
		glUniformMatrix4fv(glGetUniformLocation(lightShader->GetProgram(), "modelMatrix"), 1, false, model.values);
		
		texture = n->GetTexture();
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, texture);
		glUniform1i(glGetUniformLocation(lightShader->GetProgram(), "useTexture"), texture);

		glUniform1i(glGetUniformLocation(lightShader->GetProgram(), "bumpTex"), 1);
		glActiveTexture(GL_TEXTURE1);
		glBindTexture(GL_TEXTURE_2D, buildingBump);

		n->GetMesh()->Draw();
	}
}

void Renderer::DrawSkybox() {
	glDepthMask(GL_FALSE);
	BindShader(skyboxShader);
	UpdateShaderMatrices();
	quad->Draw();
	glDepthMask(GL_TRUE);
}

	
void Renderer::RenderScene()	{
	BuildNodeLists(root);
	SortNodeLists();

	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	DrawSkybox();
	DrawNodes();
	DrawHeightmap();
	DrawWater();
	ClearNodeLists();

}

void Renderer::ClearNodeLists() {
	transparentNodeList.clear();
	nodeList.clear();
}

void Renderer::DrawHeightmap() {
	BindShader(lightShader);

	for (int i = 0; i < 4; i++) {
		Light& l = light[i];
	}

	light[0].SetPosition(Vector3(heightMap->GetHeightmapSize().x / 2, 500.0f, heightMap->GetHeightmapSize().z / 2));
	light[1].SetPosition(Vector3(500, 100, 500));
	light[2].SetPosition(Vector3(750, 100, 2000));
	light[3].SetPosition(Vector3(1500, 100, 1500));

	light[0].SetColour(Vector4(1, 0.6, 0.2, 1));
	light[1].SetColour(Vector4(0, 0, 1, 1));
	light[2].SetColour(Vector4(0, 0, 1, 1));
	light[3].SetColour(Vector4(0, 0, 1, 1));

	light[0].SetRadius(2500);
	light[1].SetRadius(500);
	light[2].SetRadius(500);
	light[3].SetRadius(500);

	SetShaderLights(light);

	glUniform3fv(glGetUniformLocation(lightShader->GetProgram(), "cameraPos"), 1, (float*)& camera->GetPosition());

	glUniform1i(glGetUniformLocation(lightShader->GetProgram(), "diffuseTex"), 0);
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, terrainTex);

	glUniform1i(glGetUniformLocation(lightShader->GetProgram(), "bumpTex"), 1);
	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_2D, bumpmap);

	modelMatrix.ToIdentity();
	textureMatrix.ToIdentity();

	UpdateShaderMatrices();

	heightMap->Draw();
}

void Renderer::DrawWater() {
	BindShader(reflectShader);
	
	glUniform3fv(glGetUniformLocation(reflectShader->GetProgram(), "cameraPos"), 1, (float*)& camera->GetPosition());
	glUniform1i(glGetUniformLocation(reflectShader->GetProgram(), "diffuseTex"), 0);
	glUniform1i(glGetUniformLocation(reflectShader->GetProgram(), "cubeTex"), 2);
	
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, waterTex);
	
	glActiveTexture(GL_TEXTURE2);
	glBindTexture(GL_TEXTURE_CUBE_MAP, cubeMap);
	
	Vector3 hSize = heightMap->GetHeightmapSize();
	
	modelMatrix = Matrix4::Translation(hSize * 0.5f + Vector3(0, -200.0f, -1500)) * Matrix4::Scale(hSize * 0.4f) * Matrix4::Rotation(90, Vector3(1, 0, 0));
	textureMatrix = Matrix4::Translation(Vector3(waterCycle, 0.0f, waterCycle)) * Matrix4::Scale(Vector3(10, 10, 10)) * Matrix4::Rotation(waterRotate, Vector3(0, 0, 1));

	UpdateShaderMatrices();
	
	water->Draw();}void Renderer::MoveCamera() {
	if (camera->GetPosition().z < heightMap->GetHeightmapSize().z*0.33 && camera->GetPosition().x < heightMap->GetHeightmapSize().x * 0.66) {
		camera->SetPosition(camera->GetPosition() + Vector3(1.5, 0, 0));
		if (camera->GetPitch() > 10) {
			camera->SetPitch(camera->GetPitch() - 0.05);
		}
	}
	else if (camera->GetPosition().x < heightMap->GetHeightmapSize().x * 0.66) {
		camera->SetPosition(camera->GetPosition() + Vector3(0, 0, -1.5));
		camera->SetYaw(camera->GetYaw() + 0.07);
		if (camera->GetPitch() < 20) {
			camera->SetPitch(camera->GetPitch() + 0.05);
		} 
	}
	else {
		if (camera->GetPosition().z < 3100.0f) {
			camera->SetPosition(camera->GetPosition() + Vector3(0, 0, 3));
			camera->SetYaw(270);
		}
		else if (camera->GetPosition().y > 50.0f) {
			camera->SetPosition(camera->GetPosition() + Vector3(1, -1, 0));
		}
		else if (camera->GetPitch() < 25){
			camera->SetPosition(camera->GetPosition() + Vector3(0.75, 0, 0));
			camera->SetPitch(camera->GetPitch() + 0.1);
		}
	}
	if (camera->GetPosition().z < (heightMap->GetHeightmapSize().z / 3) + 1.0f && camera->GetPosition().z >(heightMap->GetHeightmapSize().z / 3)) {
		camera->SetYaw(90);
	}
}

void Renderer::RotateCamera() { }