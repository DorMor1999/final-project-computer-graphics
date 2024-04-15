
#include <stdlib.h>
#include <time.h>
#include <math.h>
#include "glut.h"
#include <vector>

using namespace std;

const double PI = 3.14;
const int GSZ = 100;

const int TH = 512; // must be a power of 2
const int TW = 512; // must be a power of 2
unsigned char tx0[TH][TW][3];

double angle = 0;

bool stopHydraulicErrosion = false;


typedef struct {
	int x;
	int z;
} POINT2D;

typedef struct {
	double x, y, z;
} POINT3;

POINT3 eye = {2,11,45 };

double sight_angle = PI;
POINT3 sight_dir = {sin(sight_angle),0,cos(sight_angle)}; // in plane X-Z
double speed = 0;
double angular_speed = 0;

double ground[GSZ][GSZ] = { 0 };
double riverWaterHight[GSZ][GSZ] = { 0 };

bool floodFillVisited[GSZ][GSZ] = { false };
POINT2D desiredPoint = {-100, -100};


typedef struct {
	double red, green, blue;
} COLOR;
COLOR roofColors[3] = { {100.0 / 255.0, 169.0 / 255.0, 169.0 / 255.0} , {178.0 / 255.0, 34.0 / 255.0, 34.0 / 255.0}, {110.0 / 255.0, 42.0 / 255.0, 42.0 / 255.0} };
int indexRoofColors = 0; //0 or 1 or 2


void UpdateGround3();
void Smooth();
void HydraulicErrosion();

void initRiverWaterHight() {
	for (int i = 0; i < GSZ; i++) {
		for (int j = 0; j < GSZ; j++) {
			riverWaterHight[i][j] = ground[i][j] - 0.001;
		}
	}
}

void setTexture(int tNum) {
	int i, j;
	int tmp;
	if (tNum == 1)// road texture 
	{
		for (int i = 0; i < TH; i++)
		{
			for (int j = 0; j < TW; j++)
			{
				tmp = rand()%20;
				if (i < 20 || i > TH - 20 || fabs(TH/2-i) < 10 && j < TW / 2) { //yellow
					tx0[i][j][0] = 220 + tmp;
					tx0[i][j][1] = 220 + tmp;
					tx0[i][j][2] = 0;
				}
				else // gray 
				{
					tx0[i][j][0] = 180 + tmp;
					tx0[i][j][1] = 180 + tmp;
					tx0[i][j][2] = 180 + tmp;
				}
			}
		}
	}
	if (tNum == 2)// croos walk texture 
	{
		for (int i = 0; i < TH; i++)
		{
			for (int j = 0; j < TW; j++)
			{
				tmp = rand() % 20;
				if (i < TH / 2 ) { //yellow
					tx0[i][j][0] = 220 + tmp;
					tx0[i][j][1] = 220 + tmp;
					tx0[i][j][2] = 0;
				}
				else // gray 
				{
					tx0[i][j][0] = 180 + tmp;
					tx0[i][j][1] = 180 + tmp;
					tx0[i][j][2] = 180 + tmp;
				}
			}
		}
	}
}

void init()
{
	//                 R     G    B
	glClearColor(0.5,0.7,1,0);// color of window background

	glEnable(GL_DEPTH_TEST);

	int i, j;
	double dist;

	srand(time(0));

	for (i = 0;i < 4000;i++)
		UpdateGround3();

	Smooth();
	
	initRiverWaterHight();

	
	//road texture
	setTexture(1);// Road
	glBindTexture(GL_TEXTURE_2D,1);
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, TW, TH, 0, GL_RGB, GL_UNSIGNED_BYTE, tx0);

	//cross walk texture
	setTexture(2);// cross walk
	glBindTexture(GL_TEXTURE_2D, 2);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, TW, TH, 0, GL_RGB, GL_UNSIGNED_BYTE, tx0);
	
}


void UpdateGround3()
{
	double delta = 0.04;
	if (rand() % 2 == 0)
		delta = -delta;
	int x1, y1, x2, y2;
	x1 = rand() % GSZ;
	y1 = rand() % GSZ;
	x2 = rand() % GSZ;
	y2 = rand() % GSZ;
	double a, b;
	if (x1 != x2)
	{
		a = (y2 - y1) / ((double)x2 - x1);
		b = y1 - a * x1;
		for(int i=0;i<GSZ;i++)
			for (int j = 0;j < GSZ;j++)
			{
				if (i < a * j + b) {
					ground[i][j] += delta;
				}
				else { 
					ground[i][j] -= delta;
				}
			}
	}


}

bool seaUp(int x, int z) {
	return x >= 0 && x < GSZ && z >= 0 && z < GSZ && 0 > ground[x][z] && 0 > riverWaterHight[x][z];
}

