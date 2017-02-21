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
#include <QtNetwork>

#include "tcpserver.h"
#include "constants.h"


// for mt19937ar.c
unsigned long genrand_int32(void);

TcpServer::TcpServer(QWidget *parent, QPushButton **_aiLibButtons)
    :   QDialog(parent), tcpServer(0), networkSession(0)
{
    for (int i = 0; i < 3; i++)
        aiLibButtons[i] = _aiLibButtons[i];

    statusLabel = new QLabel;

    for (int i = 0; i < 3; i++)
        clientLabels[i] = new QLabel(QString("[%1]").arg(i));

    quitButton = new QPushButton(tr("Cancel"));
    quitButton->setAutoDefault(false);
    startButton = new QPushButton(tr("Start"));
    startButton->setAutoDefault(true);

    nameLabel = new QLabel(tr("&Your name:"));
    nameLineEdit= new QLineEdit;
    nameLineEdit->setText(tr("名前 %1").arg(genrand_int32() % 100));
    nameLabel->setBuddy(nameLineEdit);

    QHBoxLayout *nameLayout = new QHBoxLayout;
    nameLayout->addWidget(nameLabel);
    nameLayout->addWidget(nameLineEdit);

    QDialogButtonBox *buttonBox = new QDialogButtonBox;
    buttonBox->addButton(startButton, QDialogButtonBox::AcceptRole);
    buttonBox->addButton(quitButton, QDialogButtonBox::RejectRole);

    QGridLayout *mainLayout = new QGridLayout;
    mainLayout->addWidget(statusLabel, 0, 0, 1, 2);
    mainLayout->addLayout(nameLayout, 1, 0, 1, 2);
    for (int i = 0; i < 3; i++) {
        mainLayout->addWidget(clientLabels[i], i + 2, 0, 1, 1);
        mainLayout->addWidget(aiLibButtons[i], i + 2, 1, 1, 1);
    }
    mainLayout->addWidget(buttonBox, 5, 0, 1, 2);
    mainLayout->setSizeConstraint(QLayout::SetFixedSize);

    setLayout(mainLayout);

    setWindowTitle(tr("Server settings"));

    clientConnections[0] = clientConnections[1] = clientConnections[2] = NULL;
    clientPlayerNumbers[0] = clientPlayerNumbers[1] = clientPlayerNumbers[2] = -1;

    connect(quitButton, SIGNAL(clicked()), this, SLOT(reject()));
    connect(this, SIGNAL(rejected()), this, SLOT(closeServer()));
    connect(startButton, SIGNAL(clicked()), this, SLOT(accept()));
}

void TcpServer::showEvent(QShowEvent *event)
{
    if (!tcpServer || !tcpServer->isListening()) {

        QNetworkConfigurationManager manager;
        if (manager.capabilities() & QNetworkConfigurationManager::NetworkSessionRequired) {
            // Get saved network configuration
            QSettings settings(QSettings::UserScope, QLatin1String("Nama"));
            settings.beginGroup(QLatin1String("Network"));
            const QString id = settings.value(QLatin1String("DefaultNetworkConfiguration")).toString();
            settings.endGroup();

            // If the saved network configuration is not currently discovered use the system default
            QNetworkConfiguration config = manager.configurationFromIdentifier(id);
            if ((config.state() & QNetworkConfiguration::Discovered) !=
                    QNetworkConfiguration::Discovered) {
                config = manager.defaultConfiguration();
            }

            networkSession = new QNetworkSession(config, this);
            connect(networkSession, SIGNAL(opened()), this, SLOT(sessionOpened()));

            statusLabel->setText(tr("Opening network session."));
            networkSession->open();
        } else {
            sessionOpened();
        }

    }

    QDialog::showEvent(event);
}

