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

#ifndef TCPCLIENT_H
#define TCPCLIENT_H

#include <QDialog>
#include <QTcpSocket>

QT_BEGIN_NAMESPACE
class QComboBox;
class QDialogButtonBox;
class QLabel;
class QLineEdit;
class QPushButton;
class QTcpSocket;
class QNetworkSession;
QT_END_NAMESPACE

#include "gameinfo.h"
#include "player.h"

class ClientAsker : public QObject
{
    Q_OBJECT

public:

    GameInfo *pGameInfo;
    Player *pPlayer;

public slots:
    void ask();
    void displayEndKyokuBoard(int endFlag, const int* te, const int* uradorahyouji, const int* uradora, const int* tenDiffForPlayer, int ten, int fan, int fu);
    void displayEndHanchanBoard();

signals:
    void answerReceived(int action, int optionChosen);
    void endHanchan();
};

class TcpClient : public QDialog
{
    Q_OBJECT

public:
    TcpClient(QWidget *parent = 0);
    ~TcpClient();

    QString getClientName();
    void setUuid(QString uuid);

    QString* pColorName;

    GameInfo* pGameInfo;
    Player* players;
    void setDispPlayer(int playerInd);

signals:
    void displayText(QString text);
    void emitSound(int soundEnum);
    void receivedBeginClientCommand();
    void receivedScreenStatus();
    void receivedEndClientCommand();
    void invokeAsker();
    void invokeEndKyokuBoard(int endFlag, const int* te, const int* uradorahyouji, const int* uradora, const int* tenDiffForPlayer, int ten, int fan, int fu);
    void invokeEndHanchanBoard();

protected:
    void showEvent(QShowEvent *event);

public slots:
    void receiveEnteredText(QString text);

private slots:
    void connectToServer();
    void receiveData();
    void displayError(QAbstractSocket::SocketError socketError);
    void enableConnectButton();
    void updateConnectedStatus();
    void closeClient(); // aborts connection and close dialog
    void sessionOpened();
    void sendText(const QString text);
    void sendName(const QString name);
    void sendAnswer(int action, int optionChosen);
    void relayEndHanchan();

private:
    QLabel *hostLabel;
    QLabel *nameLabel;
    QLabel *statusLabel;

    QComboBox *hostCombo;
    QLineEdit *nameLineEdit;

    QPushButton *connectButton;
    QPushButton *quitButton;
    QDialogButtonBox *buttonBox;

    QTcpSocket *tcpSocket;

    QNetworkSession *networkSession;

    ClientAsker clientAsker;
    QThread askerThread;

    QString uuid;

    // for displaying endKyokuBoard
    int endFlag;
    int tTe[IND_TESIZE];
    int uradorahyouji[5];
    int uradora[5];
    int tenDiffForPlayer[4];
    int ten, fan, fu;
};

#endif // TCPCLIENT_H
