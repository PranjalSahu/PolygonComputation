/*
 * PRANJAL SAHU
 * 09CS1036
 * Graphics Assignment 2
 * 
 * NOTE: I have used plain P3 format of .ppm images
 * Two sample images are also attached in the submission
 * 
 * 
 * 
 * */

#include <GL/gl.h>
#include <GL/glu.h>
#include <GL/glut.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <iostream>
#include <queue>
#include <list>
#include <math.h>

int intersectflag = 0;			// if 1 then take two polygons
int unionflag     = 0;			// if 1 then take two polygons

int godisp = 1;
// for right click menu
void createmymenu(void);
void menu(int value);
void disp(void);
 
static int winmenu;
static int menuid;
static int val = 0;

//flags to be used by keyboard for three different operations
int count     = 0;
int entertext = 0;
int drawline  = 0;
int cropimage = 0;
int drawpoly  = -1;		// get points of polygon
int donepoly  = 0;		// stop taking points for polygon
int polytype  = 0;		// 1 means y polygon and 2 means star polygon

char etext[100];

class mypoint{           
public:
   int x;
   int y;
   mypoint(int a, int b);
   mypoint(mypoint *t);
   mypoint();
};
mypoint starmean  = new mypoint(0, 0);		// star polygon mean coordinate
											// also for convex hull

mypoint::mypoint(int a,int b){
   x = a;
   y = b;
}
mypoint::mypoint(mypoint *t){
	x = t->x;
	y = t->y;
}
mypoint::mypoint(){
	x = 0;
	y = 0;
}

struct sortbyxasc {
    bool operator()(const mypoint* o1, const mypoint* o2) const {
        return o1->x < o2->x;
    }
};
struct sortbyxdes {
    bool operator()(const mypoint* o1, const mypoint* o2) const {
        return o2->x < o1->x;
    }
};
struct sortbyyasc {
    bool operator()(const mypoint* o1, const mypoint* o2) const {
        return o1->y < o2->y;
    }
};
struct sortbyydes {
    bool operator()(const mypoint* o1, const mypoint* o2) const {
        return o2->y < o1->y;
    }
};
struct sortbyangle {
    bool operator()(const mypoint* o1, const mypoint* o2) const{
		return atan2(o1->y-starmean.y, o1->x-starmean.x) < atan2(o2->y-starmean.y, o2->x-starmean.x);
	}
};

std::list<mypoint*> poly1;			// for storing the points of polygon
std::list<mypoint*> poly2;
std::list<mypoint*> convexpoly;		// for storing the points of convex polygon
std::list<mypoint*> maskpoly;		// final list of points which are within the polygon


int minpx = 100000;					// for polygon bounding box
int minpy = 100000;
int maxpx = -1;
int maxpy = -1;	
	
std::queue<mypoint*> myq;
std::queue<mypoint*> myq1;

int win1;
int win2 = -1;

#define MAXSIZE 1000
int mymatrix[MAXSIZE][MAXSIZE];
unsigned int w, h, d;
FILE *fp;

int px[10], py[10];
int click = 0, drawme = 0, dragging  =0;
unsigned char *image2;
unsigned char myimage[MAXSIZE][MAXSIZE][3];
unsigned char original[MAXSIZE][MAXSIZE][3];
unsigned char formaskpoly[MAXSIZE][MAXSIZE][3];			// for storing the inside pixels of a polygon

int newwidth, newheight;

int polysize[2];						// for storing the size of the polygon


mypoint points[2][MAXSIZE];


mypoint innerpoint;					// innerpoint to be used while polygon intersection

std::queue<int> p1;					// for intersection points x coordinate
std::list<int> scanq[1000];			// one queue for each scan line

// function prototypes
void mylinefun(int x1, int y1,int x2,int y2, int flag);	 

void myclear(){	
   for(int i=0;i<1000;++i)
		scanq[i].clear();	
   return;
}

void display3(){
	int nw = maxpx-minpx;
	int nh = maxpy-minpy;	
	int i, j, k;
	int mymask[nw+1][nh+1];
	
	for(i=0;i<=nw;++i)
		for(j=0;j<=nh;++j)
			mymask[i][j] = 0;
			
	int size = (int)maskpoly.size();	
	int ycount=0;
	for(i=0;i<size;++i){
		mymask[maskpoly.front()->x-minpx][maskpoly.front()->y-minpy] = 1;		//set the flag		
		maskpoly.pop_front();
		++ycount;
	}
	
	unsigned char tempimage[nw+1][nh+1][3];	
	memcpy(tempimage, image2, (nw+1)*(nh+1)*3);
	int count =0 ;
	for(i=-5+nw/2;i<=5+nw/2;++i){
		for(j=0;j<=nh;++j){
			//if(mymask[i][j] != 1){
				tempimage[i][j][0] 	= (unsigned char)0;
				tempimage[i][j][1] 	= (unsigned char)255;
				tempimage[i][j][2]  = (unsigned char)0;
				++count;
			//}
		}
	}
	glPixelStorei(GL_PACK_ALIGNMENT, 1);
	printf("ycount = %d count = %d and rest = %d\n", ycount, count, (nw+1)*(nh+1));
	glDrawPixels(nw, nh, GL_RGB, GL_UNSIGNED_BYTE, (GLvoid*)tempimage);
	glutSwapBuffers();
}

