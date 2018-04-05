//Season change in nature- Computer Graphics Lab Project

#include <iostream>
#include <stdlib.h>
#ifdef __APPLE__
#else
#include <GL/glut.h>
#endif
#include "imageloader.h"
#include "vec3f.h"
#include <stdio.h>
#include <string.h>
#include <math.h>
#include <GL/gl.h>


#define MAX_PARTICLES 1000
#define WCX		640
#define WCY		480
#define SUMMER	0
#define SNOW	3
#define	AUTUMN	2
#define RAIN	1
#define SPRING  4

using namespace std;

float slowdown = 2.0;
float velocity = 0.0;
float zoom = -40.0;
float pan = 0.0;
float tilt = 0.0;
float hailsize = 0.1;
int stopSnow = 0;
int loop;
int fall;
int earthQ =0;

//floor colors
float r = 1.0;
float g = 1.0;
float b = 1.0;
float ground_points[21][21][3];
float ground_colors[21][21][4];
float accum = -10.0;
int fog=0;

typedef struct {
  // Life
  bool alive;	// is the particle alive?
  float life;	// particle lifespan
  float fade; // decay
  // color
  float red;
  float green;
  float blue;
  // Position/direction
  float xpos;
  float ypos;
  float zpos;
  // Velocity/Direction, only goes down in y dir
  float vel;
  // Gravity
  float gravity;
}particles;

// Paticle System
particles par_sys[MAX_PARTICLES];


int flag =0;
float rHill= 0.3,gHill= 0.9,bHill=1.0;
float rSky=0.2 , gSky=0.0 , bSky= 0.8;

//Represents a terrain, by storing a set of heights and normals at 2D locations
class Terrain {
	private:
	    //no of grid points in the x and z directions
		int w; //Width
		int l; //Length
		//2d array which stores the height of the terrain at each of the grid point
		float** hs; //Heights
		//2d array which stores the normals of the terrain at each of the grid point

		Vec3f** normals;
		bool computedNormals; //Whether normals is up-to-date
	public: //make the two 2d arrays.
		Terrain(int w2, int l2) { //constructor
			w = w2;
			l = l2;

			hs = new float*[l];
			for(int i = 0; i < l; i++) {
				hs[i] = new float[w];
			}

			normals = new Vec3f*[l];
			for(int i = 0; i < l; i++) {
				normals[i] = new Vec3f[w];
			}

			computedNormals = false;
		}

		~Terrain() { //constructor for deleing
			for(int i = 0; i < l; i++) {
				delete[] hs[i];
			}
			delete[] hs;

			for(int i = 0; i < l; i++) {
				delete[] normals[i];
			}
			delete[] normals;
		}

		int width() {
			return w;
		}

		int length() {
			return l;
		}

		//Sets the height at (x, z) to y
		void setHeight(int x, int z, float y) {
			hs[z][x] = y;
			computedNormals = false;
		}

		//Returns the height at (x, z)
		float getHeight(int x, int z) {
			return hs[z][x];
		}

		//Computes the normals, if they haven't been computed yet
		void computeNormals() {
			if (computedNormals) {
				return;
			}

			//Compute the rough version of the normals
			Vec3f** normals2 = new Vec3f*[l];
			for(int i = 0; i < l; i++) {
				normals2[i] = new Vec3f[w];
			}

			for(int z = 0; z < l; z++) {
				for(int x = 0; x < w; x++) {
					Vec3f sum(0.0f, 0.0f, 0.0f);

					Vec3f out;
					if (z > 0) {
						out = Vec3f(0.0f, hs[z - 1][x] - hs[z][x], -1.0f);
					}
					Vec3f in;
					if (z < l - 1) {
						in = Vec3f(0.0f, hs[z + 1][x] - hs[z][x], 1.0f);
					}
					Vec3f left;
					if (x > 0) {
						left = Vec3f(-1.0f, hs[z][x - 1] - hs[z][x], 0.0f);
					}
					Vec3f right;
					if (x < w - 1) {
						right = Vec3f(1.0f, hs[z][x + 1] - hs[z][x], 0.0f);
					}

					if (x > 0 && z > 0) {
						sum += out.cross(left).normalize();
					}
					if (x > 0 && z < l - 1) {
						sum += left.cross(in).normalize();
					}
					if (x < w - 1 && z < l - 1) {
						sum += in.cross(right).normalize();
					}
					if (x < w - 1 && z > 0) {
						sum += right.cross(out).normalize();
					}

					normals2[z][x] = sum;
				}
			}

			//Smooth out the normals
			const float FALLOUT_RATIO = 0.5f;
			for(int z = 0; z < l; z++) {
				for(int x = 0; x < w; x++) {
					Vec3f sum = normals2[z][x];

					if (x > 0) {
						sum += normals2[z][x - 1] * FALLOUT_RATIO;
					}
					if (x < w - 1) {
						sum += normals2[z][x + 1] * FALLOUT_RATIO;
					}
					if (z > 0) {
						sum += normals2[z - 1][x] * FALLOUT_RATIO;
					}
					if (z < l - 1) {
						sum += normals2[z + 1][x] * FALLOUT_RATIO;
					}

					if (sum.magnitude() == 0) {
						sum = Vec3f(0.0f, 1.0f, 0.0f);
					}
					normals[z][x] = sum;
				}
			}

			for(int i = 0; i < l; i++) {
				delete[] normals2[i];
			}
			delete[] normals2;

			computedNormals = true;
		}

