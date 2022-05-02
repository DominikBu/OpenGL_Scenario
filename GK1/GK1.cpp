// Nagłówki
#include <iostream>
#include <stdio.h>
#include <SFML/System/Time.hpp>
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#include <GL/glew.h>
#include <SFML/Window.hpp>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <fstream>
#include <sstream>


// Kody shaderów
const GLchar* vertexSource = R"glsl(
#version 150 core
in vec3 position;
in vec3 color;
in vec3 aNormal;
out vec3 Normal;
out vec3 FragPos;
//out vec3 Color;
in vec2 aTexCoord;
out vec2 TexCoord;
	uniform mat4 model;
	uniform mat4 view;
	uniform mat4 proj;

void main(){
	TexCoord = aTexCoord;
	
	//Color = color;
	gl_Position = proj * view * model * vec4(position, 1.0);
	Normal=aNormal;
	FragPos = vec3(model * vec4(position, 1.0));

}
)glsl";

const GLchar* fragmentSource = R"glsl(
#version 150 core
in vec3 Color;
out vec4 outColor;
in vec2 TexCoord;
in vec3 Normal;
in vec3 FragPos;
uniform vec3 camPos;
uniform vec3 lightPos;
uniform sampler2D texture1;
uniform sampler2D texture2;

void main()
{
//outColor = mix(texture(texture1, TexCoord), texture(texture2, TexCoord),0.5)*vec4(Color,1.0);
//outColor=texture(texture1, TexCoord) + texture(texture2, TexCoord) + texture(texture3, TexCoord);
//outColor = vec4(Color, 1.0);
//outColor = vec4(1.0,1.0,1.0, 1.0);
//outColor=texture(texture1, TexCoord);

//ambient
//dodanie mocy i koloru światła otoczenia
	float ambientStrength = 0.1;
	vec3 ambientlightColor = vec3(1.0,1.0,1.0);
	vec4 ambient = ambientStrength * vec4(ambientlightColor, 1.0);

//diffuse
//dodanie mocy i kolor światła rozproszonego
	float diffuseStrength = 1.0;
	vec3 diffuselightColor = vec3(1.0,1.0,1.0);
	vec3 norm = normalize(Normal);
	vec3 lightDir = normalize(lightPos - FragPos);
	float diff = max(dot(norm, lightDir), 0.0);
	vec3 diffvec = diff * diffuselightColor * diffuseStrength;
	vec4 diffuse = vec4(diffvec, 1.0);

//specular
//dodanie mocy i koloru składowej rozbłysków
	float specularStrength = 1.0;
	vec3 specularlightColor = vec3(1.0,1.0,1.0);
	vec3 viewDir = normalize(camPos - FragPos);
	vec3 reflectDir = reflect(-lightDir, norm);
	float spec = pow(max(dot(viewDir, reflectDir), 0.0), 128);
	vec4 specular = specularStrength * spec * vec4(specularlightColor, 1.0);

//disstance
//wpływ odległości na jasność fragmentów 
	float dist=distance(lightPos, FragPos);
	dist = (50-dist)/50;
	dist = max(dist, 0.0);

//uwzględniamy trzy składowe otoczenia: światła rozproszonego, rozbłysków i współczynnika odległości
	outColor = (ambient + dist * diffuse + dist * spec) * texture(texture1, TexCoord);

}
)glsl";

glm::vec3 cameraPos = glm::vec3(0.0f, 0.0f, 3.0f);
GLint uniCamPos;

glm::vec3 cameraFront = glm::vec3(0.0f, 0.0f, -1.0f);
glm::vec3 cameraUp = glm::vec3(0.0f, 1.0f, 0.0f);

float dist = 13;
int tryb = 2;

float obrot = 0;
bool firstMouse = true;
int lastX, lastY;
double yaw = -90;
double pitch = 0;

void LoadModelOBJ(int& punkty_, const char* filename, int buffer)
{
	int vert_num = 0;
	int trian_num = 0;

	std::ifstream myReadFile;
	myReadFile.open(filename);

	std::string output;

	//Policzenie vertexow i faces
	if (myReadFile.is_open())
	{
		while (!myReadFile.eof())
		{
			myReadFile >> output;
			if (output == "v")
				vert_num++;

			if (output == "f")
				trian_num++;
		}
	}

	myReadFile.close();
	myReadFile.open(filename);

	//Tworzenie tablic dwuwymiarowych
	float** vert;
	vert = new float *[vert_num]; 
	for (int i = 0; i < vert_num; i++)
		vert[i] = new float[3];

	int** trian;
	trian = new int *[trian_num];
	for (int i = 0; i < trian_num; i++)
		trian[i] = new int[3];

	int licz_vert = 0;
	int licz_triang = 0;

	//Przekazanie wartości pliku do tablic
	while (!myReadFile.eof())
	{
		myReadFile >> output;
		
		if (output == "v")
		{
			myReadFile >> vert[licz_vert][0];
			myReadFile >> vert[licz_vert][1];
			myReadFile >> vert[licz_vert][2];
			licz_vert++; 
		}
		if (output == "f")
		{
			myReadFile >> trian[licz_triang][0];
			myReadFile >> trian[licz_triang][1];
			myReadFile >> trian[licz_triang][2];
			licz_triang++;
		}
		output.clear(); //Pominięcie ostatniej lini pliku generowanego przez blender w której znajduje się tylko znak nowej lini
	}

	//Główna tablica punktów
	GLfloat* vertices = new GLfloat[trian_num * 9];

	int vert_current = 0;

	//Wypełnienie tablicy 
	for (int i = 0; i < trian_num; i++)
	{
		vertices[vert_current] = vert[trian[i][0] - 1][0];
		vertices[vert_current+1] = vert[trian[i][0] - 1][1];
		vertices[vert_current+2] = vert[trian[i][0] - 1][2];

		vertices[vert_current+3 ] = vert[trian[i][1] - 1][0];
		vertices[vert_current + 4] = vert[trian[i][1] - 1][1];
		vertices[vert_current + 5] = vert[trian[i][1] - 1][2];

		vertices[vert_current + 6] = vert[trian[i][2] - 1][0];
		vertices[vert_current + 7] = vert[trian[i][2] - 1][1];
		vertices[vert_current + 8] = vert[trian[i][2] - 1][2];
		vert_current += 9;
	}

	//Wysłanie tablicy do karty graficznej
	glBindBuffer(GL_ARRAY_BUFFER, buffer);
	glBufferData(GL_ARRAY_BUFFER, sizeof(GLfloat) * trian_num * 9, vertices, GL_STATIC_DRAW);


	punkty_ = trian_num * 9;

	delete[] vertices;

	for (int i = 0; i < vert_num; i++)
		delete[] vert[i];
	delete[] vert;

	for (int i = 0; i < trian_num; i++)
		delete[] trian[i];
	delete[] trian;
}

