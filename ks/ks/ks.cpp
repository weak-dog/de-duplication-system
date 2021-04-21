#include "ks.h"
#include "ui_ks.h"

Ks::Ks(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::Ks)
{
    ui->setupUi(this);
    //初始化套接字
    tcpServer1=new QTcpServer(this);
    tcpServer2=new QTcpServer(this);
    tcpSocket1=new QTcpSocket(this);
    tcpSocket2=new QTcpSocket(this);

    tcpServer1->listen(QHostAddress::Any,10005);
    tcpServer2->listen(QHostAddress::Any,10006);

    //监听客户端消息
    connect(tcpServer1,&QTcpServer::newConnection,
            [=](){
                //取出建立好的套接字
                tcpSocket1=tcpServer1->nextPendingConnection();
                //获取对方的IP和端口
                QString ip=tcpSocket1->peerAddress().toString();
                quint16 port=tcpSocket1->peerPort();
                QString str=QString("[%1:%2]成功连接(客户端)").arg(ip).arg(port);
                ui->textEdit->append(str);

                connect(tcpSocket1,&QTcpSocket::readyRead,
                        [=]()
                        {
                            QByteArray array=tcpSocket1->readAll();
                            QStringList splitArray=QString(array).split("@");
                            qDebug()<<"------------------------------------";
                            qDebug()<<"从客户端收到消息(Ckr): "+QString(array);
                            ui->textEdit->append("--------------------------");
                            ui->textEdit->append("从客户端收到消息(Ckr): "+QString(array));
                            //计算ckr_d
                            QString cs=splitArray[0];
                            QString ckr=splitArray[1];
                            //用pubkey加密
                            QString ckrd=EncodeRSAKeyFile("/home/weakdog/ks/pubkey.pem",ckr);
                            qDebug()<<"加密ckrd结束";
                            QString ucc="upload@"+cs+"@"+ckrd;
                            qDebug()<<ucc;
                            //与csp建立连接并发送给csp
                            QString ip="127.0.0.1";
                            qint64 port=10003;
                            tcpSocket2->connectToHost(QHostAddress(ip),port);
                            tcpSocket2->write(ucc.toLatin1());
                            tcpSocket2->disconnectFromHost();
                            tcpSocket2->close();
                            tcpSocket1->disconnectFromHost();
                            tcpSocket1->close();
                        });
            });

    //监听csp消息
    connect(tcpServer2,&QTcpServer::newConnection,
            [=](){
                //取出建立好的套接字
                tcpSocket2=tcpServer2->nextPendingConnection();
                //获取对方的IP和端口
                QString ip=tcpSocket2->peerAddress().toString();
                quint16 port=tcpSocket2->peerPort();
                QString str=QString("[%1:%2]成功连接(客户端)").arg(ip).arg(port);
                ui->textEdit->append(str);
                connect(tcpSocket2,&QTcpSocket::readyRead,
                        [=]()
                        {
                            QByteArray array=tcpSocket2->readAll();
                            QStringList splitArray=QString(array).split("@");
                            tcpSocket2->disconnectFromHost();
                            tcpSocket2->close();
                            ui->textEdit->append("从csp收到消息(Ckrd): "+QString(array));
                            qDebug()<<"从csp收到消息(Ckrd): "+QString(array);
                            if(splitArray[0]=="ckrd"){//更新ckrd
                                QString cs=splitArray[1];
                                QString ckrd=splitArray[2];
                                //用prikey解密
                                QString ckr=DecodeRSAKeyFile("/home/weakdog/ks/prikey.pem",ckrd).mid(0,64);
                                //用pubkey2加密
                                QString ckrdNew=EncodeRSAKeyFile("/home/weakdog/ks/pubkey2.pem",ckr);
                                qDebug()<<"新的ckrd:"<<ckrdNew;
                                QString ccc=QString("ckrd2@%1@%2").arg(cs).arg(ckrdNew);
                                QString ip="127.0.0.1";
                                qint64 port=10003;
                                tcpSocket2->connectToHost(QHostAddress(ip),port);
                                tcpSocket2->write(ccc.toLatin1());
                                tcpSocket2->disconnectFromHost();
                                tcpSocket2->close();
                            }else if(splitArray[0]=="finish"){
                                //更换密钥
                                system("cd /home/weakdog/ks&&rm -f prikey.pem pubkey.pem");
                                system("mv /home/weakdog/ks/pubkey2.pem /home/weakdog/ks/pubkey.pem");
                                system("mv /home/weakdog/ks/prikey2.pem /home/weakdog/ks/prikey.pem");
                                ui->textEdit->append("更新d成功");
                            }else if(splitArray[0]=="changes"){//changes@ckrd
                                QString ckrd=splitArray[1];
                                qDebug()<<ckrd;
                                qDebug()<<"fuck";
                                //用prikey解密
                                QString ckr=DecodeRSAKeyFile("/home/weakdog/ks/prikey.pem",ckrd).mid(0,64);
                                QString cckr="changes@"+ckr;
                                qDebug()<<"解密成功";
                                //发送给客户端
                                QString ip="127.0.0.1";
                                qint64 port=10001;
                                tcpSocket1->connectToHost(QHostAddress(ip),port);
                                tcpSocket1->write(cckr.toUtf8().data());
                                ui->textEdit->append("发送ckr: "+cckr);
                                tcpSocket1->disconnectFromHost();
                                tcpSocket1->close();
                            }else{//下载文件
                                qDebug()<<"下载文件"<<splitArray[1];
                                QString ckrd=splitArray[1];
                                //用prikey解密
                                QString ckr=DecodeRSAKeyFile("/home/weakdog/ks/prikey.pem",ckrd).mid(0,64);
                                QString dckr="download@"+ckr;
                                //发送给客户端
                                QString ip="127.0.0.1";
                                qint64 port=10001;
                                tcpSocket1->connectToHost(QHostAddress(ip),port);
                                tcpSocket1->write(dckr.toUtf8().data());
                                ui->textEdit->append("发送dckr: "+dckr);
                                tcpSocket1->disconnectFromHost();
                                tcpSocket1->close();
                            }
                        });
            });
}

