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

#include "glwidget.h"
#include "window.h"
#include "textdisp.h"
#include "textedit.h"

// for mt19937ar.c
unsigned long genrand_int32(void);

Window::Window()
{
    setWindowIcon(QIcon(":/images/icon/nama.png"));

    /// Widgets ///
    QGridLayout* mainLayout = new QGridLayout;
    QGridLayout* rightLayout = new QGridLayout;

    QColor clearColor;
    clearColor.setRgb(34, 124, 85);

    glWidget = new GLWidget(this, 0);
    glWidget->setClearColor(clearColor);
    mainLayout->addWidget(glWidget, 0, 0);

    mainLayout->addLayout(rightLayout, 0, 1);
    mainLayout->setSpacing(0);
    mainLayout->setColumnStretch(0, 12);
    mainLayout->setColumnStretch(1, 4);
    mainLayout->setContentsMargins(0, 0, 0, 0);

    setLayout(mainLayout);
    setMinimumSize(QSize(960, 540));
    resize(sizeHint());
    //this->setGeometry(100, 100, 1280, 720);
    //this->setFixedSize(1280, 720);

    textDisp = new TextDisp;
    textDisp->setMaximumBlockCount(64);

    QHBoxLayout *buttonAILayout = new QHBoxLayout;
    checkAi = new QCheckBox(tr("AI"));
    checkAi->setChecked(false);
    connect(checkAi, SIGNAL(toggled(bool)), this, SLOT(updateIsAi(bool)));

    for (int i = 0; i < 4; i++) {
        aiLibButtons[i] = new QPushButton(tr("Native AI"));
        connect (aiLibButtons[i], SIGNAL(clicked()), this, SLOT(askAiLibFileName()));
    }
    aiLibButtons[3]->setEnabled(false);

    colorComboBox = new QComboBox;
    connect(colorComboBox, SIGNAL(currentIndexChanged(int)), this, SLOT(updateColorName(int)));
    initializeColorComboBox();

    buttonAILayout->addWidget(checkAi);
    buttonAILayout->addSpacing(10);
    buttonAILayout->addWidget(aiLibButtons[3]);
    buttonAILayout->addStretch(1);
    buttonAILayout->addWidget(colorComboBox);


    textEdit = new TextEdit;

    rightLayout->addWidget(textDisp, 0, 0);
    rightLayout->addLayout(buttonAILayout, 1, 0);
    rightLayout->addWidget(textEdit, 2, 0);
    rightLayout->setRowStretch(0, 25);
    rightLayout->setRowStretch(1, 1);
    rightLayout->setRowStretch(2, 5);

    setWindowTitle(tr("Nama"));

    /*
    QTimer *timer = new QTimer(this);
    connect(timer, SIGNAL(timeout()), this, SLOT(rotateOneStep()));
    timer->start(20);
    */

    /// GameThread ///
    gameThread.pGameInfo = &gameInfo;
    glWidget->setGameInfo(&gameInfo);
    connect(&gameThread, SIGNAL(updateScreen()), glWidget, SLOT(updateHaiPos()));
    connect(&gameInfo, SIGNAL(updateScreen()), glWidget, SLOT(updateHaiPos()));

    for (int i = 0; i < 4; i++) {
        player[i].setGameInfo(&gameInfo);
        player[i].setHaiButton(&haiButton);
        gameThread.player[i] = &player[i];
        connect(&player[i], SIGNAL(updateScreen()), glWidget, SLOT(updateHaiPos()));
        connect(&player[i], SIGNAL(actionAsked(float,float)), &haiButton, SLOT(processHaiHover(float,float)));
        connect(&player[i], SIGNAL(actionAsked(float,float)), &haiButton, SLOT(processButtonHover(float,float)));
    }

    // Hanchan specific
    for (int i = 0; i < 4; i++)
    {
        player[i].setPlayerType(PLAYER_NATIVE_AI);
        player[i].setPlayerName(player[i].getPlayerName().append(QString(" %1").arg(i)));
    }
    glWidget->setPlayerNames(player[0].getPlayerName(), player[1].getPlayerName(), player[2].getPlayerName(), player[3].getPlayerName());
    gameInfo.setPlayerNames(player[0].getPlayerName(), player[1].getPlayerName(), player[2].getPlayerName(), player[3].getPlayerName());
    dispPlayer = 0;
    //player[0].setPlayerType(PLAYER_MANUAL);
    glWidget->setPlayer(&player[0]);
    gameInfo.setDispPlayer(0);


    // glWidget related buttons
    connect(glWidget, SIGNAL(doubleClicked(float,float)), &gameInfo, SLOT(clearSummaryBoard()));
    connect(glWidget, SIGNAL(doubleClicked(float,float)), this, SLOT(controlKyoku()));

    connect(glWidget, SIGNAL(mouseMoved(float,float)), &haiButton, SLOT(processHaiHover(float,float)));
    connect(&haiButton, SIGNAL(haiHoverChanged(int*)), glWidget, SLOT(updateHaiHover(int*)));
    connect(glWidget, SIGNAL(clicked(float,float)), &haiButton, SLOT(processHaiClick(float,float)));

    connect(glWidget, SIGNAL(mouseMoved(float,float)), &haiButton, SLOT(processButtonHover(float,float)));
    connect(&haiButton, SIGNAL(buttonHoverChanged(int*,int)), glWidget, SLOT(updateButtonHover(int*,int)));
    connect(glWidget, SIGNAL(clicked(float,float)), &haiButton, SLOT(processButtonClick(float,float)));

    for (int i = 0; i < 4; i++) {
        connect(&haiButton, SIGNAL(haiClicked(int)), &player[i], SLOT(receiveHaiClick(int)));
        connect(&haiButton, SIGNAL(buttonClicked(int)), &player[i], SLOT(receiveButtonClick(int)));
    }

    gameInfo.initializeHanchan();
    connect(&gameInfo, SIGNAL(hanchanFinished()), this, SLOT(finishHanchan()));


    /// Sound ///
    sound[SOUND_TSUMO].setSource(QUrl::fromLocalFile(":/sound/tsumo.wav"));
    sound[SOUND_TSUMO].setVolume(0.3f);
    sound[SOUND_AGARI].setSource(QUrl::fromLocalFile(":/sound/agari.wav"));
    sound[SOUND_AGARI].setVolume(0.5f);
    sound[SOUND_DAHAI].setSource(QUrl::fromLocalFile(":/sound/dahai.wav"));
    sound[SOUND_DAHAI].setVolume(0.5f);
    sound[SOUND_NAKI].setSource(QUrl::fromLocalFile(":/sound/naki.wav"));
    sound[SOUND_NAKI].setVolume(0.5f);
    sound[SOUND_ASK].setSource(QUrl::fromLocalFile(":/sound/ask.wav"));
    sound[SOUND_ASK].setVolume(0.5f);
    sound[SOUND_RICHI].setSource(QUrl::fromLocalFile(":/sound/richi.wav"));
    sound[SOUND_RICHI].setVolume(0.5f);
    sound[SOUND_SHUFFLE].setSource(QUrl::fromLocalFile(":/sound/shuffle.wav"));
    sound[SOUND_SHUFFLE].setVolume(0.5f);

    connect(&gameThread, SIGNAL(emitSound(int)), this, SLOT(playSound(int)));
    for (int i = 0; i < 4; i++)
        connect(&player[i], SIGNAL(emitSound(int)), this, SLOT(playSound(int)));

    /// Network ///
    tcpServer = new TcpServer(glWidget, aiLibButtons);
    tcpServer->setWindowFlags(Qt::Dialog | Qt::CustomizeWindowHint | Qt::WindowTitleHint);

    tcpServer->pGameInfo = &gameInfo;
    tcpServer->players = player;
    tcpServer->pColorName = &colorName;

    tcpClient = new TcpClient(glWidget);
    tcpClient->setWindowFlags(Qt::Dialog | Qt::CustomizeWindowHint | Qt::WindowTitleHint);

    tcpClient->pGameInfo = &gameInfo;
    tcpClient->players = player;
    tcpClient->pColorName = &colorName;
    connect(tcpClient, SIGNAL(emitSound(int)), this, SLOT(playSound(int)));

    uuid = QUuid::createUuid().toString();
    tcpServer->setUuid(uuid);
    tcpClient->setUuid(uuid);

    // ask  server/client dialog
    dialogSC.setParent(glWidget);
    dialogSC.setWindowFlags(Qt::Dialog | Qt::CustomizeWindowHint | Qt::WindowTitleHint);

    buttonServer = new QPushButton(tr("Server"));
    buttonClient = new QPushButton(tr("Client"));
    buttonServer->setAutoDefault(false);
    buttonClient->setAutoDefault(false);

    QHBoxLayout *buttonSCLayout = new QHBoxLayout;
    buttonSCLayout->addWidget(buttonServer);
    buttonSCLayout->addWidget(buttonClient);
    buttonSCLayout->setSizeConstraint(QLayout::SetFixedSize);

    dialogSC.setLayout(buttonSCLayout);
    dialogSC.setWindowTitle(QString(""));

    connect(buttonServer, SIGNAL(clicked()), &dialogSC, SLOT(hide()));
    connect(buttonServer, SIGNAL(clicked()), tcpServer, SLOT(show()));
    connect(buttonClient, SIGNAL(clicked()), &dialogSC, SLOT(hide()));
    connect(buttonClient, SIGNAL(clicked()), tcpClient, SLOT(show()));
    connect(tcpServer, SIGNAL(accepted()), this, SLOT(serveHanchan()));
    connect(tcpServer, SIGNAL(rejected()), &dialogSC, SLOT(show()));
    connect(tcpClient, SIGNAL(rejected()), &dialogSC, SLOT(show()));

    connect(tcpClient, SIGNAL(displayText(QString)), textDisp, SLOT(insertText(QString)));
    connect(tcpServer, SIGNAL(displayText(QString)), textDisp, SLOT(insertText(QString)));

    connect(textEdit, SIGNAL(textEntered(QString)), tcpServer, SLOT(receiveEnteredText(QString)));
    connect(textEdit, SIGNAL(textEntered(QString)), tcpClient, SLOT(receiveEnteredText(QString)));

    connect(&gameThread, SIGNAL(updateScreen()), tcpServer, SLOT(distributeScreenState()));
    connect(&gameInfo, SIGNAL(updateScreen()), tcpServer, SLOT(distributeScreenState()));
    for (int i = 0; i < 4; i++) {
        connect(&player[i], SIGNAL(updateScreen()), tcpServer, SLOT(distributeScreenState()));
        connect(&player[i], SIGNAL(needAnswerViaNetwork(int)), tcpServer, SLOT(askClientForAnswer(int)));
        connect(tcpServer, SIGNAL(answerRecieved(int,int)), &player[i], SLOT(receiveNetworkAnswer(int,int)));
    }

    connect(&gameInfo, SIGNAL(sendEndKyokuBoard(int,const int*,const int*,const int*,const int*,int,int,int)),
            tcpServer, SLOT(sendEndKyokuBoard(int,const int*,const int*,const int*,const int*,int,int,int)));

    connect(tcpClient, SIGNAL(receivedBeginClientCommand()), this, SLOT(processBeginClientCommand()));
    connect(tcpClient, SIGNAL(receivedScreenStatus()), glWidget, SLOT(updateHaiPos()));
    connect(tcpClient, SIGNAL(receivedEndClientCommand()), this, SLOT(processEndClientCommand()));

    connect(&gameInfo, SIGNAL(endHanchanBoardDrawn()), this, SLOT(relayEndHanchanBoard()));

    netState = NET_STATE_NONE;
    textDisp->insertText(tr("<p style=\"color:gray;\"><b>Double click screen to start...</b></p>"));


    /// AI settings ///

    QSettings settings(QSettings::UserScope, QLatin1String("Nama"));
    settings.beginGroup(QLatin1String("AI"));
    for (int i = 0; i < 4; i++) {
        QVariant val = settings.value(QString("AI%1FileName").arg(i));
        aiLibFileNames[i].clear();
        if (!val.isNull())
            aiLibFileNames[i] = val.toString();
        validateAiLibButton(i);
    }
    settings.endGroup();
}

