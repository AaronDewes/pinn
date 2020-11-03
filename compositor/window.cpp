/****************************************************************************
**
** Copyright (C) 2017 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the examples of the Qt Wayland module
**
** $QT_BEGIN_LICENSE:BSD$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** BSD License Usage
** Alternatively, you may use this file under the terms of the BSD license
** as follows:
**
** "Redistribution and use in source and binary forms, with or without
** modification, are permitted provided that the following conditions are
** met:
**   * Redistributions of source code must retain the above copyright
**     notice, this list of conditions and the following disclaimer.
**   * Redistributions in binary form must reproduce the above copyright
**     notice, this list of conditions and the following disclaimer in
**     the documentation and/or other materials provided with the
**     distribution.
**   * Neither the name of The Qt Company Ltd nor the names of its
**     contributors may be used to endorse or promote products derived
**     from this software without specific prior written permission.
**
**
** THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
** "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
** LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
** A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
** OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
** SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
** LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
** DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
** THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
** (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
** OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE."
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "window.h"

#include <QMouseEvent>
#include <QWindow>
#include <QBackingStore>
#include <QMatrix4x4>
#include <QPainter>

#include "compositor.h"
#include <QtWaylandCompositor/qwaylandseat.h>

Window::Window()
	:QWindow()
	, m_backingStore(new QBackingStore(this))
{
}

void Window::setCompositor(Compositor *comp) {
    m_compositor = comp;
    connect(m_compositor, &Compositor::startMove, this, &Window::startMove);
    connect(m_compositor, &Compositor::startResize, this, &Window::startResize);
    connect(m_compositor, &Compositor::dragStarted, this, &Window::startDrag);
}

void Window::init()
{
    m_compositor->create();
    m_compositor->startRender();
    drawBackground();
    m_compositor->endRender();
}

void Window::drawBackground()
{
    QColor backgroundColor = QColor(0xde, 0xde, 0xde);
    QImage backgroundColorImage = QImage(this->geometry().size(), QImage::Format_RGB32);
    backgroundColorImage.fill(backgroundColor);
    QImage backgroundImage = m_backgroundImage;
    if(this->geometry().width() < backgroundImage.width() || this->geometry().height() < backgroundImage.height()) {
        backgroundImage = m_backgroundImage.scaled(this->geometry().size(), Qt::KeepAspectRatio, Qt::SmoothTransformation);
    }
    QPaintDevice *device = m_backingStore->paintDevice();
    QPainter painter(device);
    int deltaX = this->geometry().width() - backgroundImage.width();
    int deltaY = this->geometry().height() - backgroundImage.height();
    painter.drawImage(this->geometry(), backgroundColorImage);
    painter.translate(deltaX / 2, deltaY / 2);
    painter.drawImage(backgroundImage.rect(), backgroundImage);
    painter.end();
}

QPointF Window::getAnchorPosition(const QPointF &position, int resizeEdge, const QSize &windowSize)
{
    float y = position.y();
    if (resizeEdge & QWaylandXdgSurfaceV5::ResizeEdge::TopEdge)
        y += windowSize.height();

    float x = position.x();
    if (resizeEdge & QWaylandXdgSurfaceV5::ResizeEdge::LeftEdge)
        x += windowSize.width();

    return QPointF(x, y);
}

QPointF Window::getAnchoredPosition(const QPointF &anchorPosition, int resizeEdge, const QSize &windowSize)
{
    return anchorPosition - getAnchorPosition(QPointF(), resizeEdge, windowSize);
}

View *Window::viewAt(const QPointF &point)
{
    View *ret = nullptr;
    const auto views = m_compositor->views();
    for (View *view : views) {
        if (view == m_dragIconView)
            continue;
        QRectF geom(view->position(), view->size());
        if (geom.contains(point))
            ret = view;
    }
    return ret;
}

void Window::startMove()
{
    m_grabState = MoveGrab;
}

void Window::startResize(int edge, bool anchored)
{
    m_initialSize = m_mouseView->windowSize();
    m_grabState = ResizeGrab;
    m_resizeEdge = edge;
    m_resizeAnchored = anchored;
    m_resizeAnchorPosition = getAnchorPosition(m_mouseView->position(), edge, m_mouseView->surface()->destinationSize());
}

void Window::startDrag(View *dragIcon)
{
    m_grabState = DragGrab;
    m_dragIconView = dragIcon;
    m_compositor->raise(dragIcon);
}

void Window::mousePressEvent(QMouseEvent *e)
{
    if (mouseGrab())
        return;
    if (m_mouseView.isNull()) {
        m_mouseView = viewAt(e->localPos());
        if (!m_mouseView) {
            m_compositor->closePopups();
            return;
        }
        if (e->modifiers() == Qt::AltModifier || e->modifiers() == Qt::MetaModifier)
            m_grabState = MoveGrab; //start move
        else
            m_compositor->raise(m_mouseView);
        m_initialMousePos = e->localPos();
        m_mouseOffset = e->localPos() - m_mouseView->position();

        QMouseEvent moveEvent(QEvent::MouseMove, e->localPos(), e->globalPos(), Qt::NoButton, Qt::NoButton, e->modifiers());
        sendMouseEvent(&moveEvent, m_mouseView);
    }
    sendMouseEvent(e, m_mouseView);
}

void Window::mouseReleaseEvent(QMouseEvent *e)
{
    if (!mouseGrab())
        sendMouseEvent(e, m_mouseView);
    if (e->buttons() == Qt::NoButton) {
        if (m_grabState == DragGrab) {
            View *view = viewAt(e->localPos());
            m_compositor->handleDrag(view, e);
        }
        m_mouseView = nullptr;
        m_grabState = NoGrab;
    }
}

void Window::mouseMoveEvent(QMouseEvent *e)
{
    switch (m_grabState) {
    case NoGrab: {
        View *view = m_mouseView ? m_mouseView.data() : viewAt(e->localPos());
        sendMouseEvent(e, view);
        if (!view)
            setCursor(Qt::ArrowCursor);
    }
        break;
    case MoveGrab: {
        m_mouseView->setPosition(e->localPos() - m_mouseOffset);
        update();
    }
        break;
    case ResizeGrab: {
        QPoint delta = (e->localPos() - m_initialMousePos).toPoint();
        m_compositor->handleResize(m_mouseView, m_initialSize, delta, m_resizeEdge);
    }
        break;
    case DragGrab: {
        View *view = viewAt(e->localPos());
        m_compositor->handleDrag(view, e);
        if (m_dragIconView) {
            m_dragIconView->setPosition(e->localPos() + m_dragIconView->offset());
            update();
        }
    }
        break;
    }
}

void Window::sendMouseEvent(QMouseEvent *e, View *target)
{
    QPointF mappedPos = e->localPos();
    if (target)
        mappedPos -= target->position();
    QMouseEvent viewEvent(e->type(), mappedPos, e->localPos(), e->button(), e->buttons(), e->modifiers());
    m_compositor->handleMouseEvent(target, &viewEvent);
}

void Window::keyPressEvent(QKeyEvent *e)
{
    m_compositor->defaultSeat()->sendKeyPressEvent(e->nativeScanCode());
}

void Window::keyReleaseEvent(QKeyEvent *e)
{
    m_compositor->defaultSeat()->sendKeyReleaseEvent(e->nativeScanCode());
}

void Window::update()
{
    if (!isExposed())
        return;

    QRect rect(0, 0, width(), height());
    m_backingStore->beginPaint(rect);

    drawBackground();
    m_backingStore->endPaint();
    m_backingStore->flush(rect);
}

bool Window::event(QEvent *event)
{
    if (event->type() == QEvent::UpdateRequest) {
        update();
        return true;
    }
    return QWindow::event(event);
}

void Window::exposeEvent(QExposeEvent *)
{
    if (isExposed())
        update();
}

void Window::resizeEvent(QResizeEvent *resizeEvent)
{
    m_backingStore->resize(resizeEvent->size());
}