// uses stack to uvercome the recursion
void FloodFillIterative(int x, int z)
{
	vector <POINT2D> myStack;

	POINT2D current = {x, z};
	myStack.push_back(current);

	while (!myStack.empty())
	{
		// 1. extract last element from stack
		current = myStack.back();
		myStack.pop_back();
		// 2. save current point coordinates
		x = current.x;  
		z = current.z;
		if (ground[x][z] > 0 && ground[x][z] > riverWaterHight[x][z] && ((x + 2 < GSZ && ground[x + 2][z] < riverWaterHight[x + 2][z] && 0 < riverWaterHight[x + 2][z] && (seaUp(x, z + 4) || seaUp(x, z - 4))) || (x - 2 >= 0 && ground[x - 2][z] < riverWaterHight[x - 2][z] && 0 < riverWaterHight[x - 2][z] && (seaUp(x, z + 4) || seaUp(x, z - 4))) || (z + 2 < GSZ && ground[x][z + 2] < riverWaterHight[x][z + 2] && 0 < riverWaterHight[x][z + 2] && (seaUp(x + 4, z) || seaUp(x - 4, z))) || (z - 2 >= 0 && ground[x][z - 2] < riverWaterHight[x][z - 2] && 0 < riverWaterHight[x][z - 2] && (seaUp(x + 4, z) || seaUp(x - 4, z))))) {
			desiredPoint.x = x;
			desiredPoint.z = z;
			break;
		}
		else {
			// 3. add all relevant neighbour points to myStack
			if (x + 2 < GSZ && !floodFillVisited[x + 2][z])
			{
				current.x = x + 2;
				current.z = z;
				myStack.push_back(current);
			}
			// try going down
			if (x - 2 >= 0 && !floodFillVisited[x - 2][z])
			{
				current.x = x - 2;
				current.z = z;
				myStack.push_back(current);
			}
			// try going right
			if (z + 2 < GSZ && !floodFillVisited[x][z + 2])
			{
				current.x = x;
				current.z = z + 2;
				myStack.push_back(current);
			}
			// try going left
			if (z - 2 >= 0 && !floodFillVisited[x][z - 2])
			{
				current.x = x;
				current.z = z - 2;
				myStack.push_back(current);
			}
		}
		floodFillVisited[x][z] = true;
	}
}

void HydraulicErrosion() {
	

	//get random cordinates
	int x = rand() % GSZ;
	int z = rand() % GSZ;
	bool condition = false;
	do
	{
		condition = false;
		POINT3 nextPoint = { x, ground[x][z], z};

		if (x < 99) {
			POINT3 xHigherPoint = { x + 1, ground[x + 1][z], z };
			if (xHigherPoint.y < nextPoint.y) {
				nextPoint.y = xHigherPoint.y;
				nextPoint.x = xHigherPoint.x;
				nextPoint.z = xHigherPoint.z;
				condition = true;
			}
		}
		
		if (z < 99) {
			POINT3 zHigherPoint = { x, ground[x][z + 1], z + 1 };
			if (zHigherPoint.y < nextPoint.y) {
				nextPoint.y = zHigherPoint.y;
				nextPoint.x = zHigherPoint.x;
				nextPoint.z = zHigherPoint.z;
				condition = true;
			}
		}
		
		if (x > 0) {
			POINT3 xLowerPoint = { x - 1, ground[x - 1][z], z };
			if (xLowerPoint.y < nextPoint.y) {
				nextPoint.y = xLowerPoint.y;
				nextPoint.x = xLowerPoint.x;
				nextPoint.z = xLowerPoint.z;
				condition = true;
			}
		}
		
		if (z > 0) {
			POINT3 zLowerPoint = { x, ground[x][z - 1], z - 1 };
			if (zLowerPoint.y < nextPoint.y) {
				nextPoint.y = zLowerPoint.y;
				nextPoint.x = zLowerPoint.x;
				nextPoint.z = zLowerPoint.z;
				condition = true;
			}
		}
		

		//make this place lower
		ground[x][z] -= 0.0001;

		x = nextPoint.x;
		z = nextPoint.z;
	} while (condition);

}
  
void Smooth()
{
	double tmp[GSZ][GSZ];

	for(int i=1;i<GSZ-1;i++)
		for (int j = 1;j < GSZ - 1;j++)
		{
			tmp[i][j] = (ground[i-1][j-1]+ ground[i-1 ][j]+ ground[i - 1][j + 1]+
				ground[i][j - 1] + ground[i ][j] + ground[i ][j + 1]+
				ground[i + 1][j - 1] + ground[i + 1][j] + ground[i + 1][j + 1]) / 9.0;
		}

	for (int i = 1;i < GSZ - 1;i++)
		for (int j = 1;j < GSZ - 1;j++)
			ground[i][j] = tmp[i][j];

}

void SetColor(double h)
{
	h = fabs(h)/6;
	// sand
	if (h < 0.05)
		glColor3d(0.8, 0.7, 0.5);
	else	if(h<0.3)// grass
	glColor3d(0.4+0.8*h,0.6-0.6*h,0.2+ 0.2 * h);
	else if(h<0.7) // stones
		glColor3d(0.4 + 0.1 * h,  0.4+0.1*h, 0.2 + 0.1 * h);
	else // snow
		glColor3d(  h,  h, 1.1 * h);

}



