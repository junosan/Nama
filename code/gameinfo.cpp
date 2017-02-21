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

#include "gameinfo.h"
#include "constants.h"


GameInfo::GameInfo(QObject *parent) :
    QObject(parent)
{
    initializeHanchan();

    bDrawBoard = 0;
    dispPlayer = 0;
}

void GameInfo::initializeHanchan()
{
    // hanchan
    haikyuu = 25000;
    genten = 30000;
    uma1 = 20; // for top & last rank
    uma2 = 10;
    ruleFlags = (RULE_USE_AKADORA | RULE_ATAMAHANE);

    ten[0] = ten[1] = ten[2] = ten[3] = haikyuu;
    chanfonpai = HAI_TON;
    kyoku = 0;
    nRichibou = nTsumibou = 0;

    initializeKyoku();
}

void GameInfo::initializeKyoku()
{
    // kyoku specific
    sai = 2;
    prevNaki = NAKI_NO_NAKI;
    curDir = 0;
    nYama = 136;
    nKan = 0;

    kyokuOffsetForDisplay = 0;

    for (int i = 0; i < 4; i ++) {
        richiSuccess[i] = 0;
        kawa[IND_KAWASIZE * i + IND_NKAWA] = 0;
        naki[(i << 4) + (0 << 2)] = naki[(i << 4) + (1 << 2)] = naki[(i << 4) + (2 << 2)] = naki[(i << 4) + (3 << 2)] = NAKI_NO_NAKI;
        for (int j = 0; j <= IND_TSUMOHAI; j++)
            te[IND_TESIZE * i + j] = HAI_EMPTY;
        for (int j = IND_NAKISTATE; j < IND_TESIZE; j++)
            te[IND_TESIZE * i + j] = 0;
    }

    for (int i = 0; i < 5; i++) {
        dorahyouji[i] = HAI_EMPTY;
        omotedora[i] = HAI_EMPTY;
        uradora[i] = HAI_EMPTY;
    }

    emit updateScreen();
}


void GameInfo::endKyoku(int endFlag, int renchan, int howanpai, int* tenDiffForPlayer)
{

    if ((endFlag == ENDFLAG_TSUMOHO) || (endFlag == ENDFLAG_RONHO))
    {
        int i, j, k, flags;

        for (i = 0; i <= 3; i++)
        {
            int agariDir = (curDir + i) & 0x03;
            int tenDiff[4] = {0, 0, 0, 0}; // ton/nan/sha/pe

            if ((flags = (te[IND_TESIZE * agariDir + IND_YAKUMANFLAGS] & 0x0FFFFFFF)))
            {
                int nYakuman = ((flags & FLAG_YAKUMAN_KOKUSHI) != 0) + ((flags & FLAG_YAKUMAN_SUANKO) != 0) + ((flags & FLAG_YAKUMAN_DAISANGEN) != 0) + \
                    ((flags & FLAG_YAKUMAN_TSUISO) != 0) + ((flags & FLAG_YAKUMAN_SHOUSUSHI) != 0) + ((flags & FLAG_YAKUMAN_DAISUSHI) != 0) + \
                    ((flags & FLAG_YAKUMAN_RYUISO) != 0) + ((flags & FLAG_YAKUMAN_CHINROTO) != 0) + ((flags & FLAG_YAKUMAN_SUKANTSU) != 0) + \
                    ((flags & FLAG_YAKUMAN_CHURENPOTO) != 0) + ((flags & FLAG_YAKUMAN_TENHO) != 0) + ((flags & FLAG_YAKUMAN_CHIHO) != 0);

                int ten = 8000 * nYakuman * ((2 + (agariDir == 0)) << 1);

                tenDiff[agariDir] += ten + nTsumibou * 300 + nRichibou * 1000;

                if (endFlag == ENDFLAG_RONHO)
                {
                    int tTen = ten;
                    if (te[IND_TESIZE * agariDir + IND_YAKUMANFLAGS] & FLAG_YAKUMAN_PAO)
                    {
                        int temp = 8000 * (2 + (agariDir == 0));
                        tenDiff[(agariDir + ((te[IND_TESIZE * agariDir + IND_YAKUMANFLAGS] & MASK_YAUMAN_PAOTURNDIFF) >> 28)) & 0x03] -= temp;
                        tTen -= temp;
                    }

                    tenDiff[curDir] -= tTen + nTsumibou * 300;

                }
                else // ENDFLAG_TSUMOHO
                {
                    int pao = 0;
                    if (te[IND_TESIZE * agariDir + IND_YAKUMANFLAGS] & FLAG_YAKUMAN_PAO)
                    {
                        int temp = 8000 * ((2 + (agariDir == 0)) << 1);
                        tenDiff[(agariDir + ((te[IND_TESIZE * agariDir + IND_YAKUMANFLAGS] & MASK_YAUMAN_PAOTURNDIFF) >> 28)) & 0x03] -= temp + nTsumibou * 300;
                        pao = 1;
                    }

                    for (k = 1; k <= 3; k++)
                    {
                        j = (agariDir + k) & 0x03;

                        int agariOya = (agariDir == 0);
                        int imOya = (j == 0);

                        tenDiff[j] -= (((8000 * (nYakuman - pao)) << agariOya) << imOya) + nTsumibou * 100 * (1 - pao);
                    }
                }

                for (j = 0; j < 4; j++)
                    tenDiffForPlayer[(j + kyoku) & 0x03] = tenDiff[j];

                emit sendEndKyokuBoard(endFlag, te + IND_TESIZE * agariDir, uradorahyouji, uradora, tenDiffForPlayer, ten, 0, 0);
                drawEndKyokuBoard(endFlag, te + IND_TESIZE * agariDir, uradorahyouji, uradora, tenDiffForPlayer, ten, 0, 0);

                // quickest agari player gets all richibou
                nRichibou = 0;
            }
            else if (te[IND_TESIZE * agariDir + IND_YAKUFLAGS])
            {
                int machi[IND_MACHISIZE], hairi[IND_NHAIRI + 1];
                GameThread::shanten(te + IND_TESIZE * agariDir, machi, hairi);

                int agari;
                if (endFlag == ENDFLAG_TSUMOHO)
                    agari = AGARI_TSUMO;
                else if (te[IND_TESIZE * agariDir + IND_NAKISTATE] & FLAG_TE_NOT_MENZEN)
                    agari = AGARI_KUIRON;
                else
                    agari = AGARI_MENZENRON;

#ifdef FLAG_DEBUG
                printf("iHairi %d\n", iHairi[agariDir]);
#endif

                int fu = GameThread::calcFu(te + IND_TESIZE * agariDir, hairi + IND_HAIRISIZE * iHairi[agariDir], agari, chanfonpai, HAI_TON + agariDir);
                int fan = GameThread::calcFan(te + IND_TESIZE * agariDir, te[IND_TESIZE * agariDir + IND_YAKUFLAGS], omotedora, uradora, te[IND_TESIZE * agariDir + IND_NAKISTATE] & FLAG_TE_NOT_MENZEN);
                int ten = GameThread::calcTen(fu, fan, (2 + (agariDir == 0)) << 1);

                tenDiff[agariDir] += nRichibou * 1000;

                if (endFlag == ENDFLAG_RONHO)
                {
                    tenDiff[agariDir] += ten + nTsumibou * 300;
                    tenDiff[curDir] -= ten + nTsumibou * 300;
                }
                else // tsumo
                {
                    int sum = 0;
                    for (k = 1; k <= 3; k++)
                    {
                        j = (agariDir + k) & 0x03;

                        int agariOya = (agariDir == 0);
                        int imOya = (j == 0);

                        int temp = GameThread::calcTen(fu, fan, 1 << (agariOya + imOya)) + nTsumibou * 100;
                        sum += temp;
                        tenDiff[j] -= temp;
                    }
                    tenDiff[agariDir] += sum;
                }

                for (j = 0; j < 4; j++)
                    tenDiffForPlayer[(j + kyoku) & 0x03] = tenDiff[j];

                emit sendEndKyokuBoard(endFlag, te + IND_TESIZE * agariDir, uradorahyouji, uradora, tenDiffForPlayer, ten, fan, fu);
                drawEndKyokuBoard(endFlag, te + IND_TESIZE * agariDir, uradorahyouji, uradora, tenDiffForPlayer, ten, fan, fu);

                // quickest agari player gets all richibou
                nRichibou = 0;
            }

            for (j = 0; j < 4; j++) {
                ten[j] += tenDiffForPlayer[j];
                tenDiffForPlayer[j] = 0;
            }

        }
    }
    else // ryukyoku
    {
        emit sendEndKyokuBoard(endFlag, te + IND_TESIZE * curDir, uradorahyouji, uradora, tenDiffForPlayer, 0, 0, 0);
        drawEndKyokuBoard(endFlag, te + IND_TESIZE * curDir, uradorahyouji, uradora, tenDiffForPlayer, 0, 0, 0);

        for (int j = 0; j < 4; j++)
            ten[j] += tenDiffForPlayer[j];
    }

    for (int j = 0; j < 4; j++)
        richiSuccess[j] = 0; // for display purposes only

    emit updateScreen();


    int endHanchan = 0;

    // tobi
    if ((ten[0] < 0) || (ten[1] < 0) || (ten[2] < 0) || (ten[3] < 0))
        endHanchan = 1;

    // normal end condition / sudden death
    if ((((chanfonpai == HAI_NAN) && (kyoku == 3)) || (chanfonpai > HAI_NAN)) && \
        ((ten[0] >= genten) || (ten[1] >= genten) || (ten[2] >= genten) || (ten[3] >= genten)) && \
            ((!renchan) || (renchan && ((ten[kyoku & 0x03] >= genten) &&
                                        (ten[kyoku & 0x03] > ten[(kyoku + 1) & 0x03]) &&
                                        (ten[kyoku & 0x03] > ten[(kyoku + 2) & 0x03]) &&
                                        (ten[kyoku & 0x03] > ten[(kyoku + 3) & 0x03])))))
        endHanchan = 1;

    if (!endHanchan) {
        if (renchan) {
            nTsumibou++;
        }
        else {
            if (howanpai)
                nTsumibou++; // nagare-n-honba
            else
                nTsumibou = 0;

            if(kyoku < 3) {
                kyoku++;
                kyokuOffsetForDisplay = -1;
            }
            else {
                if(chanfonpai < HAI_SHA) {
                    chanfonpai++;
                    kyoku = 0;
                    kyokuOffsetForDisplay = 3;
                }
                else {
                    endHanchan = 1; // sha 4 kyoku is the last possible
                }
            }
        }
    }

    // hanchan finished
    if(endHanchan) {

        emit endHanchanBoardDrawn();
        drawEndHanchanBoard();

        emit hanchanFinished();
    }
}

