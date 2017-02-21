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

#include <QtWidgets>
#include <QtOpenGL>

#include "glwidget.h"

const GLfloat GLWidget::haiWidth = 1.9f;
const GLfloat GLWidget::haiHeight = 2.55f;
const GLfloat GLWidget::haiDepth = 1.5f;
const GLfloat GLWidget::lookAtY = -(1.9f) * 1.0f;
const GLfloat GLWidget::buttonWidth = 13.5f;
const GLfloat GLWidget::buttonHeight = 4.5f;

GLWidget::GLWidget(QWidget *parent, QGLWidget *shareWidget)
    : QGLWidget(parent, shareWidget)
{
    clearColor = Qt::white;

    phi = 270.0f;
    theta = 40.0f;
    rad = 165.0f;
    fovy = 15.0f;

    setMouseTracking(true);
    haiHoverInd[0] = haiHoverInd[1] = haiHoverInd[2] = haiHoverInd[3] = -1;
    actionButtons[0] = actionButtons[1] = actionButtons[2] = actionButtons[3] = actionButtons[4] = -1;
    actionButtonHover = -1;

#ifdef QT_OPENGL_ES_2
    program = 0;
#endif
}

GLWidget::~GLWidget()
{
}

void GLWidget::setClearColor(const QColor &color)
{
    clearColor = color;
    updateGL();
}

void GLWidget::setGameInfo(const GameInfo* gameInfo)
{
    pGameInfo = gameInfo;
}

void GLWidget::setPlayer(const Player* player)
{
    pPlayer = player;
}

void GLWidget::setPlayerNames(QString player0, QString player1, QString player2, QString player3)
{
    playerNames[0] = player0;
    playerNames[1] = player1;
    playerNames[2] = player2;
    playerNames[3] = player3;
}

void GLWidget::updateHaiHover(int* _haiHoverInd)
{
    haiHoverInd[0] = _haiHoverInd[0];
    haiHoverInd[1] = _haiHoverInd[1];
    haiHoverInd[2] = _haiHoverInd[2];
    haiHoverInd[3] = _haiHoverInd[3];

    updateHaiPos();
}

void GLWidget::updateButtonHover(int* buttonTypes, int hoverInd)
{
    for (int i = 0; i < IND_ACTIONBUTTONSIZE; i++) {
        switch(buttonTypes[i])
        {
        case ACTION_TSUMOHO:
            actionButtons[i] = BUTTON_TSUMO;
            break;
        case ACTION_KYUSHUKYUHAI:
            actionButtons[i] = BUTTON_KYUSHUKYUHAI;
            break;
        case ACTION_ANKAN:
        case ACTION_KAKAN:
        case ACTION_DAIMINKAN:
            actionButtons[i] = BUTTON_KAN;
            break;
        case ACTION_RICHI:
            actionButtons[i] = BUTTON_RICHI;
            break;
        case ACTION_RONHO:
            actionButtons[i] = BUTTON_RON;
            break;
        case ACTION_PON:
            actionButtons[i] = BUTTON_PON;
            break;
        case ACTION_CHI:
            actionButtons[i] = BUTTON_CHI;
            break;
        case ACTION_PASS:
            actionButtons[i] = BUTTON_PASS;
            break;
        default:
            actionButtons[i] = -1;
            break;
        }
    }

    actionButtonHover = hoverInd;

    updateGL();
}