void DrawFloor()
{
	int i,j;

	glColor3d(0, 0, 0.3);

	for(i=1;i<GSZ;i++)
		for (j = 1;j < GSZ;j++)
		{
			//the ground 
			glBegin(GL_POLYGON);
			SetColor(ground[i][j]);
			glVertex3d(j-GSZ/2, ground[i][j], i-GSZ/2);
			SetColor(ground[i-1][j]);
			glVertex3d(j - GSZ / 2, ground[i - 1][j], i - 1 - GSZ / 2);
			SetColor(ground[i-1][j-1]);
			glVertex3d(j - 1 - GSZ / 2, ground[i - 1][j - 1], i - 1 - GSZ / 2);
			SetColor(ground[i][j-1]);
			glVertex3d(j - 1 - GSZ / 2, ground[i ][j - 1], i - GSZ / 2);
			glEnd();
			
			//the river water
			glBegin(GL_POLYGON);
			glColor3d(0, 0.25, 0.6);
			glVertex3d(j - GSZ / 2, riverWaterHight[i][j], i - GSZ / 2);
			glVertex3d(j - GSZ / 2, riverWaterHight[i - 1][j], i - 1 - GSZ / 2);
			glVertex3d(j - 1 - GSZ / 2, riverWaterHight[i - 1][j - 1], i - 1 - GSZ / 2);
			glVertex3d(j - 1 - GSZ / 2, riverWaterHight[i][j - 1], i - GSZ / 2);
			glEnd();
			
		}

	// water sea
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glColor3d(0, 0.4, 0.7);
	glBegin(GL_POLYGON);
	glVertex3d(-GSZ / 2, 0, -GSZ / 2);
	glVertex3d(-GSZ / 2, 0, GSZ / 2);
	glVertex3d(GSZ / 2, 0, GSZ / 2);
	glVertex3d(GSZ / 2, 0, -GSZ / 2);
	glEnd();

	glDisable(GL_BLEND);

}

// true if above sea and river
bool checkpointAboveAllWater(int x, int z) {
	return x >= 0 && x < GSZ && z >= 0 && z < GSZ && ground[x][z] > 0 && ground[x][z] > riverWaterHight[x][z];
}

// cone
void DrawCone(int n, double topr, double bottomr, double topY, double downY)
{
	double alpha, teta = 2 * PI / n;
	COLOR currentColor = roofColors[indexRoofColors]; // Store the initial color
	for (alpha = 0; alpha < 2 * PI; alpha += teta)
	{
		glBegin(GL_POLYGON);
		glColor3d(currentColor.red, currentColor.green, currentColor.blue);;
		glVertex3d(topr * sin(alpha), topY, topr * cos(alpha));// 1
		glVertex3d(topr * sin(alpha + teta), topY, topr * cos(alpha + teta)); //2
		glVertex3d(bottomr * sin(alpha + teta), downY, bottomr * cos(alpha + teta));// 3
		glVertex3d(bottomr * sin(alpha), downY, bottomr * cos(alpha)); //4
		glEnd();

		// Update the current color for the next polygon
		currentColor.red -= 0.05;
		currentColor.green -= 0.05;
		currentColor.blue -= 0.05;
	}
}

// n = 4 is squre
void DrawCylinder(int n, double topY, double downY)
{
	COLOR wallColor = { 244.0 / 255.0, 164.0 / 255.0, 96.0 / 255.0 };
	double alpha, teta = 2 * PI / n;
	for (alpha = 0; alpha < 2 * PI; alpha += teta)
	{
		glBegin(GL_POLYGON);
		glColor3d(wallColor.red, wallColor.green, wallColor.blue);
		glVertex3d(sin(alpha), topY, cos(alpha));
		glVertex3d(sin(alpha + teta), topY, cos(alpha + teta));
		glVertex3d(sin(alpha + teta), downY, cos(alpha + teta));
		glVertex3d(sin(alpha), downY, cos(alpha));
		glEnd();
		wallColor.red -= 0.05;
		wallColor.green -= 0.05;
		wallColor.blue -= 0.05;
	}
}

void drawWindows(int numWindows, int numFloor, COLOR* windowColor) {
	double radius = 1.01;
	int parts = (numWindows * 2) + 1;
	double alpha = 0, teta = PI / 2;
	double x2 = sin(alpha + teta);
	double y2 = 1;
	double z2 = cos(alpha + teta);
	double x1 = sin(alpha);
	double y1 = 1;
	double z1 = cos(alpha);
	double distance = sqrt(pow(x2 - x1, 2) + pow(y2 - y1, 2) + pow(z2 - z1, 2));
	double size = distance / parts;
	double dx = (x2 - x1) / parts;
	double dz = (z2 - z1) / parts;
	for (int i = 0; i < parts; i++) {
		if (i % 2 == 1) {
			glBegin(GL_POLYGON);
			glColor3d(windowColor->red, windowColor->green, windowColor->blue);
			glVertex3d(radius * (x1 + (i * dx)), numFloor - 0.33, radius * (z1 + (i * dz)));// left top
			glVertex3d(radius * (x1 + ((i + 1) * dx)), numFloor - 0.33, radius * (z1 + ((i + 1) * dz)));//right top
			glVertex3d(radius * (x1 + ((i + 1) * dx)), numFloor - 0.66, radius * (z1 + ((i + 1) * dz)));//right bottom
			glVertex3d(radius * (x1 + (i * dx)), numFloor - 0.66, radius * (z1 + (i * dz)));//left bottom
			glEnd();
		}
	}
}




void drawFloorBilding(int numFloor, int numWindows) {
	//high layer
	DrawCylinder(4, numFloor - 0.00, numFloor - 0.33);
	//windows layer
	DrawCylinder(4, numFloor - 0.33, numFloor - 0.66);
	//bottom layer 
	DrawCylinder(4, numFloor - 0.66, numFloor - 1.00);

	//draw windows
	COLOR windowColor = { 0.0, 0.0, 0.8 };
	for (int i = 0; i <= 360; i += 90) {
		glPushMatrix();
		glRotated(i, 0, 1, 0);
		drawWindows(numWindows, numFloor, &windowColor);
		glPopMatrix();
		windowColor.blue -= 0.05;
	}
}

