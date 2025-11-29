#include "worldobject.h"
#include <QRandomGenerator>
#include <cmath>
#include <QDebug>

// --- WorldObject ---

WorldObject::WorldObject(QObject *parent) : QObject(parent) {
    _radius = 5.0;
    _curState.pos = {0, 0};
    _curState.vel = {0, 0};
}

QRectF WorldObject::boundingRect() const {
    return QRectF(-_radius, -_radius, _radius*2, _radius*2);
}

void WorldObject::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget) {
    Q_UNUSED(option); Q_UNUSED(widget);
    painter->drawEllipse(boundingRect());
}

void WorldObject::setPosition(double x, double y) {
    _curState.pos[0] = x;
    _curState.pos[1] = y;
    setPos(x, y);
}

// --- Human ---

Human::Human(QObject *parent) : WorldObject(parent) {
    _type = HUMAN;
    _curState._curStatus = NORMAL;
}


void Human::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget) {
    Q_UNUSED(option); Q_UNUSED(widget);

    if (_type == ZOMBIE) {
        painter->setBrush(Qt::darkGreen);
    } else {
        painter->setBrush(Qt::blue);
    }

    painter->setPen(Qt::black);
    painter->drawEllipse(boundingRect());
}

void Human::updateState(const QList<WorldObject *> &neighbors, double dt) {
    double x = _curState.pos[0] + _curState.vel[0] * dt;
    double y = _curState.pos[1] + _curState.vel[1] * dt;

    if(x < 0 || x > 500) _curState.vel[0] *= -1;
    if(y < 0 || y > 500) _curState.vel[1] *= -1;

    if(x < 0) x = 0; if(x > 500) x = 500;
    if(y < 0) y = 0; if(y > 500) y = 500;

    setPosition(x, y);
}

void Human::biteSlot() {
    QObject* senderObj = sender();
    if(!senderObj) return;

    WorldObject* wObj = dynamic_cast<WorldObject*>(senderObj);

    if (wObj && wObj->getType() == ZOMBIE) {
        if (_type == HUMAN) {
            _type = ZOMBIE; // Теперь мы зомби
            _curState._curStatus = AGGRESSIVE;
            update();
        }
    }
}

// --- Zombie ---

Zombie::Zombie(QObject *parent) : WorldObject(parent) {
    _type = ZOMBIE;
    _curState._curStatus = AGGRESSIVE;
    _biteRadius = 15.0; // Радиус укуса
    _isBusy = false;
    _cooldownTimer = 0;
}

void Zombie::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget) {
    painter->setBrush(Qt::darkGreen);
    painter->setPen(Qt::black);
    painter->drawEllipse(boundingRect());
}

void Zombie::updateState(const QList<WorldObject *> &neighbors, double dt) {
    double x = _curState.pos[0] + _curState.vel[0] * dt;
    double y = _curState.pos[1] + _curState.vel[1] * dt;

    if(x < 0 || x > 500) _curState.vel[0] *= -1;
    if(y < 0 || y > 500) _curState.vel[1] *= -1;

    if(x < 0) x = 0; if(x > 500) x = 500;
    if(y < 0) y = 0; if(y > 500) y = 500;

    setPosition(x, y);

    if (_isBusy) {
        _cooldownTimer++;
        if (_cooldownTimer > 50) {
            _isBusy = false;
            _cooldownTimer = 0;
        }
        return;
    }

    for (WorldObject* obj : neighbors) {
        if (obj == this || obj->getType() == ZOMBIE) continue;
        double dx = obj->getState().pos[0] - _curState.pos[0];
        double dy = obj->getState().pos[1] - _curState.pos[1];
        double dist = std::sqrt(dx*dx + dy*dy);

        if (dist <= _biteRadius) {
            Human* potentialVictim = dynamic_cast<Human*>(obj);
            if (potentialVictim) {
                connect(this, &Zombie::biteSignal, potentialVictim, &Human::biteSlot, Qt::UniqueConnection);


                emit biteSignal();

                disconnect(this, &Zombie::biteSignal, potentialVictim, &Human::biteSlot);

                _isBusy = true;
                break;
            }
        }
    }
}