void GLWidget::updateHaiPos()
{
    const GLfloat yamaStartX = -haiWidth * 8.0f; // bottom left corner
    const GLfloat yamaStartY = -haiWidth * 9.5f;
    const GLfloat kawaStartX = -haiWidth * 2.5f; // upper left corner
    const GLfloat kawaStartY = -haiWidth * 4.0f;
    const GLfloat teStartX = -haiWidth * 9.0f; // left corner (auto center)
    const GLfloat teStartY = -haiWidth * 12.5f;
    const GLfloat nakiStartX = haiWidth * 9.0f; // lower right corner
    const GLfloat nakiStartY = -haiWidth * 12.5f + (haiHeight - haiDepth) * 0.5f;
    const GLfloat hoverShift = 0.333333f;

    GLfloat cY = teStartY;
    GLfloat cZ = haiHeight / 2;
#define DEG2RAD(X) ((X) * 3.141592653589793 / 180.0)
    GLfloat eY = -rad * sin(DEG2RAD(theta));
    GLfloat eZ = rad * cos(DEG2RAD(theta));
#undef DEG2RAD

    thetaTeToEye = atan((eZ - cZ) / (cY - eY)); // radian

    int nYama = pGameInfo->nYama;
    int nKan = pGameInfo->nKan;
    int sai = pGameInfo->sai;
    int myDir = pPlayer->myDir;
    int nTotalHai = 0;


    int startDir = (4 + sai - 1 - myDir) & 0x03;
    int startInd = 17 - sai;

    // yama
    for (int i = 0; i < nYama; i++) {
        int j = i - 3 * ((i == 1) && (nYama != 136));

        if (((i == 1) && (nKan >= 1)) || \
            ((i == 0) && (nKan >= 2)) || \
            ((i == 3) && (nKan >= 3)) || \
            ((i == 2) && (nKan >= 4))) {
            //[1,0,3,2]: rinshanpai (in that order)
            // (i < 4) && (pGameInfo->wanpai[i] == HAI_EMPTY)) {
            haiOrient[nTotalHai++][0] = HAI_SKIP;
            continue;
        }

        haiOrient[nTotalHai][0] = HAI_HAKU;
        haiOrient[nTotalHai][1] = ORIENT_FLAT_FLIP;
        haiOrient[nTotalHai][2] = (startDir + (34 - (sai << 1) + j) / 34) & 0x03;

        haiPos[nTotalHai][0] = yamaStartX + haiWidth * ((startInd + j / 2) % 17);
        haiPos[nTotalHai][1] = yamaStartY;
        haiPos[nTotalHai][2] = haiDepth * (j & 0x01);
        nTotalHai++;
    }

    // dora hyouji
    for (int i = 0; i < 5; i++) {
        if(pGameInfo->omotedora[i] == HAI_EMPTY)
            break;
        haiOrient[(i << 1) + 5][0] = pGameInfo->dorahyouji[i];
        haiOrient[(i << 1) + 5][1] = ORIENT_FLAT_UP;
    }

    if (nYama != 136) {
        for (int i = 0; i < 4; i++) {
            const int* kawa = pGameInfo->kawa + IND_KAWASIZE * i;
            int nKawa = kawa[IND_NKAWA];
            int relDir = (4 + i - myDir) & 0x03;

            // kawa
            int richiShift;
            for (int j = 0; j < nKawa; j++) {
                if(j % 6 == 0)
                    richiShift = 0;

                haiOrient[nTotalHai][0] = kawa[j];
                if(kawa[j] & FLAG_HAI_RICHIHAI) {
                    haiOrient[nTotalHai][1] = ORIENT_FLAT_SIDE;
                    richiShift = 1;
                }
                else
                    haiOrient[nTotalHai][1] = ORIENT_FLAT_UP;
                haiOrient[nTotalHai][2] = relDir;

                haiPos[nTotalHai][0] = kawaStartX + haiWidth * (j % 6) + richiShift * (haiHeight - haiWidth) * 0.5f * (2 - ((kawa[j] & FLAG_HAI_RICHIHAI) != 0));
                haiPos[nTotalHai][1] = kawaStartY - haiHeight * (j / 6);
                haiPos[nTotalHai][2] = 0;
                nTotalHai++;
            }

            // te
            int nNaki;
            for (nNaki = 0; nNaki < 4; nNaki++)
                if(pGameInfo->naki[(i << 4) + (nNaki << 2)] == NAKI_NO_NAKI)
                    break;
            int nHai = 13 - 3 * nNaki;

            GLfloat moveFrac = 0.75f;

            for (int j = 0; j <= nHai; j++) {
                int originalJ = j;
                if (j == nHai) {
                    if (pGameInfo->isTsumohaiAvail[i] && ((pGameInfo->prevNaki == NAKI_CHI) || (pGameInfo->prevNaki == NAKI_PON)))
                        j = IND_TSUMOHAI;
                    else
                        break;
                }

                int hover = (j == haiHoverInd[0]) || (j == haiHoverInd[1]) || (j == haiHoverInd[2]) || (j == haiHoverInd[3]);

                if (i == myDir) {
                    haiOrient[nTotalHai][0] = pPlayer->te[j];
                    haiOrient[nTotalHai][1] = ORIENT_STAND_TILTED;
                }
                else {
                    haiOrient[nTotalHai][0] = HAI_HAKU;
                    haiOrient[nTotalHai][1] = ORIENT_STAND_UP;
                }

                haiOrient[nTotalHai][2] = relDir;

                //haiPos[nTotalHai][0] = -haiWidth * 0.5f * (nHai - 1) + haiWidth * j;
                haiPos[nTotalHai][0] = teStartX + haiWidth * originalJ;
                haiPos[nTotalHai][1] = teStartY + ((i == myDir) && hover) * hoverShift * haiHeight / tan(thetaTeToEye);
                haiPos[nTotalHai][2] = ((i == myDir) && hover) * hoverShift * haiHeight;

                if (i == myDir) {
                    haiPos[nTotalHai][1] = moveFrac * haiPos[nTotalHai][1] + (1.0f - moveFrac) * eY;
                    haiPos[nTotalHai][2] = moveFrac * (haiPos[nTotalHai][2] - haiHeight / 4) + (1.0f - moveFrac) * eZ;
                }

                nTotalHai++;
            }

            // put tsumohai apart
            if (pGameInfo->isTsumohaiAvail[i] && (pGameInfo->prevNaki != NAKI_CHI) && (pGameInfo->prevNaki != NAKI_PON)) {

                int hover = (IND_TSUMOHAI == haiHoverInd[0]) || (IND_TSUMOHAI == haiHoverInd[1]) || (IND_TSUMOHAI == haiHoverInd[2]) || (IND_TSUMOHAI == haiHoverInd[3]);

                if (i == myDir) {
                    haiOrient[nTotalHai][0] = pPlayer->te[IND_TSUMOHAI];
                    haiOrient[nTotalHai][1] = ORIENT_STAND_TILTED;
                }
                else {
                    haiOrient[nTotalHai][0] = HAI_HAKU;
                    haiOrient[nTotalHai][1] = ORIENT_STAND_UP;
                }

                haiOrient[nTotalHai][2] = relDir;

                //haiPos[nTotalHai][0] = -haiWidth * 0.5f * (nHai - 1) + haiWidth * (nHai - 3);
                haiPos[nTotalHai][0] = teStartX + haiWidth * (nHai + 0.5f);
                haiPos[nTotalHai][1] = teStartY + ((i == myDir) && hover) * hoverShift * haiHeight / tan(thetaTeToEye);
                haiPos[nTotalHai][2] = ((i == myDir) && hover) * hoverShift * haiHeight;

                if (i == myDir) {
                    haiPos[nTotalHai][1] = moveFrac * haiPos[nTotalHai][1] + (1.0f - moveFrac) * eY;
                    haiPos[nTotalHai][2] = moveFrac * (haiPos[nTotalHai][2] - haiHeight / 4) + (1.0f - moveFrac) * eZ;
                }

                nTotalHai++;
            }

            int nNakiHai = 0;
            int nSideHai = 0;
            for (int j = 0; j < nNaki; j++) {
                int nakiType = pGameInfo->naki[(i << 4) + (j << 2)];
                const int* pTe = pGameInfo->naki + (i << 4) + (j << 2) + 1;

                int p[3];
                p[0] = pTe[0];
                p[1] = pTe[1];
                p[2] = pTe[2];

                int sideN = 0;
                int added = 0;
                for (int k = 2; k >= 0; k--) {
                    haiOrient[nTotalHai][0] = p[k];

                    if (p[k] & FLAG_HAI_NOT_TSUMOHAI) {
                        haiOrient[nTotalHai][1] = ORIENT_FLAT_SIDE;
                        sideN = nTotalHai;
                        nSideHai++;
                    }
                    else
                        haiOrient[nTotalHai][1] = ORIENT_FLAT_UP;

                    haiOrient[nTotalHai][2] = relDir;

                    haiPos[nTotalHai][0] = nakiStartX - nNakiHai * haiWidth - (haiHeight - haiWidth) * (nSideHai - 0.5f * ((p[k] & FLAG_HAI_NOT_TSUMOHAI) > 0));
                    haiPos[nTotalHai][1] = nakiStartY - (haiHeight - haiWidth) * 0.5f * ((p[k] & FLAG_HAI_NOT_TSUMOHAI) > 0);
                    haiPos[nTotalHai][2] = 0;
                    nTotalHai++;
                    nNakiHai++;

                    if ((k == 2) && (!added) && ((nakiType == NAKI_ANKAN) || (nakiType == NAKI_DAIMINKAN))) {
                        p[k++] &= MASK_HAI_NUMBER;
                        added = 1;
                    }
                }

                // ankan
                if(nakiType == NAKI_ANKAN)
                    haiOrient[nTotalHai - 1][1] = haiOrient[nTotalHai - 4][1] = ORIENT_FLAT_FLIP;

                // kakan
                if(nakiType == NAKI_KAKAN) {

                    haiOrient[nTotalHai][0] = p[0] & MASK_HAI_NUMBER;
                    //if ((pGameInfo->ruleFlags & RULE_USE_AKADORA) && (haiOrient[nTotalHai][0] <= HAI_5S) && (haiOrient[nTotalHai][0] % 9 == 4) && ((p[0] & FLAG_HAI_AKADORA) == 0) && ((p[1] & FLAG_HAI_AKADORA) == 0) && ((p[2] & FLAG_HAI_AKADORA) == 0))
                    //    haiOrient[nTotalHai][0] |= FLAG_HAI_AKADORA;

                    haiOrient[nTotalHai][1] = ORIENT_FLAT_SIDE;
                    haiOrient[nTotalHai][2] = relDir;

                    haiPos[nTotalHai][0] = haiPos[sideN][0];
                    haiPos[nTotalHai][1] = haiPos[sideN][1] + haiWidth;
                    haiPos[nTotalHai][2] = 0;
                    nTotalHai++;
                }
            }
        }
    }

    haiOrient[nTotalHai][0] = HAI_EMPTY;

    updateGL();
}