void drawHouse(int numFloors, int numWindows) {
	//draw walls
	for (int i = 1; i <= numFloors; i++) {
		drawFloorBilding(i, numWindows);
	}

	// draw roof
	DrawCone(4, 0, 1, numFloors + 1.00, numFloors + 0.00);
}

bool CheckForHouse(int x, int z) {
	return x - 1 >= 0 && x + 1 < GSZ && z - 1 >= 0 && z + 1 < GSZ && checkpointAboveAllWater(x, z) && checkpointAboveAllWater(x, z - 1) && checkpointAboveAllWater(x, z + 1) && checkpointAboveAllWater(x + 1, z) && checkpointAboveAllWater(x + 1, z - 1) && checkpointAboveAllWater(x + 1, z + 1) && checkpointAboveAllWater(x - 1, z) && checkpointAboveAllWater(x - 1, z - 1) && checkpointAboveAllWater(x - 1, z + 1);
}

void flatRight() {
	int x = desiredPoint.x;
	int z = desiredPoint.z;
	int counter = 0;
	
	while (x > 0 && checkpointAboveAllWater(x, z) && checkpointAboveAllWater(x, z - 1) && checkpointAboveAllWater(x, z + 1) && checkpointAboveAllWater(x - 1, z) && checkpointAboveAllWater(x - 1, z - 1) && checkpointAboveAllWater(x - 1, z + 1) && z - 1 >= 0 && z + 1 < GSZ) {
		glEnable(GL_TEXTURE_2D);
		glBindTexture(GL_TEXTURE_2D, 1);
		glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);
		ground[x][z - 2] = ground[x][z + 2] = ground[x][z - 1] = ground[x][z + 1] = ground[x][z];
		ground[x - 1][z - 2] = ground[x - 1][z + 2] = ground[x-1][z - 1] = ground[x-1][z + 1] = ground[x-1][z];
		riverWaterHight[x][z - 2] = riverWaterHight[x][z + 2] = riverWaterHight[x][z - 1] = riverWaterHight[x][z + 1] = riverWaterHight[x][z] = -1;
		riverWaterHight[x - 1][z - 2] = riverWaterHight[x - 1][z + 2] = riverWaterHight[x-1][z - 1] = riverWaterHight[x-1][z + 1] = riverWaterHight[x-1][z] = -1;
		if (counter % 8 == 0) {//croos walk
			glBindTexture(GL_TEXTURE_2D, 2);
			glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);

			glBegin(GL_POLYGON);
			glTexCoord2d(0, 10.5); glVertex3d(z + 1 - GSZ / 2, ground[x - 1][z + 1] + 0.1, x - 1 - GSZ / 2);
			glTexCoord2d(0, 0); glVertex3d(z - 1 - GSZ / 2, ground[x - 1][z - 1] + 0.1, x - 1 - GSZ / 2);
			glTexCoord2d(1, 0); glVertex3d(z - 1 - GSZ / 2, ground[x][z - 1] + 0.1, x - GSZ / 2);
			glTexCoord2d(1, 10.5); glVertex3d(z + 1 - GSZ / 2, ground[x][z + 1] + 0.1, x - GSZ / 2);
			glEnd();

			glBindTexture(GL_TEXTURE_2D, 1);
			glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);
		}
		else {//road
			
			glBegin(GL_POLYGON);
			glTexCoord2d(0, 2); glVertex3d(z + 1 - GSZ / 2, ground[x - 1][z + 1] + 0.1, x - 1 - GSZ / 2);
			glTexCoord2d(0, 0); glVertex3d(z - 1 - GSZ / 2, ground[x - 1][z - 1] + 0.1, x - 1 - GSZ / 2);
			glTexCoord2d(1, 0); glVertex3d(z - 1 - GSZ / 2, ground[x][z - 1] + 0.1, x - GSZ / 2);
			glTexCoord2d(1, 2); glVertex3d(z + 1 - GSZ / 2, ground[x][z + 1] + 0.1, x - GSZ / 2);
			glEnd();
			
		}
		glDisable(GL_TEXTURE_2D);

		//buildings
		if (counter % 2 == 0 && CheckForHouse(x, z-2))
		{	
			indexRoofColors = counter % 3;
			int numWindows = (counter % 4) + 1;
			int numFloors = (counter % 4) + 1;
			glPushMatrix();
			glTranslated(z - 2 - GSZ / 2, ground[x][z - 2] + 0.15, x - GSZ / 2);
			glRotated(45, 0, 1, 0);
			glScaled(1, numFloors / 2 + 1, 1);
			drawHouse(numFloors, numWindows);//mid bottom always (0, 0, 0) to changed translate this with tranformation
			glPopMatrix();
		}
		if (counter % 2 == 0 && CheckForHouse(x, z + 2))
		{
			indexRoofColors = counter % 3;
			int numWindows = (counter % 4) + 1;
			int numFloors = (counter % 4) + 1;
			glPushMatrix();
			glTranslated(z + 2 - GSZ / 2, ground[x][z + 2] + 0.15, x - GSZ / 2);
			glRotated(45, 0, 1, 0);
			glScaled(1, numFloors / 2 + 1, 1);
			drawHouse(numFloors, numWindows);//mid bottom always (0, 0, 0) to changed translate this with tranformation
			glPopMatrix();
		}
			
		x--;
		counter++;
	}

	//bridge part
	while (x + 1 < GSZ) {
		if (checkpointAboveAllWater(x,z) && checkpointAboveAllWater(x, z + 1) && checkpointAboveAllWater(x, z - 1)) {
			ground[x][z - 1] = ground[x][z + 1] = ground[x][z];
			ground[x + 1][z - 1] = ground[x + 1][z + 1] = ground[x + 1][z];
			riverWaterHight[x][z - 1] = riverWaterHight[x][z + 1] = riverWaterHight[x][z] = -1;
			riverWaterHight[x + 1][z - 1] = riverWaterHight[x + 1][z + 1] = riverWaterHight[x + 1][z] = -1;
			//road
			glEnable(GL_TEXTURE_2D);
			glBindTexture(GL_TEXTURE_2D, 1);
			glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);
			glBegin(GL_POLYGON);
			glTexCoord2d(0, 2); glVertex3d(z + 1 - GSZ / 2, ground[x][z + 1] + 0.1, x - GSZ / 2);
			glTexCoord2d(0, 0); glVertex3d(z - 1 - GSZ / 2, ground[x][z - 1] + 0.1, x - GSZ / 2);
			if (checkpointAboveAllWater(x + 1, z) && checkpointAboveAllWater(x + 1, z + 1) && checkpointAboveAllWater(x + 1, z - 1)) {
				glTexCoord2d(1, 0); glVertex3d(z - 1 - GSZ / 2, ground[x + 1][z - 1] + 0.1, x + 1 - GSZ / 2);
				glTexCoord2d(1, 2); glVertex3d(z + 1 - GSZ / 2, ground[x + 1][z + 1] + 0.1, x + 1 - GSZ / 2);
			}
			else
			{
				glTexCoord2d(1, 0); glVertex3d(z - 1 - GSZ / 2, 0.5, x + 1 - GSZ / 2);
				glTexCoord2d(1, 2); glVertex3d(z + 1 - GSZ / 2, 0.5, x + 1 - GSZ / 2);
			}
			glEnd();
			glDisable(GL_TEXTURE_2D);
		}
		else {
			//road bridge
			glEnable(GL_TEXTURE_2D);
			glBindTexture(GL_TEXTURE_2D, 1);
			glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);
			glBegin(GL_POLYGON);
			glTexCoord2d(0, 2); glVertex3d(z + 1 - GSZ / 2, 0.5, x - GSZ / 2);
			glTexCoord2d(0, 0); glVertex3d(z - 1 - GSZ / 2, 0.5, x - GSZ / 2);
			if (checkpointAboveAllWater(x + 1, z) && checkpointAboveAllWater(x + 1, z + 1) && checkpointAboveAllWater(x + 1, z - 1)) {
				glTexCoord2d(1, 0); glVertex3d(z - 1 - GSZ / 2, ground[x + 1][z - 1] + 0.1, x + 1 - GSZ / 2);
				glTexCoord2d(1, 2); glVertex3d(z + 1 - GSZ / 2, ground[x + 1][z + 1] + 0.1, x + 1 - GSZ / 2);
			}
			else {
				glTexCoord2d(1, 0); glVertex3d(z - 1 - GSZ / 2, 0.5, x + 1 - GSZ / 2);
				glTexCoord2d(1, 2); glVertex3d(z + 1 - GSZ / 2, 0.5, x + 1 - GSZ / 2);
			}
			glEnd();
			glDisable(GL_TEXTURE_2D);
		}
		x++;
	}
	
}

