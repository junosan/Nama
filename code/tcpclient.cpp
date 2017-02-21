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

#include "tcpclient.h"
#include "constants.h"

// for mt19937ar.c
unsigned long genrand_int32(void);

TcpClient::TcpClient(QWidget *parent)
:   QDialog(parent), networkSession(0)
{
    hostLabel = new QLabel(tr("&Server address:"));
    nameLabel = new QLabel(tr("&Your name:"));

    hostCombo = new QComboBox;
    hostCombo->setEditable(true);

    // find out name of this machine
    QString name = QHostInfo::localHostName();
    if (!name.isEmpty()) {
        hostCombo->addItem(name);
        QString domain = QHostInfo::localDomainName();
        if (!domain.isEmpty())
            hostCombo->addItem(name + QChar('.') + domain);
    }
    if (name != QString("localhost"))
        hostCombo->addItem(QString("localhost"));

    // find out IP addresses of this machine
    QList<QHostAddress> ipAddressesList = QNetworkInterface::allAddresses();

    // add non-localhost addresses
    for (int i = 0; i < ipAddressesList.size(); ++i) {
        if (!ipAddressesList.at(i).isLoopback())
            hostCombo->addItem(ipAddressesList.at(i).toString());
    }

    // add localhost addresses
    for (int i = 0; i < ipAddressesList.size(); ++i) {
        if (ipAddressesList.at(i).isLoopback())
            hostCombo->addItem(ipAddressesList.at(i).toString());
    }


    nameLineEdit= new QLineEdit;
    nameLineEdit->setText(tr("名前 %1").arg(genrand_int32() % 100));

    hostLabel->setBuddy(hostCombo);
    nameLabel->setBuddy(nameLineEdit);

    statusLabel = new QLabel(tr("Ready to connect."));
    statusLabel->setTextFormat(Qt::PlainText);

    connectButton = new QPushButton(tr("Connect"));
    connectButton->setAutoDefault(true);
    enableConnectButton();

    quitButton = new QPushButton(tr("Cancel"));
    quitButton->setAutoDefault(false);

    QDialogButtonBox *buttonBox = new QDialogButtonBox;
    buttonBox->addButton(connectButton, QDialogButtonBox::AcceptRole);
    buttonBox->addButton(quitButton, QDialogButtonBox::RejectRole);

//    QHBoxLayout *buttonLayout = new QHBoxLayout;
//    buttonLayout->addStretch(1);
//    buttonLayout->addWidget(connectButton);
//    buttonLayout->addWidget(quitButton);
//    buttonLayout->addStretch(1);
//    buttonLayout->setContentsMargins(0, 0, 0, 0);

    tcpSocket = new QTcpSocket(this);


    connect(hostCombo, SIGNAL(editTextChanged(QString)),
            this, SLOT(enableConnectButton()));
    connect(connectButton, SIGNAL(clicked()),
            this, SLOT(connectToServer()));

    connect(quitButton, SIGNAL(clicked()), this, SLOT(reject()));
    connect(this, SIGNAL(rejected()), this, SLOT(closeClient()));

    connect(tcpSocket, SIGNAL(readyRead()), this, SLOT(receiveData()));

    connect(tcpSocket, SIGNAL(error(QAbstractSocket::SocketError)),
            this, SLOT(displayError(QAbstractSocket::SocketError)));

    connect(tcpSocket, SIGNAL(connected()), this, SLOT(updateConnectedStatus()));

    QGridLayout *mainLayout = new QGridLayout;
    mainLayout->addWidget(hostLabel, 0, 0);
    mainLayout->addWidget(hostCombo, 0, 1);
    mainLayout->addWidget(nameLabel, 1, 0);
    mainLayout->addWidget(nameLineEdit, 1, 1);
    mainLayout->addWidget(statusLabel, 2, 0, 1, 2);
    mainLayout->addWidget(buttonBox, 3, 0, 1, 2);
    setLayout(mainLayout);

    setWindowTitle(tr("Client settings"));

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

        connectButton->setEnabled(false);
        statusLabel->setText(tr("Opening network session."));
        networkSession->open();
    }

    clientAsker.moveToThread(&askerThread);
    connect(this, SIGNAL(invokeAsker()), &clientAsker, SLOT(ask()));
    connect(&clientAsker, SIGNAL(answerReceived(int,int)), this, SLOT(sendAnswer(int,int)));

    connect(this, SIGNAL(invokeEndKyokuBoard(int,const int*,const int*,const int*,const int*,int,int,int)),
            &clientAsker, SLOT(displayEndKyokuBoard(int,const int*,const int*,const int*,const int*,int,int,int)));

    connect(this, SIGNAL(invokeEndHanchanBoard()), &clientAsker, SLOT(displayEndHanchanBoard()));
    connect(&clientAsker, SIGNAL(endHanchan()), this, SLOT(relayEndHanchan()));


    askerThread.start();
}