void GameInfo::drawEndHanchanBoard()
{
    int curY;
    int tokuten[4];

    // determine rank
    int sorted[4]; // player number sorted, highest to lowest (same ten -> player # order)
    for (int i = 0; i < 4; i++) {
        sorted[i] = i;
        for (int j = 0; j < i; j++) {
            if (ten[sorted[j]] < ten[i])
            {
                for (int k = i; k > j; k--)
                    sorted[k] = sorted[k - 1];
                sorted[j] = i;
                break;
            }
        }
    }

    // richi bou goes to 1st
    ten[sorted[0]] += nRichibou * 1000;

    // tokuten = (ten(<= 400 floor; >= 500 ceil) - genten) / 1000
    // * rounding of negative number -4500 / 1000 -> -4 in c
    for (int i = 0; i < 4; i++) {
        tokuten[i] = (ten[i] + 500 - genten) / 1000 - (ten[i] + 500 - genten < 0);

        int rank;
        for (rank = 0; rank < 4; rank++)
            if (sorted[rank] == i)
                break;

        int oka = ((genten - haikyuu) << 2) / 1000;
        switch(rank)
        {
        case 0:
            tokuten[i] += uma1 + oka * (oka >= 0);
            break;
        case 1:
            tokuten[i] += uma2;
            break;
        case 2:
            tokuten[i] += -uma2;
            break;
        case 3:
            tokuten[i] += -uma1 + oka * (oka < 0);
            break;
        }
    }

//        for (int i = 0; i < 4; i++) {
//            sprintf(str, "[#%d]&nbsp;Player&nbsp;%d:&nbsp;%+d&nbsp;ten&nbsp;(%+d)<br>", i, sorted[i], ten[sorted[i]], tokuten[sorted[i]]);
//            emit dispText(QString(str));
//        }
//        emit dispText(QString("<br><br><h1>[End of hanchan]</h1><br>"));

    board.load(":/images/board/summary_board.png");

    curY = 0;
    curY += haiHeight + 4;
    curY = drawCenteredText(tr("終局"), fontHeight << 1, curY);
    curY += haiHeight;

    for (int i = 0; i < 4; i++) {
        curY = drawTokutenBox(tokuten, sorted[i], curY);
    }

    curY += haiHeight;

    bDrawBoard = 1;
    boardHeight = (curY <= board.height() ? curY : board.height());
    emit updateScreen();

    mutex.lock();
    waitConfirm.wait(&mutex);
    bDrawBoard = 0;
    mutex.unlock();
}