bool isleft(mypoint *a, mypoint *b, mypoint *c){
     return ((b->x - a->x)*(c->y - a->y) - (b->y - a->y)*(c->x - a->x)) > 0;
}

void drawpoint(int x, int y, int color){
	y = h-y;	
	glPointSize(10);
	if(color == 1)
		glColor3f(0, 1, 0);
	else
		glColor3f(0, 0, 1);
	if(intersectflag)
		glColor3f(0, 0, 1);
		
	if(color == 3){
		glColor3f(1, 0, 0);
		printf("DRAWING THE INNER POINT\n");
	}
		
	glBegin(GL_POINTS);       
      glVertex2f(-1+(x*2)/(w+0.0),-1+(2*y)/(h+0.0));
    glEnd();
    glutSwapBuffers();
}

void mytempfun(){
	int nw = maxpx-minpx;
	int nh = maxpy-minpy;		
	
	if(win2 != -1)
	  glutDestroyWindow(win2);	
	
	image2 = (unsigned char *)malloc(sizeof(unsigned char)*nh*nw*3);
	glReadPixels(minpx, w-(h-maxpy)-nh, nw, nh, GL_RGB, GL_UNSIGNED_BYTE, image2);	
		
	glutInitWindowSize(nw+1, nh+1);
    glutCreateWindow("Polygon 1");
	glDrawPixels(nw, nh, GL_RGB, GL_UNSIGNED_BYTE, (GLvoid*)image2);
	glutSwapBuffers();	
}

void showcopymasked(){
	if(win2 != -1)
		glutDestroyWindow(win2);
	
	int nw = maxpx-minpx;
	int nh = maxpy-minpy;
	
	unsigned char tempimage[w][h][3];	
	unsigned char tempimage1[w][h][3];	
	memset(tempimage1, 155, w*h*3);
	
	glReadPixels(0, 0, w, h, GL_RGB, GL_UNSIGNED_BYTE, tempimage);
	
	glutInitWindowSize(w+1, h+1);
    win2 = glutCreateWindow("Cropped Polygon");
    int size = (int)maskpoly.size();	
    
    while(!maskpoly.empty()){		
		tempimage1[maskpoly.front()->y][maskpoly.front()->x][0] = original[maskpoly.front()->y][maskpoly.front()->x][0];
		tempimage1[maskpoly.front()->y][maskpoly.front()->x][1] = original[maskpoly.front()->y][maskpoly.front()->x][1];
		tempimage1[maskpoly.front()->y][maskpoly.front()->x][2] = original[maskpoly.front()->y][maskpoly.front()->x][2];
		maskpoly.pop_front();		
	}	
	
    glDrawPixels(w, h, GL_RGB, GL_UNSIGNED_BYTE, (GLvoid*)tempimage1);
	glutSwapBuffers();	
}

void showcopymasked2(){	
	if(win2 != -1)
		glutDestroyWindow(win2);	
	int nw = maxpx-minpx;
	int nh = maxpy-minpy;
	
	unsigned char tempimage[w][h][3];	
	unsigned char tempimage1[w][h][3];	
	memset(tempimage1, 155, w*h*3);	
	glutInitWindowSize(w+1, h+1);
    win2 = glutCreateWindow("Cropped Polygon");
    int size = (int)maskpoly.size();
    
    for(int i=0;i<w;++i){
		for(int j=0;j<h;++j){
			if(intersectflag){
				if(mymatrix[i][j] == 2){
					tempimage1[j][i][0] = original[j][i][0];
					tempimage1[j][i][1] = original[j][i][1];
					tempimage1[j][i][2] = original[j][i][2];		
				}
			}
			else{
				if(mymatrix[i][j] == 1){
					tempimage1[j][i][0] = original[j][i][0];
					tempimage1[j][i][1] = original[j][i][1];
					tempimage1[j][i][2] = original[j][i][2];		
				}
			}
		}
	}	
	
    glDrawPixels(w, h, GL_RGB, GL_UNSIGNED_BYTE, (GLvoid*)tempimage1);
	glutSwapBuffers();	
	intersectflag = 0;
	unionflag = 0;
	printf("IN copy masked 2\n");
}