void Window::playSound(int soundEnum)
{
    sound[soundEnum].play();

    if ((netState == NET_STATE_SERVER) && (soundEnum != SOUND_ASK))
        tcpServer->distributeSfx(soundEnum);
}

void Window::resizeEvent (QResizeEvent * event)
{
    if(width() * 9 > height() * 16)
        resize(QSize((height() * 16 + 8) / 9, height()));

    if(height() * 16 > width() * 9)
        resize(width(), (width() * 9 + 15) / 16);

    alignDialogs();

    QWidget::resizeEvent(event);
}

void Window::moveEvent (QMoveEvent *event)
{
    alignDialogs();

    QWidget::moveEvent(event);
}

void Window::paintEvent (QPaintEvent *event)
{
    alignDialogs();

    QWidget::paintEvent(event);
}

QSize Window::sizeHint() const
{
    //return QSize(1280, 720);
    return QSize(960, 540);
}

void Window::alignDialogs()
{
    dialogSC.move(this->x() + ((glWidget->width() - dialogSC.width()) >> 1),
                  this->y() + ((glWidget->height() - dialogSC.height()) >> 1));

    tcpServer->move(this->x() + ((glWidget->width() - tcpServer->width()) >> 1),
                   this->y() + ((glWidget->height() - tcpServer->height()) >> 1));

    tcpClient->move(this->x() + ((glWidget->width() - tcpClient->width()) >> 1),
                   this->y() + ((glWidget->height() - tcpClient->height()) >> 1));
}