void GameInfo::drawEndKyokuBoard(int endFlag, const int* te, const int* uradorahyouji, const int* uradora, const int* tenDiffForPlayer, int ten, int fan, int fu)
{
    QString text;
    int curY, j;

    if ((endFlag == ENDFLAG_TSUMOHO) || (endFlag == ENDFLAG_RONHO)) {
        int flags;
        if ((flags = (te[IND_YAKUMANFLAGS] & 0x0FFFFFFF))) {
            // summary board
            board.load(":/images/board/summary_board.png");

            curY = 0;
            curY += haiHeight;

            // te
            curY = drawTe(te, curY);
            curY += haiHeight;

             // yaku
            curY = drawYaku(te, 0, flags, NULL, NULL, te[IND_NAKISTATE] & FLAG_TE_NOT_MENZEN, curY);
            curY += haiHeight;

            // ten
            text = tr("役満  ");

            text.append(QString("%1").arg(ten));
            text.append(tr("点"));

            curY = drawCenteredText(text, 26, curY);
            curY += (haiHeight >> 2);


            curY = drawTenDiff(tenDiffForPlayer, curY);

            curY += haiHeight;
        }
        else
        {
            // summary board
            board.load(":/images/board/summary_board.png");

            curY = 0;
            curY += haiHeight;

            // te
            curY = drawTe(te, curY);
            curY += (haiHeight >> 2);

            // dora
            int tTe[7];
            {
                tTe[0] = tTe[1] = HAI_BACKSIDE;
                for (j = 0; j < 5; j++) {
                    if (omotedora[j] != HAI_EMPTY)
                        tTe[j + 2] = dorahyouji[j];
                    else
                        tTe[j + 2] = HAI_BACKSIDE;
                }

                drawHais(tTe, 7, (board.width() - haiWidth * 7) >> 1, curY);
                curY += haiHeight;

                curY += (haiHeight >> 2);
            }

            // yaku
            curY = drawYaku(te, te[IND_YAKUFLAGS], 0, omotedora, uradora, te[IND_NAKISTATE] & FLAG_TE_NOT_MENZEN, curY);
            curY += (haiHeight >> 2) + 6;

            // uradora
            if (te[IND_NAKISTATE] & FLAG_TE_RICHI) {
                tTe[0] = tTe[1] = HAI_BACKSIDE;
                for (j = 0; j < 5; j++) {
                    if (uradora[j] != HAI_EMPTY)
                        tTe[j + 2] = uradorahyouji[j];
                    else
                        tTe[j + 2] = HAI_BACKSIDE;
                }

                drawHais(tTe, 7, (board.width() - haiWidth * 7) >> 1, curY);
                curY += haiHeight;

                curY += (haiHeight >> 2);
            }

            // ten
            text = QString("%1").arg(fu != 25 ? ((fu + 9) / 10) * 10 : 25);
#ifdef FLAG_DEBUG
            text = QString("%1").arg(fu);
#endif
            text.append(tr("符"));
            text.append(QString("%1").arg(fan));
            text.append(tr("飜  "));

            switch(GameThread::calcTen(fu, fan, 4))
            {
            case 8000:
                text.append(tr("満貫  "));
                break;
            case 12000:
                text.append(tr("跳満  "));
                break;
            case 16000:
                text.append(tr("倍満  "));
                break;
            case 24000:
                text.append(tr("三倍満  "));
                break;
            case 32000:
                text.append(tr("役満  "));
                break;
            }

            text.append(QString("%1").arg(ten));
            text.append(tr("点"));

            curY = drawCenteredText(text, 26, curY);
            curY += (haiHeight >> 2);

            curY = drawTenDiff(tenDiffForPlayer, curY);

            curY += haiHeight;
        }
    }
    else
    {
        switch (endFlag)
        {
        case ENDFLAG_HOWANPAIPINCHU:
            text = tr("荒牌平局");
            break;
        case ENDFLAG_NAGASHIMANGAN:
            text = tr("流し満貫");
            break;
        case ENDFLAG_KYUSHUKYUHAI:
            text = tr("九種九牌");
            break;
        case ENDFLAG_SUKAIKAN:
            text = tr("四開槓");
            break;
        case ENDFLAG_SUFONRENTA:
            text = tr("四風子連打");
            break;
        case ENDFLAG_SUCHARICHI:
            text = tr("四家立直");
            break;
        case ENDFLAG_SANCHAHO:
            text = tr("三家和");
            break;
        default: // this should not happen
            text.clear();
            break;
        }

        board.load(":/images/board/summary_board.png");

        curY = 0;
        curY += haiHeight + 4;
        curY = drawCenteredText(text, fontHeight << 1, curY);
        curY += haiHeight;
        curY = drawTenDiff(tenDiffForPlayer, curY);
        curY += haiHeight;
    }

    bDrawBoard = 1;
    boardHeight = (curY <= board.height() ? curY : board.height());
    emit updateScreen();

    mutex.lock();
    waitConfirm.wait(&mutex);
    bDrawBoard = 0;
    mutex.unlock();
    emit updateScreen();
}


int GameInfo::drawTokutenBox(const int* tokuten, int playerInd, int curY)
{
    QPainter painter(&board);
    QImage box = QImage(":/images/board/miniboard.png").scaledToWidth(270, Qt::SmoothTransformation);

    painter.drawImage(QPoint((board.width() - box.width()) >> 1, curY), box);

    QFont font(FONT_JAPANESE);
    font.setPixelSize(16);
    painter.setFont(font);
    painter.setPen(QColor(Qt::white));

    QString text = playerNames[playerInd];
    painter.drawText(QPoint(board.width() >> 1, curY + 20) - QPoint(QFontMetrics(font).boundingRect(text).center()), text);

    text = QString("%1").arg(tokuten[playerInd]);
    if (tokuten[playerInd] > 0) {
        text.prepend(QChar('+'));
        painter.setPen(QColor(Qt::cyan));
    } else if (tokuten[playerInd] == 0) {
        text.prepend(QString("±"));
        painter.setPen(QColor(Qt::white));
    }
    else
        painter.setPen(QColor(Qt::red));

    painter.drawText(QPoint((board.width() >> 1) + 5, curY + 40 - QFontMetrics(font).boundingRect(text).center().y()), text);

    painter.setPen(QColor(Qt::white));
    text = QString("%1").arg(ten[playerInd]);
    int halfwidth = QFontMetrics(font).boundingRect(text).center().x();
    painter.drawText(QPoint((board.width() >> 1) - 5 - (halfwidth << 1), curY + 40 - QFontMetrics(font).boundingRect(text).center().y()), text);

    return curY + box.height() + 5;
}