void getminmaxforboundingbox(int flag){
	int i;
	minpx = 100000;
	minpy = 100000;
	maxpx = -1;
	maxpy = -1;
	for(i=0;i<polysize[flag];++i){			// getting bounding box;
			if(points[flag][i].x < minpx)
				minpx = points[flag][i].x;
			if(points[flag][i].y < minpy)
				minpy = points[flag][i].y;
			if(points[flag][i].x > maxpx)
				maxpx = points[flag][i].x;
			if(points[flag][i].y > maxpy)
				maxpy = points[flag][i].y;
	}
}

int findinvertexset(int x, int y, int flag){
	for(int i=0;i<polysize[flag];++i){
		if(points[flag][i].x == x && points[flag][i].y == y){
			if(i==0){
				if((points[flag][0].y-points[flag][1].y)*(points[flag][0].y-points[flag][polysize[flag]-1].y) < 0)		// oppsite side
					return 0;
				else
					return 1;	
			}
			else{
				if((points[flag][i].y-points[flag][i+1].y)*(points[flag][i].y-points[flag][i-1].y) < 0)		// oppsite side
					return 0;
				else
					return 1;	
			}
		}
			
	}	
	return 0;
}

void croppolygon1(int flag){
	getminmaxforboundingbox(flag);						
	int bw = maxpx-minpx+1;				// box width
	int bh = maxpy-minpy+1;				// box height	
	int xcoor;
	myclear();							// clear the intersection points
	memset(mymatrix, 0, MAXSIZE*MAXSIZE);
	for(int y=minpy;y<=maxpy;++y){
		for(int i=0;i<polysize[flag];++i){
			if(points[flag][i].y-y == 0){
				scanq[y].push_back(points[flag][i].x);
				if(findinvertexset(points[flag][i].x, y, flag))
						scanq[y].push_back(points[flag][i].x);
			}
			if((points[flag][i].y-y)*(points[flag][i+1].y-y) < 0){		// in between the two segments
				xcoor = (int)(points[flag][i].x+((points[flag][i+1].x-points[flag][i].x+0.0)*(y-points[flag][i].y+0.0)/(points[flag][i+1].y-points[flag][i].y+0.0)));
				scanq[y].push_back(xcoor);
				if(findinvertexset(xcoor, y, flag))
					scanq[y].push_back(xcoor);
			}
		}
		if(scanq[y].size()%2 == 0 && scanq[y].size()!=0){			
			scanq[y].sort();
			while(!scanq[y].empty()){
				int temp1, temp2;
				temp1 = scanq[y].front();				
				scanq[y].pop_front();
				temp2 = scanq[y].front();
				scanq[y].pop_front();				
				for(int p=temp1;p<temp2;++p){
					maskpoly.push_back(new mypoint(p, y));
					mymatrix[p][y] = 1;
				}
			}
		}	
	}	
	showcopymasked();
	return;
}

void croppolygon2int(int flag){
	getminmaxforboundingbox(flag);
	int bw = maxpx-minpx+1;				// box width
	int bh = maxpy-minpy+1;				// box height	
	int xcoor;
	myclear();							// clear the intersection points
	for(int y=minpy;y<=maxpy;++y){
		for(int i=0;i<polysize[flag];++i){
			if(points[flag][i].y-y == 0){
				scanq[y].push_back(points[flag][i].x);
				if(findinvertexset(points[flag][i].x, y, flag))
						scanq[y].push_back(points[flag][i].x);
			}
			if((points[flag][i].y-y)*(points[flag][i+1].y-y) < 0){		// in between the two segments
				xcoor = (int)(points[flag][i].x+((points[flag][i+1].x-points[flag][i].x+0.0)*(y-points[flag][i].y+0.0)/(points[flag][i+1].y-points[flag][i].y+0.0)));
				scanq[y].push_back(xcoor);
				if(findinvertexset(xcoor, y, flag))
					scanq[y].push_back(xcoor);
			}
		}
		if(scanq[y].size()%2 == 0 && scanq[y].size()!=0){			
			scanq[y].sort();
			while(!scanq[y].empty()){
				int temp1, temp2;
				temp1 = scanq[y].front();				
				scanq[y].pop_front();
				temp2 = scanq[y].front();
				scanq[y].pop_front();				
				for(int p=temp1;p<temp2;++p){
					maskpoly.push_back(new mypoint(p, y));					
					++mymatrix[p][y];
				}
			}
		}	
	}	
	showcopymasked2();
	return;
}

