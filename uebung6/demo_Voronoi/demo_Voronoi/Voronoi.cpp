#include <QApplication>
#include <QMenu>
#include <QMenuBar>
#include <QStatusBar>
#include <QFileDialog>
#include <QGLWidget>
#include <QKeyEvent>
#include <QMessageBox>
#include <QHBoxLayout>

#define _USE_MATH_DEFINES
#include <cmath>
#include <fstream>
#include <sstream>
#include <iostream>
#include <limits>
#include <math.h>

#include "Voronoi.h"


CGMainWindow::CGMainWindow (QWidget* parent, Qt::WindowFlags flags)
    : QMainWindow (parent, flags) {
    resize (604, 614);

    // Create a nice frame to put around the OpenGL widget
    QFrame* f = new QFrame (this);
    f->setFrameStyle(QFrame::Sunken | QFrame::Panel);
    f->setLineWidth(2);

    // Create our OpenGL widget
    ogl = new CGView (this,f);

    // Create a menu
    QMenu *file = new QMenu("&File",this);
    file->addAction ("Load Polyhedron", this, SLOT(loadPolyhedron()), Qt::CTRL+Qt::Key_G);
    file->addAction ("Quit", this, SLOT(close()), Qt::CTRL+Qt::Key_Q);

    menuBar()->addMenu(file);

    // Put the GL widget inside the frame
    QHBoxLayout* layout = new QHBoxLayout();
    layout->addWidget(ogl);
    layout->setMargin(0);
    f->setLayout(layout);

    setCentralWidget(f);

    statusBar()->showMessage("Ready",1000);
}

CGMainWindow::~CGMainWindow () {}

CGView::CGView (CGMainWindow *mainwindow,QWidget* parent ) : QGLWidget (parent), quad(NULL) {
    main = mainwindow;

    bbox_on = true;

    /// Um Keyboard-Events durchzulassen
    setFocusPolicy(Qt::StrongFocus);
}

void CGView::initializeGL() {
    glClearColor(0.4,0.4,0.5,1.0);
    center = 0.0;
    zoom = .7;
    q_now = Quat4d(1,1,-1,-1);//Quat4d(0.0,0.0,0.0,1.0);

    glEnable(GL_CULL_FACE);
    glFrontFace(GL_CCW);
    glCullFace(GL_BACK);
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_NORMALIZE);
    glEnable(GL_COLOR_MATERIAL);
}

void CGMainWindow::loadPolyhedron() {
    QString filename = QFileDialog::getOpenFileName(this, "Load generator model ...", QString(), "OFF files (*.off)" );

    if (filename.isEmpty()) return;
    statusBar()->showMessage ("Loading model ...");
    std::ifstream file(filename.toLatin1());

    std::string s;
    file >> s;

    file >> ogl->vn >> ogl->fn >> ogl->en;
    std::cout << "model loaded"<< std::endl;
    std::cout << "number of vertices : " << ogl->vn << std::endl;
    std::cout << "number of faces    : " << ogl->fn << std::endl;
    std::cout << "number of edges    : " << ogl->en << std::endl;

    ogl->P1.resize(ogl->vn);

    for(int i=0;i<ogl->vn;i++) {
        file >> ogl->P1[i][0] >> ogl->P1[i][1] >> ogl->P1[i][2];
    }

    Vector3d sumAllVectors, centerOfMass;
    float xmax, xval, ymax, yval, zmax, zval, maxval;
    xmax=ymax=zmax=-std::numeric_limits<float>::max();

    for(unsigned int i=0;i<ogl->P1.size();i++) {
        xval=std::abs(ogl->P1[i][0]); //
        yval=std::abs(ogl->P1[i][1]);
        zval=std::abs(ogl->P1[i][2]);
        if(xval>xmax) xmax=xval;
        if(yval>ymax) ymax=yval;
        if(zval>zmax) zmax=zval;
        if(xmax>ymax) maxval=xmax;
        else maxval=ymax;
        if(zmax>maxval) maxval=zmax;
        sumAllVectors += ogl->P1[i];
    }
    float scal=0.9/maxval;
//std::cout << "scal : " << scal << std::endl;
//std::cout << "xmax : " << xmax << std::endl;
//std::cout << "ymax : " << ymax << std::endl;
//std::cout << "zmax : " << zmax << std::endl;
//std::cout << "maxval : " << maxval << std::endl;

    centerOfMass=sumAllVectors/ogl->vn;
    //translate center of mass of model to origin
    for(int i=0;i<ogl->vn;i++) {
        for(int j=0; j<3; j++){
            ogl->P1[i][j]= ogl->P1[i][j]-centerOfMass[j];
        }
        ogl->P1[i]*=scal;
    }

    file.close();

    ogl->updateGL();
    statusBar()->showMessage ("Loading generator model done." ,3000);
}