TcpClient::~TcpClient()
{
    askerThread.quit();
    askerThread.wait();
}

void TcpClient::connectToServer()
{
    tcpSocket->abort();
    tcpSocket->connectToHost(hostCombo->currentText(), NET_DEFAULT_PORT);

    connectButton->setEnabled(false);
    hostCombo->setEnabled(false);
    nameLineEdit->setEnabled(false);
    statusLabel->setText(tr("Connecting..."));
}

void TcpClient::closeClient()
{
    tcpSocket->abort();
    connectButton->setEnabled(true);
    //reject();
}

void TcpClient::updateConnectedStatus()
{
    statusLabel->setText(tr("Waiting for host to start..."));

    sendName(nameLineEdit->text());
}

void TcpClient::displayError(QAbstractSocket::SocketError socketError)
{
    switch (socketError) {
    case QAbstractSocket::RemoteHostClosedError:
        QMessageBox::information(this, tr("Client settings"),
                                 tr("Host closed the connection."));
        break;
    case QAbstractSocket::HostNotFoundError:
        QMessageBox::information(this, tr("Client settings"),
                                 tr("The host was not found. Please check the "
                                    "host name and port settings."));
        break;
    case QAbstractSocket::ConnectionRefusedError:
        QMessageBox::information(this, tr("Client settings"),
                                 tr("The connection was refused by the peer. "
                                    "Make sure the server is running, "
                                    "and check that the host name and port "
                                    "settings are correct."));
        break;
    default:
        QMessageBox::information(this, tr("Client settings"),
                                 tr("The following error occurred: %1.")
                                 .arg(tcpSocket->errorString()));
    }

    enableConnectButton();
}

void TcpClient::enableConnectButton()
{
    hostCombo->setEnabled(true);
    nameLineEdit->setEnabled(true);
    connectButton->setEnabled((!networkSession || networkSession->isOpen()) &&
                                 !hostCombo->currentText().isEmpty());

    statusLabel->setText(tr("Ready to connect."));
}

void TcpClient::showEvent(QShowEvent *event)
{
    if (!tcpSocket->isOpen())
        enableConnectButton();

    QDialog::showEvent(event);
}

void TcpClient::sessionOpened()
{
    // Save the used configuration
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

    enableConnectButton();
}

void TcpClient::sendText(const QString text)
{
    if (tcpSocket->isOpen()) {

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

        tcpSocket->write(block);
    }
}

void TcpClient::sendName(const QString name)
{
    if (tcpSocket->isOpen()) {

        QByteArray block;
        QDataStream out(&block, QIODevice::WriteOnly);
        out.setVersion(QDATASTREAM_VERSION);

        // format
        out << (quint32)NET_FORMAT_NAME;

        // reserve 2 bytes for size
        out << (quint16)0;

        // content
        out << name;
        out << uuid;

#ifdef DEBUG_NET
        printf("TcpClient: sent uuid %s\n", uuid.toLatin1().constData());
        fflush(stdout);
#endif

        // size
        out.device()->seek(sizeof(quint32));
        out << (quint16)(block.size() - sizeof(quint32) - sizeof(quint16));

        tcpSocket->write(block);
    }
}

void TcpClient::receiveEnteredText(QString text)
{
    text.prepend(QString("<p style=\"color:%1;\"><b>%2:</b> ").arg(*pColorName).arg(nameLineEdit->text()));
    text.append("</p>");

    sendText(text);
}

