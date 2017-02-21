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

#include "textedit.h"

TextEdit::TextEdit(QObject *)
{

}

void TextEdit::keyReleaseEvent(QKeyEvent * e)
{
    QString t;

    if((e->key() == Qt::Key_Return || e->key() == Qt::Key_Enter) && !((e->modifiers()) & (Qt::ShiftModifier))) {
        t = toPlainText();
#ifdef Q_OS_WIN
        t.remove(t.lastIndexOf(QChar('\n')), 1);
#endif
        emit textEntered(t);
        clear();
//        QTimer::singleShot(1000, this, SLOT(emitSignal()));
    }
    else
    {
        QPlainTextEdit::keyReleaseEvent(e);
    }
}