Ks::~Ks()
{
    delete ui;
}


void Ks::on_pushButton_clicked()
{
    //向csp发送更新请求
    //与csp建立连接并发送给csp
    QString ip="127.0.0.1";
    qint64 port=10003;
    QString changed="changed";
    tcpSocket2->connectToHost(QHostAddress(ip),port);
    tcpSocket2->write(changed.toLatin1());
    ui->textEdit->append("更新d");
    qDebug()<<"-----------------更新d";
    tcpSocket2->disconnectFromHost();
    tcpSocket2->close();
    //生成d'
    system("cd /home/weakdog/ks&&openssl genrsa -out prikey2.pem 1024");
    system("cd /home/weakdog/ks&&openssl rsa -in prikey2.pem -pubout -out pubkey2.pem");
}

//rsa加密
QString Ks::EncodeRSAKeyFile(const std::string& strPemFileName, QString strData)
{
    FILE* hPubKeyFile = fopen(strPemFileName.c_str(), "rb");
    QString strRet;
    RSA* pRSAPublicKey = RSA_new();
    PEM_read_RSA_PUBKEY(hPubKeyFile, &pRSAPublicKey, 0, 0);
    int nLen = RSA_size(pRSAPublicKey);
    char* pEncode = new char[nLen + 1];
    int ret = RSA_public_encrypt(strData.size(), (const unsigned char*)strData.toLatin1().data(), (unsigned char*)pEncode, pRSAPublicKey, RSA_PKCS1_PADDING);
    unsigned char* result=new unsigned char[ret];
    for(int i=0;i<ret;i++)result[i]=pEncode[i];
    RSA_free(pRSAPublicKey);
    fclose(hPubKeyFile);
    CRYPTO_cleanup_all_ex_data();
    strRet=QByteArray((char*)result,ret).toHex();
    return strRet;
}

//rsa解密
QString Ks::DecodeRSAKeyFile(const std::string& strPemFileName, QString strData)
{
    FILE* hPriKeyFile = fopen(strPemFileName.c_str(),"rb");
    RSA* pRSAPriKey = RSA_new();
    PEM_read_RSAPrivateKey(hPriKeyFile, &pRSAPriKey, 0, 0);
    int nLen = RSA_size(pRSAPriKey);
    char* pDecode = new char[nLen+1];
    unsigned char* str=new unsigned char[128];
    str=(unsigned char*)QByteArray::fromHex(strData.toLatin1()).data();
    int ret = RSA_private_decrypt(128, str, (unsigned char*)pDecode, pRSAPriKey, RSA_PKCS1_PADDING);
    char* result=new char[ret];
    for(int i=0;i<ret;i++)result[i]=(char)pDecode[i];
    delete [] pDecode;
    RSA_free(pRSAPriKey);
    fclose(hPriKeyFile);
    CRYPTO_cleanup_all_ex_data();
    QString strRet=QString(result);
    return strRet;
}