void flatLeft() {
	int x = desiredPoint.x;
	int z = desiredPoint.z;
	int counter = 0;
	
	while (x + 1 < GSZ && checkpointAboveAllWater(x, z) && checkpointAboveAllWater(x, z - 1) && checkpointAboveAllWater(x, z + 1) && checkpointAboveAllWater(x + 1, z) && checkpointAboveAllWater(x + 1, z - 1) && checkpointAboveAllWater(x + 1, z + 1) && z - 1 >= 0 && z + 1 < GSZ) {
		glEnable(GL_TEXTURE_2D);
		glBindTexture(GL_TEXTURE_2D, 1);
		glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);
		ground[x][z - 2] = ground[x][z + 2] = ground[x ][z - 1] = ground[x][z + 1] = ground[x][z];
		ground[x + 1][z - 2] = ground[x + 1][z + 2] = ground[x + 1][z - 1] = ground[x + 1][z + 1] = ground[x + 1][z];
		riverWaterHight[x][z - 2] = riverWaterHight[x][z + 2] = riverWaterHight[x][z - 1] = riverWaterHight[x][z + 1] = riverWaterHight[x][z] = -1;
		riverWaterHight[x + 1][z - 2] = riverWaterHight[x + 1][z +  2] = riverWaterHight[x + 1][z - 1] = riverWaterHight[x + 1][z + 1] = riverWaterHight[x + 1][z] = -1;
		
		if (counter % 8 == 0) {//croos walk
			glBindTexture(GL_TEXTURE_2D, 2);
			glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);

			glBegin(GL_POLYGON);
			glTexCoord2d(0, 10.5); glVertex3d(z + 1 - GSZ / 2, ground[x + 1][z + 1] + 0.1, x + 1 - GSZ / 2);
			glTexCoord2d(0, 0); glVertex3d(z - 1 - GSZ / 2, ground[x + 1][z - 1] + 0.1, x + 1 - GSZ / 2);
			glTexCoord2d(1, 0); glVertex3d(z - 1 - GSZ / 2, ground[x][z - 1] + 0.1, x - GSZ / 2);
			glTexCoord2d(1, 10.5); glVertex3d(z + 1 - GSZ / 2, ground[x][z + 1] + 0.1, x - GSZ / 2);
			glEnd();

			glBindTexture(GL_TEXTURE_2D, 1);
			glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);
		}
		else {//road

			glBegin(GL_POLYGON);
			glTexCoord2d(1, 2); glVertex3d(z + 1 - GSZ / 2, ground[x + 1][z + 1] + 0.1, x + 1 - GSZ / 2);
			glTexCoord2d(1, 0); glVertex3d(z - 1 - GSZ / 2, ground[x + 1][z - 1] + 0.1, x + 1 - GSZ / 2);
			glTexCoord2d(0, 0); glVertex3d(z - 1 - GSZ / 2, ground[x][z - 1] + 0.1, x - GSZ / 2);
			glTexCoord2d(0, 2); glVertex3d(z + 1 - GSZ / 2, ground[x][z + 1] + 0.1, x - GSZ / 2);
			glEnd();
		}
		glDisable(GL_TEXTURE_2D);

		//buildings
		if (counter % 2 == 0 && CheckForHouse(x, z - 2))
		{
			indexRoofColors = counter % 3;
			int numWindows = (counter % 4) + 1;
			int numFloors = (counter % 4) + 1;
			glPushMatrix();
			glTranslated(z - 2 - GSZ / 2, ground[x][z - 2] + 0.15, x - GSZ / 2);
			glRotated(45, 0, 1, 0);
			glScaled(1, numFloors / 2 + 1, 1);
			drawHouse(numFloors, numWindows);//mid bottom always (0, 0, 0) to changed translate this with tranformation
			glPopMatrix();
		}
		if (counter % 2 == 0 && CheckForHouse(x, z + 2))
		{
			indexRoofColors = counter % 3;
			int numWindows = (counter % 4) + 1;
			int numFloors = (counter % 4) + 1;
			glPushMatrix();
			glTranslated(z + 2 - GSZ / 2, ground[x][z + 2] + 0.15, x - GSZ / 2);
			glRotated(45, 0, 1, 0);
			glScaled(1, numFloors / 2 + 1, 1);
			drawHouse(numFloors, numWindows);//mid bottom always (0, 0, 0) to changed translate this with tranformation
			glPopMatrix();
		}

		counter++;
		x++;
	}
	
}