void croppolygon2union(int flag){
	getminmaxforboundingbox(flag);
	int bw = maxpx-minpx+1;				// box width
	int bh = maxpy-minpy+1;				// box height	
	int xcoor;
	myclear();							// clear the intersection points
	for(int y=minpy;y<=maxpy;++y){
		for(int i=0;i<polysize[flag];++i){
			if(points[flag][i].y-y == 0){
				scanq[y].push_back(points[flag][i].x);
				if(findinvertexset(points[flag][i].x, y, flag))
						scanq[y].push_back(points[flag][i].x);
			}
			if((points[flag][i].y-y)*(points[flag][i+1].y-y) < 0){		// in between the two segments
				xcoor = (int)(points[flag][i].x+((points[flag][i+1].x-points[flag][i].x+0.0)*(y-points[flag][i].y+0.0)/(points[flag][i+1].y-points[flag][i].y+0.0)));
				scanq[y].push_back(xcoor);
				if(findinvertexset(xcoor, y, flag))
					scanq[y].push_back(xcoor);
			}
		}
		if(scanq[y].size()%2 == 0 && scanq[y].size()!=0){			
			scanq[y].sort();
			while(!scanq[y].empty()){
				int temp1, temp2;
				temp1 = scanq[y].front();				
				scanq[y].pop_front();
				temp2 = scanq[y].front();
				scanq[y].pop_front();				
				for(int p=temp1;p<temp2;++p){
					maskpoly.push_back(new mypoint(p, y));
					mymatrix[p][y] = 1;					
				}
			}
		}	
	}	
	showcopymasked2();
	return;
}

void display1(void){
	glDrawPixels(newwidth, newheight, GL_RGB, GL_UNSIGNED_BYTE, (GLvoid*)image2);
	glutSwapBuffers();		
}

void drawrectangle(){
	newwidth  = px[1]-px[0];
	newheight = py[1]-py[0];	
	
	printf("drawing rectangle\n");	
	
	image2 = (unsigned char *)malloc(sizeof(unsigned char)*(newwidth+1)*(newheight+1)*3);
	glReadPixels(px[0], w-py[0]-newheight, newwidth, newheight, GL_RGB, GL_UNSIGNED_BYTE, image2);	
		
	glutInitWindowSize(newwidth+1, newheight+1);
    win2 = glutCreateWindow("New Image");
    glutDisplayFunc(display1); 		
	cropimage = 0;
}

void display2(){
	int size = (int)myq.size();
	glRasterPos2f(-1,-1);
    float barwidth = 2/(size+0.0);
	int i, temp;	
	float start = -1;	
	float pre   = -1;
	
	glBegin(GL_LINE_STRIP);
	for(i=0;i<size;++i){
		temp = original[myq.front()->y][myq.front()->x][0]+original[myq.front()->y][myq.front()->x][1]+original[myq.front()->y][myq.front()->x][2];
		glVertex2f(start, pre);
		glVertex2f(start+barwidth, -1+ 2*temp/768.0);
		//glRectf(start, -1, start+barwidth, -1+ 2*temp/768.0);
		start = start+barwidth;
		pre   = -1+ 2*temp/768.0;
		myq.pop();
	}		
	glEnd();
	glutSwapBuffers();
}

void showintensity(){	
	if(win2 != -1)
		glutDestroyWindow(win2);	
	glClearColor(0, 0, 0, 0);
	glutInitWindowSize(512, 256);					// make window of width = no. of pixels in the queue		
    win2 = glutCreateWindow("Intensity profile");    
    glutDisplayFunc(display2);
}

void putpixel(){
	int i, j, k;
	unsigned char tempimage[w][h][3];		
	glReadPixels(0, 0, w, h, GL_RGB, GL_UNSIGNED_BYTE, tempimage);
	int size = (int)myq1.size();
	for(i=0;i<size;++i){		
		tempimage[myq1.front()->y][myq1.front()->x][0] = (unsigned char)255;
		tempimage[myq1.front()->y][myq1.front()->x][1] = (unsigned char)0;
		tempimage[myq1.front()->y][myq1.front()->x][2] = (unsigned char)0;
		myq1.pop();
	}
	
	for(i=0;i<w;++i){
		for(j=0;j<h;++j){
			for(k=0;k<3;++k){
				myimage[i][j][k] = tempimage[i][j][k];
			}
		}
	}		
	glDrawPixels(w, h, GL_RGB, GL_UNSIGNED_BYTE, (GLvoid*)tempimage);
	glutSwapBuffers();	
	showintensity();	
	drawline = 0;
}

