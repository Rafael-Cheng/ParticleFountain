#include <algorithm>
#include <iostream>
#include <GL/glut.h>
#include <vector>
#include <cmath>
#include <cstdlib>
#include <ctime>
#include <string>
#include <sstream>
#include "glaux.h"
#pragma comment(lib, "glaux.lib")
#pragma comment(lib, "advapi32.lib")
#define TIMER 10
#define INTVAL 0.002

using namespace std;

/* function declaration */
void initRendering(void);
void drawFountain(void);
void update(int);
void handleResize(int w, int h);
void handleKeypress(unsigned char key, int x, int y);
float getRandomF(void){return static_cast <float> (rand()) / static_cast <float> (RAND_MAX);}
GLuint loadAlphaTexture(AUX_RGBImageRec* image, AUX_RGBImageRec* alphaChannel);
char* addAlphaChannel(AUX_RGBImageRec* image, AUX_RGBImageRec* alphaChannel);
AUX_RGBImageRec *LoadBMP(char *Filename);

/* default settings*/
int pamount = 3000; // particle amount
float psize = 0.04; // particle size
float minV = 4; // minimal velocity
float maxV = 4; // maximum velocity
float graverty = 10; // graverty
bool isSpin = false; // spinning mode
bool isColourful = false; // colourful mode

/* other parameters */
float v = 4.0f; // particle velocity
float k = 1.0f; // x, z offset range
bool isUp = true; // fountain up flag
float angle = 0.0f; // spinning angle
GLfloat ambientColor[] = {0.8f, 0.8f, 0.8f, 1.0f}; // ambient colour

/* three-dimensional vector class */
class Vector3D
{
private:
	float x;
	float y;
	float z;
public:
	Vector3D()
	{
		x = 0;
		y = 0;
		z = 0;
	}
	Vector3D(float _x, float _y, float _z)
	{
		x = _x;
		y = _y;
		z = _z;
	}
	float operator [](int i);
	Vector3D operator +(const Vector3D &V);
	Vector3D operator -(const Vector3D &V);
	Vector3D operator *(const float f);
};

float Vector3D::operator[](int i)
{
	if (i == 0)
		return x;
	else if (i == 1)
		return y;
	else if (i == 2)
		return z;
	else
		return 0;
}

Vector3D Vector3D::operator+(const Vector3D &V)
{
	return Vector3D(x + V.x, y + V.y, z + V.z);
}

Vector3D Vector3D::operator-(const Vector3D & V)
{
	return Vector3D(x + V.x, y + V.y, z + V.z);
}

Vector3D Vector3D::operator*(const float f)
{
	return Vector3D(x * f, y * f, z * f);
}

/* particle structure */
struct Particle
{
	Vector3D pos;
	Vector3D velocity;
	Vector3D colour;
	float timeLived;
	float lifeSpan;
};

/* bool compareParticles(Particle* particle1, Particle* particle2)
 * function: compare depth of two particles.
 * parameters: two particle pointers
 * return value: whether the depth(z value)  of the 1st particle
 * is smaller than the 2nd one.
 */
bool compareParticles(Particle* particle1, Particle* particle2) 
{
	    return (particle1->pos)[2] < (particle2->pos)[2];
}

/* particle system class*/
class ParticleSystem
{
	private:
		/* contains pointers to all particles */
		vector<Particle*> ps;
		GLuint textureID;
		/* initial velocity */
		Vector3D initVelocity(void)
		{
			if(isSpin)
				return Vector3D(cos(angle), sqrt(v*v - 1), sin(angle));
			else
				return Vector3D(0, v, 0);
		}
		/* void addParticle(Particle *p)
		 * function: initialize a given particle p.
		 * parameters: Particle pointer
		 * return value: void
		 */
		void addParticle(Particle *p)
		{
			Vector3D initV;
			float xV, yV, zV;
			/* launching point */
			p->pos = Vector3D(0, 0, 0);
			/* get initial velocity */
			initV = initVelocity();
			/* getRandomF return a float <- [0, 1]
			 * By multiply k and then minus k/2, v <- [-k/2, k/2]
			 */
			xV = k * getRandomF() - k/2 + initV[0];
			zV = k * getRandomF() - k/2 + initV[2];
			/* calculate yV by use formula: yV = sqrt(v^2 - (xV^2 + yV^2))*/
			yV = sqrt(v * v - (xV * xV + zV * zV)) + getRandomF();
			p->velocity = Vector3D(xV, yV, zV);
			// set waterdrop colour, white by default.
			p->colour = Vector3D(1.0f, 1.0f, 1.0f);
			p->timeLived = 0;
			// live at least for 1 seconds
			p->lifeSpan = getRandomF() + 1;
		}
	public:
		/* constructor */
		ParticleSystem(GLuint tID)
		{
			/* add all the particles to vector */
			for(int i = 0; i < pamount; ++i)
			{
				Particle *p = new Particle;
				addParticle(p);
				ps.push_back(p);
			}
			/* simulating a continuous process*/
			for(int i = 0; i < 1000; ++i)
				update();
			textureID = tID;
		}
		