void CGView::correctSimplexOrientation(std::vector<Vector3d> & simplex){

    if(simplex.size() < 3)
        return;

    if(simplex.size() == 4) {
        Vector3d a = simplex.at(0);
        Vector3d b = simplex.at(1);
        Vector3d c = simplex.at(2);
        Vector3d d = simplex.at(3);
        Vector3d n = (b-a)%(c-a);
        if((d-a).dot(n) > 0){
            simplex.at(1) = d;
            simplex.at(3) = b;
        }
    }

    if(simplex.size() == 3) {
        Vector3d a = simplex.at(0);
        Vector3d b = simplex.at(1);
        Vector3d c = simplex.at(2);
        Vector3d n = (b-a)%(c-a);
        if((Vector3d(0,0,0)-a).dot(n) < 0){
            simplex.at(0) = b;
            simplex.at(1) = a;
        }
    }
}

void CGView::drawBoundingBox() {
    double maxX = 1;
    double maxY = 1;
    double maxZ = 1;
    double minX = -maxX;
    double minY = -maxY;
    double minZ = -maxZ;

    glDisable(GL_LIGHTING);
    glColor3f(0.0,0.0,0.0);

    glBegin(GL_LINE_LOOP);
    glVertex3d(minX,minY,minZ);
    glVertex3d(maxX,minY,minZ);
    glVertex3d(maxX,maxY,minZ);
    glVertex3d(minX,maxY,minZ);
    glEnd();
    glBegin(GL_LINE_LOOP);
    glVertex3d(minX,minY,maxZ);
    glVertex3d(maxX,minY,maxZ);
    glVertex3d(maxX,maxY,maxZ);
    glVertex3d(minX,maxY,maxZ);
    glEnd();
    glBegin(GL_LINES);
    glVertex3d(minX,minY,minZ);
    glVertex3d(minX,minY,maxZ);
    glVertex3d(maxX,minY,minZ);
    glVertex3d(maxX,minY,maxZ);
    glVertex3d(minX,maxY,minZ);
    glVertex3d(minX,maxY,maxZ);
    glVertex3d(maxX,maxY,minZ);
    glVertex3d(maxX,maxY,maxZ);
    glEnd();
}



bool CGView::voronoiSurface(std::vector<Vector3d> &Q, const Vector3d &p, const Vector3d &a, const Vector3d &b,
                            const Vector3d &c, const Vector3d &normal){
    Vector3d h1=(b-a)%normal;
    Vector3d h2=(c-b)%normal;
    Vector3d h3=(a-c)%normal;
    if(((p-a)*h1<=0) && ((p-b)*h2<=0) && ((p-c)*h3<=0) && ((p-a)*normal>0)){
        Q.push_back(a);
        Q.push_back(b);
        Q.push_back(c);
        return true;
    }
    return false;
}

bool CGView::voronoiPoint(std::vector<Vector3d> &Q, const Vector3d &p, const Vector3d &a, const Vector3d &b,
                          const Vector3d &c){
    if(((p-a)*(b-a)<=0) && ((p-a)*(c-a)<=0)){
        Q.push_back(a);
        return true;
    }
    return false;
}

bool CGView::voronoiPoint(std::vector<Vector3d> &Q, const Vector3d &p, const Vector3d &a, const Vector3d &b,
                          const Vector3d &c, const Vector3d &d){

    if(((p-a)*(b-a)<=0) && ((p-a)*(d-a)<=0) && ((p-a)*(c-a)<=0)){
        Q.push_back(a);
        return true;
    }
    return false;
}

bool CGView::voronoiEdge(std::vector<Vector3d> &Q, const Vector3d &p, const Vector3d &a, const Vector3d &b,
                         const Vector3d &normal1, const Vector3d &normal2){

    Vector3d h1=(a-b)%normal1;
    Vector3d h2=(b-a)%normal2;
    if(((p-a)*(b-a)>0) && ((p-b)*(a-b)>0) && (((p-a)*h1)<=0) && ((p-b)*h2<=0)){
        Q.push_back(a);
        Q.push_back(b);
        return true;
    }
    return false;
}