void GameInfo::setDispPlayer(int playerInd)
{
    dispPlayer = (playerInd & 0x03);
}

int GameInfo::getDispPlayer()
{
    return dispPlayer;
}

int GameInfo::drawHais(const int* hai, int n, int curX, int curY)
{
    QPainter painter(&board);

    for (int i = 0; i < n; i++) {
        painter.drawImage(QPoint(curX, curY + (haiHeight - haiWidth) * ((hai[i] & FLAG_HAI_NOT_TSUMOHAI) != 0)), \
                          QImage(QString(":/images/board/%1.png").arg(hai[i] & (FLAG_HAI_AKADORA | FLAG_HAI_NOT_TSUMOHAI | MASK_HAI_NUMBER))));
        curX += haiWidth + (haiHeight - haiWidth) * ((hai[i] & FLAG_HAI_NOT_TSUMOHAI) != 0);
    }

    return curX;
}

int GameInfo::drawTe(const int* te, int curY)
{
    int gap = haiWidth >> 1;
    int nNaki = te[IND_NAKISTATE] & MASK_TE_NNAKI;
    int nHai = 13 - 3 * nNaki;
    int teWidth = 14 * haiWidth + gap;

    for (int i = 0; i < nNaki; i++) {
        switch((te[IND_NAKISTATE] >> (4 + 3 * i)) & 0x07)
        {
        case NAKI_PON:
        case NAKI_CHI:
        case NAKI_KAKAN:
            teWidth += haiHeight - haiWidth;
            break;
        case NAKI_ANKAN:
            teWidth += haiWidth;
            break;
        case NAKI_DAIMINKAN:
            teWidth += haiHeight;
            break;
        }
    }

    int curX = ((board.width() - teWidth) >> 1);

    curX = drawHais(te, nHai, curX, curY + (haiWidth << 1) - haiHeight);

    curX = drawHais(te + IND_TSUMOHAI, 1, curX, curY + (((haiWidth << 1) - haiHeight) >> 1));

    curX += gap;

    for (int i = nNaki - 1; i >= 0; i--) {
        int tempHai;
        switch((te[IND_NAKISTATE] >> (4 + 3 * i)) & 0x07)
        {
        case NAKI_PON:
        case NAKI_CHI:
            curX = drawHais(te + 10 - 3 * i, 3, curX, curY + (haiWidth << 1) - haiHeight);
            break;
        case NAKI_ANKAN:
            tempHai = HAI_BACKSIDE;
            curX = drawHais(&tempHai, 1, curX, curY + (haiWidth << 1) - haiHeight);
            curX = drawHais(te + 10 - 3 * i + 1, 1, curX, curY + (haiWidth << 1) - haiHeight);
            tempHai = te[10 - 3 * i + 2] & MASK_HAI_NUMBER;
            curX = drawHais(&tempHai, 1, curX, curY + (haiWidth << 1) - haiHeight);
            tempHai = HAI_BACKSIDE;
            curX = drawHais(&tempHai, 1, curX, curY + (haiWidth << 1) - haiHeight);
            break;
        case NAKI_DAIMINKAN:
            curX = drawHais(te + 10 - 3 * i + 0, 2, curX, curY + (haiWidth << 1) - haiHeight);
            tempHai = te[10 - 3 * i + 2] & MASK_HAI_NUMBER;
            curX = drawHais(&tempHai, 1, curX, curY + (haiWidth << 1) - haiHeight);
            curX = drawHais(te + 10 - 3 * i + 2, 1, curX, curY + (haiWidth << 1) - haiHeight);
            break;
        case NAKI_KAKAN:
            for (int j = 0; j < 3; j++) {
                curX = drawHais(te + 10 - 3 * i + j, 1, curX, curY + (haiWidth << 1) - haiHeight);
                if (te[10 - 3 * i + j] & FLAG_HAI_NOT_TSUMOHAI) {
                    tempHai = te[10 - 3 * i + j] & ~FLAG_HAI_AKADORA;
                    drawHais(&tempHai, 1, curX - haiHeight, curY - (haiHeight - haiWidth) + 3);
                }
            }
            break;
        }
    }

    return curY + (haiWidth << 1);
}


int GameInfo::drawTenDiff(const int* tenDiffForPlayer, int curY)
{
    static const int miniboardX = 180;
    static const int miniboardY = 40;
    static const int order[4] = {2, 3, 1, 0};

    QString kaze[4];
    kaze[0] = tr("東");
    kaze[1] = tr("南");
    kaze[2] = tr("西");
    kaze[3] = tr("北");

    const int xShift[4] = {0, -miniboardX * 9 / 10, miniboardX * 9 / 10, 0};
    const int yShift[4] = {0, miniboardY * 4 / 5, miniboardY * 4 / 5, miniboardY * 8 / 5};

    QString text;
    QPainter painter(&board);
    painter.setPen(QColor(Qt::white));
    QFont font(FONT_JAPANESE);

    font.setPixelSize(14);
    painter.setFont(font);

    text = QString("%1").arg(nTsumibou);
    painter.drawText(QPoint((board.width() >> 1) - 15 + 10, curY + miniboardY * 13 / 10 - 2) - QPoint(QFontMetrics(font).boundingRect(text).center()), text);

    text = QString("%1").arg(nRichibou);
    painter.drawText(QPoint((board.width() >> 1) + 15 + 10, curY + miniboardY * 13 / 10 - 2) - QPoint(QFontMetrics(font).boundingRect(text).center()), text);

    painter.save();
    painter.translate((board.width() >> 1) - 15 - 4, curY + miniboardY * 13 / 10);
    painter.rotate(90);
    painter.drawImage(QPoint(-10, -3), QImage(QString(":/images/tenbou/B100_mini.png")).scaledToWidth(20, Qt::SmoothTransformation));
    painter.restore();

    painter.save();
    painter.translate((board.width() >> 1) + 15 - 4, curY + miniboardY * 13 / 10);
    painter.rotate(90);
    painter.drawImage(QPoint(-10, -3), QImage(QString(":/images/tenbou/B1000_mini.png")).scaledToWidth(20, Qt::SmoothTransformation));
    painter.restore();

    for (int i = 0; i < 4; i++) {

        painter.drawImage(QPoint(xShift[i] + ((board.width() - miniboardX) >> 1), curY + yShift[i]), QImage(":/images/board/miniboard.png"));

        font.setFamily(FONT_CHINESE);
        font.setPixelSize(28);
        painter.setFont(font);
        text = kaze[(4 + dispPlayer + order[i] - kyoku) & 0x03];
        painter.drawText(QPoint(xShift[i] + ((board.width() - miniboardX) >> 1) + 4, curY + yShift[i] + miniboardY - 10), text);

        font.setFamily(FONT_JAPANESE);
        font.setPixelSize(14);
        painter.setFont(font);

        text = playerNames[(dispPlayer + order[i]) & 0x03];
        painter.drawText(QPoint(xShift[i] + (board.width() >> 1) - QFontMetrics(font).boundingRect(text).center().x() + 10, curY + yShift[i] + miniboardY - 23), text);

        if (tenDiffForPlayer[(dispPlayer + order[i]) & 0x03]) {

            text = QString("%1").arg(ten[(dispPlayer + order[i]) & 0x03]);
            int halfwidth = QFontMetrics(font).boundingRect(text).center().x();

            QString text2 = QString("%1").arg(tenDiffForPlayer[(dispPlayer + order[i]) & 0x03]);

            if (tenDiffForPlayer[(dispPlayer + order[i]) & 0x03] > 0) {
                text2.prepend(QChar('+'));
                painter.setPen(QColor(Qt::cyan));
            }
            else
                painter.setPen(QColor(Qt::red));

            painter.drawText(QPoint(xShift[i] + (board.width() >> 1) + 10 + 5, curY + yShift[i] + miniboardY - 7), text2);

            painter.setPen(QColor(Qt::white));
            painter.drawText(QPoint(xShift[i] + (board.width() >> 1) + 10 - 5 - (halfwidth << 1), curY + yShift[i] + miniboardY - 7), text);
        } else {
            text = QString("%1").arg(ten[(dispPlayer + order[i]) & 0x03]);
            painter.drawText(QPoint(xShift[i] + (board.width() >> 1) + 10 - QFontMetrics(font).boundingRect(text).center().x(), curY + yShift[i] + miniboardY - 7), text);
        }
    }


    return curY + miniboardY * 13 / 5;
}