		//Returns the normal at (x, z)
		Vec3f getNormal(int x, int z) {
			if (!computedNormals) {
				computeNormals();
			}
			return normals[z][x];
		}
};

//Loads a terrain from a heightmap.  The heights of the terrain range from
//-height / 2 to height / 2.
Terrain* loadTerrain(const char* filename, float height) {
	Image* image = loadBMP(filename);
	Terrain* t = new Terrain(image->width, image->height); //construct a new terrain using the image height and width
	for(int y = 0; y < image->height; y++) { //takes each pixel and sets the cheight accordingly
		for(int x = 0; x < image->width; x++) {
			unsigned char color =
				(unsigned char)image->pixels[3 * (y * image->width + x)]; //takes the red component and calculate the hright
			float h = height * ((color / 255.0f) - 0.5f);
			t->setHeight(x, y, h);
		}
	}

	delete image;
	t->computeNormals();
	return t;
}

GLuint loadTexture(Image* image) {
	GLuint textureId;
	//GLuint _textureId2;

	//The first argument is the number of textures we need, and the second is an array where OpenGL will store the id's of the textures.
	glGenTextures(1, &textureId); //Make room for our texture
	// to assign the texture id to our image data.
//	We call glBindTexture(GL_TEXTURE_2D, textureId) to let OpenGL know
// that we want to work with the texture we just created. Then, we call glTexImage2D to load the image into OpenGL.
	glBindTexture(GL_TEXTURE_2D, textureId); //Tell OpenGL which texture to edit
	//Map the image to the texture
	glTexImage2D(GL_TEXTURE_2D,                //Always GL_TEXTURE_2D
				 0,                            //0 for now
				 GL_RGB,                       //Format OpenGL uses for image
				 image->width, image->height,  //Width and height
				 0,                            //The border of the image
				 GL_RGB, //GL_RGB, because pixels are stored in RGB format
				 GL_UNSIGNED_BYTE, //GL_UNSIGNED_BYTE, because pixels are stored
				                   //as unsigned numbers
				 image->pixels);               //The actual pixel data
	return textureId; //Returns the id of the texture
}

GLuint _textureId;
GLuint _textureSpring;
GLuint _textureSkySummer;
GLuint _textureSkyRain;
GLuint _textureSkyAutumn;
GLuint _textureSkyWinter;
GLuint _textureSkySpring;
GLuint _textureLeaf1;

float _angle = 60.0f;
Terrain* _terrain;

void cleanup() {
	delete _terrain;
}

void handleKeypress(unsigned char key, int x, int y) {
	switch (key) {
		case 27: //Escape key
			cleanup();
			exit(0);
	}
}


void handleResize(int w, int h) {
	glViewport(0, 0, w, h);
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	gluPerspective(45.0, (double)w / (double)h, 1.0, 200.0);
}
void initParticles(int i) {
    par_sys[i].alive = true;
    par_sys[i].life = 1.0;
    par_sys[i].fade = float(rand()%100)/1000.0f+0.003f;

    par_sys[i].xpos = (float) (rand() % 501) - 10;
    par_sys[i].ypos = 100.0;
    par_sys[i].zpos = (float) (rand() % 501) - 10;

    par_sys[i].red = 0.5;
    par_sys[i].green = 0.5;
    par_sys[i].blue = 1.0;

    par_sys[i].vel = velocity;
    par_sys[i].gravity = -0.8;//-0.8;

}
void init( ) {
  int x, z;

    glShadeModel(GL_SMOOTH);
    glClearDepth(1.0);
    glEnable(GL_DEPTH_TEST);
	glEnable(GL_COLOR_MATERIAL);
	glEnable(GL_LIGHTING);
	glEnable(GL_LIGHT0);
	glEnable(GL_FOG);
	glEnable(GL_NORMALIZE);



    Image* image = loadBMP("s1.bmp");
	_textureSpring = loadTexture(image);
	delete image;

     image = loadBMP("grass.bmp");
	 _textureId = loadTexture(image);
	 delete image;

	   image = loadBMP("skySummer.bmp");
	_textureSkySummer = loadTexture(image);
	delete image;

	 image = loadBMP("skyRain.bmp");
	_textureSkyRain = loadTexture(image);
	delete image;
   image = loadBMP("skyAutumn.bmp");
	_textureSkyAutumn = loadTexture(image);
	delete image;

	 image = loadBMP("skySpring.bmp");
	_textureSkySpring= loadTexture(image);
	delete image;

	 image = loadBMP("skyWinter.bmp");
	_textureSkyWinter = loadTexture(image);
	delete image;






    // Initialize particles
    for (loop = 0; loop < MAX_PARTICLES; loop++) {
        initParticles(loop);
    }
}