bool CGView::voronoiEdge(std::vector<Vector3d> &Q, const Vector3d &p, const Vector3d &a, const Vector3d &b){
    Vector3d h=(b-a)%n_abc;

    if((p-a)*h>=0){
        Q.push_back(a);
        Q.push_back(b);
        return true;
    }
    return false;
}

Vector3d CGView::comTriangle(const Vector3d &a, const Vector3d &b,
                             const Vector3d &c){
    Vector3d com;
    std::vector<Vector3d> Q;
    Q.push_back(a);
    Q.push_back(b);
    Q.push_back(c);
    for(int i=0; i<3; i++){
        com+=Q[i];
    }
    return com=com/3;
}

void CGView::triangleNormal(const std::vector<Vector3d> &simplex){

    if(simplex.size()==3){
        n_abc=(simplex[1]-simplex[0])%(simplex[2]-simplex[0]);
    }

    if(simplex.size()==4){
        n_abc=(simplex[1]-simplex[0])%(simplex[2]-simplex[0]);
        n_bad=(simplex[3]-simplex[0])%(simplex[1]-simplex[0]);
        n_bdc=(simplex[3]-simplex[1])%(simplex[2]-simplex[1]);
        n_dac=(simplex[0]-simplex[3])%(simplex[2]-simplex[3]);
    }
}

Vector3d CGView::support(const Vector3d &d){
    Vector3d max;
    float pd=-std::numeric_limits<float>::max();
    for(unsigned int i=0; i<P1.size(); i++){
        float pd_cur=P1[i]*d;
        if(pd_cur>pd){
            pd=pd_cur;
            max=P1[i];
        }
    }
    return max;
}


// find feature of Q closest to p and direction of feature closest to p
bool CGView::simplexSolver(const Vector3d &p,
                           std::vector<Vector3d> &Q,
                           Vector3d &dir){


    // Q.size() == 1 not interesting, as simplex in GJK will always be at least 2

    /// %%%%%%%%%%%%%%%%%%%%%%%%
    /// %%%%%%%%%%  Q=2  %%%%%%%
    /// %%%%%%%%%%%%%%%%%%%%%%%%

    if(Q.size() == 2){
        Vector3d a = Q.at(0);
        Vector3d b = Q.at(1);
        Q.clear();

        //Test if p is in V_a, V_b

        if((p-a)*(b-a)<=0){
            Q.push_back(a);
            dir=p-a;
            return false;
        }
        if((p-b)*(a-b)<=0){
            Q.push_back(b);
            dir=p-b;
            return false;
        }

        //else: p is in V_ab

        Q.push_back(a);
        Q.push_back(b);
        dir=((b-a)%(p-a))%(b-a);
        return false;
    }

    /// %%%%%%%%%%%%%%%%%%%%%%%%
    /// %%%%%%%%%%  Q=3  %%%%%%%
    /// %%%%%%%%%%%%%%%%%%%%%%%%

    if(Q.size() == 3){
        Vector3d a = Q.at(0);
        Vector3d b = Q.at(1);
        Vector3d c = Q.at(2);
        Q.clear();

        //Test if p is in V_a, V_b, V_c

        if(voronoiPoint(Q, p, a, b, c)){
            dir=p-a;
            return false;
        }
        if(voronoiPoint(Q, p, b, a, c)){
            dir=p-b;
            return false;
        }
        if(voronoiPoint(Q, p, c, a, b)){
            dir=p-c;
            return false;
        }

        //Test if p is in V_ab, V_ca, V_cb

        if(voronoiEdge(Q, p, b, c)){
            dir=((c-b)%(p-b))%(c-b);
            return false;
        }

        if(voronoiEdge(Q, p, a, b)){
            dir=((b-a)%(p-a))%(b-a);
            return false;
        }

        if(voronoiEdge(Q, p, c, a)){
            dir=((a-c)%(p-c))%(a-c);
            return false;
        }

        //else: p is in V_abc

        Q.push_back(a);
        Q.push_back(b);
        Q.push_back(c);
        dir=n_abc;
        return false;
    }

    /// %%%%%%%%%%%%%%%%%%%%%%%%
    /// %%%%%%%%%%  Q=4  %%%%%%%
    /// %%%%%%%%%%%%%%%%%%%%%%%%

    if(Q.size() == 4){
        Vector3d a = Q.at(0);
        Vector3d b = Q.at(1);
        Vector3d c = Q.at(2);
        Vector3d d = Q.at(3);
        Q.clear();

        //Test if p is in V_a, V_b, V_c, V_d

        if(voronoiPoint(Q, p, a, b, c, d)){
            dir=p-a;
            return false;
        }
        if(voronoiPoint(Q, p, b, a, c, d)){
            dir=p-b;
            return false;
        }
        if(voronoiPoint(Q, p, c, b, a, d)){
            dir=p-c;
            return false;
        }
        if(voronoiPoint(Q, p, d, b, c, a)){
            dir=p-d;
            return false;
        }

        //Test if p is in V_ab, V_bc, V_cd, V_da, V_ca, V_bd

        if(voronoiEdge(Q, p, a, b, n_abc, n_bad)){
            dir=((b-a)%(p-a))%(b-a);
            return false;
        }

        if(voronoiEdge(Q, p, b, c, n_abc, n_bdc)){
            dir=((c-b)%(p-b))%(c-b);
            return false;
        }

        if(voronoiEdge(Q, p, b, d, n_bdc, n_bad)){
            dir=((d-b)%(p-b))%(d-b);
            return false;
        }

        if(voronoiEdge(Q, p, a, d, n_bad, n_dac)){
            dir=((d-a)%(p-a))%(d-a);
            return false;
        }

        if(voronoiEdge(Q, p, a, c, n_dac, n_abc)){
            dir=((c-a)%(p-a))%(c-a);
            return false;
        }

        if(voronoiEdge(Q, p, c, d,n_dac, n_bdc)){
            dir=((d-c)%(p-c))%(d-c);
            return false;
        }

        //Test if p is in V_abc, V_bdc, V_adb, V_dac

        if(voronoiSurface(Q, p, a, b, c, n_abc)){
            dir=n_abc;
            return false;
        }

        if(voronoiSurface(Q, p, b, d, c, n_bdc)){
            dir=n_bdc;
            return false;
        }

        if(voronoiSurface(Q, p, a, d, b, n_bad)){
            dir=n_bad;
            return false;
        }

        if(voronoiSurface(Q, p, d, a, c, n_dac)){
            dir=n_dac;
            return false;
        }

        //else: p is in V_abcd

        Q.push_back(a);
        Q.push_back(b);
        Q.push_back(c);
        Q.push_back(d);
        return true;
    }
    return false;
}