void Window::controlKyoku()
{
    if ((netState == NET_STATE_SERVER) && !gameThread.isRunning()) {
        gameThread.startKyoku();
        tcpServer->announceCommand(NET_COMMAND_BEGIN_CLIENT);
    }
    else if (!dialogSC.isVisible() && !tcpServer->isVisible() && !tcpClient->isVisible() && (netState == NET_STATE_NONE) && !gameThread.isRunning())
        dialogSC.show();
}

void Window::serveHanchan()
{
    netState = NET_STATE_SERVER;

    for (int i = 0; i < 4; i++)
        playerToClient[i] = -1;

    for (int i = 0; i < 4; i++) {
        int r;
        do {
            r = genrand_int32() % 4;
        } while (playerToClient[r] != -1);
        playerToClient[r] = i;
    }

    // Hanchan specific
    for (int i = 0; i < 4; i++)
    {
        if (playerToClient[i] == 3) { // host (displayed player)
            dispPlayer = i;

            updateIsAi(checkAi->isChecked());

            player[dispPlayer].setPlayerName(tcpServer->getClientName(playerToClient[i]));

            glWidget->setPlayer(&player[dispPlayer]);
            gameInfo.setDispPlayer(dispPlayer);

            tcpServer->distributePlayerName(player[i].getPlayerName(), uuid);
        }
        else {
            if (tcpServer->isClientAvailable(playerToClient[i])) { // client (manual or AI)
                player[i].setPlayerType(playerToClient[i]);
                player[i].setPlayerName(tcpServer->getClientName(playerToClient[i]));
                tcpServer->setClientToPlayerInd(playerToClient[i], i);

                tcpServer->distributePlayerName(player[i].getPlayerName(), tcpServer->getClientUuid(playerToClient[i]));
            }
            else { // AI on host
                if (aiLibFileNames[playerToClient[i]].isEmpty())
                    player[i].setPlayerType(PLAYER_NATIVE_AI);
                else
                    player[i].setPlayerType(PLAYER_LIB_AI, aiLibFileNames[playerToClient[i]]);

                player[i].setPlayerName(tr("AI %1").arg(3 - playerToClient[i]));

                tcpServer->distributePlayerName(player[i].getPlayerName(), uuid);
            }
        }
    }

    glWidget->setPlayerNames(player[0].getPlayerName(), player[1].getPlayerName(), player[2].getPlayerName(), player[3].getPlayerName());
    gameInfo.setPlayerNames(player[0].getPlayerName(), player[1].getPlayerName(), player[2].getPlayerName(), player[3].getPlayerName());


    gameInfo.initializeHanchan();
    for (int i = 0; i < 4; i++)
        player[i].initialize();

    tcpServer->announceCommand(NET_COMMAND_BEGIN_CLIENT);

    gameThread.startKyoku();
}