void flatUp() {
	int x = desiredPoint.x;
	int z = desiredPoint.z;
	int counter = 0;
	while (z > 0 && checkpointAboveAllWater(x, z) && checkpointAboveAllWater(x - 1, z) && checkpointAboveAllWater(x + 1, z) && checkpointAboveAllWater(x, z - 1) && checkpointAboveAllWater(x - 1, z - 1) && checkpointAboveAllWater(x + 1, z - 1) && x - 1 >= 0 && x + 1 < GSZ) {
		glEnable(GL_TEXTURE_2D);
		glBindTexture(GL_TEXTURE_2D, 1);
		glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);
		ground[x - 2][z] = ground[x + 2][z] = ground[x - 1][z] = ground[x + 1][z] = ground[x][z];
		ground[x - 2][z - 1] = ground[x + 2][z - 1] = ground[x - 1][z - 1] = ground[x + 1][z - 1] = ground[x][z - 1];
		riverWaterHight[x - 2][z] = riverWaterHight[x + 2][z] = riverWaterHight[x - 1][z] = riverWaterHight[x + 1][z] = riverWaterHight[x][z] = -1;
		riverWaterHight[x - 2][z - 1] = riverWaterHight[x + 2][z - 1] = riverWaterHight[x - 1][z-1] = riverWaterHight[x + 1][z-1] = riverWaterHight[x][z-1] = -1;
		if (counter % 8 == 0) {//croos walk
			
			glBindTexture(GL_TEXTURE_2D, 2);
			glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);

			glBegin(GL_POLYGON);
			glTexCoord2d(0, 0); glVertex3d(z - 1 - GSZ / 2, ground[x - 1][z - 1] + 0.1, x - 1 - GSZ / 2);
			glTexCoord2d(0, 10.5); glVertex3d(z - 1 - GSZ / 2, ground[x + 1][z - 1] + 0.1, x + 1 - GSZ / 2);
			glTexCoord2d(1, 10.5); glVertex3d(z - GSZ / 2, ground[x + 1][z] + 0.1, x + 1 - GSZ / 2);
			glTexCoord2d(1, 0); glVertex3d(z - GSZ / 2, ground[x - 1][z] + 0.1, x - 1 - GSZ / 2);
			glEnd();

			glBindTexture(GL_TEXTURE_2D, 1);
			glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);
		}
		else {//road

			glBegin(GL_POLYGON);
			glTexCoord2d(0, 0); glVertex3d(z - 1 - GSZ / 2, ground[x - 1][z - 1] + 0.1, x - 1 - GSZ / 2);
			glTexCoord2d(0, 2); glVertex3d(z - 1 - GSZ / 2, ground[x + 1][z - 1] + 0.1, x + 1 - GSZ / 2);
			glTexCoord2d(1, 2); glVertex3d(z - GSZ / 2, ground[x + 1][z] + 0.1, x + 1 - GSZ / 2);
			glTexCoord2d(1, 0); glVertex3d(z - GSZ / 2, ground[x - 1][z] + 0.1, x - 1 - GSZ / 2);
			glEnd();

		}
		glDisable(GL_TEXTURE_2D);

		//buildings
		if (counter % 2 == 0 && CheckForHouse(x - 2, z))
		{
			indexRoofColors = counter % 3;
			int numWindows = (counter % 4) + 1;
			int numFloors = (counter % 4) + 1;
			glPushMatrix();
			glTranslated(z - GSZ / 2, ground[x - 2][z] + 0.15, x - 2 - GSZ / 2);
			glRotated(45, 0, 1, 0);
			glScaled(1, numFloors / 2 + 1, 1);
			drawHouse(numFloors, numWindows);//mid bottom always (0, 0, 0) to changed translate this with tranformation
			glPopMatrix();
		}
		if (counter % 2 == 0 && CheckForHouse(x + 2, z))
		{
			indexRoofColors = counter % 3;
			int numWindows = (counter % 4) + 1;
			int numFloors = (counter % 4) + 1;
			glPushMatrix();
			glTranslated(z - GSZ / 2, ground[x + 2][z] + 0.15, x + 2 - GSZ / 2);
			glRotated(45, 0, 1, 0);
			glScaled(1, numFloors / 2 + 1, 1);
			drawHouse(numFloors, numWindows);//mid bottom always (0, 0, 0) to changed translate this with tranformation
			glPopMatrix();
		}

		counter++;
		z--;
	}
	
}

