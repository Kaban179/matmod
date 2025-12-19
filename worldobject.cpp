#include "worldobject.h"
#include <QRandomGenerator>
#include <cmath>
#include <QDebug>




double WorldObject::windX = 0.0;
double WorldObject::windY = 0.0;
bool WorldObject::usePetriMode = false;
double WorldObject::mu = 10000.0;
double WorldObject::R_orbit = 500.0;
double WorldObject::alpha = 0.0;

void applyExternalForces(objState &state, double dt, double mass) {
    double fx = 0.0;
    double fy = 0.0;

    fx += WorldObject::windX;
    fy += WorldObject::windY;


    if (WorldObject::usePetriMode) {
        double locY = state.pos[1] - 250.0;

        double cosTerm = std::cos(M_PI/2.0 + WorldObject::alpha);
        double distSq = std::pow(WorldObject::R_orbit, 2) + std::pow(locY, 2)
                        - 2.0 * WorldObject::R_orbit * locY * cosTerm;

        double forceMag = WorldObject::mu / distSq;

        fx += forceMag * std::cos(WorldObject::alpha);
        fy += forceMag * std::sin(WorldObject::alpha);
    }

    state.vel[0] += fx * dt;
    state.vel[1] += fy * dt;
}


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
    if (this->_type == ZOMBIE) {
        WorldObject* closestVictim = nullptr;
        double minDistSq = _searchRadius * _searchRadius;

        for (WorldObject* obj : neighbors) {
            if (obj == this || obj->getType() == ZOMBIE) continue;

            double dx = obj->getState().pos[0] - _curState.pos[0];
            double dy = obj->getState().pos[1] - _curState.pos[1];
            double dSq = dx*dx + dy*dy;

            if (dSq < minDistSq) {
                minDistSq = dSq;
                closestVictim = obj;
            }
        }

        if (closestVictim) {
            double dx = closestVictim->getState().pos[0] - _curState.pos[0];
            double dy = closestVictim->getState().pos[1] - _curState.pos[1];
            double actualDist = std::sqrt(minDistSq);
            _curState.vel[0] = (dx / actualDist) * _maxSpeed;
            _curState.vel[1] = (dy / actualDist) * _maxSpeed;

            if (actualDist < 10.0) {
                Human* h = qobject_cast<Human*>(closestVictim);
                if (h) h->biteSlot();
            }
        }
    }
    else {
        double targetVx = 0;
        double targetVy = 0;
        bool fleeing = false;

        for (WorldObject* obj : neighbors) {
            if (obj == this) continue;

            double dx = obj->getState().pos[0] - _curState.pos[0];
            double dy = obj->getState().pos[1] - _curState.pos[1];
            double dSq = dx*dx + dy*dy;

            if (obj->getType() == ZOMBIE && dSq < (_obsRadius * _obsRadius)) {
                double dist = std::sqrt(dSq);
                targetVx -= (dx / dist) * _maxSpeed;
                targetVy -= (dy / dist) * _maxSpeed;
                fleeing = true;
            }
        }

        if (fleeing) {
            _curState.vel[0] = targetVx;
            _curState.vel[1] = targetVy;
        }

    }

    applyExternalForces(_curState, dt, _mass);

    double nextX = _curState.pos[0] + _curState.vel[0] * dt;
    double nextY = _curState.pos[1] + _curState.vel[1] * dt;

    if (nextX <= 0 || nextX >= 500) { _curState.vel[0] *= -1; nextX = qBound(0.0, nextX, 500.0); }
    if (nextY <= 0 || nextY >= 500) { _curState.vel[1] *= -1; nextY = qBound(0.0, nextY, 500.0); }

    setPosition(nextX, nextY);
}




void Human::biteSlot() {
    _type = ZOMBIE;
    update();
}


Zombie::Zombie(QObject *parent) : WorldObject(parent) {
    _type = ZOMBIE;
    _curState._curStatus = AGGRESSIVE;
    _biteRadius = 15.0;
    _isBusy = false;
    _cooldownTimer = 0;
}

void Zombie::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget) {
    painter->setBrush(Qt::darkGreen);
    painter->setPen(Qt::black);
    painter->drawEllipse(boundingRect());
}

void Zombie::updateState(const QList<WorldObject *> &neighbors, double dt) {
    if (_isBusy) {
        _cooldownTimer++;
        if (_cooldownTimer > 30) {
            _isBusy = false;
            _cooldownTimer = 0;
        }
    }

    double targetVx = _curState.vel[0];
    double targetVy = _curState.vel[1];

    WorldObject* closestVictim = nullptr;
    double minDist = _searchRadius;

    double searchRadSq = _searchRadius * _searchRadius;


    for (WorldObject* obj : neighbors) {
        if (obj == this) continue;

        double dx = obj->getState().pos[0] - _curState.pos[0];
        double dy = obj->getState().pos[1] - _curState.pos[1];
        double dSq = dx*dx + dy*dy;
        if (dSq < searchRadSq) {
            double dist = std::sqrt(dSq);
            if (dist < 0.1) continue;

        if (obj->getType() == HUMAN && dist < minDist) {
            minDist = dist;
            closestVictim = obj;
        }

        if (obj->getType() == ZOMBIE && dist < _safetyRadius) {
            targetVx -= (dx / dist) * (_maxSpeed * 0.3);
            targetVy -= (dy / dist) * (_maxSpeed * 0.3);

        }
        }
    }double _mass = 1.0;

    static double windX, windY;
    static bool usePetriMode;
    static double mu, R_orbit, alpha;

    if (closestVictim && !_isBusy) {
        double dx = closestVictim->getState().pos[0] - _curState.pos[0];
        double dy = closestVictim->getState().pos[1] - _curState.pos[1];
        double dist = std::sqrt(dx*dx + dy*dy);

        targetVx = (dx / dist) * _maxSpeed;
        targetVy = (dy / dist) * _maxSpeed;

        if (dist < _biteRadius) {
            Human* h = qobject_cast<Human*>(closestVictim);
            if (h) {
                h->biteSlot();
                _isBusy = true;
                targetVx = 0;
                targetVy = 0;
            }
        }
    }

    _curState.vel[0] = targetVx;
    _curState.vel[1] = targetVy;

    applyExternalForces(_curState, dt, _mass);
    double newX = _curState.pos[0] + _curState.vel[0] * dt;
    double newY = _curState.pos[1] + _curState.vel[1] * dt;

    if (newX < 0 || newX > 500) {
        _curState.vel[0] *= -1;
        newX = (newX < 0) ? 0 : 500;
    }
    if (newY < 0 || newY > 500) {
        _curState.vel[1] *= -1;
        newY = (newY < 0) ? 0 : 500;
    }




    if (newX <= 0) { newX = 1; _curState.vel[0] *= -1; }
    if (newX >= 500) { newX = 499; _curState.vel[0] *= -1; }
    if (newY <= 0) { newY = 1; _curState.vel[1] *= -1; }
    if (newY >= 500) { newY = 499; _curState.vel[1] *= -1; }

    setPosition(newX, newY);
}
