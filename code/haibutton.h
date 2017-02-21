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

#ifndef HAIBUTTON_H
#define HAIBUTTON_H

#include <QObject>
#include <QLabel>

#include "constants.h"

class HaiButton : public QObject
{
    Q_OBJECT
public:
    explicit HaiButton(QObject *parent = 0);

    void registerHaiButton(int haiInd, int answer, int nNaki, int halfShift);
    void clearHaiButtons();

    void registerActionButton(int answer);
    void clearActionButtons();

signals:
    void haiHoverChanged(int* haiHoverInd);
    void haiClicked(int answer);

    void buttonHoverChanged(int* buttonTypes, int hoverInd);
    void buttonClicked(int answer);

public slots:
    void processHaiHover(float x, float y);
    void processHaiClick(float x, float y);

    void processButtonHover(float x, float y);
    void processButtonClick(float x, float y);

private:

    int nHaiButton;
    int haiInd[IND_HAIBUTTONSIZE];
    int haiAnswer[IND_HAIBUTTONSIZE];
    float haiCoord[IND_HAIBUTTONSIZE * 4]; // [xleft, xright, ytop, ybot] for each button, max 25 buttons currently

    int prevAnswer;
    int haiHoverInd[4];

    float lastX, lastY;

    int nActionButton;
    int actionAnswer[IND_ACTIONBUTTONSIZE];
    int prevAction;
    int resetActionButtons;
};

#endif // HAIBUTTON_H
