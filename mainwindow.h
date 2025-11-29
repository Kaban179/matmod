#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QGraphicsScene>
#include <QGraphicsView>
#include <QTimer>
#include <QLabel>
#include <QPushButton>

#include "worldobject.h"

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    void updateWorld();
    void startSimulation();
    void stopSimulation();

private:
    void setupUi();
    void spawnObjects();

    QWidget *centralWidget;
    QGraphicsView *view;
    QGraphicsScene *scene;

    QLabel *statsLabel;

    QPushButton *btnStart;
    QPushButton *btnStop;

    QTimer *timer;
    QList<WorldObject*> objects;

    const int HUMAN_COUNT = 50;
    const int ZOMBIE_COUNT = 3;
};

#endif // MAINWINDOW_H
