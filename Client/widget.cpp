#include "widget.h"
#include <QDebug>
#include "model/data.h"

Widget::Widget(QWidget *parent)
    : QWidget(parent)
{
    LOG()<<model::Message::UUID();
}

Widget::~Widget() {}
