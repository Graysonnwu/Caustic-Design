﻿#include "global.h"
// Qt
#include <QtGui>
#include <QDialog>
#include <QActionGroup>
#include <QFileDialog>
#include <QInputDialog>
#include <QClipboard>

#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "SurfaceModel.h"

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent), Ui_MainWindow()//,render(new Renderer(5, this, "Target Surface"))
{
    setupUi(this);

    connect(meshHeightLineEdit, SIGNAL(returnPressed()), this, SLOT(newMeshHeight()));
    //connect(focalLengthLineEdit, SIGNAL(returnPressed()), this, SLOT(newFocalLength()));

    connect(focalLengthLineEditX, SIGNAL(returnPressed()), this, SLOT(newFocalPlanePosX()));
    connect(focalLengthLineEditY, SIGNAL(returnPressed()), this, SLOT(newFocalPlanePosY()));
    connect(focalLengthLineEditZ, SIGNAL(returnPressed()), this, SLOT(newFocalPlanePosZ()));

    connect(focalLengthLineEditRY, SIGNAL(returnPressed()), this, SLOT(newFocalPlaneRotY()));
    connect(focalLengthLineEditRZ, SIGNAL(returnPressed()), this, SLOT(newFocalPlaneRotZ()));

    connect(focalLengthLineEditSY, SIGNAL(returnPressed()), this, SLOT(newFocalPlaneScaleY()));
    connect(focalLengthLineEditSZ, SIGNAL(returnPressed()), this, SLOT(newFocalPlaneScaleZ()));

    //setModel();
    //viewer = new Renderer(5, this, "Target Surface");
    optimizer = new TargetOptimization();
}

MainWindow::~MainWindow()
{
    //delete ui;
    delete optimizer;
}

void MainWindow::setModel()
{
    QString modelToLoad = QFileDialog::getOpenFileName(this, tr("Open 3D model to load"), ".");
    if(modelToLoad.isEmpty()) return;
    viewer->model.loadModel(modelToLoad.toStdString());
    update();
}

void MainWindow::on_actionSaveModel_triggered()
{
    std::cout << "save model" << std::endl;
    QString saveModelName = QFileDialog::getSaveFileName(this, tr("Save 3D model"));
    if(saveModelName.isEmpty()) return;
    viewer->model.exportModel(saveModelName.toUtf8().constData());
}

void MainWindow::on_actionSave_Vertices_triggered()
{
    std::cout << "save vertices" << std::endl;
    QString fileName = QFileDialog::getSaveFileName(this, tr("Save vertices"));
    if(fileName.isEmpty()) return;
    viewer->model.meshes[0].exportVertices(fileName, viewer->model.surfaceSize);
}

void MainWindow::on_actionLoadModel_triggered()
{
    setModel();
}

void MainWindow::on_actionGenerateTriangles_triggered()
{
    viewer->model.modifyMesh();
    viewer->update();
}

void MainWindow::on_actionRunTargetOptimization_triggered()
{
    optimizer->runOptimization(&(viewer->model),viewer);
}

void MainWindow::on_actionLoadLightRayReceiverPosition_triggered()
{
    QString filename = QFileDialog::getOpenFileName(this, tr("Open .dat File"), ".");
    if(filename.isEmpty()) return;
    viewer->model.loadReceiverLightPoints(filename);
    viewer->sceneUpdate();
}


void MainWindow::on_actionAxis_toggled()
{
    viewer->toggleDrawAxis();
    viewer->update();
}

void MainWindow::on_actionNormals_toggled()
{
    viewer->toggleDrawNormals();
    viewer->update();
}

void MainWindow::on_actionDesired_Normals_toggled(){
    viewer->toggleDrawDesiredNormals();
    viewer->update();
}

void MainWindow::on_actionDesired_Rays_toggled()
{
    viewer->toggleDrawDesiredRays();
    viewer->update();
}

void MainWindow::on_actionDesired_Ray_Directions_toggled()
{
    viewer->toggleDrawDesiredRayDirections();
    viewer->update();
}