bool LoadModelOBJNormalsCoord(int * punkty_, const char* filename, int buffer)
{
	int vert_num = 0;
	int triangles = 0;
	int normals = 0;
	int coord_num = 0;
	int modele = 0;			//ilosc obiektow (modeli)
	int suma_punktow = 0;	//suma punktow modeli, które już przypisaliśmy

	std::ifstream myReadFile;
	myReadFile.open(filename);
	std::string output;
	if (myReadFile.is_open()) {
		while(!myReadFile.eof()) 
		{
			myReadFile >> output;
			if (output == "v") vert_num++;
			if (output == "f") triangles++;
			if (output == "vn") normals++;
			if (output == "vt") coord_num++;

			//Obliczenie ilosci punktow dla obiektow
			if (output == "o")
			{
				if (modele == 0) modele++;
				else
				{
					punkty_[modele-1] = triangles * 3 - suma_punktow;
					suma_punktow += punkty_[modele - 1];
					modele++;
				}
			}
		}
	}

	punkty_[4] = triangles * 3 - suma_punktow; //Obliczenie ilosci punktow dla ostatniego obiektu

	myReadFile.close();
	myReadFile.open(filename);


	float** vert;
	vert = new float* [vert_num]; //przydzielenie pamięci na w wierszy

	for (int i = 0; i < vert_num; i++)
		vert[i] = new float[3];


	int** trian;
	trian = new int* [triangles]; //przydzielenie pamięci na w wierszy

	for (int i = 0; i < triangles; i++)
		trian[i] = new int[9];

	float** norm;
	norm = new float* [normals]; //przydzielenie pamięci na w wierszy

	for (int i = 0; i < normals; i++)
		norm[i] = new float[3];

	float** coord;
	coord = new float* [coord_num]; //przydzielenie pamięci na w wierszy

	for (int i = 0; i < coord_num; i++)
		coord[i] = new float[2];

	int licz_vert = 0;
	int licz_triang = 0;
	int licz_normals = 0;
	int licz_coord = 0;


	while (!myReadFile.eof()) {
		output = "";
		myReadFile >> output;
		if (output == "vn") { myReadFile >> norm[licz_normals][0]; myReadFile >> norm[licz_normals][1]; myReadFile >> norm[licz_normals][2]; licz_normals++; }
		if (output == "v") { myReadFile >> vert[licz_vert][0]; myReadFile >> vert[licz_vert][1]; myReadFile >> vert[licz_vert][2]; licz_vert++; }
		if (output == "vt") { myReadFile >> coord[licz_coord][0]; myReadFile >> coord[licz_coord][1]; licz_coord++; }

		if (output == "f") {

			for (int i = 0; i < 9; i += 3)
			{
				std::string s;
				myReadFile >> s;
				std::stringstream ss(s);

				std::vector <std::string> el;
				std::string item;


				while (getline(ss, item, '/')) {
					el.push_back(item);
				}
				trian[licz_triang][i] = std::stoi(el[0]);
				trian[licz_triang][i + 1] = std::stoi(el[1]);
				trian[licz_triang][i + 2] = std::stoi(el[2]);


			}
			licz_triang++;
		}
	}
	GLfloat* vertices = new GLfloat[triangles * 24];

	int vert_current = 0;

	for (int i = 0; i < triangles; i++)
	{
		vertices[vert_current] = vert[trian[i][0] - 1][0];
		vertices[vert_current + 1] = vert[trian[i][0] - 1][1];
		vertices[vert_current + 2] = vert[trian[i][0] - 1][2];
		vertices[vert_current + 3] = norm[trian[i][2] - 1][0];
		vertices[vert_current + 4] = norm[trian[i][2] - 1][1];
		vertices[vert_current + 5] = norm[trian[i][2] - 1][2];
		vertices[vert_current + 6] = coord[trian[i][1] - 1][0];
		vertices[vert_current + 7] = coord[trian[i][1] - 1][1];

		vertices[vert_current + 8] = vert[trian[i][3] - 1][0];
		vertices[vert_current + 9] = vert[trian[i][3] - 1][1];
		vertices[vert_current + 10] = vert[trian[i][3] - 1][2];
		vertices[vert_current + 11] = norm[trian[i][5] - 1][0];
		vertices[vert_current + 12] = norm[trian[i][5] - 1][1];
		vertices[vert_current + 13] = norm[trian[i][5] - 1][2];
		vertices[vert_current + 14] = coord[trian[i][4] - 1][0];
		vertices[vert_current + 15] = coord[trian[i][4] - 1][1];

		vertices[vert_current + 16] = vert[trian[i][6] - 1][0];
		vertices[vert_current + 17] = vert[trian[i][6] - 1][1];
		vertices[vert_current + 18] = vert[trian[i][6] - 1][2];
		vertices[vert_current + 19] = norm[trian[i][8] - 1][0];
		vertices[vert_current + 20] = norm[trian[i][8] - 1][1];
		vertices[vert_current + 21] = norm[trian[i][8] - 1][2];
		vertices[vert_current + 22] = coord[trian[i][7] - 1][0];
		vertices[vert_current + 23] = coord[trian[i][7] - 1][1];

		vert_current += 24;
	}

	glBindBuffer(GL_ARRAY_BUFFER, buffer);
	glBufferData(GL_ARRAY_BUFFER, sizeof(GLfloat) * triangles * 24, vertices, GL_STATIC_DRAW);


	


	delete[] vertices;

	for (int i = 0; i < vert_num; i++)
		delete[] vert[i];
	delete[] vert;

	for (int i = 0; i < triangles; i++)
		delete[] trian[i];
	delete[] trian;

	for (int i = 0; i < normals; i++)
		delete[] norm[i];
	delete[] norm;

	for (int i = 0; i < coord_num; i++)
		delete[] coord[i];
	delete[] coord;

	return 0;
}

