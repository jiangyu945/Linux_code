/********************************************************************************
** Form generated from reading UI file 'mycam.ui'
**
** Created by: Qt User Interface Compiler version 5.7.0
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef UI_MYCAM_H
#define UI_MYCAM_H

#include <QtCore/QVariant>
#include <QtWidgets/QAction>
#include <QtWidgets/QApplication>
#include <QtWidgets/QButtonGroup>
#include <QtWidgets/QHeaderView>
#include <QtWidgets/QLabel>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QWidget>

QT_BEGIN_NAMESPACE

class Ui_myCam
{
public:
    QLabel *label_show;
    QPushButton *Bt_CamOpen;
    QPushButton *Bt_CamClose;
    QPushButton *Bt_SaveImg;

    void setupUi(QWidget *myCam)
    {
        if (myCam->objectName().isEmpty())
            myCam->setObjectName(QStringLiteral("myCam"));
        myCam->resize(600, 400);
        label_show = new QLabel(myCam);
        label_show->setObjectName(QStringLiteral("label_show"));
        label_show->setGeometry(QRect(150, 20, 300, 250));
        Bt_CamOpen = new QPushButton(myCam);
        Bt_CamOpen->setObjectName(QStringLiteral("Bt_CamOpen"));
        Bt_CamOpen->setGeometry(QRect(150, 320, 80, 22));
        Bt_CamClose = new QPushButton(myCam);
        Bt_CamClose->setObjectName(QStringLiteral("Bt_CamClose"));
        Bt_CamClose->setGeometry(QRect(370, 320, 80, 22));
        Bt_SaveImg = new QPushButton(myCam);
        Bt_SaveImg->setObjectName(QStringLiteral("Bt_SaveImg"));
        Bt_SaveImg->setGeometry(QRect(260, 320, 80, 22));

        retranslateUi(myCam);

        QMetaObject::connectSlotsByName(myCam);
    } // setupUi

    void retranslateUi(QWidget *myCam)
    {
        myCam->setWindowTitle(QApplication::translate("myCam", "myCam", 0));
        label_show->setText(QApplication::translate("myCam", "TextLabel", 0));
        Bt_CamOpen->setText(QApplication::translate("myCam", "\346\211\223\345\274\200\346\221\204\345\203\217\345\244\264", 0));
        Bt_CamClose->setText(QApplication::translate("myCam", "\345\205\263\351\227\255\346\221\204\345\203\217\345\244\264", 0));
        Bt_SaveImg->setText(QApplication::translate("myCam", "\344\277\235\345\255\230\345\233\276\347\211\207", 0));
    } // retranslateUi

};

namespace Ui {
    class myCam: public Ui_myCam {};
} // namespace Ui

QT_END_NAMESPACE

#endif // UI_MYCAM_H