void GameInfo::clearSummaryBoard()
{
    waitConfirm.wakeAll();
}


void GameInfo::setPlayerNames(QString player0, QString player1, QString player2, QString player3)
{
    playerNames[0] = player0;
    playerNames[1] = player1;
    playerNames[2] = player2;
    playerNames[3] = player3;
}

int GameInfo::printNaki(int* te)
{
    int nNaki, i, temp, nakiKind;

    nNaki = te[IND_NAKISTATE] & MASK_TE_NNAKI;

    // te[IND_NAKISTATE]
    // 3/3/3/3/1/3
    // (te[IND_NAKISTATE] >> (4 + 3 * i)) & 0x07

    for (i = nNaki - 1; i >= 0; i--)
    {
        nakiKind = (te[IND_NAKISTATE] >> (4 + 3 * i)) & 0x07;

        if (nakiKind == NAKI_ANKAN)
        {
            temp = HAI_BACKSIDE;
            printHai(&temp, 1);
            printHai(te + 10 - 3 * i + 1, 1);
        }
        else
        {
            printHai(te + 10 - 3 * i + 0, 2);
        }

        // added hai for kan's
        if ((nakiKind == NAKI_ANKAN) || (nakiKind == NAKI_DAIMINKAN) || (nakiKind == NAKI_KAKAN))
        {
            temp = te[10 - 3 * i + 0] & MASK_HAI_NUMBER;
            printHai(&temp, 1);
        }

        if (nakiKind == NAKI_ANKAN)
        {
            temp = HAI_BACKSIDE;
            printHai(&temp, 1);
        }
        else
        {
            printHai(te + 10 - 3 * i + 2, 1);
        }

        emit dispText(QString("&nbsp;"));
    }

    return 0;
}

int GameInfo::printTe(int* te)
{
    int nNaki, nHai;
    nNaki = te[IND_NAKISTATE] & MASK_TE_NNAKI;
    nHai = 13 - 3 * nNaki;

    printHai(te, nHai);
    emit dispText(QString("&nbsp;"));

    printHai(te + 13, 1);
    emit dispText(QString("&nbsp;"));

    printNaki(te);

    return 0;
}

int GameInfo::printHai(int* te, int n)
{
    int i;
    char tile;

    char textColor[16], bgColor[16], str[256];

    for (i = 0; i < n; i++)
    {
        if(te[i] & FLAG_HAI_AKADORA)
            sprintf(bgColor, "red");
        else if(te[i] & FLAG_HAI_NOT_TSUMOHAI)
            sprintf(bgColor, "lightgray");
        else
            sprintf(bgColor, "white");

        if ((te[i] & MASK_HAI_NUMBER) <= HAI_9S)
        {
            switch ((te[i] & MASK_HAI_NUMBER) / 9)
            {
            case 0: // manzu
                sprintf(textColor, "black"); // black text
                break;
            case 1: // pinzu
                sprintf(textColor, "dodgerblue"); // bright blue text
                break;
            case 2: // sozu
                sprintf(textColor, "limegreen"); // dark green text
                break;
            }
            sprintf(str, "<h2 style=\"color:%s;background-color:%s;\">%d</h2>", textColor, bgColor, (te[i] & MASK_HAI_NUMBER) % 9 + 1);
        }
        else
        {
            switch (te[i] & MASK_HAI_NUMBER)
            {
            case HAI_TON:
                sprintf(textColor, "black"); // black text
                tile = 'E';
                break;
            case HAI_NAN:
                sprintf(textColor, "black"); // black text
                tile = 'S';
                break;
            case HAI_SHA:
                sprintf(textColor, "black"); // black text
                tile = 'W';
                break;
            case HAI_PE:
                sprintf(textColor, "black"); // black text
                tile = 'N';
                break;
            case HAI_HAKU:
                sprintf(textColor, "gray"); // gray text
                tile = 'D';
                break;
            case HAI_HATSU:
                sprintf(textColor, "limegreen"); // dark green text
                tile = 'D';
                break;
            case HAI_CHUN:
                sprintf(textColor, "red"); // bright red text
                tile = 'D';
                break;
            case HAI_BACKSIDE:
                sprintf(bgColor, "darkkhaki"); // dark yellow background
                sprintf(textColor, "white");
                tile = '_';
                break;
            case HAI_EMPTY:
            default:
                sprintf(textColor, "white"); // white text
                tile = ' ';
                break;
            }
            sprintf(str, "<h2 style=\"color:%s;background-color:%s;\">%c</h2>", textColor, bgColor, tile);
        }
        emit dispText(QString(str));
    }

    return 0;
}

