#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QGraphicsScene>
#include <QGraphicsView>
#include <QTimer>
#include <QLabel>
#include <QPushButton>
#include <QSpinBox>
#include <QVector>
#include "qcustomplot.h"
#include "worldobject.h"

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();
    void drawForceField();
    void updatePetriVisuals();


private slots:
    void updateWorld();
    void startSimulation();
    void stopSimulation();
    void onSpawnClicked();

private:
    void setupUi();
    void spawnObjects();
    void updateGraph();
    QWidget *centralWidget;


    QGraphicsView *view;
    QGraphicsScene *scene;


    QLabel *statsLabel;
    QCustomPlot *customPlot;

    QSpinBox *sbHumanCount;
    QSpinBox *sbZombieCount;
    QPushButton *btnSpawn;

    QPushButton *btnStart;
    QPushButton *btnStop;

    QTimer *timer;
    QList<WorldObject*> objects;

    double simTime;
    QVector<double> timeData;
    QVector<double> humanData;
    QVector<double> zombieData;
    QSpinBox *sbSafetyRadius;
    QSpinBox *sbSearchRadius;
    QDoubleSpinBox *sbMaxSpeed;

    QSlider *slAlpha;
    QSlider *slMu;
    QCheckBox *cbPetri;


    QList<QGraphicsLineItem*> fieldLines;
    QGraphicsEllipseItem *petriCircle = nullptr;
    QGraphicsLineItem *gravityVector = nullptr;
};

#endif // MAINWINDOW_H