void Window::finishHanchan()
{
    gameInfo.initializeHanchan();
    for (int i = 0; i < 4; i++)
        player[i].initialize();

    netState = NET_STATE_NONE;

    tcpServer->show();
}

void Window::initializeColorComboBox()
{
    colorList.clear();
    colorList << "Black"
              << "Magenta"
              << "Red"
              << "DarkRed"
              << "Purple"
              << "Olive"
              << "DarkCyan"
              << "Green"
              << "DarkGreen"
              << "DodgerBlue"
              << "Blue"
              << "DarkBlue"
              << "Gray" ;

    QImage img(16, 16, QImage::Format_RGB32);
    QPainter painter(&img);
    painter.fillRect(img.rect(), Qt::black);
    QRect reducedRect = img.rect().adjusted(1,1,-1,-1);

    for (int i = 0; i < colorList.size(); i++) {
        colorComboBox->addItem(QString(""));
        painter.fillRect(reducedRect, QColor(colorList[i]));
        colorComboBox->setItemData(i, QPixmap::fromImage(img), Qt::DecorationRole);
    }

    colorComboBox->setEditable(false);
}

void Window::updateColorName(int index)
{
    colorName = colorList[index];
}

void Window::updateIsAi(bool isAi)
{
    if (isAi) {
        if (aiLibFileNames[3].isEmpty())
            player[dispPlayer].setPlayerType(PLAYER_NATIVE_AI);
        else
            player[dispPlayer].setPlayerType(PLAYER_LIB_AI, aiLibFileNames[3]);
        aiLibButtons[3]->setEnabled(true);
    }
    else {
        player[dispPlayer].setPlayerType(PLAYER_MANUAL);
        aiLibButtons[3]->setEnabled(false);
    }
}