void TcpClient::receiveData()
{
    static bool readFormatSize = true;
    static quint32 format = 0;
    static quint16 blockSize = 0;
    static int nextPlayerNumber = 0;

    QDataStream in(tcpSocket);
    in.setVersion(QDATASTREAM_VERSION);

    while (1) {

        if (readFormatSize && \
                (tcpSocket->bytesAvailable() < ((qint64)sizeof(quint32) + (qint64)sizeof(quint16))))
            break;

        if (readFormatSize) {
            in >> format;
            in >> blockSize;
            readFormatSize = false;
        }

#ifdef DEBUG_NET
        if ((format != NET_FORMAT_TEXT) &&
                (format != NET_FORMAT_NAME) &&
                (format != NET_COMMAND_BEGIN_CLIENT) &&
                (format != NET_FORMAT_SFX) &&
                (format != NET_FORMAT_SCREEN_STATE) &&
                (format != NET_FORMAT_ASK_ACTION) &&
                (format != NET_FORMAT_END_KYOKU_BOARD) &&
                (format != NET_COMMAND_END_CLIENT)){
            printf("TcpClient: invalid data received!\nformat: %d\n", format);
            fflush(stdout);
            break;
        }
#endif

        if (tcpSocket->bytesAvailable() < blockSize)
            break;


        if (format == NET_FORMAT_TEXT) {
            QString text;
            in >> text;
            emit displayText(text);
        } else if (format == NET_FORMAT_NAME) {
            QString name, uuid;
            in >> name;
            in >> uuid;

            players[nextPlayerNumber].setPlayerName(name);
            players[nextPlayerNumber].setUuid(uuid);
#ifdef DEBUG_NET
            printf("TcpClient: received name [%d] %s\n", nextPlayerNumber, name.toLatin1().constData());fflush(stdout);
#endif
            nextPlayerNumber = (nextPlayerNumber + 1) & 0x03;
        } else if (format == NET_COMMAND_BEGIN_CLIENT) {
#ifdef DEBUG_NET
            printf("TcpClient: received begin client command\n");
            fflush(stdout);
#endif
            emit receivedBeginClientCommand();
        } else if (format == NET_FORMAT_SFX) {
            quint8 sfxEnum;
            in >> sfxEnum;

            emit emitSound(sfxEnum);
        } else if (format == NET_FORMAT_SCREEN_STATE) {
            int j;
            quint16 u16;
            qint16 s16;
            quint8 u8;
            qint8 s8;
            quint32 u32;

#define RECEIVEDATA(X,Y)    {in >> (Y); (X) = (Y);}

            // content
            for (j = 0; j < 64; j++)
                RECEIVEDATA(pGameInfo->naki[j], u16)

            for (j = 0; j < IND_KAWASIZE * 4; j++)
                RECEIVEDATA(pGameInfo->kawa[j], u16)

            for (j = 0; j < 5; j++)
                RECEIVEDATA(pGameInfo->dorahyouji[j], u16)

            for (j = 0; j < 5; j++)
                RECEIVEDATA(pGameInfo->omotedora[j], u16)

            for (j = 0; j < 4; j++) {
                RECEIVEDATA(pGameInfo->ten[j], s16)
                pGameInfo->ten[j] *= 100;
            }

            for (j = 0; j < 4; j++)
                RECEIVEDATA(pGameInfo->richiSuccess[j], u8)

            for (j = 0; j < 4; j++)
                RECEIVEDATA(pGameInfo->isTsumohaiAvail[j], u8)

            RECEIVEDATA(pGameInfo->chanfonpai, u8)
            RECEIVEDATA(pGameInfo->kyoku, u8)
            RECEIVEDATA(pGameInfo->kyokuOffsetForDisplay, s8)
            RECEIVEDATA(pGameInfo->nTsumibou, u8)
            RECEIVEDATA(pGameInfo->nRichibou, u8)
            RECEIVEDATA(pGameInfo->curDir, u8)
            RECEIVEDATA(pGameInfo->prevNaki, u8)
            RECEIVEDATA(pGameInfo->sai, u8)
            RECEIVEDATA(pGameInfo->nYama, u8)
            RECEIVEDATA(pGameInfo->nKan, u8)
            RECEIVEDATA(pGameInfo->ruleFlags, u8)

            Player* pPlayer = players + pGameInfo->getDispPlayer();

            RECEIVEDATA(pPlayer->myDir, u8)

            for (j = 0; j <= IND_TSUMOHAI; j++)
                RECEIVEDATA(pPlayer->te[j], u16)

            RECEIVEDATA(pPlayer->te[IND_NAKISTATE], u32)

#undef RECEIVEDATA

            emit receivedScreenStatus();
        } else if (format == NET_FORMAT_ASK_ACTION) {
            int j;
            Player* pPlayer = players + pGameInfo->getDispPlayer();
            qint8 s8;

#ifdef DEBUG_NET
            printf("TcpClient: player %d received action request via net\n", pGameInfo->getDispPlayer());
            fflush(stdout);
#endif

#define RECEIVEARRAY(X,Y)       for (j = 0; j < Y; j++) {in >> s8; pPlayer->X[j] = s8;}

            RECEIVEARRAY(actionAvail, IND_ACTIONAVAILSIZE)
            RECEIVEARRAY(ankanInds, IND_ANKANINDSSIZE)
            RECEIVEARRAY(kakanInds, IND_KAKANINDSSIZE)
            RECEIVEARRAY(richiInds, IND_RICHIINDSSIZE)
            RECEIVEARRAY(dahaiInds, IND_DAHAIINDSSIZE)
            RECEIVEARRAY(ponInds, IND_PONINDSSIZE)
            RECEIVEARRAY(chiInds, IND_CHIINDSSIZE)

#undef RECEIVEARRAY

            emit invokeAsker();

        } else if (format == NET_FORMAT_END_KYOKU_BOARD) {
            quint8 u8;
            quint16 u16;
            quint32 u32;
            qint16 s16;

            in >> u8;
            endFlag = u8;

            int j;

#define RECEIVEARRAY(X,Y,Z)       for (j = 0; j < Y; j++) {in >> Z; (X)[j] = Z;}

            RECEIVEARRAY(tTe, IND_NAKISTATE, u16);
            RECEIVEARRAY(tTe + IND_NAKISTATE, 3, u32);
            RECEIVEARRAY(uradorahyouji, 5, u16);
            RECEIVEARRAY(uradora, 5, u16);

#undef RECEIVEARRAY

#define RECEIVEDATA(X,Z)    {in >> (Z); (X) = (Z);}

            for (j = 0; j < 4; j++) {
                RECEIVEDATA(tenDiffForPlayer[j], s16);
                tenDiffForPlayer[j] *= 100;
            }

            RECEIVEDATA(ten, s16);
            ten *= 100;

            RECEIVEDATA(fan, u8);
            RECEIVEDATA(fu, u8);

#undef RECEIVEDATA

            emit invokeEndKyokuBoard(endFlag, tTe, uradorahyouji, uradora, tenDiffForPlayer, ten, fan, fu);
        } else if (format == NET_COMMAND_END_CLIENT) {
#ifdef DEBUG_NET
            printf("TcpClient: received end client command\n");
            fflush(stdout);
#endif

            emit invokeEndHanchanBoard();
        }

        readFormatSize = true;

    }
}