void LoadModelOBJ_EBO(int& punkty_, const char* filename, int buffer_vbo, int buffer_ebo)
{

	int vert_num = 0;
	int trian_num = 0;

	std::ifstream myReadFile;
	myReadFile.open(filename);

	std::string output;

	//Policzenie vertexow i faces
	if (myReadFile.is_open())
	{
		while (!myReadFile.eof())
		{
			myReadFile >> output;
			if (output == "v")
				vert_num++;

			if (output == "f")
				trian_num++;
		}
	}

	myReadFile.close();
	myReadFile.open(filename);

	float* vert;
	vert = new float[vert_num * 3]; //przydzielenie pamięci na vertexy

	int* element;
	element = new int[trian_num * 3];// przydzielenie pamięci na elementy

	int licz_vert = 0;
	int licz_element = 0;
	int tmp = 0;

	//Przekazanie wartości pliku do tablic
	while (!myReadFile.eof())
	{
		myReadFile >> output;
		if (output == "v")
		{
			myReadFile >> vert[licz_vert];
			myReadFile >> vert[licz_vert+1];
			myReadFile >> vert[licz_vert+2];
			licz_vert += 3;
		}
		if (output == "f")
		{
			myReadFile >> tmp;
			tmp--;
			element[licz_element] = tmp;
			myReadFile >> tmp;
			tmp--;
			element[licz_element+1] = tmp;
			myReadFile >> tmp;
			tmp--;
			element[licz_element+2] = tmp;
			licz_element += 3;
		}
		output.clear(); //Pominięcie ostatniej lini pliku generowanego przez blender w której znajduje się tylko znak nowej lini
	}

	//Przesłanie tablic do karty graficznej
	glBindBuffer(GL_ARRAY_BUFFER, buffer_vbo);
	glBufferData(GL_ARRAY_BUFFER, sizeof(GLfloat) * vert_num * 3, vert, GL_STATIC_DRAW);

	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, buffer_ebo);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(GLfloat) * trian_num * 3, element, GL_STATIC_DRAW);


	delete[] vert;
	delete[] element;
}

void kostka( int buffer)
{
	int punkty = 24;

	float vertices[] = {
		-0.5f, -0.5f, -0.5f, 0.0f, 0.0f, 0.0f, 0.0f,0.0f,
		0.5f, -0.5f, -0.5f, 1.0f, 0.0f, 0.0f, 100.0f,0.0f,
		0.5f, 0.5f, -0.5f, 1.0f, 1.0f, 0.0f, 100.0f,100.0f,
		0.5f, 0.5f, -0.5f, 1.0f, 1.0f, 0.0f, 100.0f,100.0f,
		-0.5f, 0.5f, -0.5f, 0.0f, 1.0f, 0.0f, 0.0f,100.0f,
		-0.5f, -0.5f, -0.5f, 0.0f, 0.0f, 0.0f, 0.0f,0.0f,

		-0.5f, -0.5f, 0.5f, 0.0f, 0.0f, 0.0f, 0.0f,0.0f,
		0.5f, -0.5f, 0.5f, 1.0f, 0.0f, 0.0f, 0.1f,0.0f,
		0.5f, 0.5f, 0.5f, 1.0f, 1.0f, 0.0f, 0.1f,0.1f,
		0.5f, 0.5f, 0.5f, 1.0f, 1.0f, 0.0f, 0.1f,0.1f,
		-0.5f, 0.5f, 0.5f, 0.0f, 1.0f, 0.0f, 0.0f,0.1f,
		-0.5f, -0.5f, 0.5f, 0.0f, 0.0f, 0.0f, 0.0f,0.0f,

		-0.5f, 0.5f, 0.5f, 1.0f, 0.0f, 0.0f, 0.0f,0.0f,
		-0.5f, 0.5f, -0.5f, 1.0f, 1.0f, 0.0f, 1.0f,0.0f,
		-0.5f, -0.5f, -0.5f, 0.0f, 1.0f, 0.0f, 1.0f,1.0f,
		-0.5f, -0.5f, -0.5f, 0.0f, 1.0f, 0.0f, 1.0f,1.0f,
		-0.5f, -0.5f, 0.5f, 0.0f, 0.0f, 0.0f, 0.0f,1.0f,
		-0.5f, 0.5f, 0.5f, 1.0f, 0.0f, 0.0f, 0.0f,0.0f,

		0.5f, 0.5f, 0.5f, 1.0f, 0.0f, 0.0f, 0.0f,0.0f,
		0.5f, 0.5f, -0.5f, 1.0f, 1.0f, 0.0f, 1.0f,0.0f,
		0.5f, -0.5f, -0.5f, 0.0f, 1.0f, 0.0f, 1.0f,1.0f,
		0.5f, -0.5f, -0.5f, 0.0f, 1.0f, 0.0f, 1.0f,1.0f,
		0.5f, -0.5f, 0.5f, 0.0f, 0.0f, 0.0f, 0.0f,1.0f,
		0.5f, 0.5f, 0.5f, 1.0f, 0.0f, 0.0f, 0.0f,0.0f,
		/*
		-0.5f, -0.5f, -0.5f, 0.0f, 1.0f, 0.0f,
		0.5f, -0.5f, -0.5f, 1.0f, 1.0f, 0.0f,
		0.5f, -0.5f, 0.5f, 1.0f, 0.0f, 0.0f,
		0.5f, -0.5f, 0.5f, 1.0f, 0.0f, 0.0f,
		-0.5f, -0.5f, 0.5f, 0.0f, 0.0f, 0.0f,
		-0.5f, -0.5f, -0.5f, 0.0f, 1.0f, 0.0f,

		-0.5f, 0.5f, -0.5f, 0.0f, 1.0f, 0.0f,
		0.5f, 0.5f, -0.5f, 1.0f, 1.0f, 0.0f,
		0.5f, 0.5f, 0.5f, 1.0f, 0.0f, 0.0f,
		0.5f, 0.5f, 0.5f, 1.0f, 0.0f, 0.0f,
		-0.5f, 0.5f, 0.5f, 0.0f, 0.0f, 0.0f,
		-0.5f, 0.5f, -0.5f, 0.0f, 1.0f, 0.0f*/
	};
	
	
	glBindBuffer(GL_ARRAY_BUFFER, buffer);
	glBufferData(GL_ARRAY_BUFFER, sizeof(GLfloat) * punkty * 8, vertices, GL_STATIC_DRAW);
}

