/********************************************************************************
** Form generated from reading UI file 'csp.ui'
**
** Created by: Qt User Interface Compiler version 5.15.2
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef UI_CSP_H
#define UI_CSP_H

#include <QtCore/QVariant>
#include <QtWidgets/QApplication>
#include <QtWidgets/QTextEdit>
#include <QtWidgets/QWidget>

QT_BEGIN_NAMESPACE

class Ui_Csp
{
public:
    QTextEdit *textEdit;

    void setupUi(QWidget *Csp)
    {
        if (Csp->objectName().isEmpty())
            Csp->setObjectName(QString::fromUtf8("Csp"));
        Csp->resize(800, 600);
        textEdit = new QTextEdit(Csp);
        textEdit->setObjectName(QString::fromUtf8("textEdit"));
        textEdit->setGeometry(QRect(10, 70, 750, 520));

        retranslateUi(Csp);

        QMetaObject::connectSlotsByName(Csp);
    } // setupUi

    void retranslateUi(QWidget *Csp)
    {
        Csp->setWindowTitle(QCoreApplication::translate("Csp", "Csp", nullptr));
    } // retranslateUi

};

namespace Ui {
    class Csp: public Ui_Csp {};
} // namespace Ui

QT_END_NAMESPACE

#endif // UI_CSP_H