// For Rain
void drawRain() {
  float x, y, z;
  //glClearColor(rSky, gSky, bSky, 1.0f);
  for (loop = 0; loop < MAX_PARTICLES; loop=loop+2) {
    if (par_sys[loop].alive == true) {
      x = par_sys[loop].xpos;
      y = par_sys[loop].ypos;
      z = par_sys[loop].zpos + zoom;

      // Draw particles
     glDisable(GL_TEXTURE_2D);
      glColor3f(0.5, 0.5, 1.0);
      glBegin(GL_LINES);
        glVertex3f(x, y, z);
        glVertex3f(x, y+1.5, z);
      glEnd();

      // Update values
      //Move
      // Adjust slowdown for speed!
      par_sys[loop].ypos += par_sys[loop].vel / (slowdown*1000);
      par_sys[loop].vel += par_sys[loop].gravity;
      // Decay
      par_sys[loop].life -= par_sys[loop].fade;

      if (par_sys[loop].ypos <= -2) {
        par_sys[loop].life = -1.0;
      }
      //Revive
      if (par_sys[loop].life < 0.0) {
        initParticles(loop);
      }
    }
  }
}

// For Snow
void drawSnow() {
    if(stopSnow==0)
    {


  float x, y, z;
  //glClearColor(rSky, gSky, bSky, 1.0f);
  for (loop = 0; loop < MAX_PARTICLES; loop=loop+2) {
    if (par_sys[loop].alive == true) {
      x = par_sys[loop].xpos;
      y = par_sys[loop].ypos;
      z = par_sys[loop].zpos + zoom;

      // Draw particles
      glDisable(GL_TEXTURE_2D);
      glColor3f(1.0, 1.0, 1.0);
      glPushMatrix();
      glTranslatef(x, y, z);
      glutSolidSphere(0.5, 16, 16);
      glPopMatrix();

      // Update values
      //Move
      par_sys[loop].ypos += par_sys[loop].vel / (slowdown*1000);
      par_sys[loop].vel += par_sys[loop].gravity;
      // Decay
      par_sys[loop].life -= par_sys[loop].fade;

      if (par_sys[loop].ypos <= -5) {
        int zi = z - zoom + 10;
        int xi = x + 10;
        ground_colors[zi][xi][0] = 1.0;
        ground_colors[zi][xi][2] = 1.0;
        ground_colors[zi][xi][3] += 1.0;
        if (ground_colors[zi][xi][3] > 1.0) {
          ground_points[xi][zi][1] += 0.1;
        }
        par_sys[loop].life = -1.0;
      }

      //Revive
      if (par_sys[loop].life < 0.0) {
        initParticles(loop);
      }
    }
  }
    }
}


void drawScene() {
     GLUquadric *quadric;
    quadric = gluNewQuadric();
     int i, j;
  float x, y, z;
  glClearColor(rSky, gSky, bSky, 1.0f);

	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);



	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();


	glTranslatef(0.0f, 0.0f, -10.0f);
	glRotatef(30.0f, 1.0f, 0.0f, 0.0f);
	glRotatef(-_angle, 0.0f, 1.0f, 0.0f);

	GLfloat ambientColor[] = {0.4f, 0.4f, 0.4f, 1.0f};
	glLightModelfv(GL_LIGHT_MODEL_AMBIENT, ambientColor);

	GLfloat lightColor0[] = {0.6f, 0.6f, 0.6f, 1.0f};
	GLfloat lightPos0[] = {-0.5f, 0.8f, 0.1f, 0.0f};
	glLightfv(GL_LIGHT0, GL_DIFFUSE, lightColor0);
	glLightfv(GL_LIGHT0, GL_POSITION, lightPos0);

	if(fog ==1)
    {
         GLfloat fogcolour[4]= {1.0,1.0,1.0,1.0};

        glFogfv(GL_FOG_COLOR,fogcolour);              /* Define the fog colour */
                          /* How dense */
        glFogi(GL_FOG_MODE,GL_EXP);
        glFogf(GL_FOG_DENSITY,0.08);                   /* exponential decay */
        glFogf(GL_FOG_START,3.0);                   /* Where wwe start fogging */
        glFogf(GL_FOG_END,100.0);                       /* end */
        glHint(GL_FOG_HINT, GL_FASTEST);              /* compute per vertex */
        glEnable(GL_FOG);/* ENABLE */

    }
    else if(fog==0)
    {

        glDisable(GL_FOG);
    }




    glPushMatrix();
    glRotatef(0.0,0.0,0.0,1.0); // orbits the planet around the sun
    glTranslatef(1.40,0.0,0.0);        // sets the radius of the orbit
    glRotatef(0.0,0.0,1.0,1.0); // revolves the planet on its axis
    glColor3f(4.0, 4.0, 4.0);          // green

    glEnable(GL_TEXTURE_2D);
    if (fall == RAIN)
    {
        glBindTexture(GL_TEXTURE_2D, _textureSkyRain);
    }
    else if (fall == SUMMER)
    {
        glBindTexture(GL_TEXTURE_2D, _textureSkySummer);
    }
     else if (fall == SNOW)
    {
        glBindTexture(GL_TEXTURE_2D, _textureSkyWinter);
    }
     else if (fall == AUTUMN)
    {
        glBindTexture(GL_TEXTURE_2D, _textureSkyAutumn);
    }
     else if (fall == SPRING)
    {
        glBindTexture(GL_TEXTURE_2D, _textureSkySpring);
    }

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    gluQuadricTexture(quadric, 20);
    gluSphere(quadric,75,40,40);
    glDisable(GL_TEXTURE_2D);
    glPopMatrix();



	glEnable(GL_TEXTURE_2D);
	if(fall == SPRING|| fall==AUTUMN)
    {
        glBindTexture(GL_TEXTURE_2D, _textureSpring);
    }
    else
    {
        glBindTexture(GL_TEXTURE_2D, _textureId);
    }



	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);





    	//1st tree
	glDisable(GL_TEXTURE_2D);

	if(fall == SNOW)
    {
         glColor3f(1.0,1.0,1.0);
    }
    else if(fall == AUTUMN)
    {
         glColor3f(1.0,1.0,0.0);
    }

    else
    {
        glColor3f(0.196, 0.804, 0.196);
    }

      glPushMatrix();
      glTranslatef(3.5,1.0,-3.0);
      glTexCoord3f(5.0f, 5.0f,5.0f);
      glutSolidSphere(0.6, 16, 16);

      glPopMatrix();