void GLWidget::initializeGL()
{
    makeObject();

    // lighting
    // http://www.glprogramming.com/red/chapter05.html
    glShadeModel (GL_SMOOTH);
    glEnable(GL_LIGHTING);
    glEnable(GL_LIGHT0);
    glEnable(GL_LIGHT1);


    glEnable (GL_BLEND);
    glBlendFunc (GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    // aa
    //glEnable (GL_BLEND);
    //glEnable (GL_POLYGON_SMOOTH);
    //glBlendFunc (GL_SRC_ALPHA_SATURATE, GL_ONE);

    glEnable(GL_DEPTH_TEST);

    glEnable(GL_CULL_FACE);
#ifndef QT_OPENGL_ES_2
    glEnable(GL_TEXTURE_2D);
#endif

#ifdef QT_OPENGL_ES_2

#define PROGRAM_VERTEX_ATTRIBUTE 0
#define PROGRAM_TEXCOORD_ATTRIBUTE 1

    QGLShader *vshader = new QGLShader(QGLShader::Vertex, this);
    const char *vsrc =
        "attribute highp vec4 vertex;\n"
        "attribute mediump vec4 texCoord;\n"
        "varying mediump vec4 texc;\n"
        "uniform mediump mat4 matrix;\n"
        "void main(void)\n"
        "{\n"
        "    gl_Position = matrix * vertex;\n"
        "    texc = texCoord;\n"
        "}\n";
    vshader->compileSourceCode(vsrc);

    QGLShader *fshader = new QGLShader(QGLShader::Fragment, this);
    const char *fsrc =
        "uniform sampler2D texture;\n"
        "varying mediump vec4 texc;\n"
        "void main(void)\n"
        "{\n"
        "    gl_FragColor = texture2D(texture, texc.st);\n"
        "}\n";
    fshader->compileSourceCode(fsrc);

    program = new QGLShaderProgram(this);
    program->addShader(vshader);
    program->addShader(fshader);
    program->bindAttributeLocation("vertex", PROGRAM_VERTEX_ATTRIBUTE);
    program->bindAttributeLocation("texCoord", PROGRAM_TEXCOORD_ATTRIBUTE);
    program->link();

    program->bind();
    program->setUniformValue("texture", 0);

#endif
}

void GLWidget::paintGL()
{
    static GLfloat mat_specular[] = { 1.0, 1.0, 1.0, 1.0 };
    static GLfloat mat_diffuse[] = { 0.8, 0.8, 0.8, 1.0 };
    static GLfloat mat_diffuse_transparent[] = { 0.8, 0.8, 0.8, 0.7 };
    static GLfloat mat_diffuse_red[] = { 1.0, 0.7, 0.7, 1.0 };
    static GLfloat mat_shininess[] = { 128.0 };
    static GLfloat light_position0[] = { 0.0, 0.0, 15.0, 1.0 };
    static GLfloat light_ambient[] = { 0.7, 0.7, 0.7, 1.0 };
    static GLfloat light_diffuse[] = { 1.0, 1.0, 1.0, 1.0 };
    float yinc = 75.0f / tan(thetaTeToEye);
    GLfloat light_position1[] = { -5.0f, -106.1f + yinc, 126.4f - 75.0f, 1.0f };

    glLightfv(GL_LIGHT0, GL_AMBIENT,  light_ambient);
    glLightfv(GL_LIGHT1, GL_DIFFUSE,  light_diffuse);

    glLightf(GL_LIGHT0, GL_LINEAR_ATTENUATION, 1.0 / 50.0);
    glLightf(GL_LIGHT1, GL_QUADRATIC_ATTENUATION, 1.0 / 900.0);

    qglClearColor(clearColor);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glEnableClientState(GL_VERTEX_ARRAY);
    glEnableClientState(GL_NORMAL_ARRAY);
    glEnableClientState(GL_TEXTURE_COORD_ARRAY);

    glLoadIdentity();

#define DEG2RAD(X) ((X) * 3.141592653589793 / 180.0)
    gluLookAt(rad * sin(DEG2RAD(theta)) * cos(DEG2RAD(phi)), rad * sin(DEG2RAD(theta)) * sin(DEG2RAD(phi)), rad * cos(DEG2RAD(theta)),
              0.0, lookAtY, 0.0,  0.0, 0.0, 1.0);
#undef DEG2RAD

    glLightfv(GL_LIGHT0, GL_POSITION, light_position0);
    glLightfv(GL_LIGHT1, GL_POSITION, light_position1);

    // hai
    glVertexPointer(3, GL_FLOAT, 0, vertices[OBJ_HAI].constData());
    glNormalPointer(GL_FLOAT, 0, normals[OBJ_HAI].constData());
    glTexCoordPointer(2, GL_FLOAT, 0, texCoords[OBJ_HAI].constData());

    for(int i = 0; i < IND_HAIPOSSIZE; i++) {
        int hai = (haiOrient[i][0] & MASK_HAI_NUMBER);
        if(hai == HAI_EMPTY)
            break;
        else if(hai == HAI_SKIP)
            continue;
        else if(haiOrient[i][0] & FLAG_HAI_AKADORA)
            hai += 30 - ((hai / 9) << 3);

        glPushMatrix();

        glRotatef(90.0f * haiOrient[i][2], 0.0f, 0.0f, 1.0f);

        glTranslatef(haiPos[i][0], haiPos[i][1], haiPos[i][2]);

        switch(haiOrient[i][1])
        {
        case ORIENT_FLAT_UP:
            glTranslatef(0.0f, 0.0f, haiDepth / 2);
            glRotatef(180.0f, 1.0f, 0.0f, 0.0f);
            break;
        case ORIENT_FLAT_SIDE:
            glTranslatef(0.0f, 0.0f, haiDepth / 2);
            glRotatef(90.0f, 0.0f, 0.0f, 1.0f);
            glRotatef(180.0f, 1.0f, 0.0f, 0.0f);
            break;
        case ORIENT_FLAT_FLIP:
            glTranslatef(0.0f, 0.0f, haiDepth / 2);
            break;
        case ORIENT_STAND_UP:
            glTranslatef(0.0f, 0.0f, haiHeight / 2);
            glRotatef(-90.0f, 1.0f, 0.0f, 0.0f);
            break;
        case ORIENT_STAND_SIDE:
            glTranslatef(0.0f, 0.0f, haiWidth / 2);
            glRotatef(-90.0f, 0.0f, 1.0f, 0.0f);
            glRotatef(-90.0f, 1.0f, 0.0f, 0.0f);
            break;
        case ORIENT_STAND_TILTED:
            glTranslatef(0.0f, 0.0f, haiHeight / 2);
            glRotatef(-thetaTeToEye * 180.0 / 3.141592653589793, 1.0f, 0.0f, 0.0f);
            glRotatef(-90.0f, 1.0f, 0.0f, 0.0f);
        }

        if ((haiOrient[i][0] & FLAG_HAI_LAST_DAHAI) && !(haiOrient[i][0] & FLAG_HAI_TSUMOGIRI))
            glMaterialfv(GL_FRONT, GL_AMBIENT_AND_DIFFUSE, mat_diffuse_red);
        else if ((haiOrient[i][0] & FLAG_HAI_TRANSPARENT) ||
                 ((haiOrient[i][0] & FLAG_HAI_LAST_DAHAI) && (haiOrient[i][0] & FLAG_HAI_TSUMOGIRI)))
            glMaterialfv(GL_FRONT, GL_AMBIENT_AND_DIFFUSE, mat_diffuse_transparent);
        else
            glMaterialfv(GL_FRONT, GL_AMBIENT_AND_DIFFUSE, mat_diffuse);
        glMaterialfv(GL_FRONT, GL_SPECULAR, mat_specular);
        glMaterialfv(GL_FRONT, GL_SHININESS, mat_shininess);

        // front face
        glBindTexture(GL_TEXTURE_2D, textures[hai]);
        //glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glDrawArrays(GL_TRIANGLE_FAN, 0 * 4, 4);

        // front edges and corners
        for (int j = 9; j <= 16; ++j) {
            glBindTexture(GL_TEXTURE_2D, textures[HAI_HAKU]);
            glDrawArrays(GL_TRIANGLE_FAN, j * 4, 4 - (j >= 13));
        }

        // side face and edges
        for (int j = 1; j <= 8; ++j) {
            glBindTexture(GL_TEXTURE_2D, textures[HAI_SIDE]);
            //glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_MIRRORED_REPEAT);
            glDrawArrays(GL_TRIANGLE_FAN, j * 4, 4);
        }

        // back face, edges, and corners
        for (int j = 17; j <= 25; ++j) {
            glBindTexture(GL_TEXTURE_2D, textures[HAI_BACKSIDE]);
            glDrawArrays(GL_TRIANGLE_FAN, j * 4, 4 - (j >= 22));
        }

        glPopMatrix();
    }


    // status board (drawn in perspective projection)
    statusPixmap.load(QString(":/images/board/status_board.png"));

    QPainter painter;
    painter.begin(&statusPixmap);

    painter.setRenderHint(QPainter::Antialiasing);
    QString text;
    QRect fontBoundingRect;

    QString kaze[4];
    kaze[0] = tr("東");
    kaze[1] = tr("南");
    kaze[2] = tr("西");
    kaze[3] = tr("北");

    int kyokuY = 135;

    QFont font = QFont(FONT_CHINESE);

    font.setPixelSize(36);
    painter.setFont(font);
    text = QString("%1%2%3").arg(kaze[pGameInfo->chanfonpai - HAI_TON]).arg(pGameInfo->kyoku + pGameInfo->kyokuOffsetForDisplay+ 1).arg(tr("局"));
    fontBoundingRect = QFontMetrics(font).boundingRect(text);
    painter.drawText(QPoint(150, kyokuY) - QPoint(fontBoundingRect.center()), text);

    painter.save();
    painter.translate(120, kyokuY + 40);
    painter.rotate(90);
    painter.drawPixmap(QPoint(-15, -4), QPixmap(QString(":/images/tenbou/B100_mini.png")).scaledToWidth(30, Qt::SmoothTransformation));
    painter.restore();

    painter.save();
    painter.translate(170, kyokuY + 40);
    painter.rotate(90);
    painter.drawPixmap(QPoint(-15, -4), QPixmap(QString(":/images/tenbou/B1000_mini.png")).scaledToWidth(30, Qt::SmoothTransformation));
    painter.restore();

    font.setFamily(FONT_JAPANESE);
    font.setPixelSize(24);
    painter.setFont(font);
    text = QString("%1").arg(pGameInfo->nTsumibou, -2);
    painter.drawText(QPoint(130, kyokuY + 48), text);
    text = QString("%1").arg(pGameInfo->nRichibou, -2);
    painter.drawText(QPoint(180, kyokuY + 48), text);

    for (int i = 0; i < 4; i++) {

        text = kaze[(pPlayer->myDir + i) & 0x03];
        font.setFamily(FONT_CHINESE);
        font.setPixelSize(56);
        painter.setFont(font);
        painter.drawText(QPoint(77,260), text);

        font.setFamily(FONT_JAPANESE);
        font.setPixelSize(28);
        painter.setFont(font);
        text = tr("%1").arg(pGameInfo->ten[(pPlayer->myDir + pGameInfo->kyoku + pGameInfo->kyokuOffsetForDisplay + i) & 0x03]);
        painter.drawText(QPoint(135,238), text);

        font.setFamily(FONT_JAPANESE);
        font.setPixelSize(26);
        painter.setFont(font);
        text = playerNames[(pPlayer->myDir + pGameInfo->kyoku + pGameInfo->kyokuOffsetForDisplay + i) & 0x03];
        painter.drawText(QPoint(135,265), text);

        if (pGameInfo->richiSuccess[(pPlayer->myDir + i) & 0x03])
            painter.drawPixmap(QPoint(62, 279), QPixmap(QString(":/images/tenbou/B1000.png")).scaledToWidth(175, Qt::SmoothTransformation));

        painter.translate(150, 150);
        painter.rotate(-90);
        painter.translate(-150, -150);
    }
    painter.end();

    deleteTexture(textures[TEXTURE_STATUSBOARD]);

    QImage img = statusPixmap.toImage();
    glGenTextures(1, &textures[TEXTURE_STATUSBOARD]);
    glBindTexture(GL_TEXTURE_2D, textures[TEXTURE_STATUSBOARD]);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, img.width(), img.height(), 0, GL_BGRA, GL_UNSIGNED_BYTE, img.bits());

    glVertexPointer(3, GL_FLOAT, 0, vertices[OBJ_BUTTON].constData());
    glTexCoordPointer(2, GL_FLOAT, 0, texCoords[OBJ_BUTTON].constData());
    glNormalPointer(GL_FLOAT, 0, normals[OBJ_BUTTON].constData());

    glPushMatrix();

    glTranslatef(0.0f, haiWidth / 4, 0.0f);
    glScalef(6 * haiWidth / buttonWidth, 6 * haiWidth / buttonHeight, 1.0f);

    glDrawArrays(GL_TRIANGLE_FAN, 0, 4);

    glPopMatrix();


    // button & other flat projection displays

    if ((actionButtons[0] != -1) || (pGameInfo->bDrawBoard) || (pPlayer->te[IND_NAKISTATE] & (FLAG_TE_FURITEN | FLAG_TE_AGARIHOUKI))) {

        glMatrixMode(GL_PROJECTION);
        glPushMatrix();
        glLoadIdentity();
        glOrtho(0.0, 100.0, 0.0, (100.0 * height()) / width(), -10.0, 10.0);

        glMatrixMode(GL_MODELVIEW);

        glVertexPointer(3, GL_FLOAT, 0, vertices[OBJ_BUTTON].constData());
        glTexCoordPointer(2, GL_FLOAT, 0, texCoords[OBJ_BUTTON].constData());
        glNormalPointer(GL_FLOAT, 0, normals[OBJ_BUTTON].constData());

        for (int i = 0; i < IND_ACTIONBUTTONSIZE; i++) {

            if (actionButtons[i] == -1)
                break;

            glPushMatrix();
            glLoadIdentity();

            GLfloat startX = 12.0f;
            GLfloat startY = 14.0f;

            glTranslatef(startX + buttonWidth * (4 - i), startY, 0.0f);

            if (i != actionButtonHover)
                glDisable(GL_LIGHTING);

            glBindTexture(GL_TEXTURE_2D, textures[actionButtons[i]]);
            //glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
            glDrawArrays(GL_TRIANGLE_FAN, 0, 4);

            if (i != actionButtonHover)
                glEnable(GL_LIGHTING);

            glPopMatrix();
        }

        if (pGameInfo->bDrawBoard) {
            deleteTexture(textures[TEXTURE_SUMMARYBOARD]);

            glGenTextures(1, &textures[TEXTURE_SUMMARYBOARD]);
            glBindTexture(GL_TEXTURE_2D, textures[TEXTURE_SUMMARYBOARD]);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, pGameInfo->board.width(), pGameInfo->boardHeight, 0, GL_BGRA, GL_UNSIGNED_BYTE, pGameInfo->board.bits());

            glPushMatrix();
            glLoadIdentity();
            glDisable(GL_LIGHTING);

            glTranslatef(50.0f, (50.0f * height()) / width(), 0.0f);
            glScalef(100.0f / 960.0f * pGameInfo->board.width() / buttonWidth, 100.0f / 960.0f * pGameInfo->boardHeight / buttonHeight, 1.0f);

            //glBindTexture(GL_TEXTURE_2D, textures[TEXTURE_SUMMARYBOARD]);
            glDrawArrays(GL_TRIANGLE_FAN, 0, 4);

            glEnable(GL_LIGHTING);
            glPopMatrix();
        }

        if (pPlayer->te[IND_NAKISTATE] & (FLAG_TE_FURITEN | FLAG_TE_AGARIHOUKI)) {
            bottomBoard.load(":/images/board/bottom_board.png");

            painter.begin(&bottomBoard);
            painter.setPen(QColor(255, 255, 255, 128));

            font.setFamily(FONT_JAPANESE);
            font.setPixelSize(20);
            painter.setFont(font);

            if (pPlayer->te[IND_NAKISTATE] & FLAG_TE_AGARIHOUKI)
                text = tr("アガリ放棄");
            else if (pPlayer->te[IND_NAKISTATE] & FLAG_TE_FURITEN)
                text = tr("フリテン");

            painter.drawText(QPoint(bottomBoard.width() >> 1, bottomBoard.height() >> 1) - QPoint(QFontMetrics(font).boundingRect(text).center()), text);
            painter.end();

            deleteTexture(textures[TEXTURE_BOTTOMBOARD]);

            glGenTextures(1, &textures[TEXTURE_BOTTOMBOARD]);
            glBindTexture(GL_TEXTURE_2D, textures[TEXTURE_BOTTOMBOARD]);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, bottomBoard.width(), bottomBoard.height(), 0, GL_BGRA, GL_UNSIGNED_BYTE, bottomBoard.bits());

            glPushMatrix();
            glLoadIdentity();
            glDisable(GL_LIGHTING);

            glTranslatef(50.0f, 50.0f * bottomBoard.height() / width(), 0.0f);
            glScalef(100.0f / 960.0f * bottomBoard.width() / buttonWidth, 100.0f / 960.0f * bottomBoard.height() / buttonHeight, 1.0f);

            glDrawArrays(GL_TRIANGLE_FAN, 0, 4);

            glEnable(GL_LIGHTING);
            glPopMatrix();
        }

        glMatrixMode(GL_PROJECTION);
        glPopMatrix();
        glMatrixMode(GL_MODELVIEW);

    }

    glDisableClientState(GL_TEXTURE_COORD_ARRAY);
    glDisableClientState(GL_NORMAL_ARRAY);
    glDisableClientState(GL_VERTEX_ARRAY);

}