void TcpServer::sessionOpened()
{
    // Save the used configuration
    if (networkSession) {
        QNetworkConfiguration config = networkSession->configuration();
        QString id;
        if (config.type() == QNetworkConfiguration::UserChoice)
            id = networkSession->sessionProperty(QLatin1String("UserChoiceConfiguration")).toString();
        else
            id = config.identifier();

        QSettings settings(QSettings::UserScope, QLatin1String("Nama"));
        settings.beginGroup(QLatin1String("Network"));
        settings.setValue(QLatin1String("DefaultNetworkConfiguration"), id);
        settings.endGroup();
    }

    tcpServer = new QTcpServer(this);
    if (!tcpServer->listen(QHostAddress::Any, NET_DEFAULT_PORT)) {
        QMessageBox::information(this, tr("Server settings"),
                              tr("Unable to start the server: %1.")
                              .arg(tcpServer->errorString()));
        startButton->setEnabled(false);
    }

    ipAddress.clear();
    QList<QHostAddress> ipAddressesList = QNetworkInterface::allAddresses();

    // use the first non-localhost IPv4 address
    for (int i = 0; i < ipAddressesList.size(); ++i) {
        if (ipAddressesList.at(i) != QHostAddress::LocalHost &&
            ipAddressesList.at(i).toIPv4Address()) {
            ipAddress = ipAddressesList.at(i).toString();
            break;
        }
    }
    // if we did not find one, use IPv4 localhost
    if (ipAddress.isEmpty())
        ipAddress = QHostAddress(QHostAddress::LocalHost).toString();


    connect(tcpServer, SIGNAL(newConnection()), this, SLOT(saveClient()));

    updateConnectionLabel();
    startButton->setEnabled(true);

    QString text = tr("<p style=\"color:gray;\"><b>%1 entered the room.</b></p>").arg(nameLineEdit->text());
    distributeText(text);
}

void TcpServer::closeServer()
{
    if (tcpServer) {
        if (tcpServer->isListening()) {

            QString text = tr("<p style=\"color:gray;\"><b>%1 left the room.</b></p>").arg(nameLineEdit->text());
            distributeText(text);

            tcpServer->close();
        }
        tcpServer->deleteLater();
        tcpServer = NULL;

        clientConnections[0] = clientConnections[1] = clientConnections[2] = NULL;
    }

    //reject();
}

void TcpServer::saveClient()
{
    int assigned = 0;
    for (int i = 0; i < 3; i++) {
        if (clientConnections[i] == NULL) {
            clientConnections[i] = tcpServer->nextPendingConnection();
            connect(clientConnections[i], SIGNAL(readyRead()), this, SLOT(receiveData()));
            connect(clientConnections[i], SIGNAL(disconnected()), clientConnections[i], SLOT(deleteLater()));
            connect(clientConnections[i], SIGNAL(destroyed(QObject*)), this, SLOT(deleteDisconnectedClient(QObject*)));

            updateConnectionLabel();
            assigned = 1;
            break;
        }
    }

    // 3 slots full
    if(!assigned) {
        tcpServer->nextPendingConnection()->disconnectFromHost();
    }

}

void TcpServer::updateConnectionLabel()
{
    statusLabel->setText(tr("Listening on %1").arg(ipAddress));

    for (int i = 0; i < 3; i++) {
        if (clientConnections[i]) {
            clientLabels[i]->setText(tr("[%1] %2").arg(i + 1).arg(clientNames[i]));
            aiLibButtons[i]->setEnabled(false);
        }
        else {
            clientLabels[i]->setText(tr("[%1] Waiting (AI)").arg(i + 1));
            aiLibButtons[i]->setEnabled(true);
        }
    }
}

void TcpServer::deleteDisconnectedClient(QObject* obj)
{
    for (int i = 0; i < 3; i++) {
        if (clientConnections[i] == obj) {
            clientConnections[i] = NULL;

            QString text = tr("<p style=\"color:gray;\"><b>%1 left the room.</b></p>").arg(clientNames[i]);
            distributeText(text);

        }
    }

    updateConnectionLabel();
}

void TcpServer::receiveEnteredText(QString text)
{
    text.prepend(QString("<p style=\"color:%1;\"><b>%2:</b> ").arg(*pColorName).arg(nameLineEdit->text()));
    text.append("</p>");

    distributeText(text);
}

