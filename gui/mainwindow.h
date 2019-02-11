#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QTableWidget>
#include "Vhrmcpu.h"

extern vluint64_t main_time;

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();

protected:
    void keyPressEvent(QKeyEvent *e);

private slots:
    void clkTick();

    void on_pbB_toggled(bool checked);

    void on_pbA_pressed();

    void on_clkPeriod_valueChanged(int period);

    void on_pbReset_toggled(bool checked);

    void on_pbRcommit_pressed();

    void on_pbSave_pressed();

private:
    Ui::MainWindow *ui;
    QTimer *m_timer;
    bool clk;
    int counter;
    Vhrmcpu * top;

    QStringList m_TableHeader;

    void updateUI();
};

#endif // MAINWINDOW_H