QString TcpClient::getClientName()
{
    return nameLineEdit->text();
}

void TcpClient::sendAnswer(int action, int optionChosen)
{
    if (tcpSocket->isOpen()) {

        QByteArray block;
        QDataStream out(&block, QIODevice::WriteOnly);
        out.setVersion(QDATASTREAM_VERSION);

        // format
        out << (quint32)NET_FORMAT_ACTION_ANSWER;

        // reserve 2 bytes for size
        out << (quint16)0;

        // content
        out << (qint8)action;
        out << (qint8)optionChosen;

        // size
        out.device()->seek(sizeof(quint32));
        out << (quint16)(block.size() - sizeof(quint32) - sizeof(quint16));

        tcpSocket->write(block);
    }
}

void TcpClient::setDispPlayer(int playerInd)
{
    clientAsker.pGameInfo = pGameInfo;
    clientAsker.pPlayer = players + (playerInd & 0x03);
}

void TcpClient::relayEndHanchan()
{
    pGameInfo->initializeHanchan();
    for (int i = 0; i < 4; i++)
        players[i].initialize();

    emit receivedEndClientCommand();
}

void TcpClient::setUuid(QString _uuid)
{
    uuid = _uuid;
}

void ClientAsker::ask()
{
    int action, optionChosen = -1;
    action = pPlayer->askAction(&optionChosen);

    // send answer back
    emit answerReceived(action, optionChosen);
}

void ClientAsker::displayEndKyokuBoard(int endFlag, const int *te, const int *uradorahyouji, const int *uradora, const int *tenDiffForPlayer, int ten, int fan, int fu)
{
    pGameInfo->drawEndKyokuBoard(endFlag, te, uradorahyouji, uradora, tenDiffForPlayer, ten, fan, fu);
}

void ClientAsker::displayEndHanchanBoard()
{
    pGameInfo->drawEndHanchanBoard();

    emit endHanchan();
}