int GameInfo::printYaku(const int* te, int yaku, int yakuman, const int* omotedora, const int* uradora, int isNotMenzen)
{
    int i, j, nDora, naki;

    if (yakuman == 0x00000000)
    {
        if (yaku & FLAG_YAKU_RICHI)
            emit dispText(QString("richi&emsp;&emsp;1&nbsp;fan<br>"));

        if (yaku & FLAG_YAKU_DOUBLERICHI)
            emit dispText(QString("double richi&emsp;2&nbsp;fan<br>"));

        if (yaku & FLAG_YAKU_IPPATSU)
            emit dispText(QString("ippatsu&emsp;&emsp;1&nbsp;fan<br>"));

        if (yaku & FLAG_YAKU_MENZENTSUMO)
            emit dispText(QString("menzen tsumo&emsp;1&nbsp;fan<br>"));

        if (yaku & FLAG_YAKU_TANYAO)
            emit dispText(QString("tanyao&emsp;&emsp;1&nbsp;fan<br>"));

        if (yaku & FLAG_YAKU_PINFU)
            emit dispText(QString("pinfu&emsp;&emsp;1&nbsp;fan<br>"));

        if (yaku & FLAG_YAKU_IPEKO)
            emit dispText(QString("ipeko&emsp;&emsp;1&nbsp;fan<br>"));

        if (yaku & FLAG_YAKU_RYANPEKO)
            emit dispText(QString("ryanpeko&emsp;3&nbsp;fan<br>"));

        if (yaku & FLAG_YAKU_SHOUSANGEN)
            emit dispText(QString("shousangen&emsp;2&nbsp;fan<br>"));

        if (yaku & FLAG_YAKU_HAKU)
            emit dispText(QString("yakuhai: haku&emsp;1&nbsp;fan<br>"));

        if (yaku & FLAG_YAKU_HATSU)
            emit dispText(QString("yakuhai: hatsu&emsp;1&nbsp;fan<br>"));

        if (yaku & FLAG_YAKU_CHUN)
            emit dispText(QString("yakuhai: chun&emsp;1&nbsp;fan<br>"));

        if (yaku & FLAG_YAKU_CHANFONPAI)
            emit dispText(QString("chanfonpai&emsp;1&nbsp;fan<br>"));

        if (yaku & FLAG_YAKU_MENFONPAI)
            emit dispText(QString("menfonpai&emsp;1&nbsp;fan<br>"));

        if (yaku & FLAG_YAKU_RINSHAN)
            emit dispText(QString("rinshan&emsp;&emsp;1&nbsp;fan<br>"));

        if (yaku & FLAG_YAKU_CHANKAN)
            emit dispText(QString("chankan&emsp;&emsp;1&nbsp;fan<br>"));

        if (yaku & FLAG_YAKU_HAITEI)
            emit dispText(QString("haitei&emsp;&emsp;1&nbsp;fan<br>"));

        if (yaku & FLAG_YAKU_HOUTEI)
            emit dispText(QString("houtei&emsp;&emsp;1&nbsp;fan<br>"));

        if (yaku & FLAG_YAKU_SANSHOKU) {
            if (isNotMenzen)
                emit dispText(QString("sanshoku&emsp;1&nbsp;fan<br>"));
            else
                emit dispText(QString("sanshoku&emsp;2&nbsp;fan<br>"));
        }

        if (yaku & FLAG_YAKU_ITTSU) {
            if (isNotMenzen)
                emit dispText(QString("ittsu&emsp;&emsp;1&nbsp;fan<br>"));
            else
                emit dispText(QString("ittsu&emsp;&emsp;2&nbsp;fan<br>"));
        }

        if (yaku & FLAG_YAKU_CHANTA) {
            if (isNotMenzen)
                emit dispText(QString("chanta&emsp;&emsp;1&nbsp;fan<br>"));
            else
                emit dispText(QString("chanta&emsp;&emsp;2&nbsp;fan<br>"));
        }

        if (yaku & FLAG_YAKU_JUNCHAN) {
            if (isNotMenzen)
                emit dispText(QString("junchan&emsp;&emsp;2&nbsp;fan<br>"));
            else
                emit dispText(QString("junchan&emsp;&emsp;3&nbsp;fan<br>"));
        }

        if (yaku & FLAG_YAKU_CHITOI)
            emit dispText(QString("chitoi&emsp;&emsp;2&nbsp;fan<br>"));

        if (yaku & FLAG_YAKU_TOITOI)
            emit dispText(QString("toitoi&emsp;&emsp;2&nbsp;fan<br>"));

        if (yaku & FLAG_YAKU_SANANKO)
            emit dispText(QString("sananko&emsp;&emsp;2&nbsp;fan<br>"));

        if (yaku & FLAG_YAKU_HONROTO)
            emit dispText(QString("honroto&emsp;&emsp;2&nbsp;fan<br>"));

        if (yaku & FLAG_YAKU_SANDOKO)
            emit dispText(QString("sandoko&emsp;&emsp;2&nbsp;fan<br>"));

        if (yaku & FLAG_YAKU_SANKANTSU)
            emit dispText(QString("sankantsu&emsp;2&nbsp;fan<br>"));

        if (yaku & FLAG_YAKU_HONITSU) {
            if (isNotMenzen)
                emit dispText(QString("honitsu&emsp;&emsp;2&nbsp;fan<br>"));
            else
                emit dispText(QString("honitsu&emsp;&emsp;3&nbsp;fan<br>"));
        }

        if (yaku & FLAG_YAKU_CHINITSU) {
            if (isNotMenzen)
                emit dispText(QString("chinitsu&emsp;5&nbsp;fan<br>"));
            else
                emit dispText(QString("chinitsu&emsp;6&nbsp;fan<br>"));
        }

        nDora = 0;
        if (omotedora != NULL)
            for (i = 0; i < 5; i++)
            {
                if (omotedora[i] == HAI_EMPTY)
                    break;
                for (j = 0; j <= IND_TSUMOHAI; j++)
                    if ((te[j] & MASK_HAI_NUMBER) == omotedora[i])
                        nDora += 1;
                for (j = 0; j < (te[IND_NAKISTATE] & MASK_TE_NNAKI); j++)
                {
                    naki = (te[IND_NAKISTATE] >> (4 + 3 * j)) & 0x07;
                    if (((naki == NAKI_ANKAN) || (naki == NAKI_DAIMINKAN) || (naki == NAKI_KAKAN)) && \
                        ((te[10 - 3 * j] & MASK_HAI_NUMBER) == omotedora[i]))
                        nDora += 1;
                }
            }
        if ((uradora != NULL) && (te[IND_NAKISTATE] & FLAG_TE_RICHI))
            for (i = 0; i < 5; i++)
            {
                if (uradora[i] == HAI_EMPTY)
                    break;
                for (j = 0; j <= IND_TSUMOHAI; j++)
                    if ((te[j] & MASK_HAI_NUMBER) == uradora[i])
                        nDora += 1;
                for (j = 0; j < (te[IND_NAKISTATE] & MASK_TE_NNAKI); j++)
                {
                    naki = (te[IND_NAKISTATE] >> (4 + 3 * j)) & 0x07;
                    if (((naki == NAKI_ANKAN) || (naki == NAKI_DAIMINKAN) || (naki == NAKI_KAKAN)) && \
                        ((te[10 - 3 * j] & MASK_HAI_NUMBER) == uradora[i]))
                        nDora += 1;
                }
            }
        for (i = 0; i <= IND_TSUMOHAI; i++)
            if (te[i] & FLAG_HAI_AKADORA)
                nDora += 1;
        if (nDora) {
            char str[32];
            sprintf(str, "dora&emsp;&emsp;%d&nbsp;fan<br>", nDora);
            emit dispText(QString(str));
            }
    }
    else // print yakumans
    {
        if (yakuman & FLAG_YAKUMAN_KOKUSHI)
            emit dispText(QString("kokushimusou<br>"));

        if (yakuman & FLAG_YAKUMAN_SUANKO)
            emit dispText(QString("suanko<br>"));

        if (yakuman & FLAG_YAKUMAN_DAISANGEN)
            emit dispText(QString("daisangen<br>"));

        if (yakuman & FLAG_YAKUMAN_TSUISO)
            emit dispText(QString("tsuiso<br>"));

        if (yakuman & FLAG_YAKUMAN_SHOUSUSHI)
            emit dispText(QString("shousushi<br>"));

        if (yakuman & FLAG_YAKUMAN_DAISUSHI)
            emit dispText(QString("daisushi<br>"));

        if (yakuman & FLAG_YAKUMAN_RYUISO)
            emit dispText(QString("ryuiso<br>"));

        if (yakuman & FLAG_YAKUMAN_CHINROTO)
            emit dispText(QString("chinroto<br>"));

        if (yakuman & FLAG_YAKUMAN_SUKANTSU)
            emit dispText(QString("sukantsu<br>"));

        if (yakuman & FLAG_YAKUMAN_CHURENPOTO)
            emit dispText(QString("churenpoto<br>"));

        if (yakuman & FLAG_YAKUMAN_TENHO)
            emit dispText(QString("tenho<br>"));

        if (yakuman & FLAG_YAKUMAN_CHIHO)
            emit dispText(QString("chiho<br>"));
    }

    return 0;
}

