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

#ifndef WINDOW_H
#define WINDOW_H

#include <QWidget>
#include <QtMultimedia/QSoundEffect>

#include "textdisp.h"
#include "textedit.h"
#include "gamethread.h"
#include "player.h"
#include "haibutton.h"
#include "tcpserver.h"
#include "tcpclient.h"

class GLWidget;

class Window : public QWidget
{
    Q_OBJECT
    QThread clientThread;

public:
    Window();

    QSize sizeHint() const;

protected:
    void resizeEvent (QResizeEvent *event);
    void moveEvent (QMoveEvent *event);
    void paintEvent (QPaintEvent *event);

public slots:
    void playSound(int soundEnum);

private slots:
    void controlKyoku();
    void serveHanchan();
    void finishHanchan();
    void updateColorName(int index);
    void updateIsAi(bool isAi);
    void processBeginClientCommand();
    void processEndClientCommand();
    void relayEndHanchanBoard();

    void askAiLibFileName();

private:
    GameThread gameThread;
    GameInfo gameInfo;
    Player player[4];
    HaiButton haiButton;

    void alignDialogs();
    TcpServer *tcpServer;
    TcpClient *tcpClient;
    QDialog dialogSC;
    int netState;

    QString colorName;
    int dispPlayer;
    int playerToClient[4]; // socket# = clientOrder[player#]

    GLWidget *glWidget;
    TextDisp* textDisp;
    TextEdit* textEdit;

    QPushButton *buttonServer, *buttonClient;
    QCheckBox *checkAi;
    QComboBox *colorComboBox;

    void initializeColorComboBox();

    QSoundEffect sound[IND_SOUNDFXSIZE];

    QStringList colorList;

    QString uuid;

    QPushButton* aiLibButtons[4];
    QString aiLibFileNames[4];
    bool validateAiLibButton(int buttonInd);
};

#endif