void GLWidget::resizeGL(int width, int height)
{
    GLdouble ar = (GLdouble) width / height;
    glViewport(0, 0, width, height);

#if !defined(QT_OPENGL_ES_2)
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
#ifndef QT_OPENGL_ES
    // glOrtho(-0.5, +0.5, +0.5, -0.5, 4.0, 15.0);
    // glOrtho(-30.0 * ar, +30.0 * ar, +30.0, -30.0, -10.0, 10.0);
    gluPerspective(fovy , ar, 64.0, 256.0);
#else
    glOrthof(-0.5, +0.5, +0.5, -0.5, 4.0, 15.0);
#endif
    glMatrixMode(GL_MODELVIEW);

#endif

}

void GLWidget::mouseDoubleClickEvent(QMouseEvent *event)
{
    float x = static_cast<float>(event->x()) / static_cast<float>(this->width());
    float y = static_cast<float>(event->y()) / static_cast<float>(this->height());

    emit doubleClicked(x, y);
    QGLWidget::mouseDoubleClickEvent(event);
    return;
}

void GLWidget::mousePressEvent(QMouseEvent *event)
{
    lastPos = event->pos();
}

void GLWidget::mouseMoveEvent(QMouseEvent *event)
{
    float x = static_cast<float>(event->x()) / static_cast<float>(this->width());
    float y = static_cast<float>(event->y()) / static_cast<float>(this->height());

    emit mouseMoved(x, y);
    QGLWidget::mouseMoveEvent(event);
    return;

    /*
    int dx = -(event->x() - lastPos.x());
    int dy = -(event->y() - lastPos.y());


    if (event->buttons() & Qt::LeftButton) {
        phi += dx / 10.0f;
        if(phi > 360.0f)
            phi = 0.0f;
        if(phi < 0.0f)
            phi = 360.0f;

        theta += dy / 10.0f;
        if(theta > 180.0f)
            theta = 180.0f;
        if(theta < 0.0f)
            theta = 0.0f;
        updateGL();
    } else if (event->buttons() & Qt::RightButton) {
        rad -= dy / 10.0f;
        updateGL();
    } else if (event->buttons() & Qt::MiddleButton) {
        fovy += dy / 10.0f;

        if (fovy < 5.0f)
            fovy = 5.0f;

        glMatrixMode(GL_PROJECTION);
        glLoadIdentity();
        gluPerspective(fovy , 4.0f/3.0f, 1.0, 256.0);
        glMatrixMode(GL_MODELVIEW);

        updateGL();
    }
    lastPos = event->pos();

//    static char str[256];
//    sprintf(str, "theta %f deg<br>rad %f units<br>fovy %f deg<br>", theta, rad, fovy);
//    emit viewChanged(QString(str));
    */
}

