/* This file is part of qterminalwidget and has been modified for HeLL IDE.
 *
 * This file has originally been written by simber86
 * and released at https://code.google.com/p/qterminalwidget/
 *
 * This is a modified version by Matthias Ernst for use within HeLL IDE.
 * It has been modified initially at Nov 2, 2014.
 *
 * qterminalwidget and HeLL IDE are free software: you can redistribute them
 * and/or modify them under the terms of the GNU General Public License as
 * published by the Free Software Foundation, either version 3 of the License,
 * or (at your option) any later version.
 *
 * HeLL IDE is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#include "qconsolewidget.h"
#include "qmalbolgedebugger.h"
#include <QKeyEvent>
#include <QProcess>
#include <QDebug>
#include <QMetaObject>
#include <QMimeData>
#include <QCoreApplication>


QConsoleWidget::QConsoleWidget(QWidget *parent) :
    QTextEdit(parent)
{
    setUndoRedoEnabled(false);
    setAcceptRichText(false);
    setAcceptDrops(false);
    fixedPosition = 0;
    setTextColor(Qt::black);
    setTextBackgroundColor(Qt::white);
    QFont font;
    font.setWeight(QFont::Normal);
    font.setFamily("monospace");
    font.setStyleHint(QFont::TypeWriter);
    setFont(font);
    this->t = NULL;
    this->child_process = NULL;
    this->suppress_output = false;
    this->_EOF = false;

    setContextMenuPolicy(Qt::NoContextMenu);
}

bool QConsoleWidget::is_running() {
    return (this->child_process != NULL);
}

bool QConsoleWidget::cleanUp() {
    if (this->t != NULL) {
        return false;
    }
    if (this->child_process != NULL) {
        return false;
    }

        clear();
        this->links.clear();
        setFontWeight(QFont::Normal);
        setTextColor(Qt::black);
        setFontUnderline(false);
        fixedPosition = 0;
        this->_EOF = false;
        return true;
}

void QConsoleWidget::execute(QIOWorker* thread_worker, QString output_on_start) {

    if (thread_worker == NULL)
        return;

    if (!this->cleanUp()) {
        delete thread_worker;
        return;
    }

    this->t = new QThread();
    if (this->t == NULL) {
        delete thread_worker;
        return;
    }

    this->suppress_output = false;
    OnChildInfoWrite(output_on_start);
    fixedPosition = textCursor().position();
    this->child_process = thread_worker;

    this->child_process->moveToThread(this->t);

    qRegisterMetaType<QProcess::ExitStatus>("QProcess::ExitStatus");

    connect(this->child_process, SIGNAL(StdErrWrite(QString)), this, SLOT(OnChildStdErrWrite(QString)));
    connect(this->child_process, SIGNAL(InfoWrite(QString)), this, SLOT(OnChildInfoWrite(QString)));
    connect(this->child_process, SIGNAL(StdOutWrite(QString)), this, SLOT(OnChildStdOutWrite(QString)),Qt::QueuedConnection);
    connect(this->child_process, SIGNAL(Terminated(int,QProcess::ExitStatus)), this, SLOT(OnChildTerminated(int, QProcess::ExitStatus)));
    if (qobject_cast<QMalbolgeDebugger*>(this->child_process)!=0) {
        connect(this->child_process, SIGNAL(clear_userinput()), this, SLOT(clear_userinput()));
    }


    connect(this->t, SIGNAL(started()), this->child_process, SLOT(Execute()));
    this->t->start();
    emit startExecute();

}

void QConsoleWidget::stop() {
    this->suppress_output = true;
    this->child_process->Stop();
    OnChildInfoWrite("Kill signal sent.\n", true);
}

void QConsoleWidget::keyPressEvent(QKeyEvent* event) {
    bool accept = false;
    int key = event->key();

    if (key == Qt::Key_C && event->modifiers() == Qt::ControlModifier){
        this->copy();
        accept = false;
    } else if (key == Qt::Key_A && event->modifiers() == Qt::ControlModifier){
        selectAll();
        accept = false;
    } else if (key == Qt::Key_V && event->modifiers() == Qt::ControlModifier){
        paste();
        accept = false;
    } else if (key == Qt::Key_X && event->modifiers() == Qt::ControlModifier){
        cut();
        accept = false;
    }else if (this->child_process != NULL && !this->_EOF) {
        if (key == Qt::Key_Backspace) {
            selection_to_modifyable_area(accept);
        } else if (key == Qt::Key_Delete){
            if (!textCursor().hasSelection()) {
                if (textCursor().position() >= fixedPosition) {
                    accept = true;
                } else {
                    accept = false;
                }
            }else{
                remove();
                accept = false;
            }
        } else if (key == Qt::Key_Return) {
            accept = false;
            int count = toPlainText().count() - fixedPosition;
            QString cmd = toPlainText().right(count);
            int left = cmd.length() - toPlainText().count() + textCursor().selectionStart();
            QString cmd2;
            if (left > 0)
                cmd2 = cmd.left(left);
            this->child_process->StdInRead(QString().append(cmd2).append("\n"));
            selection_to_modifyable_area();
            QTextEdit::keyPressEvent(event);
            fixedPosition = textCursor().position();
        } else if (key == Qt::Key_Up || event->key() == Qt::Key_Left || event->key() == Qt::Key_Down || event->key() == Qt::Key_Right || key == Qt::Key_Shift
                   || key == Qt::Key_Alt || key == Qt::Key_Home || key == Qt::Key_End || key == Qt::Key_PageUp || key == Qt::Key_PageDown || key == Qt::Key_Control
                   || key == Qt::Key_AltGr) {
            accept = true;
        } else if (key == Qt::Key_D && event->modifiers() == Qt::ControlModifier){
            // selection_to_modifyable_area();
            this->child_process->StdInEOF();
            QTextCursor cursor = textCursor();
            cursor.setPosition(fixedPosition);
            setTextCursor(cursor);
            moveCursor(QTextCursor::End, QTextCursor::KeepAnchor);
            textCursor().removeSelectedText();
            _EOF = true;
            accept = false;
        } else {
            selection_to_modifyable_area();
            accept = true;
        }
    }else{
        accept = false;
    }
    setFontWeight(QFont::Normal);
    setTextColor(Qt::black);
    if (accept) {
        QTextEdit::keyPressEvent(event);
    }
}

void QConsoleWidget::OnChildStdOutWrite(QString szOutput) {
    OnChildStdOutWrite(szOutput, false);
}

void QConsoleWidget::OnChildStdOutWrite(QString szOutput, bool force) {

    if (!force && this->suppress_output)
        return;
    QTextCursor cursor = textCursor();
    int c_offset = cursor.position() - fixedPosition;
    cursor.setPosition(fixedPosition);
    setTextCursor(cursor);
    textCursor().insertText(szOutput);

    fixedPosition = textCursor().position();
    if (c_offset > 0) {
        cursor = textCursor();
        cursor.setPosition(fixedPosition + c_offset);
        setTextCursor(cursor);
    }
    ensureCursorVisible();
}

void QConsoleWidget::OnChildInfoWrite(QString szOutput) {
    OnChildInfoWrite(szOutput, false);
}

void QConsoleWidget::OnChildInfoWrite(QString szOutput, bool force) {

    if (!force && this->suppress_output)
        return;
    QTextCursor cursor = textCursor();
    int c_offset = cursor.position() - fixedPosition;
    cursor.setPosition(fixedPosition);
    setTextCursor(cursor);
    setFontWeight(QFont::Bold);
    textCursor().insertText(szOutput);

    setFontWeight(QFont::Normal);
    fixedPosition = textCursor().position();
    if (c_offset > 0) {
        cursor = textCursor();
        cursor.setPosition(fixedPosition + c_offset);
        setTextCursor(cursor);
    }
    ensureCursorVisible();
}

void QConsoleWidget::OnChildStdErrWrite(QString szOutput) {
    OnChildStdErrWrite(szOutput, false);
}

void QConsoleWidget::OnChildStdErrWrite(QString szOutput, bool force) {

    if (!force && this->suppress_output)
        return;

    QTextCursor cursor = textCursor();
    int c_offset = cursor.position() - fixedPosition;
    cursor.setPosition(fixedPosition);
    setTextCursor(cursor);
    setTextColor(Qt::red);
    setFontWeight(QFont::Bold);

    QRegExp re("line ([0-9]+) column ([0-9]+)");
    int pos_re = 0;
    while ((pos_re = re.indexIn(szOutput)) >= 0) {
        LineLink link;
        link.line = re.cap(1).toInt();
        link.column = re.cap(2).toInt();
        textCursor().insertText(szOutput.left(pos_re));
        szOutput = szOutput.mid(pos_re);
        setFontUnderline(true);
        link.start = cursor.position();
        textCursor().insertText(szOutput.left(re.matchedLength()));
        link.end = cursor.position();
        setFontUnderline(false);
        szOutput = szOutput.mid(re.matchedLength());
        this->links.append(link);
    }
    textCursor().insertText(szOutput);


    setFontWeight(QFont::Normal);
    setTextColor(Qt::black);
    fixedPosition = textCursor().position();
    if (c_offset > 0) {
        cursor = textCursor();
        cursor.setPosition(fixedPosition + c_offset);
        setTextCursor(cursor);
    }
    ensureCursorVisible();
}

void QConsoleWidget::clear_userinput() {
    QTextCursor cursor = textCursor();
    cursor.setPosition(fixedPosition);
    setTextCursor(cursor);
    moveCursor(QTextCursor::End, QTextCursor::KeepAnchor);
    textCursor().removeSelectedText();
}

void QConsoleWidget::OnChildTerminated(int exitCode, QProcess::ExitStatus exitStatus) {

    if (exitStatus == QProcess::CrashExit) {
        OnChildStdErrWrite("Process crashed.\n", true);
    }else if (exitCode != 0) {
        OnChildStdErrWrite(QString("Process terminated with exit code ").append(QString().setNum(exitCode)).append(".\n"), true);
    }
    disconnect(this->child_process,0,0,0);
    disconnect(this->t, 0,this->child_process,0);
    while (!t->isFinished()) {
        QCoreApplication::processEvents();
    }

    OnChildInfoWrite(this->child_process->get_termination_message(),true);

    //this->child_process->moveToThread(QThread::currentThread());
    delete this->child_process;
    this->child_process = NULL;

    clear_userinput();

    delete this->t;
    this->t = NULL;
    this->suppress_output = false;
    emit stopExecute();
}

void QConsoleWidget::insertFromMimeData (const QMimeData* source) {

    if (this->child_process == NULL)
        return;

    selection_to_modifyable_area();
    setFontWeight(QFont::Normal);
    setTextColor(Qt::black);
    textCursor().insertText(source->text());
}

void QConsoleWidget::selection_to_modifyable_area(bool& extra_space_for_backspace) {
    QTextCursor cursor = textCursor();
    if (cursor.hasSelection()) {
        if (cursor.selectionStart() < fixedPosition) {
            int anchor = cursor.anchor();
            int pos = cursor.position();
            cursor.setPosition((anchor < fixedPosition?fixedPosition:anchor),QTextCursor::MoveAnchor);
            cursor.setPosition((pos < fixedPosition?fixedPosition:pos),QTextCursor::KeepAnchor);
        }
        extra_space_for_backspace = cursor.hasSelection();
    } else {
        if (cursor.position() < fixedPosition) {
            cursor.setPosition(fixedPosition,QTextCursor::MoveAnchor);
        }
        extra_space_for_backspace = (cursor.position() > fixedPosition);
    }
    if (extra_space_for_backspace)
        setTextCursor(cursor);
}

void QConsoleWidget::selection_to_modifyable_area() {
    QTextCursor cursor = textCursor();
    if (cursor.hasSelection()) {
        if (cursor.selectionStart() < fixedPosition) {
            int anchor = cursor.anchor();
            int pos = cursor.position();
            cursor.setPosition((anchor < fixedPosition?fixedPosition:anchor),QTextCursor::MoveAnchor);
            cursor.setPosition((pos < fixedPosition?fixedPosition:pos),QTextCursor::KeepAnchor);
        }
    } else {
        if (cursor.position() < fixedPosition) {
            cursor.setPosition(fixedPosition,QTextCursor::MoveAnchor);
        }
    }
    setTextCursor(cursor);
}

void QConsoleWidget::cut () {
    QTextEdit::copy();
    remove();
}

void QConsoleWidget::mousePressEvent(QMouseEvent *e) {
    this->setReadOnly(true);
    QTextEdit::mousePressEvent(e);
    this->setReadOnly(false);
}

void QConsoleWidget::mouseReleaseEvent(QMouseEvent *e) {
    if (e->button() == Qt::LeftButton) {
        QTextCursor cursor = cursorForPosition(e->pos());
        int line, column;
        bool link = this->getLinkDestination(cursor.position(), line, column);
        if (link) {
            emit gotoHeLLCodePosition(line, column);
        }
    }
    this->setReadOnly(true);
    QTextEdit::mouseReleaseEvent(e);
    this->setReadOnly(false);
}

void QConsoleWidget::mouseMoveEvent(QMouseEvent *e) {
    this->setReadOnly(true);
    QTextEdit::mouseMoveEvent(e);
    this->setReadOnly(false);
}

void QConsoleWidget::resizeEvent(QResizeEvent *e) {
    QTextEdit::resizeEvent(e);
    ensureCursorVisible();
}

bool QConsoleWidget::getLinkDestination(int textPos, int& line, int& column) {
    foreach (LineLink link, this->links) {
        if (link.start <= textPos && link.end >= textPos) {
            line = link.line;
            column = link.column;
            return true;
        }
    }
    return false;
}

void QConsoleWidget::remove() {
    if (textCursor().hasSelection()) {
        if (textCursor().selectionEnd() > fixedPosition) {
            selection_to_modifyable_area();
            textCursor().removeSelectedText();
        }
    }

}