int GameInfo::drawYakuLine(const QString &yaku, int fan, int fontHeight, int curY)
{
    QPainter painter(&board);
    painter.setPen(QColor(Qt::white));
    QFont font(FONT_CHINESE);
    font.setPixelSize(fontHeight);
    painter.setFont(font);

    curY += fontHeight;
    painter.drawText(QPoint((board.width() - 7 * haiWidth) >> 1, curY), yaku);

    QString text = (QString("%1").arg(fan)).append(tr("飜"));
    painter.drawText(QPoint(((board.width() + 7 * haiWidth) >> 1) - (QFontMetrics(font).boundingRect(text).center().x() << 1), curY), text);

    return curY;
}

int GameInfo::drawYaku(const int* te, int yaku, int yakuman, const int* omotedora, const int* uradora, int isNotMenzen, int curY)
{
    int i, j, nDora, naki;

    if (yakuman == 0x00000000)
    {
        int fontHeight = 26;

        if (yaku & FLAG_YAKU_RICHI)
            curY = drawYakuLine(tr("立直"), 1, fontHeight, curY);

        if (yaku & FLAG_YAKU_DOUBLERICHI)
            curY = drawYakuLine(tr("ダブル立直"), 2, fontHeight, curY);

        if (yaku & FLAG_YAKU_IPPATSU)
            curY = drawYakuLine(tr("一発"), 1, fontHeight, curY);

        if (yaku & FLAG_YAKU_MENZENTSUMO)
            curY = drawYakuLine(tr("門前清自摸和"), 1, fontHeight, curY);

        if (yaku & FLAG_YAKU_TANYAO)
            curY = drawYakuLine(tr("断么九"), 1, fontHeight, curY);

        if (yaku & FLAG_YAKU_PINFU)
            curY = drawYakuLine(tr("平和"), 1, fontHeight, curY);

        if (yaku & FLAG_YAKU_IPEKO)
            curY = drawYakuLine(tr("一盃口"), 1, fontHeight, curY);

        if (yaku & FLAG_YAKU_RYANPEKO)
            curY = drawYakuLine(tr("二盃口"), 3, fontHeight, curY);

        if (yaku & FLAG_YAKU_RINSHAN)
            curY = drawYakuLine(tr("嶺上開花"), 1, fontHeight, curY);

        if (yaku & FLAG_YAKU_CHANKAN)
            curY = drawYakuLine(tr("槍槓"), 1, fontHeight, curY);

        if (yaku & FLAG_YAKU_HAITEI)
            curY = drawYakuLine(tr("海底摸月"), 1, fontHeight, curY);

        if (yaku & FLAG_YAKU_HOUTEI)
            curY = drawYakuLine(tr("河底撈魚"), 1, fontHeight, curY);

        if (yaku & FLAG_YAKU_SANSHOKU) {
            if (isNotMenzen)
                curY = drawYakuLine(tr("三色同順"), 1, fontHeight, curY);
            else
                curY = drawYakuLine(tr("三色同順"), 2, fontHeight, curY);
        }

        if (yaku & FLAG_YAKU_ITTSU) {
            if (isNotMenzen)
                curY = drawYakuLine(tr("一気通貫"), 1, fontHeight, curY);
            else
                curY = drawYakuLine(tr("一気通貫"), 2, fontHeight, curY);
        }

        if (yaku & FLAG_YAKU_CHANTA) {
            if (isNotMenzen)
                curY = drawYakuLine(tr("混全帯么九"), 1, fontHeight, curY);
            else
                curY = drawYakuLine(tr("混全帯么九"), 2, fontHeight, curY);
        }

        if (yaku & FLAG_YAKU_JUNCHAN) {
            if (isNotMenzen)
                curY = drawYakuLine(tr("純全帯么九"), 2, fontHeight, curY);
            else
                curY = drawYakuLine(tr("純全帯么九"), 3, fontHeight, curY);
        }

        if (yaku & FLAG_YAKU_CHITOI)
            curY = drawYakuLine(tr("七対子"), 2, fontHeight, curY);

        if (yaku & FLAG_YAKU_TOITOI)
            curY = drawYakuLine(tr("対々和"), 2, fontHeight, curY);

        if (yaku & FLAG_YAKU_SANANKO)
            curY = drawYakuLine(tr("三暗刻"), 2, fontHeight, curY);

        if (yaku & FLAG_YAKU_HONROTO)
            curY = drawYakuLine(tr("混老頭"), 2, fontHeight, curY);

        if (yaku & FLAG_YAKU_SANDOKO)
            curY = drawYakuLine(tr("三色同刻"), 2, fontHeight, curY);

        if (yaku & FLAG_YAKU_SANKANTSU)
            curY = drawYakuLine(tr("三槓子"), 2, fontHeight, curY);

        if (yaku & FLAG_YAKU_HONITSU) {
            if (isNotMenzen)
                curY = drawYakuLine(tr("混一色"), 2, fontHeight, curY);
            else
                curY = drawYakuLine(tr("混一色"), 3, fontHeight, curY);
        }

        if (yaku & FLAG_YAKU_CHINITSU) {
            if (isNotMenzen)
                curY = drawYakuLine(tr("清一色"), 5, fontHeight, curY);
            else
                curY = drawYakuLine(tr("清一色"), 6, fontHeight, curY);
        }

        if (yaku & FLAG_YAKU_SHOUSANGEN)
            curY = drawYakuLine(tr("小三元"), 2, fontHeight, curY);

        if (yaku & FLAG_YAKU_HAKU)
            curY = drawYakuLine(tr("役牌 白"), 1, fontHeight, curY);

        if (yaku & FLAG_YAKU_HATSU)
            curY = drawYakuLine(tr("役牌 發"), 1, fontHeight, curY);

        if (yaku & FLAG_YAKU_CHUN)
            curY = drawYakuLine(tr("役牌 中"), 1, fontHeight, curY);

        if (yaku & FLAG_YAKU_CHANFONPAI)
            curY = drawYakuLine(tr("場風牌"), 1, fontHeight, curY);

        if (yaku & FLAG_YAKU_MENFONPAI)
            curY = drawYakuLine(tr("自風牌"), 1, fontHeight, curY);

        nDora = 0;
        if (omotedora != NULL)
            for (i = 0; i < 5; i++)
            {
                if (omotedora[i] == HAI_EMPTY)
                    break;
                for (j = 0; j <= IND_TSUMOHAI; j++)
                    if ((te[j] & MASK_HAI_NUMBER) == omotedora[i])
                        nDora += 1;
                for (j = 0; j < (te[IND_NAKISTATE] & MASK_TE_NNAKI); j++)
                {
                    naki = (te[IND_NAKISTATE] >> (4 + 3 * j)) & 0x07;
                    if (((naki == NAKI_ANKAN) || (naki == NAKI_DAIMINKAN) || (naki == NAKI_KAKAN)) && \
                        ((te[10 - 3 * j] & MASK_HAI_NUMBER) == omotedora[i]))
                        nDora += 1;
                }
            }
        if (nDora)
            curY = drawYakuLine(tr("ドラ"), nDora, fontHeight, curY);

        nDora = 0;
        for (i = 0; i <= IND_TSUMOHAI; i++)
            if (te[i] & FLAG_HAI_AKADORA)
                nDora += 1;
        if (nDora)
            curY = drawYakuLine(tr("赤ドラ"), nDora, fontHeight, curY);

        nDora = 0;
        if ((uradora != NULL) && (te[IND_NAKISTATE] & FLAG_TE_RICHI))
            for (i = 0; i < 5; i++)
            {
                if (uradora[i] == HAI_EMPTY)
                    break;
                for (j = 0; j <= IND_TSUMOHAI; j++)
                    if ((te[j] & MASK_HAI_NUMBER) == uradora[i])
                        nDora += 1;
                for (j = 0; j < (te[IND_NAKISTATE] & MASK_TE_NNAKI); j++)
                {
                    naki = (te[IND_NAKISTATE] >> (4 + 3 * j)) & 0x07;
                    if (((naki == NAKI_ANKAN) || (naki == NAKI_DAIMINKAN) || (naki == NAKI_KAKAN)) && \
                        ((te[10 - 3 * j] & MASK_HAI_NUMBER) == uradora[i]))
                        nDora += 1;
                }
            }
        if (nDora)
            curY = drawYakuLine(tr("裏ドラ"), nDora, fontHeight, curY);
    }
    else // print yakumans
    {
        int fontHeight = 40;
        int gap = 5;

        if (yakuman & FLAG_YAKUMAN_KOKUSHI)
            curY = drawCenteredText(tr("国士無双"), fontHeight, curY + gap) + gap;

        if (yakuman & FLAG_YAKUMAN_SUANKO)
            curY = drawCenteredText(tr("四暗刻"), fontHeight, curY + gap) + gap;

        if (yakuman & FLAG_YAKUMAN_DAISANGEN)
            curY = drawCenteredText(tr("大三元"), fontHeight, curY + gap) + gap;

        if (yakuman & FLAG_YAKUMAN_TSUISO)
            curY = drawCenteredText(tr("字一色"), fontHeight, curY + gap) + gap;

        if (yakuman & FLAG_YAKUMAN_SHOUSUSHI)
            curY = drawCenteredText(tr("小四喜"), fontHeight, curY + gap) + gap;

        if (yakuman & FLAG_YAKUMAN_DAISUSHI)
            curY = drawCenteredText(tr("大四喜"), fontHeight, curY + gap) + gap;

        if (yakuman & FLAG_YAKUMAN_RYUISO)
            curY = drawCenteredText(tr("緑一色"), fontHeight, curY + gap) + gap;

        if (yakuman & FLAG_YAKUMAN_CHINROTO)
            curY = drawCenteredText(tr("清老頭"), fontHeight, curY + gap) + gap;

        if (yakuman & FLAG_YAKUMAN_SUKANTSU)
            curY = drawCenteredText(tr("四槓子"), fontHeight, curY + gap) + gap;

        if (yakuman & FLAG_YAKUMAN_CHURENPOTO)
            curY = drawCenteredText(tr("九蓮宝燈"), fontHeight, curY + gap) + gap;

        if (yakuman & FLAG_YAKUMAN_TENHO)
            curY = drawCenteredText(tr("天和"), fontHeight, curY + gap) + gap;

        if (yakuman & FLAG_YAKUMAN_CHIHO)
            curY = drawCenteredText(tr("地和"), fontHeight, curY + gap) + gap;
    }

    return curY;
}

int GameInfo::drawCenteredText(const QString &text, int fontHeight, int curY)
{
    QPainter painter(&board);
    QFont font(FONT_CHINESE);

    curY += (fontHeight >> 1);

    painter.setPen(QColor(Qt::white));
    font.setPixelSize(fontHeight);
    painter.setFont(font);
    painter.drawText(QPoint(board.width() >> 1, curY) - QPoint(QFontMetrics(font).boundingRect(text).center()), text);

    curY += (fontHeight >> 1);

    return curY;
}