// flag = 1 means normal operation
// flag = 2 means only store points for intersection 
void mylinefun(int x1, int y1,int x2,int y2, int flag){	 
	 int oy1 = y1;	 
	 
	 int swap    = 0;
	 int acrossx = 0;
	 
	 int t1;
	 int xt, yt;
	 
	 if(y2 < y1){				// negative slope
		acrossx = 1;
		y2 = 2*(y1-y2)+y2;	
	 }
	 
	 if(flag == 1){
		myq.push(new mypoint(x1, y1));
		myq1.push(new mypoint(x1, y1));
	 }
	 else
		 scanq[y1].push_back(x1);			// directly access by y
	 
	 if(y2-y1>x2-x1){
		swap = 1;			// swapping the coordinates
		t1 = x1;
		x1 = y1;			// getting image across y = x line
		y1 = t1;
		t1 = x2;
		x2 = y2;
		y2 = t1;
	 }
	 
	 int i, j;
	 unsigned char image1[w][h][3]; 
     int dx  = x2 - x1;
     int dy  = y2 - y1;
     int y   = y1;
     int x   = x1;
     int p = 2*dy-dx;     
     
     int dy2 = 2*dy;
     int dy2minusdx2 = 2*(dy-dx);
     
     while(x<x2){
		 x++;
		 if(p<0)
			 p = p + dy2;
		else{
			 p = p + dy2minusdx2;
			 y = y + 1;
		}		
		if(swap){
			xt = y;			// getting the original line by swapping again
			yt = x;			// swapping across  y = x line
		}
		else{
			xt = x;
			yt = y;
		}
		
		if(acrossx){
			yt = yt -2*(yt-oy1);
		}		
		if(flag == 1){
			myq.push(new mypoint(xt, yt));
			myq1.push(new mypoint(xt, yt));
		}
		else
			scanq[yt].push_back(xt);			// directly access by y
	 }	
	 if(flag == 1)		
		putpixel();		
}

void write(int x, int y){	
	//x = px[1];y = py[1];
	float x1 = (2/(w+0.0))*x;
	y = h-y;
	float y1 = (2/(h+0.0))*y;  
	printf("writing text to screen\n");
	glColor3f(1.0f, 0.0f, 0.0f);
    glRasterPos2f(-1+x1, -1+y1);
    int len = strlen(etext);    
    for(int i = 0; i < len; i++){ 
        glutBitmapCharacter(GLUT_BITMAP_HELVETICA_18, etext[i]);
    }
    entertext = 0;        
    glutSwapBuffers();
    glRasterPos2f(-1, -1);
    count = 0;
}

bool isequal(mypoint *a, mypoint *b){
	return (a->x==b->x)&&(a->y==b->y);	
}

void copyit(mypoint *a, mypoint *b){
	a->x = b->x;
	a->y = b->y;
}

void drawconvexpolygon(){
	int i;
	int size = poly1.size();
	mypoint *temp1 = new mypoint(0, 0);				// top two elements of stack
	mypoint *temp2 = new mypoint(0, 0);
	
	for(i=0;i<size;++i){
		temp1 = poly1.front();
		temp1->y = h-temp1->y;
		poly1.pop_front();
		poly1.push_back(new mypoint(temp1));
	}	
	
	poly1.sort(sortbyyasc());							// 2
														// 1
	starmean.x     = poly1.front()->x;					//	starmean = miny
	starmean.y     = poly1.front()->y;
	poly1.pop_front();									// remove that point
	poly1.sort(sortbyangle());							// sort the remaining point	
	
	convexpoly.push_front(new mypoint(starmean));		// again put the leftmost point
	convexpoly.push_front(new mypoint(poly1.front()));	// push in the convex hull set	
	poly1.pop_front();		
	
	temp1->x = starmean.x;								// top two points of the stack
	temp1->y = starmean.y;								// top two points of the stack
	copyit(temp2,convexpoly.front());
	
	int csize;
	while(poly1.size() != 0){
		csize = (int)convexpoly.size();
		//printf("csize  = %d\n", csize);
		if(csize<2 || isleft(temp1, temp2, poly1.front())){
			copyit(temp1, convexpoly.front());
			convexpoly.push_front(new mypoint(poly1.front()));
			copyit(temp2,convexpoly.front());			
			poly1.pop_front();			
		}
		else{			
			convexpoly.pop_front();			
			copyit(temp2, convexpoly.front());			
			if(convexpoly.size()>1){
				convexpoly.pop_front();
				copyit(temp1, convexpoly.front());
				convexpoly.push_front(new mypoint(temp2));
			}			
		}
	}
	
	int tx1, ty1;
	printf("------------------------\n\n");
	size = (int)convexpoly.size();
	polysize[0] = size;								// for cropping
	glBegin(GL_LINE_LOOP);
		for(int i=0;i<size;++i){
			tx1  = convexpoly.front()->x;
			ty1  = convexpoly.front()->y;
			points[0][size-i-1].x = tx1;				// store them for cropping 
			points[0][size-i-1].y = ty1;
			glVertex2f(-1+2*tx1/(w+0.0), -1+2*ty1/(h+0.0));			
			convexpoly.pop_front();
		}			
	glEnd();
	glutSwapBuffers();
	points[0][polysize[0]].x = points[0][0].x;				// make it circular
	points[0][polysize[0]].y = points[0][0].y;
	if(intersectflag)
		croppolygon2int(0);
	else if(unionflag)
		croppolygon2union(0);
	else
		croppolygon1(0);	
	convexpoly.clear();
	myclear();
}

