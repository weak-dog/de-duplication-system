#include "csp.h"
#include "ui_csp.h"

Csp::Csp(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::Csp)
{
    type=0;
    ui->setupUi(this);
    //连接数据库
    if(QSqlDatabase::contains("qt_sql_default_connection"))
        database = QSqlDatabase::database("qt_sql_default_connection");
    else
        database=QSqlDatabase::addDatabase("QSQLITE");
    database.setDatabaseName("csp.db");
    if(!database.open()){
        qDebug()<<"Error: Fail to connect database"<<database.lastError();
    }else{
        qDebug()<<"Succeed to connect database";
    }
    sql_query=QSqlQuery(database);

    firstUpload=true;
    secondButUpload=false;
    //初始化套接字
    tcpServer1=new QTcpServer(this);
    tcpServer2=new QTcpServer(this);
    tcpServer3=new QTcpServer(this);
    tcpSocket1=new QTcpSocket(this);//客户端消息
    tcpSocket2=new QTcpSocket(this);//ks消息
    tcpSocket3=new QTcpSocket(this);//客户端文件
    //绑定监听
    tcpServer1->listen(QHostAddress::Any,10002);
    tcpServer2->listen(QHostAddress::Any,10003);
    tcpServer3->listen(QHostAddress::Any,10004);

    //监听客户端连接(消息端口)
    connect(tcpServer1,&QTcpServer::newConnection,
            [=]()
            {
                //取出建立好的套接字
                tcpSocket1=tcpServer1->nextPendingConnection();
                QString ip=tcpSocket1->peerAddress().toString();
                quint16 port=tcpSocket1->peerPort();
                QString str=QString("[%1:%2]成功连接(消息)").arg(ip).arg(port);
                ui->textEdit->append(str);

                //接受客户端消息
                connect(tcpSocket1,&QTcpSocket::readyRead,
                        [=]()
                {
                    QByteArray array=tcpSocket1->readAll();
                    messages.append(QString(array));
                    QStringList splitArray=QString(array).split("@");
                    qDebug()<<QString(array);
                    ui->textEdit->append("从客户端收到消息: "+QString(array));
                    if(messages.size()==1){
                        //收到的是命令，查找第三部分判断指令种类
                        if(splitArray[2]=="login"){//登录
                            ui->textEdit->append("客户端请求登录");
                            QString account=splitArray[0];
                            QString password=splitArray[1];
                            bool result=checkLogin(account,password);
                            if(result){//登录成功
                                tcpSocket1->write(QString("success").toUtf8().data());
                                ui->textEdit->append("登录成功");
                            }else{
                                tcpSocket1->write(QString("failure").toUtf8().data());
                                ui->textEdit->append("登录失败");
                            }
                            messages.clear();
                        }else if(splitArray[2]=="upload"){//上传
                            type=2;
                            ui->textEdit->append("客户端请求上传文件");
                            fileHash=splitArray[4];
                            bool isFirstUpload=checkFirst(fileHash);//是否为首次上传
                            if(isFirstUpload==true){//首次上传
                                ui->textEdit->append("首次上传");
                                firstUpload=true;
                                tcpSocket1->write(QString("first").toUtf8().data());
                                fileSize=splitArray[3].toInt();
                                //打开文件
                            }else{//后续上传
                                //查询数据库得到CSs
                                QStringList Css=getCs(fileHash);
                                QString CS;
                                for(int i=0;i<Css.size()-1;i++)CS+=(Css[i]+"#");
                                CS+=Css[Css.size()-1];
                                firstUpload=false;
                                QString res_CS=QString("second@%1").arg(CS);
                                ui->textEdit->append(res_CS);
                                tcpSocket1->write(res_CS.toUtf8().data());
                            }
                        }else if(splitArray[2]=="download"){//下载
                            type=3;
                            qDebug()<<"-----------客户端请求下载文件";
                            fileHash=splitArray[3];
                            //从数据库查找css
                            QStringList Css=getCs(fileHash);
                            QString CS;
                            for(int i=0;i<Css.size()-1;i++)CS+=(Css[i]+"#");
                            CS+=Css[Css.size()-1];
                            //发送css
                            tcpSocket1->write(CS.toLatin1());
                        }else{//更新s，收到@@changes@fileHash
                            qDebug()<<"客户端请求更新s";
                            type=4;
                            fileHash=splitArray[3];
                            qDebug()<<"收到哈希值:"<<fileHash;
                            //查询所有cs
                            QStringList Css=getCs(fileHash);
                            QString CS;
                            for(int i=0;i<Css.size()-1;i++)CS+=(Css[i]+"#");
                            CS+=Css[Css.size()-1];
                            QString res_CS=QString("changes@%1").arg(CS);
                            ui->textEdit->append(res_CS);
                            tcpSocket1->write(res_CS.toLatin1());
                        }
                    }else{
                        if(type==2){//上传
                            if(firstUpload){//首次上传
                                //打开文件,以CS的前20个字符作为文件名
                                QString cs=splitArray[0];
                                QString pow=splitArray[1];
                                filePath= "/home/weakdog/cspfiles/"+cs.mid(0, 30);
                                recvSize=0;
                                file.setFileName(filePath);
                                bool isOk=file.open(QIODevice::WriteOnly);
                                if(isOk==false){
                                    ui->textEdit->append("open error");
                                }
                                //将文件信息存入数据库
                                QString sq=QString("INSERT INTO files (h,pow,cs) VALUES('%1','%2','%3')").arg(fileHash).arg(pow).arg(cs);
                                ui->textEdit->append(sq);
                                if(!sql_query.exec(sq)){
                                    ui->textEdit->append("插入数据库失败");
                                }else{
                                    ui->textEdit->append("插入数据库成功");
                                }
                            }else{//不是首次上传
                                if(splitArray[0]=="mismatch"){//mismatch@cs@pow@user
                                    //打开文件,以CS的前20个字符作为文件名
                                    QString cs=splitArray[1];
                                    QString pow=splitArray[2];
                                    filePath= "/home/weakdog/cspfiles/"+cs.mid(0, 30);
                                    recvSize=0;
                                    file.setFileName(filePath);
                                    bool isOk=file.open(QIODevice::WriteOnly);
                                    if(isOk==false){
                                        ui->textEdit->append("open error");
                                    }
                                    //将文件信息存入数据库
                                    QString sq=QString("INSERT INTO files (h,pow,cs) VALUES('%1','%2','%3')").arg(fileHash).arg(pow).arg(cs);
                                    ui->textEdit->append(sq);
                                    if(!sql_query.exec(sq)){
                                        ui->textEdit->append("插入数据库失败");
                                    }else{
                                        ui->textEdit->append("插入数据库成功");
                                    }
                                }else{//match@cs@pow
                                    QString cs=splitArray[1];
                                    QString pow=splitArray[2];
                                    sql_query.exec(QString("select * from files where cs='%1' and pow='%2'").arg(cs).arg(pow));
                                    if(sql_query.next()){
                                        tcpSocket1->write("success");
                                        ui->textEdit->append("所有权证明成功");
                                    }else{
                                        tcpSocket1->write("fail");
                                        ui->textEdit->append("所有权证明失败");
                                        }
                                    }
                                }
                            messages.clear();
                        }else if(type==3){//下载，接收cs
                            qDebug()<<"-----------------接收cs";
                            QString cs=splitArray[0];
                            qDebug()<<cs;
                            //查询数据库，找到ckrd发给ks，并发送文件
                            QString sq=QString("select ckr_d from files where cs='%1'").arg(cs);
                            ui->textEdit->append(sq);
                            sql_query.exec(sq);
                            sql_query.next();
                            QString ckrd=sql_query.value(0).toString();
                            filePath="/home/weakdog/cspfiles/"+cs.mid(0,30);
                            sendSize=0;
                            QFileInfo info(filePath);
                            fileSize=info.size();
                            ui->textEdit->append("filesize:"+QString::number(fileSize));
                            file.setFileName(filePath);
                            file.open(QIODevice::ReadOnly);
                            //向client发送文件大小信息
                            tcpSocket1->write(QString::number(fileSize).toLatin1());
                            //发送文件内容
                            sendData();
                            //连接ks发送ckrd
                            QString ip="127.0.0.1";
                            qint64 port=10006;
                            QString down="download@"+ckrd;
                            ui->textEdit->append(down);
                            tcpSocket2->connectToHost(QHostAddress(ip),port);
                            tcpSocket2->write(down.toLatin1());
                            ui->textEdit->append("向ks发送Ckrd成功");
                            tcpSocket2->disconnectFromHost();
                            tcpSocket2->close();
                            messages.clear();
                        }else if(type==4){
                            if(messages.size()==2){//收到changes@cs@pow,进行所有权证明
                                cs=splitArray[1];
                                QString pow=splitArray[2];
                                sql_query.exec(QString("select * from files where cs='%1' and pow='%2'").arg(cs).arg(pow));
                                if(sql_query.next()){
                                    QString ckrd_=sql_query.value(3).toString();
                                    fileName=sql_query.value(1).toString().mid(0,30);
                                    QString cckrd="changes@"+ckrd_;
                                    //连接ks发送ckrd
                                    QString ip="127.0.0.1";
                                    qint64 port=10006;
                                    ui->textEdit->append(cckrd);
                                    tcpSocket2->connectToHost(QHostAddress(ip),port);
                                    tcpSocket2->write(cckrd.toLatin1());
                                    ui->textEdit->append("向ks发送changes@ckrd成功");
                                    tcpSocket2->disconnectFromHost();
                                    tcpSocket2->close();
                                    //向ks发送ckrd:changes@ckrd
                                    tcpSocket1->write("success");
                                    ui->textEdit->append("所有权证明成功");
                                }else{
                                    tcpSocket1->write("fail");
                                    ui->textEdit->append("所有权证明失败");
                                    messages.clear();
                                }
                            }else{//收到新的cs@pow
                                qDebug()<<"收到新的cs@pow";
                                QString cs2=splitArray[0];
                                QString pow2=splitArray[1];
                                qDebug()<<"cs2:"<<cs2;
                                qDebug()<<"pow2:"<<pow2;
                                //更新数据库
                                QString sq=QString("update files set pow = '%1' where cs='%3'").arg(pow2).arg(cs);
                                sql_query.exec(sq);
                                QString sq2=QString("update files set cs = '%1' where h='%2' and pow='%3'").arg(cs2).arg(fileHash).arg(pow2);
                                sql_query.exec(sq2);
                                messages.clear();
                                //更改文件名
                                QString filename2=cs2.mid(0,30);
                                QString rename=QString("mv /home/weakdog/cspfiles/%1 /home/weakdog/cspfiles/%2").arg(fileName).arg(filename2);
                                system(rename.toLatin1().data());
                            }
                        }
                        }
                    }
                        );
            });

    //监听ks端连接
    connect(tcpServer2,&QTcpServer::newConnection,
            [=]()
            {
                //取出建立好的套接字
                tcpSocket2=tcpServer2->nextPendingConnection();
                //获取对方的IP和端口
                QString ip=tcpSocket2->peerAddress().toString();
                quint16 port=tcpSocket2->peerPort();
                QString str=QString("[%1:%2]成功连接").arg(ip).arg(port);

                connect(tcpSocket2,&QTcpSocket::readyRead,
                        [=]()
                        {
                            QByteArray array=tcpSocket2->readAll();
                            QStringList splitArray=QString(array).split("@");
                            tcpSocket2->disconnectFromHost();
                            tcpSocket2->close();
                            qDebug()<<"从ks收到消息(Ckrd): "+QString(array);
                            if(splitArray[0]=="changed"){//ks请求更新d
                                qDebug()<<"------------ks请求更新d";
                                CsCkrd.clear();
                                //查询数据库，得到所有cs@ckrd
                                sql_query.exec(QString("select * from files"));
                                while(sql_query.next())
                                {
                                    QString cs=sql_query.value(1).toString();
                                    QString ckrd=sql_query.value(3).toString();
                                    QString csckrd=cs+"@"+ckrd;
                                    qDebug()<<"csckrd:"<<csckrd;
                                    CsCkrd.append(csckrd);
                                }
                                sentCkrd=0;
                                if(CsCkrd.size()>0){
                                    //连接ks发送ckrd
                                    QString ip="127.0.0.1";
                                    qint64 port=10006;
                                    QString ccc="ckrd@"+CsCkrd[sentCkrd];
                                    tcpSocket2->connectToHost(QHostAddress(ip),port);
                                    tcpSocket2->write(ccc.toLatin1());
                                    tcpSocket2->disconnectFromHost();
                                    tcpSocket2->close();
                                    ui->textEdit->append("发送ckrd成功");
                                    sentCkrd++;
                                }
                                //发送ckrd@cs@ckrd
                            }else if(splitArray[0]=="upload"){//upload@cs@ckrd
                                //更新数据库
                                QString sq=QString("update files set ckr_d = '%1' where cs='%2'").arg(splitArray[2]).arg(splitArray[1]);
                                sql_query.exec(sq);
                            }else if(splitArray[0]=="ckrd2"){//ckrd2@cs@ckrd
                                //更新数据库
                                QString sq=QString("update files set ckr_d = '%1' where cs='%2'").arg(splitArray[2]).arg(splitArray[1]);
                                sql_query.exec(sq);
                                //发送下一ckrd@cs@ckrd，用一个全局变量存储
                                if(sentCkrd<CsCkrd.size()){
                                    QString ccc="ckrd@"+CsCkrd[sentCkrd];
                                    QString ip="127.0.0.1";
                                    qint64 port=10006;
                                    tcpSocket2->connectToHost(QHostAddress(ip),port);
                                    tcpSocket2->write(ccc.toLatin1());
                                    tcpSocket2->disconnectFromHost();
                                    tcpSocket2->close();
                                    sentCkrd++;
                                }
                                if(sentCkrd==CsCkrd.size()){//发送结束标志
                                    QString finish="finish";
                                    QString ip="127.0.0.1";
                                    qint64 port=10006;
                                    tcpSocket2->connectToHost(QHostAddress(ip),port);
                                    tcpSocket2->write(finish.toLatin1());
                                    tcpSocket2->disconnectFromHost();
                                    tcpSocket2->close();
                                    sentCkrd++;
                                    ui->textEdit->append("更新ckrd成功");
                                }
                            }
                        });
            });

    //监听客户端连接(文件端口)
    connect(tcpServer3,&QTcpServer::newConnection,
            [=]()
            {
                //取出建立好的套接字
                tcpSocket3=tcpServer3->nextPendingConnection();
                //获取对方的IP和端口
                QString ip=tcpSocket3->peerAddress().toString();
                quint16 port=tcpSocket3->peerPort();
                QString str=QString("[%1:%2]成功连接(文件)").arg(ip).arg(port);
                ui->textEdit->append(str);

                //接收文件
                connect(tcpSocket3,&QTcpSocket::readyRead,
                        [=]()
                        {
                            QByteArray buf=tcpSocket3->readAll();
                            //文件信息
                            qint64 len=file.write(buf);
                            recvSize+=len;
                            qDebug()<<recvSize;
                            if(recvSize==fileSize){
                                file.close();
                                ui->textEdit->append("接收文件完成");
                                QMessageBox::information(this,"完成","文件接收完成");
                            }
                        }
                        );
            });
}