void TcpServer::distributeText(QString text)
{
    if (tcpServer && tcpServer->isListening()) {

        QByteArray block;
        QDataStream out(&block, QIODevice::WriteOnly);
        out.setVersion(QDATASTREAM_VERSION);

        // format
        out << (quint32)NET_FORMAT_TEXT;

        // reserve 2 bytes for size
        out << (quint16)0;

        // content
        out << text;

        // size
        out.device()->seek(sizeof(quint32));
        out << (quint16)(block.size() - sizeof(quint32) - sizeof(quint16));

        emit displayText(text);
        for (int i = 0; i < 3; i++) {
            if((clientConnections[i] != NULL) && (clientConnections[i]->isOpen()))
                clientConnections[i]->write(block);
        }
    }
}

void TcpServer::receiveData()
{
    static bool readFormatSize[3] = {true, true, true};
    static quint32 format[3] = {0, 0, 0};
    static quint16 blockSize[3] = {0, 0, 0};

    for (int i = 0; i < 3; i++) {
        if ((clientConnections[i] != NULL) && (clientConnections[i]->isReadable())) {

            QTcpSocket *tcpSocket = clientConnections[i];
            QDataStream in(tcpSocket);
            in.setVersion(QDATASTREAM_VERSION);

            while (1) {
                if (readFormatSize[i] && \
                        (tcpSocket->bytesAvailable() < ((qint64)sizeof(quint32) + (qint64)sizeof(quint16))))
                    break;

                if (readFormatSize[i]) {
                    in >> format[i];
                    in >> blockSize[i];
                    readFormatSize[i] = false;
                }

#ifdef DEBUG_NET
                if ((format[i] != NET_FORMAT_TEXT) &&
                    (format[i] != NET_FORMAT_NAME) &&
                    (format[i] != NET_FORMAT_ACTION_ANSWER)) {
                    printf("TcpServer: invalid data received!\nformat[%d]: %d\n", i, format[i]);
                    fflush(stdout);
                    break;
                }
#endif

                if (tcpSocket->bytesAvailable() < blockSize[i])
                    break;

                if (format[i] == NET_FORMAT_TEXT) {
                    QString text;
                    in >> text;

                    distributeText(text);

                } else if (format[i] == NET_FORMAT_NAME) {
                    in >> clientNames[i];
                    in >> clientUuids[i];

#ifdef DEBUG_NET
                    printf("TcpServer: received uuid %s\n", clientUuids[i].toLatin1().constData());
                    fflush(stdout);
#endif

                    updateConnectionLabel();

                    QString text;
                    text = tr("<p style=\"color:gray;\"><b>%1 entered the room.</b></p>").arg(clientNames[i]);

                    distributeText(text);
                } else if (format[i] == NET_FORMAT_ACTION_ANSWER) {
                    qint8 action, optionChosen;
                    in >> action;
                    in >> optionChosen;

                    emit answerRecieved(action, optionChosen);
                }

                readFormatSize[i] = true;
            }
        }
    }
}

bool TcpServer::isClientAvailable(int ind)
{
    if (tcpServer && tcpServer->isListening() && (ind >= 0) && (ind <= 2))
        return clientConnections[ind] != NULL;
    else
        return false;
}

QString TcpServer::getClientName(int ind) // ind == 3 for host's name
{
    if (ind == 3)
        return nameLineEdit->text();
    else if (isClientAvailable(ind))
        return clientNames[ind];
    else
        return QString(tr("Invalid call"));
}

QString TcpServer::getClientUuid(int ind) // ind == 3 for host's name
{
    if (ind == 3)
        return QUuid::createUuid().toString();
    else if (isClientAvailable(ind))
        return clientUuids[ind];
    else
        return QString(tr("Invalid call"));
}

void TcpServer::distributePlayerName(QString playerName, QString playerUuid)
{
    if (tcpServer && tcpServer->isListening()) {

        QByteArray block;
        QDataStream out(&block, QIODevice::WriteOnly);
        out.setVersion(QDATASTREAM_VERSION);

        // format
        out << (quint32)NET_FORMAT_NAME;

        // reserve 2 bytes for size
        out << (quint16)0;

        // content
        out << playerName;
        out << playerUuid;

        // size
        out.device()->seek(sizeof(quint32));
        out << (quint16)(block.size() - sizeof(quint32) - sizeof(quint16));

        for (int i = 0; i < 3; i++) {
            if((clientConnections[i] != NULL) && (clientConnections[i]->isOpen())) {
                clientConnections[i]->write(block);
#ifdef DEBUG_NET
                printf("TcpServer: distribute name %s\n", playerName.toLatin1().constData());
                fflush(stdout);
#endif
            }
        }
    }
}