void drawstarpolygon(int flag){	
	poly1.sort(sortbyangle());
	int tx1, ty1;
	printf("------------------------\n\n");
	int size = (int)poly1.size();
	polysize[0] = size;	
	glBegin(GL_LINE_LOOP);
		for(int i=0;i<size;++i){
			tx1  = poly1.front()->x;
			ty1  = poly1.front()->y;
			points[flag][size-i-1].x = tx1;				// store them for cropping 
			points[flag][size-i-1].y = h-ty1;
			glVertex2f(-1+2*tx1/(w+0.0), -1+2*(h-ty1)/(h+0.0));			
			poly1.pop_front();
		}			
	glEnd();
	glutSwapBuffers();
	points[flag][polysize[0]].x = points[flag][0].x;
	points[flag][polysize[0]].y = points[flag][0].y;
	croppolygon1(0);
	myclear();
}

void drawypolygon(){
	printf("Draw the polygon\n");
	poly1.sort(sortbyyasc());			// sorting by y
	mypoint *min = new mypoint(poly1.front()->x, poly1.front()->y);		// getting the top and bottom most point
	mypoint *max = new mypoint(poly1.back()->x, poly1.back()->y);	
	printf("max (%d, %d) min (%d, %d)\n", max->x, max->y, min->x, min->y);
	int size = poly1.size();
	
	std::list<mypoint*> left;			// for storing the points of polygon on left side
	std::list<mypoint*> right;			// for storing the points of polygon on right side
	std::list<mypoint*> finalpoly;
	
	int i;
	left.push_back(new mypoint(poly1.front()->x, poly1.front()->y));				// put max and min in left one
	poly1.pop_front();		// remove first point
	
	
	for(i=0;i<size-2;++i){
		if(isleft(min, max, poly1.front()))				// which side of line
			left.push_back(new mypoint(poly1.front()->x, poly1.front()->y));
		else
			right.push_back(new mypoint(poly1.front()->x, poly1.front()->y));
		poly1.pop_front();	
	}
	left.push_back(new mypoint(poly1.front()->x, poly1.front()->y));				// put max and min in left one
	poly1.pop_front();	
	
	left.sort(sortbyyasc());		// sorting from top to bottom
	right.sort(sortbyydes());		// sorting from bottom to top
	
	printf("------------------------\n\n");
	size = (int)left.size();
	for(i=0;i<size;++i){
		//printf("(%d %d)\n", left.front()->x, left.front()->y);
		finalpoly.push_back(new mypoint(left.front()));		
		drawpoint(left.front()->x, left.front()->y, 1);
		left.pop_front();	
	}
	size = (int)right.size();
	for(i=0;i<size;++i){
		//printf("(%d %d)\n", right.front()->x, right.front()->y); 
		finalpoly.push_back(new mypoint(right.front()));		
		drawpoint(right.front()->x, right.front()->y, 2);
		right.pop_front();	
	}
	
	int tx1, ty1;
	printf("------------------------\n\n");
	size = (int)finalpoly.size();		
	polysize[0] = size;
	glBegin(GL_LINE_LOOP);
		for(i=0;i<size;++i){
			tx1  = finalpoly.front()->x;
			ty1  = finalpoly.front()->y;
			points[0][size-i-1].x = tx1;				// store them for cropping 
			points[0][size-i-1].y = h-ty1;
			glVertex2f(-1+2*tx1/(w+0.0), -1+2*(h-ty1)/(h+0.0));			
			finalpoly.pop_front();	
		}			
	glEnd();
	glutSwapBuffers();
	points[0][polysize[0]].x = points[0][0].x;
	points[0][polysize[0]].y = points[0][0].y;
	croppolygon1(0);
	myclear();
	return;
}