		/* destructor */
		~ParticleSystem()
		{
			Particle *p = nullptr;
			for(unsigned int i = 0; i < ps.size(); ++i)
				delete ps[i];
		}

		/* void update(void)
		 * function: update status of particle system.
		 * parameter: void
		 * return value: void
		 */
		void update(void)
		{
			float dt = 0;
			/* in every interval, calculate status of every particle. */
			while(dt < float(TIMER)/1000)
			{
				/* calculate all the particles */
				for(int i = 0; i < pamount; ++i)
				{
					Particle *p = ps[i];
					p->pos = p->pos + p->velocity * INTVAL;
					/* simulatig graverty */
					p->velocity = p->velocity + Vector3D(0.0, - INTVAL * graverty, 0.0);
					p->timeLived += INTVAL;
					/* add a new particle when a particle is dead. */
					if(p->timeLived > p->lifeSpan)
						addParticle(p);
				}
				dt += INTVAL;
			}
		}
	
		/* void draw(void)
		 * function: draw particles of the particle system.
		 * parameter: void
		 * return value: void
		 */
		void draw(void)
		{
			/* sort all particles by depth for blending */
			sort(ps.begin(), ps.end(), compareParticles);
			/* enable, bind and apply texture */
			glEnable(GL_TEXTURE_2D);
			glBindTexture(GL_TEXTURE_2D, textureID);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
			/* draw particles in squares */
			glBegin(GL_QUADS);
			for(unsigned int i = 0; i < ps.size(); ++i)
			{
				Particle *p = ps[i];
				/* add fading effect by changing alpha value */
				glColor4f(p->colour[0], p->colour[1], p->colour[2],
				(1 - p->timeLived / p->lifeSpan));
				/* get radius of particles */
				float r = psize / 2;
				/* set texture coordinate */
				glTexCoord2f(0, 0);
				/* set normal */
				glNormal3f(0.0f, 0.0f, 1.0f);
				/* set vertex */
				glVertex3f(p->pos[0] - r, p->pos[1] - r, p->pos[2]);
				glTexCoord2f(0, 1);
				glVertex3f(p->pos[0] - r, p->pos[1] + r, p->pos[2]);
				glTexCoord2f(1, 1);
				glVertex3f(p->pos[0] + r, p->pos[1] + r, p->pos[2]);
				glTexCoord2f(1, 0);
				glVertex3f(p->pos[0] + r, p->pos[1] - r, p->pos[2]);
			}
			glEnd();
		}
};

/* global variables */
ParticleSystem *particleSystem;
/* texture id */
GLuint tID;
/* used as a timer to change colour */
int colourTimer = 0;

/* functions deal with user input */

/* bool isNum(string str)
 * function: judge if str is a number 
 * parameter: a string
 * return value: if the string only only contains numbers 
 * and one dot(for floating number)
 */
bool isNum(string str)
{
	int counter = 0;

	for (unsigned int i = 0; i < str.length(); ++i)
	{
		if (isdigit(str[i]))
			continue;
		else if (str[i] == '.' && counter == 0)
			counter++;
		else
			return false;
	}
	return true;
}

/* bool isInteger(string str)
 * function: judge if str is an integer 
 * parameter: a string
 * return value: if the string only contains numbers
 */
bool isInteger(string str)
{
	for (unsigned int i = 0; i < str.length(); ++i)
		if (!isdigit(str[i]))
			return false;
	return true;
}

/* int str2int(string s)
 * function: convert string to int 
 * parameter: a string
 * function: return the corresponding integer of the string
 */
int str2int(string s)
{
	int num;
	stringstream ss(s);
	ss >> num;
	return num;

}

/* float str2float(string s)
 * function: convert string to float 
 * parameter: a string
 * return value: return the corresponding float of the string
 */
float str2float(string s)
{
	float num;
	stringstream ss(s);
	ss >> num;
	return num;

}