void flatDown() {
	int x = desiredPoint.x;
	int z = desiredPoint.z;
	int counter = 0;
	while (z + 1 < GSZ && checkpointAboveAllWater(x, z) && checkpointAboveAllWater(x - 1, z) && checkpointAboveAllWater(x + 1, z) && checkpointAboveAllWater(x, z  + 1) && checkpointAboveAllWater(x - 1, z + 1) && checkpointAboveAllWater(x + 1, z + 1) && x - 1 >= 0 && x + 1 < GSZ) {
		glEnable(GL_TEXTURE_2D);
		glBindTexture(GL_TEXTURE_2D, 1);
		glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);
		ground[x - 2][z] = ground[x + 2][z] = ground[x - 1][z] = ground[x + 1][z] = ground[x][z];
		ground[x - 2][z + 1] = ground[x + 2][z + 1] = ground[x - 1][z + 1] = ground[x + 1][z + 1] = ground[x][z + 1];
		riverWaterHight[x - 2][z] = riverWaterHight[x + 2][z] = riverWaterHight[x - 1][z] = riverWaterHight[x + 1][z] = riverWaterHight[x][z] = -1;
		riverWaterHight[x - 2][z + 1] = riverWaterHight[x + 2][z + 1] = riverWaterHight[x - 1][z + 1] = riverWaterHight[x + 1][z + 1] = riverWaterHight[x][z + 1] = -1;
		
		if (counter % 8 == 0) {//croos walk

			glBindTexture(GL_TEXTURE_2D, 2);
			glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);

			glBegin(GL_POLYGON);
			glTexCoord2d(0, 0); glVertex3d(z - GSZ / 2, ground[x - 1][z] + 0.1, x - 1 - GSZ / 2);
			glTexCoord2d(0, 10.5); glVertex3d(z - GSZ / 2, ground[x + 1][z] + 0.1, x + 1 - GSZ / 2);
			glTexCoord2d(1, 10.5); glVertex3d(z + 1 - GSZ / 2, ground[x + 1][z + 1] + 0.1, x + 1 - GSZ / 2);
			glTexCoord2d(1, 0); glVertex3d(z + 1 - GSZ / 2, ground[x - 1][z + 1] + 0.1, x - 1 - GSZ / 2);
			glEnd();

			glBindTexture(GL_TEXTURE_2D, 1);
			glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);
		}
		else {//road

			glBegin(GL_POLYGON);
			glTexCoord2d(0, 0); glVertex3d(z - GSZ / 2, ground[x - 1][z] + 0.1, x - 1 - GSZ / 2);
			glTexCoord2d(0, 2); glVertex3d(z - GSZ / 2, ground[x + 1][z] + 0.1, x + 1 - GSZ / 2);
			glTexCoord2d(1, 2); glVertex3d(z + 1 - GSZ / 2, ground[x + 1][z + 1] + 0.1, x + 1 - GSZ / 2);
			glTexCoord2d(1, 0); glVertex3d(z + 1 - GSZ / 2, ground[x - 1][z + 1] + 0.1, x - 1 - GSZ / 2);
			glEnd();

		}
		glDisable(GL_TEXTURE_2D);

		//buildings
		if (counter % 2 == 0 && CheckForHouse(x - 2, z))
		{
			indexRoofColors = counter % 3;
			int numWindows = (counter % 4) + 1;
			int numFloors = (counter % 4) + 1;
			glPushMatrix();
			glTranslated(z - GSZ / 2, ground[x - 2][z] + 0.15, x - 2 - GSZ / 2);
			glRotated(45, 0, 1, 0);
			glScaled(1, numFloors / 2 + 1, 1);
			drawHouse(numFloors, numWindows);//mid bottom always (0, 0, 0) to changed translate this with tranformation
			glPopMatrix();
		}
		if (counter % 2 == 0 && CheckForHouse(x + 2, z))
		{
			indexRoofColors = counter % 3;
			int numWindows = (counter % 4) + 1;
			int numFloors = (counter % 4) + 1;
			glPushMatrix();
			glTranslated(z - GSZ / 2, ground[x + 2][z] + 0.15, x + 2 - GSZ / 2);
			glRotated(45, 0, 1, 0);
			glScaled(1, numFloors / 2 + 1, 1);
			drawHouse(numFloors, numWindows);//mid bottom always (0, 0, 0) to changed translate this with tranformation
			glPopMatrix();
		}

		counter++;
		z++;
	}
	
}