void mousemotion(int button, int state, int x, int y){   	   
   printf("%d %d %d %d %d %d\n", button, state, x, y, x, h-y);
   if(donepoly){	   
	   donepoly = 0;
	   drawpoly = 0;
	   if(polytype == 1)
		drawypolygon();				
	   else if(polytype == 2){
		    starmean.x = starmean.x/poly1.size();
		    starmean.y = starmean.y/poly1.size();
			drawstarpolygon(0);		
	   }
	   else{	
		   drawconvexpolygon();
	   }
	   return;
   }		
   if(state == GLUT_DOWN){	   
		px[count] = x;
		py[count] = y;
		++count;
   }
   else{
	   if(drawpoly && button == 0){
		   if(drawpoly == 1){					// taking points of polygon
				drawpoly = 2;					// ignore the point
				starmean.x=0;
				starmean.y=0;
				return;
		   }
		   printf("taking point\n");
		   if(drawpoly == 2){
				drawpoint(x, y, 1);
				poly1.push_back(new mypoint(x, y));	
				starmean.x = starmean.x+x;
				starmean.y = starmean.y+y;
		   }
		   return;
	   }		
	   if(entertext){
			count = 0;
			printf("Enter your text\n");
			gets(etext);
			write(x, y);
			return;
	   }			
	   if(count>=2){
		    count = 0;
			if(drawline){
			   int tx[2], ty[2];
			   if(px[0]<px[1]){
				   tx[0] = px[0];	ty[0] = py[0];
				   tx[1] = px[1];	ty[1] = py[1];
			   }
			   else{
				   tx[0] = px[1];	ty[0] = py[1];
				   tx[1] = px[0];	ty[1] = py[0];
			    }			   	
				mylinefun(tx[0], h-ty[0], tx[1], h-ty[1], 1);		// change it to support other slopes
			}
			else if(cropimage){
			   drawrectangle();
			}			
		}
   }
}

void display(void){		
	if(!godisp)
		return;
	int k;
	glClear (GL_COLOR_BUFFER_BIT);				
    
	printf("width = %d and height = %d\n", w, h);

    unsigned char image1[w][h][3];    
	unsigned int a;
	int i, j;
	
	for(i=0;i<w;++i){
		for(j=0;j<h;++j){
			fscanf(fp, "%u", &a);
			image1[i][j][0] = (unsigned char)a;
			myimage[i][j][0] = image1[i][j][0];			
			fscanf(fp, "%u", &a);
			image1[i][j][1] = (unsigned char)a;
			myimage[i][j][1] = image1[i][j][1];			
			fscanf(fp, "%u", &a);
			image1[i][j][2] = (unsigned char)a;
			myimage[i][j][2] = image1[i][j][2];			
		}
	}	 
	
	unsigned char temp[3];	
	
	for(i=0;i<h/2;++i){			// displaying upright image
		for(j=0;j<w;++j){
			for(k=0;k<3;++k){
				temp[k]	 		 	  = image1[i][j][k];
				image1[i][j][k]  	  = image1[h-i-1][j][k];
				image1[h-i-1][j][k]   = temp[k];
				original[h-i-1][j][k] = image1[h-i-1][j][k];
				original[i][j][k]     = image1[i][j][k];
			}
		}
	}
	
	for(i=0;i<w;++i){			// copying again upright image
		for(j=0;j<h;++j){
			myimage[i][j][0] = image1[i][j][0];
			myimage[i][j][1] = image1[i][j][1];
			myimage[i][j][2] = image1[i][j][2];
		}
	}
	
	// copying again the image with line
	/*for(i=0;i<w;++i){
		for(j=0;j<h;++j){
			image1[i][j][0] = myimage[i][j][0];
			image1[i][j][1] = myimage[i][j][1];
			image1[i][j][2] = myimage[i][j][2];
		}
	}
	*/
	glDrawPixels(w, h, GL_RGB, GL_UNSIGNED_BYTE, (GLvoid*)image1);
	glutSwapBuffers();
	godisp = 0;
}

void keyboard(unsigned char key, int x, int y){
   count = 0;
   if(key == 'r'){				// reset operation
	//glDrawPixels(w, h, GL_RGB, GL_UNSIGNED_BYTE, (GLvoid*)image1);
	//glutSwapBuffers(); 	
   }   
   else if(key == 't'){			// enter text
		entertext = 1;
		cropimage = 0;
		drawline  = 0;
   }
   else if(key == 'c'){			// crop image
	   entertext = 0;
	   cropimage = 1;
	   drawline  = 0;
   }
   else if(key == 'l'){			// draw line
	   entertext = 0;
	   cropimage = 0;
	   drawline  = 1;
   }
}

