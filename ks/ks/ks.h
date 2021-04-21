#ifndef KS_H
#define KS_H

#include <QMainWindow>
#include <QFileDialog>
#include <QMessageBox>
#include <QFile>
#include <QTcpServer>
#include <QTcpSocket>
#include <QTimer>
#include <QFileInfo>
#include <QDebug>
#include <openssl/rsa.h>
#include <openssl/err.h>
#include <openssl/pem.h>
#include <string.h>

QT_BEGIN_NAMESPACE
namespace Ui { class Ks; }
QT_END_NAMESPACE

class Ks : public QMainWindow
{
    Q_OBJECT

public:
    Ks(QWidget *parent = nullptr);
    ~Ks();
    //rsa加密
    QString EncodeRSAKeyFile(const std::string& strPemFileName, QString strData);
    //rsa解密
    QString DecodeRSAKeyFile(const std::string& strPemFileName, QString strData);
private slots:
    void on_pushButton_clicked();

private:
    Ui::Ks *ui;
    QTcpServer *tcpServer1;//监听客户端消息
    QTcpServer *tcpServer2;//监听csp消息
    QTcpSocket *tcpSocket1;//与客户端消息传输
    QTcpSocket *tcpSocket2;//与csp消息传输
};
#endif // KS_H