void flattenRoad() {
	flatRight();
	//river water from right so we build city from left
	if (desiredPoint.x + 2 < GSZ) {
		bool isRiverWaterRight = riverWaterHight[desiredPoint.x + 2][desiredPoint.z] > 0 && riverWaterHight[desiredPoint.x + 2][desiredPoint.z] > ground[desiredPoint.x + 2][desiredPoint.z];
		if (isRiverWaterRight)
		{
			//flatRight();
			return;
		}
	}
	//river water from left so we build city from right
	if (desiredPoint.x - 2 >= 0) {
		bool isRiverWaterLeft = riverWaterHight[desiredPoint.x - 2][desiredPoint.z] > 0 && riverWaterHight[desiredPoint.x - 2][desiredPoint.z] > ground[desiredPoint.x - 2][desiredPoint.z];
		if (isRiverWaterLeft)
		{
			//flatLeft();
			return;
		}
	}
	//river water from up so we build city from down
	if (desiredPoint.z + 2 < GSZ) {
		bool isRiverWaterUp = riverWaterHight[desiredPoint.x][desiredPoint.z + 2] > 0 && riverWaterHight[desiredPoint.x][desiredPoint.z + 2] > ground[desiredPoint.x][desiredPoint.z + 2];
		if (isRiverWaterUp)
		{
			//flatUp();
			return;
		}
	}
	//river water from down so we build city from up
	if (desiredPoint.z - 2 < GSZ) {
		bool isRiverWaterDown = riverWaterHight[desiredPoint.x][desiredPoint.z - 2] > 0 && riverWaterHight[desiredPoint.x][desiredPoint.z - 2] > ground[desiredPoint.x][desiredPoint.z - 2];
		if (isRiverWaterDown)
		{
			//flatDown();
			return;
		}
	}
}



// put all the drawings here
void display()
{
	glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT); // clean frame buffer and Z-buffer

	glMatrixMode(GL_PROJECTION);
	glLoadIdentity(); // unity matrix of projection

	glFrustum(-1, 1, -1, 1, 0.75, 300);
	gluLookAt(eye.x, eye.y, eye.z,  // eye position
		eye.x+ sight_dir.x, eye.y-0.3, eye.z+sight_dir.z,  // sight dir
		0, 1, 0);


	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity(); // unity matrix of model

	

	DrawFloor();
	
	if (!stopHydraulicErrosion && desiredPoint.x == -100) {
		for (int i = 0; i < 2; i++)
		{
			HydraulicErrosion();
		}
	}
	else {
		if (desiredPoint.x == -100) {
			int randomX = rand() % GSZ;
			int randomZ = rand() % GSZ;
			FloodFillIterative(randomX, randomZ);
		}
	}

	
	if (desiredPoint.x != -100) {
		flattenRoad(); 
	}
	
	
	


	
	


	

	glutSwapBuffers(); // show all
}



void idle() 
{
	int i, j;
	double dist;
	angle += 0.02;


	// ego-motion  or locomotion
	sight_angle += angular_speed;
	// the direction of our sight (forward)
	sight_dir.x = sin(sight_angle);
	sight_dir.z = cos(sight_angle);
	// motion
	eye.x += speed * sight_dir.x;
	eye.y += speed * sight_dir.y;
	eye.z += speed * sight_dir.z;


	glutPostRedisplay();
}


void SpecialKeys(int key, int x, int y)
{
	switch (key)
	{
	case GLUT_KEY_LEFT: // turns the user to the left
		angular_speed += 0.0004;
		break;
	case GLUT_KEY_RIGHT:
		angular_speed -= 0.0004;
		break;
	case GLUT_KEY_UP: // increases the speed
		speed+= 0.005;
		break;
	case GLUT_KEY_DOWN:
		speed -= 0.005;
		break;
	case GLUT_KEY_PAGE_UP:
		eye.y += 0.1;
		break;
	case GLUT_KEY_PAGE_DOWN:
		eye.y -= 0.1;
		break;

	}
}

void mouse(int button, int state, int x, int y)
{
	if (button == GLUT_LEFT_BUTTON && state == GLUT_DOWN)
		stopHydraulicErrosion = !stopHydraulicErrosion;
}

void main(int argc, char* argv[]) 
{
	glutInit(&argc, argv);
	glutInitDisplayMode(GLUT_RGB | GLUT_DOUBLE|GLUT_DEPTH);
	glutInitWindowSize(600, 600);
	glutInitWindowPosition(400, 100);
	glutCreateWindow("First Example");

	glutDisplayFunc(display);
	glutIdleFunc(idle);
	glutSpecialFunc(SpecialKeys);
	glutMouseFunc(mouse);
	

	init();

	glutMainLoop();
}