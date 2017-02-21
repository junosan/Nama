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

#ifndef GAMEINFO_H
#define GAMEINFO_H

#include <QObject>

#include "constants.h"
#include "gamethread.h"
#include "glwidget.h"
#include "textdisp.h"

class GameInfo : public QObject
{
    Q_OBJECT

public:
    explicit GameInfo(QObject *parent = 0);

    void setDispPlayer(int playerInd);
    int getDispPlayer();

    void initializeHanchan();
    void initializeKyoku();
    void endKyoku(int endFlag, int renchan, int howanpai, int* tenDiffForPlayer);
    void setPlayerNames(QString player0, QString player1, QString player2, QString player3); // player #

    void drawEndKyokuBoard(int endFlag, const int* te, const int* uradorahyouji, const int* uradora, const int* tenDiffForPlayer, int ten, int fan, int fu);
    void drawEndHanchanBoard();

    //int isTsumohaiAvail(int dir) const;

    int nRichibou, nTsumibou;
    int chanfonpai, kyoku, kyokuOffsetForDisplay;
    int ten[4]; // player #
    int ruleFlags, sai;

    int kawa[IND_KAWASIZE * 4];
    int naki[64]; // grouped into 4 ints for each naki [nakiType, hai0, hai1, hai2] for each dir (i.e. 16 ints per dir)
    int dorahyouji[5], omotedora[5];
    int nYama, nKan, curDir, prevNaki;
    int richiSuccess[4];
    int isTsumohaiAvail[4];

    // not shared with GameThread
    int haikyuu, genten, uma1, uma2;

    QImage board;
    int bDrawBoard, boardHeight;

signals:
    void dispText(QString str);
    void updateScreen();
    void endHanchanBoardDrawn();
    void hanchanFinished();
    void sendEndKyokuBoard(int endFlag, const int* te, const int* uradorahyouji, const int* uradora, const int* tenDiffForPlayer, int ten, int fan, int fu);

public slots:
    void clearSummaryBoard();

private:
    QMutex mutex;
    QWaitCondition waitConfirm;
    friend class GameThread;

    int printTe(int* te);
    int printHai(int* te, int n);
    int printNaki(int* te);
    int printYaku(const int* te, int yaku, int yakuman, const int* omotedora, const int* uradora, int isNotMenzen);

    int drawHais(const int* hai, int n, int curX, int curY); // returns new curX
    int drawTe(const int* te, int curY); // returns new curY
    int drawYaku(const int* te, int yaku, int yakuman, const int* omotedora, const int* uradora, int isNotMenzen, int curY); // returns new curY
    int drawYakuLine(const QString &yaku, int fan, int fontHeight, int curY); // returns new curY
    int drawTenDiff(const int* tenDiffForPlayer, int curY); // returns new curY
    int drawCenteredText(const QString &text, int fontHeight, int curY); // returns new curY
    int drawTokutenBox(const int* tokuten, int playerInd, int curY); // returns new curY

    // only the server needs these
    int iHairi[4];
    int te[IND_TESIZE * 4];
    int wanpai[14];
    int uradorahyouji[5], uradora[5];
    //

    int dispPlayer, isDispPlayerAuto;
    QString playerNames[4]; // player #

    // unit: pixels
    static const int haiWidth = 30;
    static const int haiHeight = 40;
    static const int fontHeight = 30;

};

#endif // GAMEINFO_H