void Window::processBeginClientCommand()
{
    netState = NET_STATE_CLIENT;

    gameInfo.clearSummaryBoard();

    tcpClient->hide();

    // dispPlayer
    dispPlayer = -1;
    for (int i = 0; i < 4; i++) {
        if (0 == QString::compare(uuid, player[i].getUuid(), Qt::CaseSensitive)) {
            dispPlayer = i;
            break;
        }
    }

#ifdef DEBUG_NET
    if (dispPlayer == -1) {
        printf("TcpClient: could not find uuid!\n");
        fflush(stdout);
    } else {
        printf("TcpClient: assigned to player %d\n", dispPlayer);
        fflush(stdout);
    }
#endif

    glWidget->setPlayerNames(player[0].getPlayerName(), player[1].getPlayerName(), player[2].getPlayerName(), player[3].getPlayerName());
    gameInfo.setPlayerNames(player[0].getPlayerName(), player[1].getPlayerName(), player[2].getPlayerName(), player[3].getPlayerName());

    glWidget->setPlayer(&player[dispPlayer]);
    gameInfo.setDispPlayer(dispPlayer);
    tcpClient->setDispPlayer(dispPlayer);

    updateIsAi(checkAi->isChecked());
}

void Window::processEndClientCommand()
{
    netState = NET_STATE_NONE;

    tcpClient->show();
}

void Window::relayEndHanchanBoard()
{
    tcpServer->announceCommand(NET_COMMAND_END_CLIENT);
}

void Window::askAiLibFileName()
{
    QPushButton* sender = (QPushButton*)this->sender();

    int buttonInd = -1;
    for (int i = 0; i < 4; i++)
        if (aiLibButtons[i] == sender) {
            buttonInd = i;
            break;
        }

    if ((buttonInd < 0) || (buttonInd >= 4))
        return;

    int redo = 1;

    do {
#ifdef Q_OS_OSX
        aiLibFileNames[buttonInd] = QFileDialog::getOpenFileName(this, tr("Open AI library"),
                                                 aiLibFileNames[buttonInd],
                                                 tr("AI library (*.dylib)"));
#endif
#ifdef Q_OS_WIN
        aiLibFileNames[buttonInd] = QFileDialog::getOpenFileName(this, tr("Open AI library"),
                                                 aiLibFileNames[buttonInd],
                                                 tr("AI library (*.dll)"));
#endif

        if (aiLibFileNames[buttonInd].isNull()) {
            aiLibButtons[buttonInd]->setText(tr("Native AI"));
        } else {
            aiLibFileNames[buttonInd].truncate(aiLibFileNames[buttonInd].lastIndexOf(QChar('.')));

            if (!validateAiLibButton(buttonInd)) {
                QMessageBox::information(this, tr("Open AI library"),
                                         tr("Unable to load selected library."));
                continue;
            }
        }

        // set settings
        QSettings settings(QSettings::UserScope, QLatin1String("Nama"));
        settings.beginGroup(QLatin1String("AI"));
        settings.setValue(QString("AI%1FileName").arg(buttonInd), aiLibFileNames[buttonInd]);
        settings.endGroup();

        redo = 0;
    } while (redo);
}

bool Window::validateAiLibButton(int buttonInd)
{
    if (aiLibFileNames[buttonInd].isEmpty()) {
        aiLibButtons[buttonInd]->setText(tr("Native AI"));
        return false;
    }

    QLibrary aiLib(aiLibFileNames[buttonInd]);

    AiLibInitFunc aiLibInit = (AiLibInitFunc)aiLib.resolve("initialize");
    AiLibAskFunc aiLibAsk = (AiLibAskFunc)aiLib.resolve("askAction");

    bool isSuccess = aiLibInit && aiLibAsk;

    if (isSuccess) {
        QString fileName = aiLibFileNames[buttonInd];
        fileName.remove(0, 1 + fileName.lastIndexOf(QChar('/')));
        aiLibButtons[buttonInd]->setText(fileName);
    }
    else
    {
        aiLibButtons[buttonInd]->setText(tr("Native AI"));
    }

    return isSuccess;
}