bool CGView::GJK(){

    Vector3d dir=P1[0];
    Vector3d v=support(dir);

    std::vector<Vector3d> Q;
    Q.push_back(v);
    dir=v*(-1);
    Vector3d p=Vector3d(0.0,0.0,0.0);

    while(true){
        v=support(dir);
        Q.push_back(v);
        correctSimplexOrientation(Q);
        triangleNormal(Q);
        if(v*dir<0){
            lastSimplex=Q;
            return false; //no intersection
        }
        if(simplexSolver(p,Q,dir)){
            lastSimplex=Q;
            return true; //intersection
        }
    }
}


void CGView::paintGL() {
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    glTranslated(0.0,0.0,-3.0);

    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    Matrix4d R(q_now);
    Matrix4d RT = R.transpose();
    glMultMatrixd(RT.ptr());

    glScaled(zoom,zoom,zoom);
    if (bbox_on) drawBoundingBox();

    GLUquadricObj *quadric;
    glColor3d(1,1,0);
    quadric = gluNewQuadric();
    gluSphere( quadric , 0.02 , 10 , 10);
    glPushMatrix();


    //Draw off-model
    glDisable (GL_CULL_FACE);
    if(!P1.empty()){
        Vector3d color=Vector3d(0.0,1.0,0.0);
        if(GJK()) {
            color=Vector3d(1.0,0.0,0.0);
        }
        for(unsigned int i=0;i<P1.size();i++) {

            glColor3d(color[0], color[1], color[2]);
            glPushMatrix();
            glTranslated(P1[i][0],P1[i][1],P1[i][2]);

            gluQuadricDrawStyle(quadric, GLU_FILL);

            gluSphere( quadric , .005 , 10 , 10);
            glPopMatrix();
        }
    }


    glColor3d(0,0,1);
    glBegin(GL_LINES);
    for(unsigned int i = 0; i < lastSimplex.size(); i++){
        for(unsigned int j = i+1; j < lastSimplex.size(); j++){
            Vector3d a(lastSimplex[i][0], lastSimplex[i][1], lastSimplex[i][2]);
            Vector3d b(lastSimplex[j][0], lastSimplex[j][1], lastSimplex[j][2]);
            glVertex3dv(a.ptr());
            glVertex3dv(b.ptr());
        }
    }
    glEnd();
}

