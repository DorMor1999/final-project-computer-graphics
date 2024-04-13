
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
				if (i < 20 || i > TH - 20 || fabs(TH/2-i) < 10 && j < TW / 2) {
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
		if (ground[x][z] > 0 && ground[x][z] > riverWaterHight[x][z] && ((x + 2 < GSZ && ground[x + 2][z] < riverWaterHight[x + 2][z]) || (x - 2 >= 0 && ground[x - 2][z] < riverWaterHight[x - 2][z]) || (z + 2 < GSZ && ground[x][z + 2] < riverWaterHight[x][z + 2]) || (z - 2 >= 0 && ground[x][z - 2] < riverWaterHight[x][z - 2]))) {
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
	return ground[x][z] > 0 && ground[x][z] > riverWaterHight[x][z];
}

void DrawRoad(int x, int z) {
	glEnable(GL_TEXTURE_2D);
	glBindTexture(GL_TEXTURE_2D, 1);
	glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);
	glBegin(GL_POLYGON);
	//glVertex3d();
	glEnd();
	glDisable(GL_TEXTURE_2D);
}

void flatRight() {
	int x = desiredPoint.x;
	int z = desiredPoint.z;
	ground[desiredPoint.x][desiredPoint.z] = 12;
	while (x - 2 >= 0 && checkpointAboveAllWater(x - 2, z)) {
		ground[x - 2][z] = ground[x - 1][z] = ground[x][z];
		riverWaterHight[x - 2][z] = riverWaterHight[x - 1][z] = riverWaterHight[x][z] = -1;
		//glEnable(GL_TEXTURE_2D);
		//glBindTexture(GL_TEXTURE_2D, 1);
		//glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);
		glColor3d(0.0, 0.0, 0.0);
		glBegin(GL_POLYGON);
		glVertex3d(x - GSZ / 2, ground[x][z]+ 0.1, z - GSZ / 2);
		glVertex3d(x - 1 - GSZ / 2, ground[x - 1][z] + 0.1, z - GSZ / 2);
		glVertex3d(x - 1 - GSZ / 2, ground[x - 1][z - 1] + 0.1, z - 1 - GSZ / 2);
		glVertex3d(x - GSZ / 2, ground[x][z - 1] + 0.1, z - 1 - GSZ / 2);
		glEnd();
		//glDisable(GL_TEXTURE_2D);
		x = x - 2;
	}
}

void flatLeft() {
	int x = desiredPoint.x;
	int z = desiredPoint.z;
	while (x + 2 < GSZ && checkpointAboveAllWater(x + 2, z)) {
		ground[x + 2][z] = ground[x + 1][z] = ground[x][z];
		riverWaterHight[x + 2][z] = riverWaterHight[x + 1][z] = riverWaterHight[x][z] = -1;
		x = x + 2;
	}
}

void flatUp() {
	int x = desiredPoint.x;
	int z = desiredPoint.z;
	while (z - 2 >= 0 && checkpointAboveAllWater(x, z - 2)) {
		ground[x][z - 2] = ground[x][z - 1] = ground[x][z];
		riverWaterHight[x][z - 2] = riverWaterHight[x][z - 1] = riverWaterHight[x][z] = -1;
		z = z - 2;
	}
}

void flatDown() {
	int x = desiredPoint.x;
	int z = desiredPoint.z;
	while (z + 2 < GSZ && checkpointAboveAllWater(x, z + 2)) {
		ground[x][z + 2] = ground[x][z + 1] = ground[x][z];
		riverWaterHight[x][z + 2] = riverWaterHight[x][z + 1] = riverWaterHight[x][z] = -1;
		z = z + 2;
	}
}

void flattenRoad() {
	
	//river water from right so we build city from left
	if (desiredPoint.x + 2 < GSZ) {
		bool isRiverWaterRight = riverWaterHight[desiredPoint.x + 2][desiredPoint.z] > 0 && riverWaterHight[desiredPoint.x + 2][desiredPoint.z] > ground[desiredPoint.x + 2][desiredPoint.z];
		if (isRiverWaterRight)
		{
			flatRight();
			return;
		}
	}
	//river water from left so we build city from right
	if (desiredPoint.x - 2 >= 0) {
		bool isRiverWaterLeft = riverWaterHight[desiredPoint.x - 2][desiredPoint.z] > 0 && riverWaterHight[desiredPoint.x - 2][desiredPoint.z] > ground[desiredPoint.x - 2][desiredPoint.z];
		if (isRiverWaterLeft)
		{
			flatLeft();
			return;
		}
	}
	//river water from up so we build city from down
	if (desiredPoint.z + 2 < GSZ) {
		bool isRiverWaterUp = riverWaterHight[desiredPoint.x][desiredPoint.z + 2] > 0 && riverWaterHight[desiredPoint.x][desiredPoint.z + 2] > ground[desiredPoint.x][desiredPoint.z + 2];
		if (isRiverWaterUp)
		{
			flatUp();
			return;
		}
	}
	//river water from down so we build city from up
	if (desiredPoint.z - 2 < GSZ) {
		bool isRiverWaterDown = riverWaterHight[desiredPoint.x][desiredPoint.z - 2] > 0 && riverWaterHight[desiredPoint.x][desiredPoint.z - 2] > ground[desiredPoint.x][desiredPoint.z - 2];
		if (isRiverWaterDown)
		{
			flatDown();
			return;
		}
	}
	
	
	
	//for (int i = 0; i + desiredPoint.x < GSZ; i++) {
	//	ground[i + desiredPoint.x][desiredPoint.z] = 4;
	//}
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