int main(int argc, char **argv)
{
	glutInit(&argc, argv);
	// double buffer, RGB colour pattern, depth buffer
	glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB | GLUT_DEPTH);
	glutInitWindowPosition(100, 100);
	glutInitWindowSize(500, 500);

	string input;
    char choice;
	/* interactive logic 
	 * get user configure for the fountain
	 */
	cout << "choose mode: default settings(d) / advanced settings(a)" << endl;
	cin >> input;
	choice = input[0];
	
	while (choice != 'a' && choice != 'A' && choice != 'd' && choice != 'D' || input.length() != 1)
	{
		cout << "invalid choice, choose again:" << endl;
		cin >> input;
		choice = input[0];
	}
	if(choice == 'a' || choice == 'A')
	{
		cout << "particle number(0-50000):" << endl;
		cin >> input;
		while (!isInteger(input) || str2int(input) < 0 || str2int(input) > 50000)
		{
			cout << "invalid input, input again" << endl;
			cin >> input;
		}
		pamount = str2int(input);
		cout << "particle size(0-1):" << endl;
		cin >> input;
		while (!isNum(input) || str2float(input) < 0 || str2float(input) > 1)
		{
			cout << "invalid input, input again" << endl;
			cin >> input;
		}
		psize = str2float(input);
    	cout << "minimal velocity(0-10):" << endl;
		cin >> input;
		while (!isNum(input) || str2float(input) < 0 || str2float(input) > 10)
		{
			cout << "invalid input, input again" << endl;
			cin >> input;
		}
		minV = str2float(input);
		v = minV;
		cout << "maximum velocity(minV-10):" << endl;
		cin >> input;
		while (!isNum(input) || str2float(input) < minV || str2float(input) > 10)
		{
			cout << "invalid input, input again" << endl;
			cin >> input;
		}
		maxV = str2float(input);
		cout << "graverty(0-20):" << endl;
		cin >> input;
		while (!isNum(input) || str2float(input) < 0 || str2float(input) > 20)
		{
			cout << "invalid input, input again" << endl;
			cin >> input;
		}
		graverty = str2float(input);
		cout << "is Spin? (y/n)" << endl;
    	cin >> input;
		while ((input[0] != 'y' && input[0] != 'Y' && input[0] != 'n' && input[0] != 'N') || input.length() != 1)
		{
			cout << "invalid input, input again" << endl;
			cin >> input;
		}
		choice = input[0];
    	if(choice == 'y' || choice == 'Y')
        	isSpin = true;
    	else
        	isSpin = false;	
		cout << "is Colourful? (y/n)" << endl;
		cin >> input;
		while ((input[0] != 'y' && input[0] != 'Y' && input[0] != 'n' && input[0] != 'N') || input.length() != 1)
		{
			cout << "invalid input, input again" << endl;
			cin >> input;
		}
		choice = input[0];
		if(choice == 'y' || choice == 'Y')
			isColourful = true;
		else
			isColourful = false;
	}
	glutCreateWindow("Particle Systems");
	initRendering();
	/* init random seed */
	srand(static_cast <unsigned> (time(0)));
	particleSystem = new ParticleSystem(tID);

	/* register functions */
	glutDisplayFunc(drawFountain);
	glutKeyboardFunc(handleKeypress);
	glutReshapeFunc(handleResize);
	glutTimerFunc(TIMER, update, 0);
	glutMainLoop();

	return 0;
}

/* void initRendering(void)
 * function: initial rendering, load texture and get textureID.
 * parameter: void
 * return value: void
 */
void initRendering(void)
{
	/* enable depth function */
	glEnable(GL_DEPTH_TEST);
	/*enable colour texture */
	glEnable(GL_COLOR_MATERIAL);
	glEnable(GL_BLEND);
	/* enable lighting */
	glEnable(GL_LIGHTING);
	glEnable(GL_LIGHT0);
	glEnable(GL_NORMALIZE);
	/* alpha blending */
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	/* load texture */
	AUX_RGBImageRec *image = LoadBMP("RGB.bmp");
	AUX_RGBImageRec *alphaChannel = LoadBMP("alpha.bmp");
	tID = loadAlphaTexture(image, alphaChannel);
	delete image;
	delete alphaChannel;
}

/* AUX_RGBImageRec *LoadBMP(char *Filename)
 * function: load an image by its name and return a AUX_RGBImageRec pointer
 * parameter: the filename of a image
 * return value: AUX_RGBImageRec containing image information
 */
AUX_RGBImageRec *LoadBMP(char *Filename)
{
	int flag;
	FILE *File = NULL;
	/* return null if the image is not available */
	if (!Filename)                    
	{
		return NULL;                
	}
	flag = fopen_s(&File, Filename, "r");
	if (!flag)
	{
		/* close the handler */
		fclose(File);            
		/* load the bitmap and return a pointer */
		return auxDIBImageLoad(Filename);     
	}
	return NULL;
}


/* void handleResize(int w, int h) 
 * function: resize window and set perspective 
 * parameters: width and height of the window
 * return value: void
 */
void handleResize(int w, int h) 
{
	/* set position and size of view point */
	glViewport(0, 0, w, h);
	glMatrixMode(GL_PROJECTION);
	/* identify matrix */
	glLoadIdentity();
	/* set perspective */
	gluPerspective(45.0, (double)w / (double)h, 3.0, 20.0);
}

