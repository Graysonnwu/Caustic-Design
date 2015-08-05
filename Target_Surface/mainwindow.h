#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include "global.h"
#include <QMainWindow>
#include "SurfaceModel.h"
#include "rendering.h"
#include "ui_mainwindow.h"

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow, public Ui_MainWindow
{
    Q_OBJECT


public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();
    void setModel();

private:
    //Ui::MainWindow *ui;
    //Renderer* render;

protected slots:
    void on_actionSaveModel_triggered();
    void on_actionLoadModel_triggered();
    //void on_actionCreate_Object_triggered();
};

#endif // MAINWINDOW_H