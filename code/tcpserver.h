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

#ifndef TCPSERVER_H
#define TCPSERVER_H

#include <QDialog>

QT_BEGIN_NAMESPACE
class QLabel;
class QPushButton;
class QLineEdit;
class QTcpServer;
class QNetworkSession;
class QTcpSocket;
QT_END_NAMESPACE

#include "gameinfo.h"
#include "player.h"

class TcpServer : public QDialog
{
    Q_OBJECT

public:
    TcpServer(QWidget *parent = 0, QPushButton **aiLibButtons = 0);

    bool isClientAvailable(int ind);
    QString getClientName(int ind); // ind == 3 for host's name
    QString getClientUuid(int ind);
    void setClientToPlayerInd(int clientInd, int playerInd);
    void setUuid(QString uuid);

    GameInfo* pGameInfo;
    Player* players;

    QString* pColorName;

    void distributePlayerName(QString playerName, QString playerUuid);

protected:
    void showEvent(QShowEvent *event);

signals:
    void displayText(QString text);
    void answerRecieved(int action, int optionChosen);

public slots:
    void receiveEnteredText(QString text);

    void distributeText(QString text);
    void distributeSfx(int sfxEnum);
    void distributeScreenState();
    void announceCommand(int commandEnum);
    void askClientForAnswer(int clientInd);
    void sendEndKyokuBoard(int endFlag, const int* te, const int* uradorahyouji, const int* uradora, const int* tenDiffForPlayer, int ten, int fan, int fu);

private slots:
    void sessionOpened();
    void closeServer(); // closes server and dialog
    void updateConnectionLabel();
    void saveClient();
    void deleteDisconnectedClient(QObject* obj);
    void receiveData();


private:
    QLabel *statusLabel, *nameLabel;
    QLineEdit *nameLineEdit;
    QPushButton *quitButton, *startButton;

    QString ipAddress;
    QTcpServer *tcpServer;
    QNetworkSession *networkSession;

    QLabel *clientLabels[3];
    QPushButton *aiLibButtons[3];
    QTcpSocket *clientConnections[3];
    QString clientNames[3];
    QString clientUuids[3];
    int clientPlayerNumbers[3];

    QString uuid;
};

#endif // TCPSERVER_H
