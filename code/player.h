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

#ifndef PLAYER_H
#define PLAYER_H

#include <QObject>
#include <QMutex>
#include <QWaitCondition>
#include <QLibrary>

#include "constants.h"
#include "textdisp.h"
#include "gamethread.h"
#include "gameinfo.h"
#include "haibutton.h"
#include "namaai_global.h"

class GameInfo;
class HaiButton;

typedef void (*AiLibInitFunc)();
typedef AiAnswer (*AiLibAskFunc)(AiData);

class Player : public QObject
{
    Q_OBJECT

public:

	Player();

    void initialize();

    void setPlayerType(int type, QString libFile = QString(""));

    void setGameInfo(const GameInfo* p);
    void setHaiButton(HaiButton* p);

    void setPlayerName(QString name);
    QString getPlayerName();

    void setUuid(QString uuid);
    QString getUuid();

    int registerAction(int actionType, const int* inds);
	int askAction(int* pOptionChosen); // this calls clearActions() inside

	int te[IND_NAKISTATE + 1];
    int myDir;

    // Public for TcpServer and TcpClient
    int actionAvail[IND_ACTIONAVAILSIZE];
    // last index for storing -1 to mark the end
    int ankanInds[IND_ANKANINDSSIZE]; // ankan (3 x 4)
    int kakanInds[IND_KAKANINDSSIZE]; // kakan (3 x 1)
    int richiInds[IND_RICHIINDSSIZE]; // richi (14 x 1)
    int dahaiInds[IND_DAHAIINDSSIZE]; // dahai (14 x 1)
    int ponInds[IND_PONINDSSIZE]; // pon (2 x 2)
    int chiInds[IND_CHIINDSSIZE]; // chi (5 x 2)

signals:
    void dispText(QString str);
    void updateScreen();
    void actionAsked(float x, float y);
    void emitSound(int soundEnum);
    void needAnswerViaNetwork(int clientInd);

public slots:
    void receiveAnswer(QString str);
    void receiveHaiClick(int haiAnswer);
    void receiveButtonClick(int buttonAnswer);
    void receiveNetworkAnswer(int action, int optionChosen);

private:
    QMutex mutex;
    QWaitCondition waitAnswer;
    friend class GameThread;

    QString playerName, uuid;

    QString answer;
    int haiAnswer;
    int buttonAnswer;
    int networkAnswerAction;
    int networkAnswerOptionChosen;

	int clearActions();
    int printSituation();
    int printHai(const int *te, int n);
    int printNaki(const int *te);
    int printTe(const int *te);
    int printMachi(const int* machi);
    int printHairi(const int *te, const int *hairi);

    int countVisible(int *hai);

    int playerType;

    const GameInfo* pGameInfo;
    TextDisp* pDisp;
    HaiButton* pHaiButton;

    QLibrary aiLib;
    AiLibInitFunc aiLibInit;
    AiLibAskFunc aiLibAsk;
    AiData aiLibData;
    void updateAiLibData();
    AiAnswer validateAiLibAnswer(AiAnswer ans, int firstAction);
};

#endif