void GLWidget::mouseReleaseEvent(QMouseEvent * event)
{
    float x = static_cast<float>(event->x()) / static_cast<float>(this->width());
    float y = static_cast<float>(event->y()) / static_cast<float>(this->height());

    emit clicked(x, y);
    QGLWidget::mouseReleaseEvent(event);
    return;
}

int GLWidget::myBindTexture(const char* fileName)
{
   GLuint id;

   QImage img = QPixmap(fileName).toImage();
   glGenTextures(1, &id);
   glBindTexture(GL_TEXTURE_2D, id);
   glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
   glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_MIRRORED_REPEAT);
   glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
   glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
   glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, img.width(), img.height(), 0, GL_BGRA, GL_UNSIGNED_BYTE, img.bits());

   return id;
}

void GLWidget::makeObject()
{
    static const double corner = 0.2;
    static const double eX = 1.0 - corner / haiWidth;
    static const double eY = 1.0 - corner / haiHeight;
    static const double eZ = 1.0 - corner / haiDepth;

    textures[HAI_1M] = myBindTexture(":/images/hai/m1.png");
    textures[HAI_2M] = myBindTexture(":/images/hai/m2.png");
    textures[HAI_3M] = myBindTexture(":/images/hai/m3.png");
    textures[HAI_4M] = myBindTexture(":/images/hai/m4.png");
    textures[HAI_5M] = myBindTexture(":/images/hai/m5.png");
    textures[HAI_6M] = myBindTexture(":/images/hai/m6.png");
    textures[HAI_7M] = myBindTexture(":/images/hai/m7.png");
    textures[HAI_8M] = myBindTexture(":/images/hai/m8.png");
    textures[HAI_9M] = myBindTexture(":/images/hai/m9.png");
    textures[HAI_A5M] = myBindTexture(":/images/hai/m5a.png");

    textures[HAI_1P] = myBindTexture(":/images/hai/p1.png");
    textures[HAI_2P] = myBindTexture(":/images/hai/p2.png");
    textures[HAI_3P] = myBindTexture(":/images/hai/p3.png");
    textures[HAI_4P] = myBindTexture(":/images/hai/p4.png");
    textures[HAI_5P] = myBindTexture(":/images/hai/p5.png");
    textures[HAI_6P] = myBindTexture(":/images/hai/p6.png");
    textures[HAI_7P] = myBindTexture(":/images/hai/p7.png");
    textures[HAI_8P] = myBindTexture(":/images/hai/p8.png");
    textures[HAI_9P] = myBindTexture(":/images/hai/p9.png");
    textures[HAI_A5P] = myBindTexture(":/images/hai/p5a.png");

    textures[HAI_1S] = myBindTexture(":/images/hai/s1.png");
    textures[HAI_2S] = myBindTexture(":/images/hai/s2.png");
    textures[HAI_3S] = myBindTexture(":/images/hai/s3.png");
    textures[HAI_4S] = myBindTexture(":/images/hai/s4.png");
    textures[HAI_5S] = myBindTexture(":/images/hai/s5.png");
    textures[HAI_6S] = myBindTexture(":/images/hai/s6.png");
    textures[HAI_7S] = myBindTexture(":/images/hai/s7.png");
    textures[HAI_8S] = myBindTexture(":/images/hai/s8.png");
    textures[HAI_9S] = myBindTexture(":/images/hai/s9.png");
    textures[HAI_A5S] = myBindTexture(":/images/hai/s5a.png");

    textures[HAI_TON] = myBindTexture(":/images/hai/z1.png");
    textures[HAI_NAN] = myBindTexture(":/images/hai/z2.png");
    textures[HAI_SHA] = myBindTexture(":/images/hai/z3.png");
    textures[HAI_PE] = myBindTexture(":/images/hai/z4.png");
    textures[HAI_HAKU] = myBindTexture(":/images/hai/z5.png");
    textures[HAI_HATSU] = myBindTexture(":/images/hai/z6.png");
    textures[HAI_CHUN] = myBindTexture(":/images/hai/z7.png");

    textures[HAI_SIDE] = myBindTexture(":/images/hai/side.png");
    textures[HAI_BACKSIDE] = myBindTexture(":/images/hai/backside.png");

    textures[TEXTURE_BOTTOMBOARD] = myBindTexture(":/images/board/bottom_board.png");
    textures[TEXTURE_SUMMARYBOARD] = myBindTexture(":/images/board/summary_board.png");
    textures[TEXTURE_STATUSBOARD] = myBindTexture(":/images/board/status_board.png");

    textures[BUTTON_TSUMO] = myBindTexture(":/images/action/tsumo.png");
    textures[BUTTON_KYUSHUKYUHAI] = myBindTexture(":/images/action/kyushukyuhai.png");
    textures[BUTTON_KAN] = myBindTexture(":/images/action/kan.png");
    textures[BUTTON_RICHI] = myBindTexture(":/images/action/richi.png");
    textures[BUTTON_RON] = myBindTexture(":/images/action/ron.png");
    textures[BUTTON_PON] = myBindTexture(":/images/action/pon.png");
    textures[BUTTON_CHI] = myBindTexture(":/images/action/chi.png");
    textures[BUTTON_PASS] = myBindTexture(":/images/action/pass.png");

    // OBJ_HAI
    static const double coords[26][4][3] = {
        // front faces (HAI_#)
        { { +eX, -eY, -1.0 }, { -eX, -eY, -1.0 }, { -eX, +eY, -1.0 }, { +eX, +eY, -1.0 } },
        // side faces (HAI_SIDE)
        { { +eX, -1.0, +eZ }, { -eX, -1.0, +eZ }, { -eX, -1.0, -eZ }, { +eX, -1.0, -eZ } },
        { { -eX, +1.0, +eZ }, { +eX, +1.0, +eZ }, { +eX, +1.0, -eZ }, { -eX, +1.0, -eZ } },
        { { -1.0, -eY, +eZ }, { -1.0, +eY, +eZ }, { -1.0, +eY, -eZ }, { -1.0, -eY, -eZ } },
        { { +1.0, +eY, +eZ }, { +1.0, -eY, +eZ }, { +1.0, -eY, -eZ }, { +1.0, +eY, -eZ } },
        // side edges (HAI_SIDE)
        { { +1.0, -eY, +eZ }, { +eX, -1.0, +eZ }, { +eX, -1.0, -eZ }, { +1.0, -eY, -eZ } },
        { { +eX, +1.0, +eZ }, { +1.0, +eY, +eZ }, { +1.0, +eY, -eZ }, { +eX, +1.0, -eZ } },
        { { -eX, -1.0, +eZ }, { -1.0, -eY, +eZ }, { -1.0, -eY, -eZ }, { -eX, -1.0, -eZ } },
        { { -1.0, +eY, +eZ }, { -eX, +1.0, +eZ }, { -eX, +1.0, -eZ }, { -1.0, +eY, -eZ } },
        // front edges (HAI_HAKU)
        { { +1.0, +eY, -eZ }, { +1.0, -eY, -eZ }, { +eX, -eY, -1.0 }, { +eX, +eY, -1.0 } },
        { { -1.0, -eY, -eZ }, { -1.0, +eY, -eZ }, { -eX, +eY, -1.0 }, { -eX, -eY, -1.0 } },
        { { +eX, -1.0, -eZ }, { -eX, -1.0, -eZ }, { -eX, -eY, -1.0 }, { +eX, -eY, -1.0 } },
        { { -eX, +1.0, -eZ }, { +eX, +1.0, -eZ }, { +eX, +eY, -1.0 }, { -eX, +eY, -1.0 } },
        // front corners (HAI_HAKU)
        { { +1.0, -eY, -eZ }, { +eX, -1.0, -eZ }, { +eX, -eY, -1.0 }, { +eX, -eY, -1.0 } },
        { { -eX, -1.0, -eZ }, { -1.0, -eY, -eZ }, { -eX, -eY, -1.0 }, { -eX, -eY, -1.0 } },
        { { -1.0, +eY, -eZ }, { -eX, +1.0, -eZ }, { -eX, +eY, -1.0 }, { -eX, +eY, -1.0 } },
        { { +eX, +1.0, -eZ }, { +1.0, +eY, -eZ }, { +eX, +eY, -1.0 }, { +eX, +eY, -1.0 } },
        // back face (HAI_BACKSIDE)
        { { -eX, -eY, +1.0 }, { +eX, -eY, +1.0 }, { +eX, +eY, +1.0 }, { -eX, +eY, +1.0 } },
        // back edges (HAI_BACKSIDE)
        { { +1.0, -eY, +eZ }, { +1.0, +eY, +eZ }, { +eX, +eY, +1.0 }, { +eX, -eY, +1.0 } },
        { { -1.0, +eY, +eZ }, { -1.0, -eY, +eZ }, { -eX, -eY, +1.0 }, { -eX, +eY, +1.0 } },
        { { -eX, -1.0, +eZ }, { +eX, -1.0, +eZ }, { +eX, -eY, +1.0 }, { -eX, -eY, +1.0 } },
        { { +eX, +1.0, +eZ }, { -eX, +1.0, +eZ }, { -eX, +eY, +1.0 }, { +eX, +eY, +1.0 } },
        // back corners (HAI_BACKSIDE)
        { { +eX, -1.0, +eZ }, { +1.0, -eY, +eZ }, { +eX, -eY, +1.0 }, { +eX, -eY, +1.0 } },
        { { -1.0, -eY, +eZ }, { -eX, -1.0, +eZ }, { -eX, -eY, +1.0 }, { -eX, -eY, +1.0 } },
        { { -eX, +1.0, +eZ }, { -1.0, +eY, +eZ }, { -eX, +eY, +1.0 }, { -eX, +eY, +1.0 } },
        { { +1.0, +eY, +eZ }, { +eX, +1.0, +eZ }, { +eX, +eY, +1.0 }, { +eX, +eY, +1.0 } }
    };

    const QVector3D* p;
    QVector3D norm;

    for (int i = 0; i < 26; ++i) {
        for (int j = 0; j < 4; ++j) {
            texCoords[OBJ_HAI].append
                (QVector2D(j == 0 || j == 3, j == 2 || j == 3));
            vertices[OBJ_HAI].append
                (QVector3D(haiWidth / 2.0 * coords[i][j][0], haiHeight / 2.0 * coords[i][j][1],
                           haiDepth / 2.0 * coords[i][j][2]));
        }
        p = vertices[OBJ_HAI].constData();
        norm = QVector3D::crossProduct(p[(i << 2) + 1] - p[(i << 2) + 0], p[(i << 2) + 3] - p[(i << 2) + 0]).normalized();
        for(int j = 0; j < 4; j++) {
            normals[OBJ_HAI].append(norm);
        }
    }


    // OBJ_BUTTON
    static const GLfloat buttonCoord[4][3] = {
        { +1.0f, +1.0f, 0.0f }, { -1.0f, +1.0f, 0.0f }, { -1.0f, -1.0f, 0.0f }, { +1.0f, -1.0f, 0.0f }
    };

    for (int j = 0; j < 4; ++j) {
        texCoords[OBJ_BUTTON].append
            (QVector2D(j == 0 || j == 3, j == 2 || j == 3));
        vertices[OBJ_BUTTON].append
            (QVector3D(buttonWidth / 2.0f * buttonCoord[j][0], buttonHeight / 2.0f * buttonCoord[j][1], 0));
    }
    p = vertices[OBJ_BUTTON].constData();
    norm = QVector3D::crossProduct(p[1] - p[0], p[3] - p[0]).normalized();
    for(int j = 0; j < 4; j++) {
        normals[OBJ_BUTTON].append(norm);
    }

}