//2nd tree
    glColor3f(0.722, 0.525, 0.043);

      glPushMatrix();
      glTranslatef(3.0,0.0,-5.0);
     glRotatef(240.0,1.5,2.0,1.3);
      glTexCoord3f(5.0f, 5.0f,0.0f);
      glutSolidCone(0.1,2.0,100,100);

      glPopMatrix();
      if(fall == SNOW)
    {
         glColor3f(1.0,1.0,1.0);
    }
    else if(fall == AUTUMN)
    {
         glColor3f(1.000, 0.498, 0.314);
    }

    else
    {
        glColor3f(0.0, 0.9, 0.0);
    }


      glPushMatrix();
      glTranslatef(-2.5,1.0,-3.0);
      glTexCoord3f(5.0f, 5.0f,5.0f);
      glutSolidSphere(0.6, 16, 16);

      glPopMatrix();

      if(fall == SNOW)
    {
         glColor3f(1.0,1.0,1.0);
    }
    else if(fall == AUTUMN)
    {
        glColor3f(1.000, 0.498, 0.314);
    }

    else
    {
        glColor3f(0.0, 0.9, 0.0);
    }


      glPushMatrix();
      glTranslatef(-4.5,1.4,-1.0);
      glTexCoord3f(5.0f, 5.0f,5.0f);
      glutSolidSphere(0.6, 16, 16);

      glPopMatrix();

      if(fall == SNOW)
    {
         glColor3f(0.561, 0.737, 0.561);
    }
    else if(fall == AUTUMN)
    {
         glColor3f(1.000, 0.843, 0.000);
    }

    else
    {
       glColor3f(0.7, 0.7, 0.0);
    }


      glPushMatrix();
      glTranslatef(-3.2,1.6,-2.0);
      glTexCoord3f(5.0f, 5.0f,5.0f);
      glutSolidSphere(0.8, 16, 16);

      glPopMatrix();

       glColor3f(0.722, 0.525, 0.043); //daal

      glPushMatrix();
      glTranslatef(-0.5,0.0,6.0);
     glRotatef(240.0,1.5,2.0,1.3);
      glTexCoord3f(5.0f, 5.0f,0.0f);
      glutSolidCone(0.1,2.0,100,100);

      glPopMatrix();
       if(fall == SNOW)
    {
         glColor3f(1.0, 0.737, 0.561);
    }
    else if(fall == AUTUMN)
    {
         glColor3f(0.502, 0.502, 0.000);
    }

    else
    {
        glColor3f(0.722, 0.525, 0.043);
    }




      glPushMatrix();
      glTranslatef(2.0,0.0,-5.0);
     glRotatef(240.0,1.5,2.0,1.3);
      glTexCoord3f(5.0f, 5.0f,0.0f);
      glutSolidCone(0.1,2.0,100,100);

      glPopMatrix();



      glColor3f(0.0, 0.0, 0.0);

      glPushMatrix();
      glTranslatef(-3.0,0.0,-2.0);
     glRotatef(240.0,1.5,2.0,1.3);
      glTexCoord3f(5.0f, 5.0f,0.0f);
      glutSolidCone(0.1,2.0,100,100);

      glPopMatrix();

      glColor3f(0.8, 0.6, 0.0);

      glPushMatrix();
      glTranslatef(-0.50,0.0,1.0);
     glRotatef(240.0,1.5,2.0,1.3);
      glTexCoord3f(5.0f, 5.0f,0.0f);
      glutSolidCone(0.1,2.0,100,100);

      glPopMatrix();


       glColor3f(0.9, 0.8, 0.0);

      glPushMatrix();
      glTranslatef(-0.50,0.0,-5.0);
     glRotatef(240.0,1.5,2.0,1.3);
      glTexCoord3f(5.0f, 5.0f,0.0f);
      glutSolidCone(0.1,2.0,100,100);

      glPopMatrix();

        //house top
        if(fall==SNOW)
        {

           glColor3f(0.871, 0.722, 0.529);
        }
        else
        {
             glColor3f(0.855, 0.647, 0.125);
        }




      glPushMatrix();
      glTranslatef(-3.0,0.5,-1.0);
     glRotatef(240.0,1.5,2.0,1.3);
      glTexCoord3f(5.0f, 5.0f,0.0f);
      glutSolidCone(0.8,1.0,100,100);

      glPopMatrix();
    //house bottom
     glColor3f(0.545, 0.271, 0.075);

      glPushMatrix();
      glTranslatef(-3.0,0.0,-1.0);
     glRotatef(240.0,1.5,2.0,1.3);
      glTexCoord3f(5.0f, 5.0f,0.0f);
      glutSolidCube(1.0);

      glPopMatrix();

        //house top2
        if(fall==SNOW)
        {

           glColor3f(0.871, 0.722, 0.529);
        }
        else
        {
            glColor3f(0.722, 0.525, 0.043);
        }


      glPushMatrix();
      glTranslatef(-1.0,0.5,-2.0);
     glRotatef(240.0,1.5,2.0,1.3);
      glTexCoord3f(5.0f, 5.0f,0.0f);
      glutSolidCone(0.8,1.0,100,100);

      glPopMatrix();
    //house bottom2
     glColor3f(0.4, 0.0, 0.0040);

      glPushMatrix();
      glTranslatef(-1.0,0.0,-2.0);
     glRotatef(240.0,1.5,2.0,1.3);
      glTexCoord3f(5.0f, 5.0f,0.0f);
      glutSolidCube(1.0);

      glPopMatrix();

       glColor3f(0.8, 0.68, 0.0040);

      glPushMatrix();
      glTranslatef(-0.7,0.0,-2.0);
     glRotatef(240.0,1.5,2.0,1.3);
      glTexCoord3f(5.0f, 5.0f,0.0f);
      glutSolidCube(0.5);

      glPopMatrix();
       glColor3f(0.8, 0.68, 0.0040);

      glPushMatrix();
      glTranslatef(-1.0,0.0,-1.7);
     glRotatef(240.0,1.5,2.0,1.3);
      glTexCoord3f(5.0f, 5.0f,0.0f);
      glutSolidCube(0.5);

      glPopMatrix();

           //house top3
           if(fall==SNOW)
            {

                glColor3f(0.871, 0.722, 0.529);
            }
        else
        {
           glColor3f(0.824, 0.412, 0.118);
        }




      glPushMatrix();
      glTranslatef(-1.0,0.5,2.0);
     glRotatef(240.0,1.5,2.0,1.3);
      glTexCoord3f(5.0f, 5.0f,0.0f);
      glutSolidCone(0.8,1.0,100,100);

      glPopMatrix();
    //house bottom3
     glColor3f(0.545, 0.271, 0.075);

      glPushMatrix();
      glTranslatef(-1.0,0.0,2.0);
     glRotatef(240.0,1.5,2.0,1.3);
      glTexCoord3f(5.0f, 5.0f,0.0f);
      glutSolidCube(1.0);

      glPopMatrix();

        glColor3f(1.0, 1.0, 0.0);

      glPushMatrix();
      glTranslatef(-0.7,0.0,2.0);
     glRotatef(240.0,1.5,2.0,1.3);
      glTexCoord3f(5.0f, 5.0f,0.0f);
      glutSolidCube(0.5);

      glPopMatrix();




        glColor3f(0.922, 0.525, 0.043);//daal
      glPushMatrix();
      glTranslatef(6.0,-1.0,-0.5);
     glRotatef(240.0,1.5,2.0,1.3);
      glTexCoord3f(5.0f, 5.0f,0.0f);
      glutSolidCone(0.2,3.0,100,100);

      glPopMatrix();
      if(fall == SNOW)
    {
         glColor3f(0.561, 0.737, 0.561);
    }
    else if(fall == AUTUMN)
    {
          glColor3f(1.000, 0.843, 0.000);
    }

    else
    {
        glColor3f(0.498, 1.000, 0.000);
    }


      glPushMatrix();
      glTranslatef(3.5,3.0,-3.3);
      glTexCoord3f(5.0f, 5.0f,0.0f);
      glutSolidSphere(0.5, 16, 16);

      glPopMatrix();

      if(fall == SNOW)
    {
         glColor3f(1.0, 1.0, 1.0);
    }
    else if(fall == AUTUMN)
    {
          glColor3f(1.000, 0.843, 0.000);
    }

    else
    {
        glColor3f(0.0,1.0,0.0);
    }


      glPushMatrix();
      glTranslatef(1.5,2.8,4.4);
      glRotatef(240.0,1.5,2.0,1.3);
      glTexCoord3f(5.0f, 5.0f,0.0f);
      glutSolidCone(0.4,1.0,100,100);

      glPopMatrix();

      if(fall == SNOW)
    {
         glColor3f(0.561, 0.737, 0.561);
    }
    else if(fall == AUTUMN)
    {
         glColor3f(1.000, 0.843, 0.000);
    }

    else
    {
        glColor3f(0.486, 0.988, 0.000);
    }


      glPushMatrix();
      glTranslatef(1.5,2.2,4.5);
      glRotatef(240.0,1.5,2.0,1.3);
      glTexCoord3f(5.0f, 5.0f,0.0f);
      glutSolidCone(0.6,1.0,100,100);

      glPopMatrix();

      if(fall == SNOW)
    {
         glColor3f(0.561, 0.737, 0.561);
    }
    else if(fall == AUTUMN)
    {
         glColor3f(1.0,1.0,0.0);
    }

    else
    {
        glColor3f(0.000, 0.392, 0.000);
    }



      glPushMatrix();
      glTranslatef(1.5,1.6,4.6);
      glRotatef(240.0,1.5,2.0,1.3);
      glTexCoord3f(5.0f, 5.0f,0.0f);
      glutSolidCone(0.9,1.0,100,100);

      glPopMatrix();




       glColor3f(0.20, 0.0, 0.20);
      glPushMatrix();
      glTranslatef(2.5,-2.0,5.3);
      glRotatef(240.0,1.5,2.0,1.3);
      glTexCoord3f(5.0f, 5.0f,0.0f);
      glutSolidCone(0.1,5.0,100,100);

      glPopMatrix();

      if(fall == SNOW)
    {
         glColor3f(1.0,1.0,1.0);
    }
    else if(fall == AUTUMN)
    {
         glColor3f(1.000, 0.843, 0.000);
    }

    else
    {
        glColor3f(0.0,1.0,0.0);
    }


      glPushMatrix();
      glTranslatef(1.8,2.8,-4.6);
      glRotatef(240.0,1.5,2.0,1.3);
      glTexCoord3f(5.0f, 5.0f,0.0f);
      glutSolidCone(0.2,1.0,100,100);

      glPopMatrix();

        if(fall == SNOW)
    {
         glColor3f(1.0,1.0,1.0);
    }
    else if(fall == AUTUMN)
    {
         glColor3f(1.000, 0.388, 0.278);
    }

    else
    {
        glColor3f(0.486, 0.988, 0.000);
    }


      glPushMatrix();
      glTranslatef(1.8,2.2,-4.7);
      glRotatef(240.0,1.5,2.0,1.3);
      glTexCoord3f(5.0f, 5.0f,0.0f);
      glutSolidCone(0.4,1.0,100,100);

      glPopMatrix();

      if(fall == SNOW)
    {
         glColor3f(0.561, 0.737, 0.561);
    }
    else if(fall == AUTUMN)
    {
         glColor3f(1.000, 0.498, 0.314);
    }

    else
    {
         glColor3f(0.000, 0.392, 0.000);
    }



      glPushMatrix();
      glTranslatef(1.8,1.6,-4.8);
      glRotatef(240.0,1.5,2.0,1.3);
      glTexCoord3f(5.0f, 5.0f,0.0f);
      glutSolidCone(0.6,1.0,100,100);

      glPopMatrix();

      if(fall == SNOW)
    {
         glColor3f(0.561, 0.737, 0.561);
    }
    else if(fall == AUTUMN)
    {
          glColor3f(1.000, 0.647, 0.000);
    }

    else
    {
        glColor3f(0.8,1.0,0.0);
    }



      glPushMatrix();
      glTranslatef(2.8,2.8,2.3);
      glRotatef(240.0,1.5,2.0,1.3);
      glTexCoord3f(5.0f, 5.0f,0.0f);
      glutSolidCone(0.2,1.0,100,100);

      glPopMatrix();
      if(fall == SNOW)
    {
         glColor3f(0.561, 0.737, 0.561);
    }
    else if(fall == AUTUMN)
    {
          glColor3f(1.000, 0.549, 0.000);
    }

    else
    {
         glColor3f(0.5,1.0,0.0);
    }


      glPushMatrix();
      glTranslatef(2.8,2.8,-1.0);
      glRotatef(240.0,1.5,2.0,1.3);
      glTexCoord3f(5.0f, 5.0f,0.0f);
      glutSolidCone(0.2,1.0,100,100);

      glPopMatrix();

      if(fall == SNOW)
    {
         glColor3f(0.561, 0.737, 0.561);
    }
    else if(fall == AUTUMN)
    {
        glColor3f(1.000, 0.549, 0.000);
    }

    else
    {
          glColor3f(0.8,1.0,0.3);
    }


      glPushMatrix();
      glTranslatef(5.8,1.0,-0.5);
      glRotatef(240.0,1.5,2.0,1.3);
      glTexCoord3f(5.0f, 5.0f,0.0f);
      glutSolidCone(0.5,2.5,100,100);

      glPopMatrix();

      if(fall == SNOW)
    {
          glColor3f(0.561, 0.737, 0.561);
    }
    else if(fall == AUTUMN)
    {
          glColor3f(1.000, 0.549, 0.000);
    }

    else
    {
        glColor3f(0.8,1.0,0.3);
    }



      glPushMatrix();
      glTranslatef(1.0,2.0,5.5);
      glRotatef(240.0,1.5,2.0,1.3);
      glTexCoord3f(5.0f, 5.0f,0.0f);
      glutSolidCone(0.5,1.5,100,100);

      glPopMatrix();


    //flower
   /*   if(fall == SPRING)
    {
         glColor3f(1.0,0.0,0.0);


      glPushMatrix();
      glTranslatef(5.5,0.0,-0.5);
      glTexCoord3f(5.0f, 5.0f,5.0f);
      glutSolidSphere(0.1, 16, 16);

      glPopMatrix();

      glColor3f(1.0,1.0,0.0);


      glPushMatrix();
      glTranslatef(5.6,0.0,-0.5);
      glTexCoord3f(5.0f, 5.0f,5.0f);
      glutSolidSphere(0.1, 16, 16);

      glPopMatrix();

      glColor3f(1.0,1.0,0.0);


      glPushMatrix();
      glTranslatef(5.4,0.0,-0.5);
      glTexCoord3f(5.0f, 5.0f,5.0f);
      glutSolidSphere(0.1, 16, 16);

      glPopMatrix();

       glColor3f(1.0,1.0,0.0);


      glPushMatrix();
      glTranslatef(5.4,0.0,-0.6);
      glTexCoord3f(5.0f, 5.0f,5.0f);
      glutSolidSphere(0.1, 16, 16);

      glPopMatrix();

        glColor3f(1.0,1.0,0.0);


      glPushMatrix();
      glTranslatef(5.6,0.0,-0.6);
      glTexCoord3f(5.0f, 5.0f,5.0f);
      glutSolidSphere(0.1, 16, 16);

      glPopMatrix();

      glColor3f(1.0,1.0,0.0);


      glPushMatrix();
      glTranslatef(5.5,0.0,-0.35);
      glTexCoord3f(5.0f, 5.0f,5.0f);
      glutSolidSphere(0.1, 16, 16);

      glPopMatrix();

      //2nd flower

       glColor3f(1.0,0.0,0.0);


      glPushMatrix();
      glTranslatef(4.5,0.0,-0.5);
      glTexCoord3f(5.0f, 5.0f,5.0f);
      glutSolidSphere(0.1, 16, 16);

      glPopMatrix();

      glColor3f(1.0,1.0,0.0);


      glPushMatrix();
      glTranslatef(4.6,0.0,-0.5);
      glTexCoord3f(5.0f, 5.0f,5.0f);
      glutSolidSphere(0.1, 16, 16);

      glPopMatrix();

      glColor3f(1.0,0.0,1.0);


      glPushMatrix();
      glTranslatef(3.9,0.0,-0.5);
      //glTexCoord3f(4.5f, 5.0f,5.0f);
      glutSolidSphere(0.1, 16, 16);

      glPopMatrix();

     glColor3f(1.0,0.0,1.0);


      glPushMatrix();
      glTranslatef(3.9,0.0,-0.6);
      glTexCoord3f(5.0f, 5.0f,5.0f);
      glutSolidSphere(0.1, 16, 16);

      glPopMatrix();

        glColor3f(1.0,0.0,1.0);


      glPushMatrix();
      glTranslatef(4.1,0.0,-0.6);
      glTexCoord3f(5.0f, 5.0f,5.0f);
      glutSolidSphere(0.1, 16, 16);

      glPopMatrix();

       glColor3f(1.0,0.0,1.0);


      glPushMatrix();
      glTranslatef(4.0,0.0,-0.35);
      glTexCoord3f(5.0f, 5.0f,5.0f);
      glutSolidSphere(0.1, 16, 16);

      glPopMatrix();

    }*/








      glEnable(GL_TEXTURE_2D);
        if (fall== SNOW)
    {
        glDisable(GL_TEXTURE_2D);
    }



	float scale = 15.0f / max(_terrain->width() - 1, _terrain->length() - 1);
	glScalef(scale, scale, scale);
	glTranslatef(-(float)(_terrain->width() - 1) / 2,
				 0.0f,
				 -(float)(_terrain->length() - 1) / 2);

	glColor3f(r, g, b);
	for(int z = 0; z < _terrain->length() - 1; z++) {
		//Makes OpenGL draw a triangle at every three consecutive vertices
		glBegin(GL_TRIANGLE_STRIP);
		for(int x = 0; x < _terrain->width(); x++) {
			Vec3f normal = _terrain->getNormal(x, z);
			glNormal3f(normal[0], normal[1], normal[2]);
			glTexCoord3f(0.0f, 0.0f,0.0f);

			glVertex3f(x, _terrain->getHeight(x, z), z);
			normal = _terrain->getNormal(x, z + 1);
			glNormal3f(normal[0], normal[1], normal[2]);
			glTexCoord3f(100.0f, 5.0f,0.0f);
			glVertex3f(x, _terrain->getHeight(x, z + 1), z + 1);
		}
		glEnd();
	}



  glRotatef(pan, 0.0, 1.0, 0.0);
  glRotatef(tilt, 1.0, 0.0, 0.0);

  // GROUND?!
  glColor3f(r, g, b);
  glBegin(GL_QUADS);
    // along z - y const
    for (i = -10; i+1 < 11; i++) {
      // along x - y const
      for (j = -10; j+1 < 11; j++) {
        glColor3fv(ground_colors[i+10][j+10]);
        glVertex3f(ground_points[j+10][i+10][0],
              ground_points[j+10][i+10][1],
              ground_points[j+10][i+10][2] + zoom);
        glColor3fv(ground_colors[i+10][j+1+10]);
        glVertex3f(ground_points[j+1+10][i+10][0],
              ground_points[j+1+10][i+10][1],
              ground_points[j+1+10][i+10][2] + zoom);
        glColor3fv(ground_colors[i+1+10][j+1+10]);
        glVertex3f(ground_points[j+1+10][i+1+10][0],
              ground_points[j+1+10][i+1+10][1],
              ground_points[j+1+10][i+1+10][2] + zoom);
        glColor3fv(ground_colors[i+1+10][j+10]);
        glVertex3f(ground_points[j+10][i+1+10][0],
              ground_points[j+10][i+1+10][1],
              ground_points[j+10][i+1+10][2] + zoom);
      }

    }
  glEnd();
  // Which Particles
  if (fall == RAIN) {
    drawRain();
  }
  else if (fall == SNOW) {
    drawSnow();
  }


	glutSwapBuffers();
}
void my_keyboard(unsigned char key, int x, int y)
{

	switch (key) {

		case 'w':


		    glDisable(GL_TEXTURE_2D);

		    r = 1.0;
		    g = 1.0;
		    b = 1.0;
           // earthQ=0;

		    rSky = 0.690;
		    gSky = 0.769;
		    bSky =  0.871;

		    fall = SNOW;

            drawSnow();

			break;

		case 'r':
		     r = 0.3;
			 g = 0.9;
			 b  = 1.0;
			 fog =0;
			 //earthQ=0;

		     rSky = 0.663;
             gSky = 0.663;
		     bSky = 0.663;
			 fall = RAIN;
            drawRain();
			break;

		case 's':
			 r = 0.678;
			 g = 1.0;
			 b = 0.184;
			  fog =0;
			 // earthQ=0;


            rSky = 0.118;
            gSky = 0.565;
		    bSky =   1.0;
			 fall = SUMMER;
			 break;
	   case 'a':
			 r = 1.0;
			 g = 0.588;
			 b  = 0.0;
			  fog =0;
			  //earthQ=0;

			  rSky = 0.690;
		     gSky =  0.844;
		     bSky =   1.0;
			 fall = AUTUMN;
			 break;
        case 'p':
			 r = 1.0;
			 g = 1.0;
			 b  = 1.0;
			 fog =0;
			 //earthQ=0;


			  rSky = 0.000;
		    gSky = 0.0;
		    bSky =  1.0;
			 fall = SPRING;
			 break;
        case 'e':
            earthQ = 1;
            break;

            case 'x':
            earthQ = 0;
            break;


	  default:
			break;
	}
}