void TcpServer::distributeSfx(int sfxEnum)
{
    if (tcpServer && tcpServer->isListening()) {

        QByteArray block;
        QDataStream out(&block, QIODevice::WriteOnly);
        out.setVersion(QDATASTREAM_VERSION);

        // format
        out << (quint32)NET_FORMAT_SFX;

        // reserve 2 bytes for size
        out << (quint16)0;

        // content
        out << (quint8)sfxEnum;

        // size
        out.device()->seek(sizeof(quint32));
        out << (quint16)(block.size() - sizeof(quint32) - sizeof(quint16));

        for (int i = 0; i < 3; i++) {
            if((clientConnections[i] != NULL) && (clientConnections[i]->isOpen())) {
                if ((sfxEnum == SOUND_TSUMO) && (players[clientPlayerNumbers[i]].myDir == pGameInfo->curDir))
                    continue;
                clientConnections[i]->write(block);
            }
        }
    }
}

void TcpServer::announceCommand(int commandEnum)
{
    if (tcpServer && tcpServer->isListening()) {

        QByteArray block;
        QDataStream out(&block, QIODevice::WriteOnly);
        out.setVersion(QDATASTREAM_VERSION);

        // format
        out << (quint32)commandEnum;

        // size is 0 for commands
        out << (quint16)0;

        for (int i = 0; i < 3; i++) {
            if((clientConnections[i] != NULL) && (clientConnections[i]->isOpen())) {
                clientConnections[i]->write(block);
            }
        }
    }
}

void TcpServer::distributeScreenState()
{
    int j;

    if (tcpServer && tcpServer->isListening()) {

        QByteArray blockC; // common for all players
        QDataStream outC(&blockC, QIODevice::WriteOnly);
        outC.setVersion(QDATASTREAM_VERSION);

        // format
        outC << (quint32)NET_FORMAT_SCREEN_STATE;

        // reserve 2 bytes for size
        outC << (quint16)0;

        // content
        for (j = 0; j < 64; j++)
            outC << (quint16)(pGameInfo->naki[j]);

        for (j = 0; j < IND_KAWASIZE * 4; j++)
            outC << (quint16)(pGameInfo->kawa[j]);

        for (j = 0; j < 5; j++)
            outC << (quint16)(pGameInfo->dorahyouji[j]);

        for (j = 0; j < 5; j++)
            outC << (quint16)(pGameInfo->omotedora[j]);

        for (j = 0; j < 4; j++)
            outC << (qint16)(pGameInfo->ten[j] / 100);

        for (j = 0; j < 4; j++)
            outC << (quint8)(pGameInfo->richiSuccess[j]);

        for (j = 0; j < 4; j++)
            outC << (quint8)(pGameInfo->isTsumohaiAvail[j]);

        outC << (quint8)(pGameInfo->chanfonpai);
        outC << (quint8)(pGameInfo->kyoku);
        outC << (qint8)(pGameInfo->kyokuOffsetForDisplay);
        outC << (quint8)(pGameInfo->nTsumibou);
        outC << (quint8)(pGameInfo->nRichibou);
        outC << (quint8)(pGameInfo->curDir);
        outC << (quint8)(pGameInfo->prevNaki);
        outC << (quint8)(pGameInfo->sai);
        outC << (quint8)(pGameInfo->nYama);
        outC << (quint8)(pGameInfo->nKan);
        outC << (quint8)(pGameInfo->ruleFlags);

        for (int i = 0; i < 3; i++) {
            if((clientConnections[i] != NULL) && (clientConnections[i]->isOpen())) {

                QByteArray blockS; // specific for a player
                QDataStream outS(&blockS, QIODevice::WriteOnly);
                outS.setVersion(QDATASTREAM_VERSION);

                Player* pPlayer = players + clientPlayerNumbers[i];

                outS << (quint8)(pPlayer->myDir);

                for (j = 0; j <= IND_TSUMOHAI; j++)
                    outS << (quint16)(pPlayer->te[j]);

                outS << (quint32)(pPlayer->te[IND_NAKISTATE]);

                // size
                outC.device()->seek(sizeof(quint32));
                outC << (quint16)(blockC.size() + blockS.size() - sizeof(quint32) - sizeof(quint16));

                clientConnections[i]->write(blockC);
                clientConnections[i]->write(blockS);
            }
        }
    }
}

