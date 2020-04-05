/* This file is part of qterminalwidget and has been modified for HeLL IDE.
 *
 * This file has originally been written by simber86
 * and released at https://code.google.com/p/qterminalwidget/
 *
 * This is a modified version by Matthias Lutter for use within HeLL IDE.
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

#ifndef QCONSOLEWIDGET_H
#define QCONSOLEWIDGET_H

#include <QTextEdit>
#include <QThread>
#include "qioprocessworker.h"

typedef struct LineLink {
    int start;
    int end;
    int line;
    int column;
} LineLink;

class QConsoleWidget : public QTextEdit
{
    Q_OBJECT
public:
    explicit QConsoleWidget(QWidget *parent = 0);
    void execute(QIOWorker* thread_worker, QString output_on_start);
    void stop();
    bool is_running();
    bool cleanUp();
    bool getLinkDestination(int textPos, int& line, int& column);

private:
    volatile int fixedPosition;
    // TODO: redirect
    QIOWorker* child_process;
    QThread* t;
    QList<LineLink> links;
    bool _EOF;
    volatile bool suppress_output;
    void OnChildStdOutWrite(QString out, bool force);
    void OnChildInfoWrite(QString out, bool force);
    void OnChildStdErrWrite(QString out, bool force);


protected:
    void keyPressEvent(QKeyEvent* event);
    void insertFromMimeData (const QMimeData* source);
    void selection_to_modifyable_area();
    void selection_to_modifyable_area(bool& extra_space_for_backspace);
    void mouseReleaseEvent(QMouseEvent *e);
    void mousePressEvent(QMouseEvent *e);
    void mouseMoveEvent(QMouseEvent *e);
    void resizeEvent(QResizeEvent *e);

signals:
    void startExecute();
    void stopExecute();
    void gotoHeLLCodePosition(int line, int column);

public slots:
    void OnChildStdOutWrite(QString out);
    void OnChildInfoWrite(QString out);
    void OnChildStdErrWrite(QString out);
    void OnChildTerminated(int exitCode, QProcess::ExitStatus exitStatus);
    void cut();
    void remove();
    void clear_userinput();


};

#endif // QCONSOLEWIDGET_H
