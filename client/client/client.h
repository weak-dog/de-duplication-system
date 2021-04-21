#ifndef CLIENT_H
#define CLIENT_H

#include <QWidget>
#include <QFileDialog>
#include <QMessageBox>
#include <QFile>
#include <QTcpServer>
#include <QTcpSocket>
#include <QFileInfo>
#include <QDebug>
#include "security.h"
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QSqlError>
#include <QSqlTableModel>

namespace Ui {
class Client;
}

class Client : public QWidget
{
    Q_OBJECT

public:
    explicit Client(QString account,QString password,QWidget *parent = nullptr);
    ~Client();
    void sendData();//发送文件数据
    QString calcCs(QString s);
    bool DecCs(QString cs,QString& s);
    QString compareCs(QString Cs,QString& s);

private slots:
    void on_pushButtonSelect_clicked();
    void on_pushButtonUpload_clicked();
    void on_pushButtonDownload_clicked();
    void on_pushButtonChange_clicked();

private:
    Ui::Client *ui;
    QTcpServer *tcpServer1;//监听ks消息
    QTcpSocket *tcpSocket1;//与csp消息传输
    QTcpSocket *tcpSocket2;//与ks消息传输
    QTcpSocket *tcpSocket3;//与csp文件传输
    //命令元素
    QString account;
    QString password;
    QString command;
    QString hash;
    qint32 type;
    //接收消息
    QByteArray array;//消息缓冲区
    QStringList splitArray;//接收消息分片
    QStringList messages;//消息列表
    //数据库操作
    QSqlDatabase database;
    QSqlQuery sql_query;
    //文件传输
    QFile file;//文件对象
    QString filePath;//文件路径
    QString fileName;//文件名
    QString fileHash;//文件哈希值
    qint64 fileSize;//文件大小
    qint64 recvSize;//已接收大小
    qint64 sendSize;//已发送文件大小
    //ui相关
    QSqlTableModel * fileModel;//文件列表
    QStringList users;//共享用户
    unsigned char* key;//随机秘钥
    QString s;
};

#endif // CLIENT_H