void ustawKamereKlawisze(GLint _uniView, float _time)
{
	float cameraSpeed = 0.000002f*_time;							//prędkość kamery

	if (sf::Keyboard::isKeyPressed(sf::Keyboard::Up))		//Klawisz w góre - idziemy do przodu
	{
		cameraPos += cameraSpeed * cameraFront;
	}
	if(sf::Keyboard::isKeyPressed(sf::Keyboard::Down))		//Klawisz w dół - idziemy do tyłu
	{
		cameraPos -= cameraSpeed * cameraFront;
	}
	if (sf::Keyboard::isKeyPressed(sf::Keyboard::Left))		//Klawisz w lewo - poruszamy się w lewo
	{
		cameraPos -= glm::normalize(glm::cross(cameraFront, cameraUp)) * cameraSpeed;
	}
	if (sf::Keyboard::isKeyPressed(sf::Keyboard::Right))	//Klawisz w prawo - poruszamy się w prawo
	{
		cameraPos += glm::normalize(glm::cross(cameraFront, cameraUp)) * cameraSpeed;
	}

	glUniform3fv(uniCamPos, 1, glm::value_ptr(cameraPos));

	//Aktualizacja kamery
	glm::mat4 view;
	view = glm::lookAt(cameraPos, cameraPos + cameraFront, cameraUp);
	glUniformMatrix4fv(_uniView, 1, GL_FALSE, glm::value_ptr(view));
}

void ustawKamereMysz(GLint _uniView, float _elapsedTime, const sf::Window& _window)
{
	sf::Vector2i localPosition = sf::Mouse::getPosition(_window);	//Wyznaczenie logalnej pozycji kursora

	sf::Vector2i position;
	bool relocation = false;

	//Gdy mysz chce przejść poza krawędź, przechodzi na drugą strone
	if (localPosition.x <= 0)
	{
		position.x = _window.getSize().x;
		position.y = localPosition.y;
		relocation = true;
	}
	if (localPosition.x >= _window.getSize().x-1)
	{
		position.x = 0;
		position.y = localPosition.y;
		relocation = true;
	}
	if (localPosition.y <= 0)
	{
		position.x = localPosition.x;
		position.y = _window.getSize().y;
		relocation = true;
	}
	if (localPosition.y >= _window.getSize().y - 1)
	{
		position.y = 0;
		position.x = localPosition.x;
		relocation = true;
	}

	//Ustawienie nowego położenia myszy
	if (relocation)
	{
		sf::Mouse::setPosition(position, _window);
		firstMouse = true;
	}

	localPosition = sf::Mouse::getPosition(_window);		//Pobranie nowego położenia myszy

	//Nowa ostatnia pozycja gdy mysz przeskoczyła na drugą stronę obrazu
	if (firstMouse)
	{
		lastX = localPosition.x;
		lastY = localPosition.y;
		firstMouse = false;
	}

	//wyznaczenie zmiany położenia
	double xoffset = localPosition.x - lastX;
	double yoffset = localPosition.y - lastY;

	//zapamiętanie ostatniej pozycji
	lastX = localPosition.x;
	lastY = localPosition.y;

	//Ustawienie szybkości kamery
	double sensitivity = 0.001f;
	double cameraSpeed = 0.005f * _elapsedTime;

	//Aktualizacja kątów ustawienia kamery
	xoffset *= sensitivity;
	yoffset *= sensitivity;
	yaw += xoffset * cameraSpeed;
	pitch -= yoffset * cameraSpeed;

	//Zablokowanie pełnego ruchu w pionie
	if (pitch > 89.0f)
		pitch = 89.0f;
	if (pitch < -89.0f)
		pitch = -89.0f;

	//Wyznaczenie nowych wartości wektora pochylenia kamery
	glm::vec3 front;
	front.x = cos(glm::radians(yaw)) * cos(glm::radians(pitch));
	front.y = sin(glm::radians(pitch));
	front.z = sin(glm::radians(yaw)) * cos(glm::radians(pitch));
	cameraFront = glm::normalize(front);

	//Aktualizacjia widoku
	glm::mat4 view;
	view = glm::lookAt(cameraPos, cameraPos + cameraFront, cameraUp);
	glUniformMatrix4fv(_uniView, 1, GL_FALSE, glm::value_ptr(view));
}