/* void handleKeypress(unsigned char key, int x, int y) 
 * function: terminate programme 
 * parameters: ascii value of the key pressed and the current mouse coordinates x and y
 * return value: void
 */
void handleKeypress(unsigned char key, int x, int y)
{
    switch (key) {
		/* return key */
        case 13:
            delete particleSystem;
            exit(0);
    }   
}

/* void drawFountain(void)
 * function: clear window set environmental colour 
 * and redraw the fountain
 * parameter: void
 * return value: void
 */
void drawFountain(void)
{
	/* clear color and depth buffer */
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	/* set ambient lighting */
	glLightModelfv(GL_LIGHT_MODEL_AMBIENT, ambientColor);
	/* set another lighting */
	GLfloat lightColor0[] = {0.5f, 0.5f, 0.5f, 1.0f};
	GLfloat lightPos0[] = {0.0f, 0.0f, 3.0f, 1.0f};
	glLightfv(GL_LIGHT0, GL_DIFFUSE, lightColor0);
	glLightfv(GL_LIGHT0, GL_POSITION, lightPos0);
	/* manipulate modelview matrix */
	glMatrixMode(GL_MODELVIEW);
	/* load identity matrix */
	glLoadIdentity();
	/* adjust position */
	glTranslatef(0.0f, -1.0f, -5.0f);
	/* draw fountain */
	particleSystem->draw();

	glutSwapBuffers();
}

/* void update(int value)
 * function: update scene 
 * parameter: a value which users can use to control behaviour of
 * the update function
 * return value: void
 */
void update(int value)
{
	particleSystem->update();
	/* when in rising stage, increase v */
	if(isUp)
		v += 0.05;
	/* otherwise decrease v */
	else
		v -= 0.05;
	/* when v hits the max velocity, change isUp to false
	 * so that the velocity will decrease.
	 */
	if(v >= maxV)
		isUp = false;
	/* when v reach the bottom, alter isUp to true
	 * so that the velocity will grow.
	 */
	if(v <= minV)
		isUp = true;
	/* when colour is enabled, change ambient color */
	if(isColourful)
	{
		/* change colour every 100 ms */
		if(!(colourTimer % 100))
			for(int i = 0; i < 3; ++i)
				ambientColor[i] = getRandomF();
		colourTimer += TIMER;
	}
	/* if spin is enabled, increase angle gradually */
	if(isSpin)
	{
		angle += 0.01f;
		if(angle > 360)
			angle -= 360;
	}
	glutPostRedisplay();
	glutTimerFunc(TIMER, update, 0);
}

/* GLuint loadAlphaTexture(AUX_RGBImageRec *image, AUX_RGBImageRec *alphaChannel)
 * function: load image into a texture, using the specified grayscale image as an
 * alpha channel and returns the id of the texture
 * parameter: AUX_RGBImageRec pointers of two images
 * return value: a textureID which is made from the two images
 */
GLuint loadAlphaTexture(AUX_RGBImageRec *image, AUX_RGBImageRec *alphaChannel)
{
    char *pixels = addAlphaChannel(image, alphaChannel);
    GLuint textureId;
	/* generate a texture id */
    glGenTextures(1, &textureId);
    glBindTexture(GL_TEXTURE_2D, textureId);
	/* load pixels to texture */
    glTexImage2D(GL_TEXTURE_2D,
                 0,
                 GL_RGBA,
                 image->sizeX, image->sizeY,
                 0,
                 GL_RGBA,
                 GL_UNSIGNED_BYTE,
                 pixels);
    
    delete pixels;
    return textureId;
}

/* char *addAlphaChannel(AUX_RGBImageRec *image, AUX_RGBImageRec *alphaChannel)
 * function:load alphaChannel to an image
 * parameter: AUX_RGBImageRec pointers of two images
 * return value: a char pointer, the first 3 elements of every entry it points is the R, G, B
 * value of the 1st image and the 4th element is the R value of the 2nd image(as alpha value).
 */
char *addAlphaChannel(AUX_RGBImageRec *image, AUX_RGBImageRec *alphaChannel)
{
    char *pixels = new char[image->sizeX * image->sizeY * 4];
    for(int h = 0; h < image->sizeY; ++h) 
	{
        for(int w = 0; w < image->sizeX; ++w) 
		{
            for(int c = 0; c < 3; ++c) 
			{
				/* assign original pixels */
                pixels[4 * (h * image->sizeX + w) + c] =
                    image->data[3 * (h * image->sizeX + w) + c]; 
            }
			/* append alpha channel */
            pixels[4 * (h * image->sizeX + w) + 3] =
                alphaChannel->data[3 * (h * image->sizeX + w)];
        }
    }   
    return pixels;
}
