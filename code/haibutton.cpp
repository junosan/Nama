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

#include "haibutton.h"

HaiButton::HaiButton(QObject *parent) :
    QObject(parent)
{
    lastX = lastY = 0.0f;
    prevAnswer = -1;
    clearHaiButtons();
    prevAction = -1;
    clearActionButtons();
}

void HaiButton::clearHaiButtons()
{
    nHaiButton = 0;
    haiHoverInd[0] = haiHoverInd[1] = haiHoverInd[2] = haiHoverInd[3] = -1;
    processHaiHover(lastX, lastY);
}

void HaiButton::clearActionButtons()
{
    nActionButton = 0;
    actionAnswer[0] = actionAnswer[1] = actionAnswer[2] = actionAnswer[3] = actionAnswer[4] = -1;
    resetActionButtons = 1;
    processButtonHover(lastX, lastY);
}


void HaiButton::registerHaiButton(int _haiInd, int answer, int nNaki, int halfShift)
{
    static const float leftx = 6.689252E-01 - 13 * 4.852490E-02;
    static const float haiWidth = 4.852490E-02;
    static const float topy = 8.677150E-01;
    static const float boty = 9.541388E-01;

    haiInd[nHaiButton] = _haiInd;
    haiAnswer[nHaiButton] = answer;

    haiCoord[(nHaiButton << 2) + 0] = leftx + (_haiInd + halfShift * 0.5f) * haiWidth - (_haiInd == IND_TSUMOHAI) * 3 * nNaki * haiWidth;
    haiCoord[(nHaiButton << 2) + 1] = leftx + (_haiInd + 1 + halfShift * 0.5f) * haiWidth - (_haiInd == IND_TSUMOHAI) * 3 * nNaki * haiWidth;
    haiCoord[(nHaiButton << 2) + 2] = topy;
    haiCoord[(nHaiButton << 2) + 3] = boty;

    nHaiButton++;

//    processHaiHover(lastX, lastY);
}

void HaiButton::registerActionButton(int answer)
{
    actionAnswer[nActionButton++] = answer;

//    emit buttonHoverChanged(actionAnswer, -1);
//    processButtonHover(lastX, lastY);
}

void HaiButton::processHaiHover(float x, float y)
{
    if ((x < 0.0f) && (y < 0.0f)) {
        x = lastX;
        y = lastY;
        prevAnswer = 3 * IND_NAKISTATE; //  just a large number that cannot overlap with any real answer
    }

    lastX = x;
    lastY = y;

    int i;
    for (i = 0; i < nHaiButton; i++) {
        if ((x >= haiCoord[(i << 2) + 0]) && (x <= haiCoord[(i << 2) + 1]) && \
            (y >= haiCoord[(i << 2) + 2]) && (y <= haiCoord[(i << 2) + 3]))
            break;
    }

    if (i == nHaiButton) {
        if (prevAnswer != -1) {
            haiHoverInd[0] = haiHoverInd[1] = haiHoverInd[2] = haiHoverInd[3] = -1;
            prevAnswer = -1;
            emit haiHoverChanged(haiHoverInd);
        }
        return;
    }

    if (haiAnswer[i] != prevAnswer) {
        int nHover = 0;
        for (int j = 0; j < nHaiButton; j++)
            if (haiAnswer[j] == haiAnswer[i])
                haiHoverInd[nHover++] = haiInd[j];

        for (int j = nHover; j < 4; j++)
            haiHoverInd[j] = -1;

        prevAnswer = haiAnswer[i];

#ifdef FLAG_DEBUG
                printf("hover %d th out of %d hai buttons\n", haiAnswer[i], nHaiButton);
                fflush(stdout);
#endif

        emit haiHoverChanged(haiHoverInd);
        return;
    }

    return;
}

void HaiButton::processHaiClick(float , float )
{
    if (prevAnswer != -1) {
        emit haiClicked(prevAnswer);
    }
}

void HaiButton::processButtonClick(float , float )
{
    if (prevAction != -1) {
        emit buttonClicked(prevAction);
    }
}

void HaiButton::processButtonHover(float x, float y)
{
    static const float buttonWidth = 13.5f / 100.0f;
    static const float buttonHeight = 4.5f / 75.0f;
    static float startX = 12.0f / 100.0f - buttonWidth / 2; // x is 0 at left, 1 at right
    static float startY = 1.0f - (14.0f / 75.0f) - buttonHeight / 2; // y is 0 at top, 1 at bottom

    if ((x < 0.0f) && (y < 0.0f)) {
        x = lastX;
        y = lastY;
        prevAction = ACTION_PASS + 1; //  just a large number that cannot overlap with any real answer
    }

    lastX = x;
    lastY = y;

    int i;
    for (i = 0; i < nActionButton; i++) {
        if ((x >= startX + (4 - i) * buttonWidth) && (x <= startX + (5 - i) * buttonWidth) && \
            (y >= startY) && (y <= startY + buttonHeight))
            break;
    }

    if (i == nActionButton) {
        if ((prevAction != -1) || (resetActionButtons == 1)) {
            prevAction = -1;
            resetActionButtons = 0;
            emit buttonHoverChanged(actionAnswer, -1);
        }
        return;
    }

    if (actionAnswer[i] != prevAction) {
        prevAction = actionAnswer[i];
        emit buttonHoverChanged(actionAnswer, i);
        return;
    }

    return;
}