void StereoProjection(GLuint shaderProgram_, float _left, float _right, float _bottom, float _top, float _near, float _far, float _zero_plane, float _dist, float _eye)
{
	//    Perform the perspective projection for one eye's subfield.
	//    The projection is in the direction of the negative z-axis.
	//            _left=-6.0;
	//            _right=6.0;
	//            _bottom=-4.8;
	   //             _top=4.8;
	//    [default: -6.0, 6.0, -4.8, 4.8]
	//    left, right, bottom, top = the coordinate range, in the plane of zero parallax setting,
	//         which will be displayed on the screen.
	//         The ratio between (right-left) and (top-bottom) should equal the aspect
	//    ratio of the display.


	   //                  _near=6.0;
	   //                  _far=-20.0;
	//    [default: 6.0, -6.0]
	//    near, far = the z-coordinate values of the clipping planes.

	   //                  _zero_plane=0.0;
	//    [default: 0.0]
	//    zero_plane = the z-coordinate of the plane of zero parallax setting.

	//    [default: 14.5]
	  //                     _dist=10.5;
	//   dist = the distance from the center of projection to the plane of zero parallax.

	//    [default: -0.3]
	  //                 _eye=-0.3;
	//    eye = half the eye separation; positive for the right eye subfield,
	//    negative for the left eye subfield.

	float   _dx = _right - _left;
	float   _dy = _top - _bottom;

	float   _xmid = (_right + _left) / 2.0;
	float   _ymid = (_top + _bottom) / 2.0;

	float   _clip_near = _dist + _zero_plane - _near;
	float   _clip_far = _dist + _zero_plane - _far;

	float  _n_over_d = _clip_near / _dist;

	float   _topw = _n_over_d * _dy / 2.0;
	float   _bottomw = -_topw;
	float   _rightw = _n_over_d * (_dx / 2.0 - _eye);
	float   _leftw = _n_over_d * (-_dx / 2.0 - _eye);

	// Create a fustrum, and shift it off axis
	glm::mat4 proj = glm::frustum(_leftw, _rightw, _bottomw, _topw, _clip_near, _clip_far);

	proj = glm::translate(proj, glm::vec3(-_xmid - _eye, -_ymid, 0));

	GLint uniProj = glGetUniformLocation(shaderProgram_, "proj");
	glUniformMatrix4fv(uniProj, 1, GL_FALSE, glm::value_ptr(proj));
}