void reset(){
	maskpoly.clear();
	entertext = 0;
	cropimage = 0;
	drawline  = 0;
	drawpoly  = 0;
	donepoly  = 0;
	intersectflag = 0;
	unionflag = 0;
	memset(mymatrix, 0, MAXSIZE*MAXSIZE);
	unsigned char tempimage[w][h][3];    
	unsigned int a;
	int i, j;
	
	for(i=0;i<w;++i){
		for(j=0;j<h;++j){
			tempimage[i][j][0] = original[i][j][0];			
			tempimage[i][j][1] = original[i][j][1];
			tempimage[i][j][2] = original[i][j][2];
			myimage[i][j][0]   = original[i][j][0];
			myimage[i][j][1]   = original[i][j][1];
			myimage[i][j][2]   = original[i][j][2];
		}
	}	 
	glRasterPos2f(-1, -1);
	glDrawPixels(w, h, GL_RGB, GL_UNSIGNED_BYTE, (GLvoid*)tempimage);
	glutSwapBuffers();
}

void copypolygon(){
	for(int i=0;i<1000;++i)
		points[1][i] = points[0][i];
	polysize[1] = polysize[0];	
}

void menufun(int value){
	count = 0;
	if(value == 0){
		glutDestroyWindow(winmenu);
		exit(0);
	}
	else if(value == 1){		// draw line
	   entertext = 0;	   cropimage = 0;	   drawline  = 1;   drawpoly  = 0;
	} 	
	else if(value == 2){		// crop image
	   entertext = 0;	   cropimage = 1;	   drawline  = 0;   drawpoly  = 0;
	}
	else if(value == 3){		// Annotate point
	   entertext = 1;	   cropimage = 0;	   drawline  = 0;	drawpoly  = 0;	
	}
	else if(value == 4){		// reset
		reset();
	}
	else if(value == 6){		// star monotone
		drawpoly = 1;
		donepoly = 0;
		polytype = 2;
	}
	else if(value == 7){		// y monotone
		drawpoly = 1;
		donepoly = 0;
		polytype = 1;
	}
	else if(value == 8){		// convex polygon
		drawpoly = 1;
		donepoly = 0;
		polytype = 3;
	}
	else if(value == 9){		// done polygon	
		drawpoly = 0;
		donepoly = 1;		
	}
	else if(value == 10){		// intersect
		copypolygon();			// copy the first polygon into polygon2 and save the size also
		intersectflag = 1;
	}
	else if(value == 11){
		copypolygon();			// copy the first polygon into polygon2 and save the size also
		unionflag = 1;
	}
}

void createmymenu(void){		
	int polygonmenu = glutCreateMenu(menufun);
	glutAddMenuEntry("Closed Contour", 5);
	glutAddMenuEntry("Star polygon", 6);
	glutAddMenuEntry("Y monotone", 7);
	glutAddMenuEntry("Convex Hull", 8);
	
	menuid = glutCreateMenu(menufun);	
	glutAddMenuEntry("Draw Line", 1);
	glutAddMenuEntry("Crop Image", 2);
	glutAddMenuEntry("Annotate", 3);
	glutAddSubMenu("Polygon", polygonmenu);
	glutAddMenuEntry("Done", 9);
	glutAddMenuEntry("Intersection", 10);
	glutAddMenuEntry("Union", 11);
	glutAddMenuEntry("Reset", 4);
	glutAddMenuEntry("Exit", 0);
	glutAttachMenu(GLUT_RIGHT_BUTTON);
}

int main(int argc, char** argv){	
	if(argc != 2){
		printf("Enter the file name\n");
		exit(0);
	}	
	
	fp = fopen(argv[1],"r");		
	
	char buff[16];
	char *t;
	
	fgets(buff, sizeof(buff), fp);
	
	if(buff[0] != 'P'||buff[1] != '3') {
		printf("Not correct image file");
		exit(0);
	}	
	
	do{ 
        t = fgets(buff, 256, fp);
        if ( t == NULL ) 
			exit(0);
    }while( strncmp(buff, "#", 1) == 0 );
    
    int r = sscanf(buff, "%u %u", &w, &h);
    if ( r < 2 ) 
		exit(0);
 
    r = fscanf(fp, "%u", &d);
    if((r < 1)||( d != 255 )) 
		exit(0);
		
    fseek(fp, 1, SEEK_CUR);
    
	glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_RGBA | GLUT_DOUBLE);
    glutInitWindowSize(w, h);
    winmenu = glutCreateWindow("Draw pixels test");
	createmymenu(); 
	
    glutDisplayFunc(display);
    glutMouseFunc(mousemotion);
    glutKeyboardFunc(keyboard);
	glutMainLoop();
    return 0;	
}