void MainWindow::newMeshHeight()
{
    std::string txt = meshHeightLineEdit->text().toStdString();
    bool ok;

    float newScale = meshHeightLineEdit->text().toFloat(&ok);
    if(!ok)
    {
        std::cerr << "illegal value" << std::endl;
        return;
    }

    meshWidthLineEdit->setText(meshHeightLineEdit->text());

    newScale *= 0.5; // we go from -0.5 to 0.5
    std::cout << "New Mesh Height: " << newScale << std::endl;

    viewer->model.rescaleMeshes(newScale);
    viewer->model.surfaceSize = newScale;

    viewer->update();
}

void MainWindow::newFocalPlanePosX()
{
    std::string txt = focalLengthLineEditX->text().toStdString();
    bool ok;

    float newLength = focalLengthLineEditX->text().toFloat(&ok);
    if(!ok)
    {
        std::cerr << "illegal value" << std::endl;
        return;
    }

    viewer->model.setFocalPlanePosX(newLength);
    viewer->sceneUpdate();
}

void MainWindow::newFocalPlanePosY()
{
    std::string txt = focalLengthLineEditY->text().toStdString();
    bool ok;

    float newLength = focalLengthLineEditY->text().toFloat(&ok);
    if(!ok)
    {
        std::cerr << "illegal value" << std::endl;
        return;
    }

    viewer->model.setFocalPlanePosY(newLength);
    viewer->sceneUpdate();
}

void MainWindow::newFocalPlanePosZ()
{
    std::string txt = focalLengthLineEditZ->text().toStdString();
    bool ok;

    float newLength = focalLengthLineEditZ->text().toFloat(&ok);
    if(!ok)
    {
        std::cerr << "illegal value" << std::endl;
        return;
    }

    viewer->model.setFocalPlanePosZ(newLength);
    viewer->sceneUpdate();
}

void MainWindow::newFocalPlaneRotY() 
{
    std::string txt = focalLengthLineEditRY->text().toStdString();
    bool ok;

    float newLength = focalLengthLineEditRY->text().toFloat(&ok);
    if(!ok)
    {
        std::cerr << "illegal value" << std::endl;
        return;
    }

    viewer->model.setFocalPlaneRotY(newLength * (3.14159265f/180.0f));
    viewer->model.updateTargetPlaneRotationMatrix();
    viewer->sceneUpdate();
}

void MainWindow::newFocalPlaneRotZ()
{
    std::string txt = focalLengthLineEditRZ->text().toStdString();
    bool ok;

    float newLength = focalLengthLineEditRZ->text().toFloat(&ok);
    if(!ok)
    {
        std::cerr << "illegal value" << std::endl;
        return;
    }

    viewer->model.setFocalPlaneRotZ(newLength * (3.14159265f/180.0f));
    viewer->model.updateTargetPlaneRotationMatrix();
    viewer->sceneUpdate();
}

void MainWindow::newFocalPlaneScaleY()
{
    std::string txt = focalLengthLineEditSY->text().toStdString();
    bool ok;

    float newLength = focalLengthLineEditSY->text().toFloat(&ok);
    if(!ok)
    {
        std::cerr << "illegal value" << std::endl;
        return;
    }

    viewer->model.setFocalPlaneScaleY(newLength);
    viewer->sceneUpdate();
}

void MainWindow::newFocalPlaneScaleZ()
{
    std::string txt = focalLengthLineEditSZ->text().toStdString();
    bool ok;

    float newLength = focalLengthLineEditSZ->text().toFloat(&ok);
    if(!ok)
    {
        std::cerr << "illegal value" << std::endl;
        return;
    }

    viewer->model.setFocalPlaneScaleZ(newLength);
    viewer->sceneUpdate();
}

void MainWindow::on_actionShoot_Ray_toggled(){
    vector<glm::highp_dvec3> direction;
    vector<glm::highp_dvec3> redirect;
    vector<glm::highp_dvec3> endpoint;
    viewer->model.shootRay(direction, redirect, endpoint);
    viewer->setRay(direction, redirect, endpoint);
    viewer->update();
}

void MainWindow::on_actionExit_triggered()
{
    QApplication::quit();
}
