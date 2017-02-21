/*
   Copyright 2014 Hosang Yoon

   Licensed under the Apache License, Version 2.0 (the "License");
   you may not use this file except in compliance with the License.
   You may obtain a copy of the License at

       http://www.apache.org/licenses/LICENSE-2.0

   Unless required by applicable law or agreed to in writing, software
   distributed under the License is distributed on an "AS IS" BASIS,
   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
   See the License for the specific language governing permissions and
   limitations under the License.
*/

#ifndef GLWIDGET_H
#define GLWIDGET_H

#include <QtWidgets>
#include <QGLWidget>

#ifdef Q_OS_WIN
    #include <GL/glu.h>
#endif
#ifdef Q_OS_OSX
    #include <OpenGL/glu.h>
#endif

#include "constants.h"
#include "player.h"
#include "gameinfo.h"

QT_FORWARD_DECLARE_CLASS(QGLShaderProgram);

class GameInfo;
class Player;

class GLWidget : public QGLWidget
{
    Q_OBJECT

public:
    explicit GLWidget(QWidget *parent = 0, QGLWidget *shareWidget = 0);
    ~GLWidget();

    void setClearColor(const QColor &color);
    void setGameInfo(const GameInfo* gameInfo);
    void setPlayer(const Player* player);
    void setPlayerNames(QString player0, QString player1, QString player2, QString player3); // player #

public slots:
    void updateHaiPos();
    void updateHaiHover(int* haiHoverInd);
    void updateButtonHover(int* buttonTypes, int hoverInd);

signals:
    void clicked(float x, float y);
    void doubleClicked(float x, float y);
    void mouseMoved(float x, float y);
//    void viewChanged(QString str);

protected:
    void initializeGL();
    void paintGL();
    void resizeGL(int width, int height);
    void mousePressEvent(QMouseEvent *event);
    void mouseMoveEvent(QMouseEvent *event);
    void mouseReleaseEvent(QMouseEvent *event);
    void mouseDoubleClickEvent(QMouseEvent *event);

private:
    void makeObject();

    int haiHoverInd[4];
    int actionButtons[IND_ACTIONBUTTONSIZE];
    int actionButtonHover;

    QString playerNames[4]; // player #

    QColor clearColor;
    QPoint lastPos;

    GLfloat phi;
    GLfloat theta, thetaTeToEye;
    GLfloat rad;
    GLfloat fovy;

    QPixmap statusPixmap;
    QImage bottomBoard;

    int myBindTexture(const char* fileName);
    GLuint textures[HAI_EMPTY];
    QVector<QVector3D> vertices[IND_OBJSIZE];
    QVector<QVector3D> normals[IND_OBJSIZE];
    QVector<QVector2D> texCoords[IND_OBJSIZE];

    const Player* pPlayer;
    const GameInfo* pGameInfo;

    int haiOrient[IND_HAIPOSSIZE][3]; // haiID, orient, relDir
    GLfloat haiPos[IND_HAIPOSSIZE][3]; // x, y, z

    static const GLfloat haiWidth;
    static const GLfloat haiHeight;
    static const GLfloat haiDepth;
    static const GLfloat lookAtY;
    static const GLfloat buttonWidth;
    static const GLfloat buttonHeight;

#ifdef QT_OPENGL_ES_2
    QGLShaderProgram *program;
#endif
};

#endif