int main()
{
	sf::ContextSettings settings;
	settings.depthBits = 24;
	settings.stencilBits = 8;
	settings.antialiasingLevel = 4;

	// Okno renderingu
	sf::Window window(sf::VideoMode(800, 800, 32), "OpenGL", sf::Style::Fullscreen | sf::Style::Close, settings);

	window.setMouseCursorVisible(false);
	window.setMouseCursorGrabbed(true);


	// Inicjalizacja GLEW
	glewExperimental = GL_TRUE;
	glewInit();

	// Utworzenie VAO (Vertex Array Object)
	GLuint vao;
	glGenVertexArrays(1, &vao);
	glBindVertexArray(vao);

	GLuint ebo;
	glGenBuffers(1, &ebo);
	
	// Utworzenie VBO (Vertex Buffer Object)
	// i skopiowanie do niego danych wierzchołkowych
	GLuint vbo;
	glGenBuffers(1, &vbo);

	//int punkty = 0;

	//kostka(vbo);
	//LoadModelOBJ(punkty, "pokoj.obj", vbo);
	//LoadModelOBJ_EBO(punkty, "pokoj.obj", vbo, ebo);
	
	glEnable(GL_MULTISAMPLE);
	glEnable(GL_CULL_FACE);

	int punkty[5];												//tablica przechowywujaca ilosc punktow danego obiektu
	LoadModelOBJNormalsCoord(punkty, "models/pokoj2.obj", vbo);


	// Utworzenie i skompilowanie shadera wierzchołków
	GLuint vertexShader = glCreateShader(GL_VERTEX_SHADER);
	glShaderSource(vertexShader, 1, &vertexSource, NULL);
	glCompileShader(vertexShader);

	GLint status;													//zmienna do przechowywania statusu błędu
	glGetShaderiv(vertexShader, GL_COMPILE_STATUS, &status);		//nadanie zmiennej wartości

	bool AllShaders = true;											//zmienna która będzie przechowywać czy wystąpił błąd,
																	//po zkompilowaniu wszystkich shaderów program sie zakonczy jest był błąd,

	if (!status)													//warunek gdy wystąpił błąd
	{
		std::cout << "Compilation vertexShader ERROR" << std::endl;	//informacja o błędzie w konsoli
		char buffer[512];											//buffer który będzie przechowywał informacje o błędach

		glGetShaderInfoLog(vertexShader, 512, NULL, buffer);		//zapisanie informacji o błędach do buffera
		std::cout << buffer << std::endl;							//wypisanie błędów
		AllShaders = false;	
	}
	else
	{
		std::cout << "Compilation vertexShader OK" << std::endl;	//informacja o poprawnym zkompilowaniu shadera
	}

	// Utworzenie i skompilowanie shadera fragmentów
	GLuint fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
	glShaderSource(fragmentShader, 1, &fragmentSource, NULL);
	glCompileShader(fragmentShader);

	GLint status2;													//zmienna do przechowywania statusu błędu
	glGetShaderiv(fragmentShader, GL_COMPILE_STATUS, &status2);		//nadanie zmiennej wartości

	if (!status2)													//warunek gdy wystąpił błąd
	{	
		std::cout << "Compilation fragmentShader ERROR" << std::endl;//informacja o błędzie w konsoli
		char buffer[512];											//buffer który będzie przechowywał informacje o błędach

		glGetShaderInfoLog(fragmentShader, 512, NULL, buffer);		//zapisanie informacji o błędach do buffera
		std::cout << buffer << std::endl;
		AllShaders = false;
	}
	else
	{
		std::cout << "Compilation fragmentShader OK" << std::endl;	//informacja o poprawnym zkompilowaniu shadera
	}

	if (!AllShaders)												//po wszystkich shaderach sprawdzany warunek, czy wszystkie shadery zostały poprawnie skompilowane
		return 1;													//jeśli nie zamknij program

	// Zlinkowanie obu shaderów w jeden wspólny program
	GLuint shaderProgram = glCreateProgram();
	glAttachShader(shaderProgram, vertexShader);
	glAttachShader(shaderProgram, fragmentShader);
	glBindFragDataLocation(shaderProgram, 0, "outColor");
	glLinkProgram(shaderProgram);
	glUseProgram(shaderProgram);

	// Specifikacja formatu danych wierzchołkowych
	GLint posAttrib = glGetAttribLocation(shaderProgram, "position");
	glEnableVertexAttribArray(posAttrib);
	glVertexAttribPointer(posAttrib, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(GLfloat), 0);
	//GLint colAttrib = glGetAttribLocation(shaderProgram, "color");
	//glEnableVertexAttribArray(colAttrib);
	//glVertexAttribPointer(colAttrib, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(GLfloat), (void*)(3 * sizeof(GLfloat)));

	GLint NorAttrib = glGetAttribLocation(shaderProgram, "aNormal");
	glEnableVertexAttribArray(NorAttrib);
	glVertexAttribPointer(NorAttrib, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(GLfloat), (void*)(3 * sizeof(GLfloat)));

	GLint TexCoord = glGetAttribLocation(shaderProgram, "aTexCoord");
	glEnableVertexAttribArray(TexCoord);
	glVertexAttribPointer(TexCoord, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(GLfloat), (void*)(6 * sizeof(GLfloat)));

	glm::mat4 model = glm::mat4(1.0f);														//Stworzenie macierzy modelu
	//model = glm::rotate(model, glm::radians(20.0f), glm::vec3(0.0f, 0.0f, 1.0f));			//Obliczanie rotacji modelu

	GLint uniTrans = glGetUniformLocation(shaderProgram, "model");							//ustawienie lokalizacji uniform model w skaderze
	glUniformMatrix4fv(uniTrans, 1, GL_FALSE, glm::value_ptr(model));						//Wysłanie modelu

	GLint uniView = glGetUniformLocation(shaderProgram, "view");							//ustawienie lokalizacji uniform view w skaderze (wystarczy raz)

	glm::mat4 proj = glm::perspective(glm::radians(45.0f), 800.0f / 800.0f, 0.06f, 100.0f);	//stworzenie macierzy projekcji
	GLint uniProj = glGetUniformLocation(shaderProgram, "proj");							//ustawienie lokalizacji uniform proj w skaderze	
	glUniformMatrix4fv(uniProj, 1, GL_FALSE, glm::value_ptr(proj));							//wysłanie projekcji

	glm::vec3 lightPos(5.0f, 2.0f, -4.0f);
	GLint uniLightPos = glGetUniformLocation(shaderProgram, "lightPos");
	glUniform3fv(uniLightPos, 1, glm::value_ptr(lightPos));

	uniCamPos = glGetUniformLocation(shaderProgram, "camPos");


	GLint prymityw = GL_TRIANGLE_FAN;
	//GLint mouse_x = 0;
	//GLint mouse_y = 0;

	glEnable(GL_DEPTH_TEST);

	unsigned int texture[5];								//Tworzenie identyfikatora tekstury

	glGenTextures(1, &texture[0]);						//Generowanie tekstury
	glBindTexture(GL_TEXTURE_2D, texture[0]);				//Ustawienie tekstury jako bieżącej

	//Parametry do filtrowania
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

	//glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	//glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY_EXT, 8.0f);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR_MIPMAP_LINEAR);

	//Wczytanie pliku z naszą teksturą nr. 1
	int width, height, nrChannels;
	stbi_set_flip_vertically_on_load(true);
	unsigned char* data = stbi_load("txt/metal.bmp", &width, &height, &nrChannels, 0);
	if (data)
	{
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, data);
		glGenerateMipmap(GL_TEXTURE_2D);
	}
	else
	{
		std::cout << "Error przy ladowaniu textury 1." << std::endl;
	}
	stbi_image_free(data);
								

	glGenTextures(1, &texture[1]);						//Generowanie tekstury
	glBindTexture(GL_TEXTURE_2D, texture[1]);				//Ustawienie tekstury jako bieżącej

	//Parametry do filtrowania
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

	//Wczytanie pliku z naszą teksturą nr. 2
	
	stbi_set_flip_vertically_on_load(true);
	data = stbi_load("txt/obraz.bmp", &width, &height, &nrChannels, 0);
	if (data)
	{
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, data);
		glGenerateMipmap(GL_TEXTURE_2D);
	}
	else
	{
		std::cout << "Error przy ladowaniu textury nr.2" << std::endl;
	}
	stbi_image_free(data);
	
	
	glGenTextures(1, &texture[2]);						//Generowanie tekstury
	glBindTexture(GL_TEXTURE_2D, texture[2]);				//Ustawienie tekstury jako bieżącej

	//Parametry do filtrowania
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

	//Wczytanie pliku z naszą teksturą nr. 3

	stbi_set_flip_vertically_on_load(true);
	data = stbi_load("txt/crate.bmp", &width, &height, &nrChannels, 0);
	if (data)
	{
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, data);
		glGenerateMipmap(GL_TEXTURE_2D);
	}
	else
	{
		std::cout << "Error przy ladowaniu textury nr.3" << std::endl;
	}
	stbi_image_free(data);

	glGenTextures(1, &texture[3]);						//Generowanie tekstury
	glBindTexture(GL_TEXTURE_2D, texture[3]);				//Ustawienie tekstury jako bieżącej

	//Parametry do filtrowania
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

	//Wczytanie pliku z naszą teksturą nr. 4

	stbi_set_flip_vertically_on_load(true);
	data = stbi_load("txt/wood.bmp", &width, &height, &nrChannels, 0);
	if (data)
	{
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, data);
		glGenerateMipmap(GL_TEXTURE_2D);
	}
	else
	{
		std::cout << "Error przy ladowaniu textury nr.4" << std::endl;
	}
	stbi_image_free(data);

	glGenTextures(1, &texture[4]);						//Generowanie tekstury
	glBindTexture(GL_TEXTURE_2D, texture[4]);				//Ustawienie tekstury jako bieżącej

	//Parametry do filtrowania
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

	//Wczytanie pliku z naszą teksturą nr. 5

	stbi_set_flip_vertically_on_load(true);
	data = stbi_load("txt/table.bmp", &width, &height, &nrChannels, 0);
	if (data)
	{
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, data);
		glGenerateMipmap(GL_TEXTURE_2D);
	}
	else
	{
		std::cout << "Error przy ladowaniu textury nr.5" << std::endl;
	}
	stbi_image_free(data);

	//Powiązanie tekstur w shaderach
	glUniform1i(glGetUniformLocation(shaderProgram, "texture1"), 0);


	sf::Clock clock;
	sf::Time time;
	window.setFramerateLimit(60);		//Ustawiona wartość fps
	int licznik = 0;
	
	// Rozpoczęcie pętli zdarzeń
	bool running = true;
	while (running) {
		time = clock.restart();		//zapisanie czasu wykonania pojedyńczej klatki
		licznik++;					//zwiększenie licznika
		float ffps = 1000000 / time.asMicroseconds();	//Obliczanie ilości fps	
		if (licznik > ffps)		//Aktualizacja ilości fps po pewnym czasie
		{
			window.setTitle(std::to_string(ffps));
			licznik = 0;
		}
		
		sf::Event windowEvent;
		while (window.pollEvent(windowEvent)) {
			switch (windowEvent.type) {
			case sf::Event::Closed:
				running = false;
				break;
			case sf::Event::KeyPressed:							//Zdarzenie naciśnięcia klawisza
				switch (windowEvent.key.code) {
				case sf::Keyboard::Escape:
					running = false;
					break;
				case sf::Keyboard::Num1:
					prymityw = GL_POINTS;
					break;
				case sf::Keyboard::Num2:
					prymityw = GL_LINES;
					break;
				case sf::Keyboard::Num3:
					prymityw = GL_LINE_STRIP;
					break;
				case sf::Keyboard::Num4:
					prymityw = GL_LINE_LOOP;
					break;
				case sf::Keyboard::Num5:
					prymityw = GL_TRIANGLES;
					break;
				case sf::Keyboard::Num6:
					prymityw = GL_TRIANGLE_STRIP;
					break;
				case sf::Keyboard::Num7:
					prymityw = GL_TRIANGLE_FAN;
					break;
				case sf::Keyboard::Num8:
					prymityw = GL_QUADS;
					break;
				case sf::Keyboard::Num9:
					prymityw = GL_QUAD_STRIP;
					break;
				case sf::Keyboard::Num0:
					prymityw = GL_POLYGON;
					break;
				case sf::Keyboard::A:
					dist += 0.01;
					break;
				case sf::Keyboard::B:
					dist -= 0.01;
					break;
				case sf::Keyboard::Q:
					tryb = 0;
					break;
				case sf::Keyboard::W:
					tryb = 1;
					break;
				case sf::Keyboard::E:
					tryb = 2;
					break;
				}
				break;
			case sf::Event::MouseMoved:			//ruch kursora
				ustawKamereMysz(uniView, time.asMicroseconds(), window);
				break;
			}
		}

		ustawKamereKlawisze(uniView, time.asMicroseconds());

		// Nadanie scenie koloru czarnego
		glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		
		// Wymiana buforów tylni/przedni

		//ładowanie więcej niż jednej tekstury w tym samym czasie
		/*
		glActiveTexture(GL_TEXTURE0);	
		glBindTexture(GL_TEXTURE_2D, texture1);		//Ustawiamy teksturę1 jako bierzącą
		glActiveTexture(GL_TEXTURE1);
		glBindTexture(GL_TEXTURE_2D, texture2);		//Ustawiamy teksturę2 jako bierzącą
		
		
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, texture1);		//Ustawiamy teksturę1 jako bierzącą

		glDrawArrays(GL_TRIANGLES, 0, 12);

		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, texture2);		//Ustawiamy teksturę2 jako bierzącą

		glDrawArrays(GL_TRIANGLES, 12, 24);
		*/

		
		/*
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, texture3);		//Ustawiamy teksturę3 jako bierzącą
		glActiveTexture(GL_TEXTURE1);
		glBindTexture(GL_TEXTURE_2D, texture3);		//Ustawiamy teksturę3 jako bierzącą

		glDrawArrays(GL_TRIANGLES, 0, 12);
		
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, texture1);		//Ustawiamy teksturę1 jako bierzącą
		glActiveTexture(GL_TEXTURE1);
		glBindTexture(GL_TEXTURE_2D, texture2);		//Ustawiamy teksturę2 jako bierzącą
		
		glDrawArrays(GL_TRIANGLES, 12, 24);
		*/
		glActiveTexture(GL_TEXTURE0);
		int suma_punktow = 0; //zmienna przechowywujaca aktualna pozycje punktow
		switch (tryb)
		{
		case 0:	//projekcja stereoskopowa asymetryczna
			glViewport(0, 0, window.getSize().x, window.getSize().y);						//odpowienie umiejscowienie obrazu na ekranie
			glDrawBuffer(GL_BACK_LEFT);						//Dla oka lewego
			StereoProjection(shaderProgram, -6, 6, -4.8, 4.8, 12.99, -100, 0, dist, -0.05);	//Odpowiednie przekształcenia projekcji
			glColorMask(true, false, false, false);			//Odpowiedni filtr koloru
			//glDrawArrays(prymityw, 0, punkty[0]);					//Narysowanie
			//glDrawElements(prymityw, punkty, GL_UNSIGNED_INT, 0);

			//Rysowanie
			for (int i = 0; i < 5; i++)
			{
				glBindTexture(GL_TEXTURE_2D, texture[i]);
				glDrawArrays(prymityw, suma_punktow, punkty[i]);
				suma_punktow += punkty[i];
			}
			suma_punktow = 0;

			glClear(GL_DEPTH_BUFFER_BIT);
			glDrawBuffer(GL_BACK_RIGHT);					//Dla oka prawego
			StereoProjection(shaderProgram, -6, 6, -4.8, 4.8, 12.99, -100, 0, dist, 0.05); //Odpowiednie przekształcenia projekcji
			glColorMask(false, false, true, false);			//Odpowiedni filtr koloru
			//glDrawArrays(prymityw, 0, punkty[0]);					//Narysowanie
			//glDrawElements(prymityw, punkty, GL_UNSIGNED_INT, 0);

			//Rysowanie
			for (int i = 0; i < 5; i++)
			{
				glBindTexture(GL_TEXTURE_2D, texture[i]);
				glDrawArrays(prymityw, suma_punktow, punkty[i]);
				suma_punktow += punkty[i];
			}
			suma_punktow = 0;

			glColorMask(true, true, true, true);			//Ustawienie 
			break;
		case 1:	// projekcja typu side-by-side
			glViewport(0, 0, window.getSize().x / 2, window.getSize().y);						//odpowienie umiejscowienie obrazu na ekranie
			StereoProjection(shaderProgram, -6, 6, -4.8, 4.8, 12.99, -100, 0, dist, -0.05);	//Odpowiednie przekształcenia projekcji
			//glDrawArrays(prymityw, 0, punkty[0]);					//Narysowanie
			//glDrawElements(prymityw, punkty, GL_UNSIGNED_INT, 0);

			//Rysowanie
			for (int i = 0; i < 5; i++)
			{
				glBindTexture(GL_TEXTURE_2D, texture[i]);
				glDrawArrays(prymityw, suma_punktow, punkty[i]);
				suma_punktow += punkty[i];
			}
			suma_punktow = 0;

			glViewport(window.getSize().x / 2, 0, window.getSize().x / 2, window.getSize().y);//odpowienie umiejscowienie obrazu na ekranie
			StereoProjection(shaderProgram, -6, 6, -4.8, 4.8, 12.99, -100, 0, dist, 0.05);//Odpowiednie przekształcenia projekcji
			//glDrawArrays(prymityw, 0, punkty[0]);//Narysowanie
			//glDrawElements(prymityw, punkty, GL_UNSIGNED_INT, 0);

			//Rysowanie
			for (int i = 0; i < 5; i++)
			{
				glBindTexture(GL_TEXTURE_2D, texture[i]);
				glDrawArrays(prymityw, suma_punktow, punkty[i]);
				suma_punktow += punkty[i];
			}
			suma_punktow = 0;

			
			break;
		case 2:	//obraz normalny(tryb mono)
			
			glViewport(0, 0, window.getSize().x, window.getSize().y);						//odpowienie umiejscowienie obrazu na ekranie

			//glBindTexture(GL_TEXTURE_2D, texture5);
			//glDrawArrays(prymityw, 0, 1000);	
			//glDrawElements(prymityw, punkty, GL_UNSIGNED_INT, 0);

			//Rysowanie
			for (int i = 0; i < 5; i++)
			{
				glBindTexture(GL_TEXTURE_2D, texture[i]);
				glDrawArrays(prymityw, suma_punktow, punkty[i]);
				suma_punktow += punkty[i];
			}
			suma_punktow = 0;

			break;
		}


		window.display();
	}

	// Kasowanie programu i czyszczenie buforów
	glDeleteProgram(shaderProgram);
	glDeleteShader(fragmentShader);
	glDeleteShader(vertexShader);
	glDeleteBuffers(1, &vbo);
	glDeleteVertexArrays(1, &vao);
	// Zamknięcie okna renderingu
	window.close();
	return 0;
}