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

#ifndef TEXTEDIT_H
#define TEXTEDIT_H

#include <QPlainTextEdit>

#include "textdisp.h"
#include "gamethread.h"

class TextEdit : public QPlainTextEdit
{
    Q_OBJECT

public:
    explicit TextEdit(QObject *parent = 0);

signals:
    void textEntered(QString str);

private slots:

protected:
    void keyReleaseEvent(QKeyEvent * e);
};

#endif // TEXTEDIT_H