//发送文件内容
void Csp::sendData()
{
    qint64 len=0;
    do{
        char buf[4*1024]={0};
        //往文件中读数据
        len=file.read(buf,sizeof(buf));
        //发送数据
        tcpSocket3->write(buf,len);
        sendSize+=len;
    }while(len>0);
    //是否发送文件完毕
    if(sendSize==fileSize){
        ui->textEdit->append("文件发送完毕");
        file.close();
        //断开客户端
    }
}

Csp::~Csp()
{
    delete ui;
}

//查询数据库判断密码是否正确
bool Csp::checkLogin(QString account, QString password){
    sql_query.exec(QString("select * from login where account='%1' and password='%2'").arg(account).arg(password));
    if(sql_query.next()){
        return true;
    }else{
        return false;
    }
}

//查询数据库判断是否为首次上传
bool Csp::checkFirst(QString fileHash){
    sql_query.exec(QString("select * from files where h='%1'").arg(fileHash));
    if(sql_query.next()){
        return false;
    }else{
        return true;
    }
}

//查询数据库得到哈希值对应的所有cs
QStringList Csp::getCs(QString fileHash){
    QStringList Css;
    sql_query.exec(QString("select cs from files where h='%1'").arg(fileHash));
    while(sql_query.next())
    {
        Css.append(sql_query.value(0).toString());
    }
    return Css;
}
