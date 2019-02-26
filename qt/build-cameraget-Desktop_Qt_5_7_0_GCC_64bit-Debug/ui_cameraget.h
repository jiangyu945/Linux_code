/********************************************************************************
** Form generated from reading UI file 'cameraget.ui'
**
** Created by: Qt User Interface Compiler version 5.7.0
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef UI_CAMERAGET_H
#define UI_CAMERAGET_H

#include <QtCore/QVariant>
#include <QtWidgets/QAction>
#include <QtWidgets/QApplication>
#include <QtWidgets/QButtonGroup>
#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QHeaderView>
#include <QtWidgets/QLabel>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QWidget>

QT_BEGIN_NAMESPACE

class Ui_cameraget
{
public:
    QLabel *label;
    QLabel *label_2;
    QWidget *widget;
    QHBoxLayout *horizontalLayout;
    QPushButton *open;
    QPushButton *pic;
    QPushButton *closeCam;

    void setupUi(QWidget *cameraget)
    {
        if (cameraget->objectName().isEmpty())
            cameraget->setObjectName(QStringLiteral("cameraget"));
        cameraget->resize(600, 442);
        label = new QLabel(cameraget);
        label->setObjectName(QStringLiteral("label"));
        label->setGeometry(QRect(40, 30, 250, 300));
        label_2 = new QLabel(cameraget);
        label_2->setObjectName(QStringLiteral("label_2"));
        label_2->setGeometry(QRect(310, 30, 250, 300));
        widget = new QWidget(cameraget);
        widget->setObjectName(QStringLiteral("widget"));
        widget->setGeometry(QRect(89, 384, 401, 41));
        horizontalLayout = new QHBoxLayout(widget);
        horizontalLayout->setSpacing(6);
        horizontalLayout->setContentsMargins(11, 11, 11, 11);
        horizontalLayout->setObjectName(QStringLiteral("horizontalLayout"));
        horizontalLayout->setContentsMargins(0, 0, 0, 0);
        open = new QPushButton(widget);
        open->setObjectName(QStringLiteral("open"));

        horizontalLayout->addWidget(open);

        pic = new QPushButton(widget);
        pic->setObjectName(QStringLiteral("pic"));

        horizontalLayout->addWidget(pic);

        closeCam = new QPushButton(widget);
        closeCam->setObjectName(QStringLiteral("closeCam"));

        horizontalLayout->addWidget(closeCam);


        retranslateUi(cameraget);

        QMetaObject::connectSlotsByName(cameraget);
    } // setupUi

    void retranslateUi(QWidget *cameraget)
    {
        cameraget->setWindowTitle(QApplication::translate("cameraget", "cameraget", 0));
        label->setText(QApplication::translate("cameraget", "TextLabel", 0));
        label_2->setText(QApplication::translate("cameraget", "TextLabel", 0));
        open->setText(QApplication::translate("cameraget", "Open Camera", 0));
        pic->setText(QApplication::translate("cameraget", "Capture Image", 0));
        closeCam->setText(QApplication::translate("cameraget", "Close Camera", 0));
    } // retranslateUi

};

namespace Ui {
    class cameraget: public Ui_cameraget {};
} // namespace Ui

QT_END_NAMESPACE

#endif // UI_CAMERAGET_H