void CGView::resizeGL(int width, int height) {
    glViewport(0,0,width,height);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    if (width > height) {
        double ratio = width/(double) height;
        gluPerspective(45,ratio,1.0,100.0);
        //glOrtho(-1,1,-1,1,-1,1);
    }
    else {
        double ratio = height/(double) width;
        gluPerspective(45,1.0/ratio,1.0,100.0);
        //glOrtho(-1,1,-1,1,-1,1);
    }
    glMatrixMode (GL_MODELVIEW);
}

void CGView::worldCoord(int x, int y, int z, Vector3d &v) {
    GLint viewport[4];
    glGetIntegerv(GL_VIEWPORT,viewport);
    GLdouble M[16], P[16];
    glGetDoublev(GL_PROJECTION_MATRIX,P);
    glGetDoublev(GL_MODELVIEW_MATRIX,M);
    gluUnProject(x,viewport[3]-1-y,z,M,P,viewport,&v[0],&v[1],&v[2]);
}

void CGView::mousePressEvent(QMouseEvent *event) {
    oldX = event->x();
    oldY = event->y();
}

void CGView::mouseReleaseEvent(QMouseEvent*) {}

void CGView::wheelEvent(QWheelEvent* event) {
    if (event->delta() < 0) zoom *= 1.2; else zoom *= 1/1.2;
    updateGL();
}

void CGView::mouseToTrackball(int x, int y, int W, int H, Vector3d &v) {
    if (W > H) {
        v[0] = (2.0*x-W)/H;
        v[1] = 1.0-y*2.0/H;
    } else {
        v[0] = (2.0*x-W)/W;
        v[1] = (H-2.0*y)/W;
    }
    double d = v[0]*v[0]+v[1]*v[1];
    if (d > 1.0) {
        v[2] = 0.0;
        v /= sqrt(d);
    } else v[2] = sqrt(1.0-d*d);
}

Quat4d CGView::trackball(const Vector3d& u, const Vector3d& v) {
    Vector3d uxv = u % v;
    Quat4d ret(uxv[0],uxv[1],uxv[2],1.0+u*v);
    ret.normalize();
    return ret;
}

void CGView::mouseMoveEvent(QMouseEvent* event) {
    Vector3d p1,p2;

    mouseToTrackball(oldX,oldY,width(),height(),p1);
    mouseToTrackball(event->x(),event->y(),width(),height(),p2);

    Quat4d q = trackball(p1,p2);
    q_now = q * q_now;
    q_now.normalize();

    oldX = event->x();
    oldY = event->y();

    updateGL();
}

void CGView::keyPressEvent( QKeyEvent * event) 
{
    switch (event->key()) {
    case Qt::Key_Q :
        for(unsigned int i=0; i<P1.size(); i++){
            P1[i][0]-=0.1;
        }
        for(unsigned int i=0; i<lastSimplex.size(); i++){
            lastSimplex[i][0]-=0.1;
        }
        break;
    case Qt::Key_W :
        for(unsigned int i=0; i<P1.size(); i++){
            P1[i][0]+=0.1;;
        }
        for(unsigned int i=0; i<lastSimplex.size(); i++){
            lastSimplex[i][0]+=0.1;
        }
        break;
    case Qt::Key_E :
        for(unsigned int i=0; i<P1.size(); i++){
            P1[i][1]-=0.1;
        }
        for(unsigned int i=0; i<lastSimplex.size(); i++){
            lastSimplex[i][1]-=0.1;
        }
        break;
    case Qt::Key_R :
        for(unsigned int i=0; i<P1.size(); i++){
            P1[i][1]+=0.1;;
        }
        for(unsigned int i=0; i<lastSimplex.size(); i++){
            lastSimplex[i][1]+=0.1;
        }
        break;
    case Qt::Key_D :
        for(unsigned int i=0; i<P1.size(); i++){
            P1[i][2]-=0.1;
        }
        for(unsigned int i=0; i<lastSimplex.size(); i++){
            lastSimplex[i][2]-=0.1;
        }
        break;
    case Qt::Key_F :
        for(unsigned int i=0; i<P1.size(); i++){
            P1[i][2]+=0.1;
        }
        for(unsigned int i=0; i<lastSimplex.size(); i++){
            lastSimplex[i][2]+=0.1;
        }
        break;
    }
    updateGL();
}

int main (int argc, char **argv) {
    QApplication app(argc, argv);

    if (!QGLFormat::hasOpenGL()) {
        qWarning ("This system has no OpenGL support. Exiting.");
        return 1;
    }

    CGMainWindow *w = new CGMainWindow(NULL);

    w->show();

    return app.exec();
}