void my_mouse(int button, int state, int x, int y)
{
   switch (button) {
      case GLUT_LEFT_BUTTON:


             if(fall==SNOW)
             {
                 stopSnow=1;
                  fog =1;
             }


         break;
          case GLUT_RIGHT_BUTTON:

                if(fall==SNOW)
                {
                  fog =0;
                  stopSnow=0;
                }

         break;

      default:
         break;
   }
}

void update(int value) {
        if(earthQ==1)
        {
             fall += 1;
            if (fall > 4) {
                fall -= 5.0;
            }
        }
        else{
                 fall += 0;


        }


        glutPostRedisplay();
        glutTimerFunc(2500, update, 0);


}

void reshape(int w, int h) {
    if (h == 0) h = 1;

    glViewport(0, 0, w, h);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();

    gluPerspective(45, (float) w / (float) h, .1, 200);

    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
}

void idle ( ) {
  glutPostRedisplay();
}

int main(int argc, char** argv) {
	glutInit(&argc, argv);
	glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB | GLUT_DEPTH);
	glutInitWindowSize(800, 400);

	glutCreateWindow("Season Change");

	init();

	_terrain = loadTerrain("map5.bmp", 20);

	glutDisplayFunc(drawScene);
	glutKeyboardFunc(my_keyboard);
	glutMouseFunc(my_mouse);
	glutReshapeFunc(handleResize);
	glutTimerFunc(2500, update, 0);

	glutReshapeFunc(reshape);
    glutIdleFunc(idle);

	glutMainLoop();
	return 0;
}