void TcpServer::setClientToPlayerInd(int clientInd, int playerInd)
{
   clientPlayerNumbers[clientInd % 3] = playerInd & 0x03;
}

void TcpServer::askClientForAnswer(int clientInd)
{
    if (tcpServer && tcpServer->isListening()) {

        QByteArray block;
        QDataStream out(&block, QIODevice::WriteOnly);
        out.setVersion(QDATASTREAM_VERSION);

        // format
        out << (quint32)NET_FORMAT_ASK_ACTION;

        // reserve 2 bytes for size
        out << (quint16)0;

        // content
        int j;
        Player* pPlayer = players + clientPlayerNumbers[clientInd];

#ifdef DEBUG_NET
        printf("TcpServer: player %d asking action via net\n", clientPlayerNumbers[clientInd]);
        fflush(stdout);
#endif

#define SENDARRAY(X,Y)      for (j = 0; j < Y; j++) out << (qint8)(pPlayer->X[j]);

        SENDARRAY(actionAvail, IND_ACTIONAVAILSIZE)
        SENDARRAY(ankanInds, IND_ANKANINDSSIZE)
        SENDARRAY(kakanInds, IND_KAKANINDSSIZE)
        SENDARRAY(richiInds, IND_RICHIINDSSIZE)
        SENDARRAY(dahaiInds, IND_DAHAIINDSSIZE)
        SENDARRAY(ponInds, IND_PONINDSSIZE)
        SENDARRAY(chiInds, IND_CHIINDSSIZE)

#undef SENDARRAY

        // size
        out.device()->seek(sizeof(quint32));
        out << (quint16)(block.size() - sizeof(quint32) - sizeof(quint16));

        if((clientConnections[clientInd] != NULL) && (clientConnections[clientInd]->isOpen())) {
            clientConnections[clientInd]->write(block);
        }
        else {
            // client dc'd : turn to AI mode
            pPlayer->setPlayerType(PLAYER_NATIVE_AI);

            int action, optionChosen;
            action = pPlayer->askAction(&optionChosen);

            emit answerRecieved(action, optionChosen);
        }

    }
}

void TcpServer::sendEndKyokuBoard(int endFlag, const int *te, const int *uradorahyouji, const int *uradora, const int *tenDiffForPlayer, int ten, int fan, int fu)
{
    int j;

    if (tcpServer && tcpServer->isListening()) {

        QByteArray block;
        QDataStream out(&block, QIODevice::WriteOnly);
        out.setVersion(QDATASTREAM_VERSION);

        // format
        out << (quint32)NET_FORMAT_END_KYOKU_BOARD;

        // reserve 2 bytes for size
        out << (quint16)0;

        // content
        out << (quint8)endFlag;

#define SENDARRAY(X,Y,Z)    for (j = 0; j < Y; j++) out << (Z)((X)[j]);

        SENDARRAY(te, IND_NAKISTATE, quint16);
        SENDARRAY(te + IND_NAKISTATE, 3, quint32);
        SENDARRAY(uradorahyouji, 5, quint16);
        SENDARRAY(uradora, 5, quint16);

#undef SENDARRAY

        for (j = 0; j < 4; j++)
            out << (qint16)(tenDiffForPlayer[j] / 100);

        out << (qint16)(ten / 100);
        out << (quint8)fan;
        out << (quint8)fu;

        // size
        out.device()->seek(sizeof(quint32));
        out << (quint16)(block.size() - sizeof(quint32) - sizeof(quint16));

        for (int i = 0; i < 3; i++) {
            if((clientConnections[i] != NULL) && (clientConnections[i]->isOpen())) {
                clientConnections[i]->write(block);
            }
        }
    }
}

void TcpServer::setUuid(QString _uuid)
{
    uuid = _uuid;
}
