#include "client.h"
#include "ui_client.h"

Client::Client(QString account_,QString password_,QWidget *parent) :
    QWidget(parent),
    ui(new Ui::Client)
{
    ui->setupUi(this);
    key=new unsigned char[32];
    account=account_;
    password=password_;
    if(QSqlDatabase::contains("qt_sql_default_connection"))
        database = QSqlDatabase::database("qt_sql_default_connection");
    else
        database=QSqlDatabase::addDatabase("QSQLITE");
    database.setDatabaseName("client.db");
    if(!database.open()){
        qDebug()<<"Error: Fail to connect database"<<database.lastError();
    }else{
        qDebug()<<"Succeed to connect database";
    }
    fileModel=new QSqlTableModel(this);
    fileModel->setTable("fileName");
    ui->tableView->setModel(fileModel);
    fileModel->select();
    fileModel->setHeaderData(0,Qt::Horizontal,"文件名");
    sql_query=QSqlQuery(database);
    type=0;
    //初始化套接字
    tcpServer1=new QTcpServer(this);
    tcpSocket1=new QTcpSocket(this);
    tcpSocket2=new QTcpSocket(this);
    tcpSocket3=new QTcpSocket(this);
    tcpServer1->listen(QHostAddress::Any,10001);

    //监听ks消息
    connect(tcpServer1,&QTcpServer::newConnection,
            [=](){
                //取出建立好的套接字
                tcpSocket2=tcpServer1->nextPendingConnection();
                //获取对方的IP和端口
                QString ip=tcpSocket2->peerAddress().toString();
                quint16 port=tcpSocket2->peerPort();
                QString str=QString("[%1:%2]成功连接(ks)").arg(ip).arg(port);
                ui->textEdit->append("ks:"+str);
                connect(tcpSocket2,&QTcpSocket::readyRead,
                        [=]()
                        {
                            array=tcpSocket2->readAll();
                            tcpSocket2->disconnect();
                            tcpSocket2->close();
                            splitArray=QString(array).split("@");
                            if(splitArray[0]=="download"){
                                qDebug()<<"---------------下载文件的ckr-----------";
                                //计算hs
                                QString hs=Security().hmacS(s.toLatin1(),fileHash.toLatin1());
                                qDebug()<<"----------------------hs:"<<hs;
                                unsigned char* hsc=new unsigned char[64];
                                hsc=(unsigned char*)(hs.toLatin1().data());
                                qDebug()<<"------------收到ckr"<<splitArray[1];
                                //解密,打开文件
                                QFile tmp;
                                tmp.setFileName("/home/weakdog/clientfiles/ckr.txt");
                                tmp.open(QIODevice::WriteOnly);
                                tmp.write(QByteArray::fromHex(splitArray[1].toLatin1()));
                                tmp.close();
                                Security().Decrypt_File(hsc,"/home/weakdog/clientfiles/ckr.txt");
                                QFile re("/home/weakdog/clientfiles/ckr.txt");
                                re.remove();
                                //读取文件得到kr
                                QFile tmp2;
                                tmp2.setFileName("/home/weakdog/clientfiles/ckr.txt.decrypted");
                                tmp2.open(QIODevice::ReadOnly);
                                QByteArray krb=tmp2.readAll();
                                tmp2.remove();
                                unsigned char * kr=new unsigned char[32];
                                kr = (unsigned char *)krb.data();
                                //解密文件
                                Security().Decrypt_File(kr,filePath);
                                //删除密文
                                QFile re2(filePath);
                                re2.close();
                                //更改文件名
                                QString rename=QString("mv %1 %2").arg(filePath+".decrypted").arg(filePath);
                                qDebug()<<rename;
                                system(rename.toLatin1().data());
                            }else{//changes@ckr
                                //计算hs
                                qDebug()<<"-----------------更新s的ckr";
                                QString hs=Security().hmacS(s.toLatin1(),fileHash.toLatin1());
                                qDebug()<<"olds"<<s;
                                qDebug()<<"oldhs:"<<hs;
                                unsigned char* hsc=new unsigned char[64];
                                hsc=(unsigned char*)(hs.toLatin1().data());
                                //解密,打开文件
                                QFile tmp;
                                tmp.setFileName("/home/weakdog/clientfiles/ckr.txt");
                                tmp.open(QIODevice::WriteOnly);
                                tmp.write(QByteArray::fromHex(splitArray[1].toLatin1()));
                                tmp.close();
                                Security().Decrypt_File(hsc,"/home/weakdog/clientfiles/ckr.txt");
                                //读取文件得到kr
                                QFile tmp2;
                                tmp2.setFileName("/home/weakdog/clientfiles/ckr.txt.decrypted");
                                tmp2.open(QIODevice::ReadOnly);
                                QByteArray krb=tmp2.readAll();
                                tmp2.close();
                                key = (unsigned char *)krb.data();
                                //生成新的s
                                unsigned char* sb=new unsigned char[32];
                                Security().get_rand_key(sb,32);//generate random key
                                s=QByteArray((char*)sb,32).toHex();
                                qDebug()<<"news:"<<s;
                                QString Cs=calcCs(s);
                                qDebug()<<"newcs:"<<Cs;
                                //计算新的hs
                                hs=Security().hmacS(s.toLatin1(),fileHash.toLatin1());
                                qDebug()<<"newhs: "<<hs;
                                //计算新的pow
                                QString pow=Security().hmac(s.toLatin1(),filePath);
                                qDebug()<<"newpow:"<<pow;
                                QString cp=Cs+"@"+pow;
                                tcpSocket1->write(cp.toLatin1());
                                ui->textEdit->append("发送新的cs@pow成功");
                                //计算新的ckr
                                QByteArray kr=QByteArray((char*)key,32);
                                //计算ckr
                                QFile tmp3;
                                tmp3.setFileName("/home/weakdog/clientfiles/kr.txt");
                                tmp3.open(QIODevice::WriteOnly);
                                tmp3.write(kr);
                                tmp3.close();
                                //加密文件
                                hsc=(unsigned char*)(hs.toLatin1().data());
                                Security().Encrypt_File(hsc,"/home/weakdog/clientfiles/kr.txt");
                                ui->textEdit->append("加密kr完成");
                                QFile tmp4;
                                //读取文件ckr
                                tmp4.setFileName("/home/weakdog/clientfiles/kr.txt.encrypted");
                                tmp4.open(QIODevice::ReadOnly);
                                QByteArray qb=tmp4.readAll();
                                tmp4.remove();
                                QString ckr2=qb.toHex();
                                qDebug()<<"ckr2: "<<ckr2;
                                QString cc=Cs+"@"+ckr2;
                                QString ip="127.0.0.1";
                                qint64 port=10005;
                                tcpSocket2->connectToHost(QHostAddress(ip),port);
                                tcpSocket2->write(cc.toLatin1());
                                tcpSocket1->disconnectFromHost();
                                tcpSocket1->close();
                                tcpSocket2->disconnectFromHost();
                                tcpSocket2->close();
                                ui->textEdit->append("发送cs2@Ckr2成功");
                            }
                        });
            });

    //从csp接收消息
    connect(tcpSocket1,&QTcpSocket::readyRead,
            [=]()
            {
                array=tcpSocket1->readAll();
                messages.append(array);
                splitArray=QString(array).split("@");
                ui->textEdit->append("收到消息"+QString(array));
                if(type==1){//上传
                    if(messages.size()==1){//接收的为结果
                        if(splitArray[0]=="first"){//首次上传
                            ui->textEdit->append("本次上传为首次上传");
                            //calc Cs
                            //generate 32 random sb
                            unsigned char* sb=new unsigned char[32];
                            Security().get_rand_key(sb,32);//generate random key
                            s=QByteArray((char*)sb,32).toHex();
                            QString Cs=calcCs(s);
                            //calc Pow
                            QString pow=Security().hmac(s.toLatin1(),filePath);
                            //发送Cs@pow
                            QString cp=QString("%1@%2").arg(Cs).arg(pow);
                            qint32 len2=tcpSocket1->write(cp.toUtf8().data());
                            if(len2>0)ui->textEdit->append("发送CS@POW成功");
                            else ui->textEdit->append("发送CS@POW失败");
                            //发送文件
                            sendData();
                            //计算hs
                            QString hs=Security().hmacS(s.toLatin1(),fileHash.toLatin1());
                            qDebug()<<"hs: "<<hs;
                            //kr转qbytearray
                            QByteArray kr=QByteArray((char*)key,32);
                            //计算ckr
                            QFile tmp;
                            tmp.setFileName("/home/weakdog/clientfiles/kr.txt");
                            tmp.open(QIODevice::WriteOnly);
                            tmp.write(kr);
                            tmp.close();
                            qDebug()<<"写入kr成功";
                            //加密文件
                            unsigned char* hsc=new unsigned char[64];
                            hsc=(unsigned char*)(hs.toLatin1().data());
                            Security().Encrypt_File(hsc,"/home/weakdog/clientfiles/kr.txt");
                            QFile re("/home/weakdog/clientfiles/kr.txt");
                            re.remove();
                            qDebug()<<"---------加密kr完成";
                            QFile tmp2;
                            //读取文件ckr
                            tmp2.setFileName("/home/weakdog/clientfiles/kr.txt.encrypted");
                            tmp2.open(QIODevice::ReadOnly);
                            QByteArray qb=tmp2.readAll();
                            tmp2.remove();
                            QString ckr=qb.toHex();
                            qDebug()<<"-------------------生成ckr: "<<ckr;
                            QString cc=Cs+"@"+ckr;
                            ui->textEdit->append(cc);
                            //连接ks发送cs@Ckr
                            QString ip="127.0.0.1";
                            qint64 port=10005;
                            tcpSocket2->connectToHost(QHostAddress(ip),port);
                            tcpSocket2->write(cc.toLatin1());
                            ui->textEdit->append("发送cc@Ckr成功");
                            //断开连接
                            tcpSocket1->disconnectFromHost();
                            tcpSocket1->close();
                            tcpSocket2->disconnectFromHost();
                            tcpSocket2->close();
                            tcpSocket3->disconnectFromHost();
                            tcpSocket3->close();
                            //插入数据库
                            QString sq=QString("INSERT INTO file (filename,h) VALUES('%1','%2')").arg(fileName).arg(fileHash);
                            ui->textEdit->append(sq);
                            if(!sql_query.exec(sq)){
                                ui->textEdit->append("插入数据库失败");
                                qDebug() << sql_query.lastError();
                            }else{
                                ui->textEdit->append("插入数据库成功");
                                fileModel->select();
                            }
                            messages.clear();
                        }else{//非首次上传，second@Css
                            ui->textEdit->append("本次上传非首次上传");
                            QString Css=splitArray[1];
                            QString s;
                            QString compRes=compareCs(Css,s);
                            if(compRes=="mismatch"){//first upload
                                //send mismatch@cs@pow@users
                                unsigned char* sb=new unsigned char[32];
                                Security().get_rand_key(sb,32);//generate random key
                                QString s=QByteArray((char*)sb,32).toHex();
                                QString Cs=calcCs(s);
                                //calc Pow
                                QString pow=Security().hmac(s.toLatin1(),filePath);
                                //发送mismatch@Cs@pow@users
                                QString share;
                                for(int i=0;i<users.size()-1;i++){
                                    share.append(users[i]+"#");
                                }
                                share.append(users[users.size()-1]);
                                QString cp=QString("mismatch@%1@%2@%3").arg(Cs).arg(pow).arg(share);
                                qint32 len2=tcpSocket1->write(cp.toUtf8().data());
                                if(len2>0)ui->textEdit->append("发送mismatch@CS@POW@user成功");
                                else ui->textEdit->append("发送失败");
                                //发送文件
                                sendData();
                                //计算hs
                                QString hs=Security().hmacS(s.toLatin1(),fileHash.toLatin1());
                                qDebug()<<"hs: "<<hs;
                                //kr转qbytearray
                                QByteArray kr=QByteArray((char*)key,32);
                                //计算ckr
                                QFile tmp;
                                tmp.setFileName("/home/weakdog/clientfiles/kr.txt");
                                tmp.open(QIODevice::WriteOnly);
                                tmp.write(kr);
                                tmp.close();
                                qDebug()<<"写入kr成功";
                                //加密文件
                                unsigned char* hsc=new unsigned char[64];
                                hsc=(unsigned char*)(hs.toLatin1().data());
                                Security().Encrypt_File(hsc,"/home/weakdog/clientfiles/kr.txt");
                                ui->textEdit->append("加密kr完成");
                                QFile re("/home/weakdog/clientfiles/kr.txt");
                                re.remove();
                                QFile tmp2;
                                //读取文件ckr
                                tmp2.setFileName("/home/weakdog/clientfiles/kr.txt.encrypted");
                                tmp2.open(QIODevice::ReadOnly);
                                QByteArray qb=tmp2.readAll();
                                tmp2.remove();
                                QString ckr=qb.toHex();
                                qDebug()<<"ckr: "<<ckr;
                                QString cc=Cs+"@"+ckr;
                                ui->textEdit->append(cc);
                                //连接ks发送cs@Ckr
                                QString ip="127.0.0.1";
                                qint64 port=10005;
                                tcpSocket2->connectToHost(QHostAddress(ip),port);
                                tcpSocket2->write(cc.toLatin1());
                                ui->textEdit->append("发送cc@Ckr成功");
                                //断开连接
                                tcpSocket1->disconnectFromHost();
                                tcpSocket1->close();
                                tcpSocket2->disconnectFromHost();
                                tcpSocket2->close();
                                tcpSocket3->disconnectFromHost();
                                tcpSocket3->close();
                                //插入数据库
                                QString sq=QString("INSERT INTO file (filename,h) VALUES('%1','%2')").arg(fileName).arg(fileHash);
                                ui->textEdit->append(sq);
                                if(!sql_query.exec(sq)){
                                    ui->textEdit->append("插入数据库失败");
                                    qDebug() << sql_query.lastError();
                                }else{
                                    ui->textEdit->append("插入数据库成功");
                                    fileModel->select();
                                }
                                messages.clear();
                            }else{
                                file.close();
                                QString pow=Security().hmac(s.toLatin1(),filePath);
                                //发送match@cs@pow
                                QString mcp=QString("match@%1@%2").arg(compRes).arg(pow);
                                tcpSocket1->write(mcp.toUtf8().data());
                                ui->textEdit->append("发送pow成功");
                            }
                        }
                    }else{//非首次上传:接收的是所有权证明结果
                        QString powres=QString(array);
                        if(powres=="success"){
                            ui->textEdit->append("所有权证明成功");
                            //断开连接
                            tcpSocket1->disconnectFromHost();
                            tcpSocket1->close();
                            tcpSocket3->disconnectFromHost();
                            tcpSocket3->close();
                            //插入数据库
                            QString sq=QString("INSERT INTO file (filename,h) VALUES('%1','%2')").arg(fileName).arg(fileHash);
                            ui->textEdit->append(sq);
                            if(!sql_query.exec(sq)){
                                ui->textEdit->append("插入数据库失败");
                                qDebug() << sql_query.lastError();
                            }else{
                                ui->textEdit->append("插入数据库成功");
                                fileModel->select();
                            }
                        }else{
                            ui->textEdit->append("所有权证明失败");
                            //断开连接
                            tcpSocket1->disconnectFromHost();
                            tcpSocket1->close();
                            tcpSocket3->disconnectFromHost();
                            tcpSocket3->close();
                        }
                        messages.clear();
                    }
                }else if(type==2){//下载
                    if(messages.size()==1){//接收到Css
                        QString compRes=compareCs(splitArray[0],s);
                        qDebug()<<"---------------compRes"<<compRes;
                        //发送cs
                        tcpSocket1->write(compRes.toLatin1());
                        ui->textEdit->append("cs发送成功");
                    }else{//接收到文件大小
                        recvSize=0;
                        fileSize=QString(array).toInt();
                        ui->textEdit->append("收到文件大小为:"+QString::number(fileSize));
                        messages.clear();
                    }
                }else if(type==3){//更新s：changes@cs
                    if(messages.size()==1){//接收的为changes@css
                        QString s;
                        QString compRes=compareCs(splitArray[1],s);
                        //计算pow
                        ui->textEdit->append(filePath);
                        QString pow=Security().hmac(s.toLatin1(),filePath);
                        //发送changes@cs@pow
                        QString ccp=QString("changes@%1@%2").arg(compRes).arg(pow);
                        tcpSocket1->write(ccp.toLatin1());
                        qDebug()<<"发送ccp成功:"+ccp;
                    }else{//收到所有权证明结果
                        if(splitArray[0]=="success"){
                            qDebug()<<"所有权证明成功";
                            ui->textEdit->append("所有权证明成功");
                        }else{
                            ui->textEdit->append("所有权证明失败");
                            //断开连接
                            tcpSocket1->disconnectFromHost();
                            tcpSocket1->close();
                            tcpSocket3->disconnectFromHost();
                            tcpSocket3->close();
                        }
                        messages.clear();
                    }
                }
            }
            );

    //从csp接收文件
    connect(tcpSocket3,&QTcpSocket::readyRead,
            [=]()
            {
                QByteArray buf=tcpSocket3->readAll();
                qint64 len=file.write(buf);
                recvSize+=len;
                qDebug()<<recvSize;
                if(recvSize==fileSize){
                    file.close();
                    ui->textEdit->append("接收文件完成");
                    QMessageBox::information(this,"完成","文件接收完成");
                    //断开连接
                    tcpSocket1->disconnectFromHost();
                    tcpSocket1->close();
                    tcpSocket3->disconnectFromHost();
                    tcpSocket3->close();
                    recvSize=0;
                }
            }
            );
}

Client::~Client()
{
    delete ui;
}

//选择文件,生成随机密文
void Client::on_pushButtonSelect_clicked()
{
    filePath=QFileDialog::getOpenFileName(this,"open","../");
    if(filePath.isEmpty()==false){//如果选择文件路径有效
        ui->textEdit->append(filePath);
        fileName.clear();
        fileSize=0;
        sendSize=0;
        //计算文件哈希值
        fileHash=Security().sha_f(filePath);
        //TODO 生成随机秘钥
        Security().get_rand_key(key, 32);
        Security().Encrypt_File(key,filePath);//Cm
        //获取密文信息
        QFileInfo info(filePath);
        fileName=info.fileName();
        QString filePath2=filePath+".encrypted";
        info=QFileInfo(filePath2);
        fileSize=info.size();
        qDebug()<<fileSize;
        //以只读方式打开文件
        file.setFileName(filePath2);
        bool isOk=file.open(QIODevice::ReadOnly);
        if(isOk==false){
            ui->textEdit->append("open false");
            file.close();
        }
    }else{
         ui->textEdit->append("select file fault");
    }
}

//发送文件
void Client::on_pushButtonUpload_clicked()
{
    qDebug()<<"------------------------请求上传文件";
    //获取服务器的IP和端口
    QString ip="127.0.0.1";
    qint64 port=10002;
    qint64 port2=10004;
    tcpSocket1->connectToHost(QHostAddress(ip),port);
    tcpSocket3->connectToHost(QHostAddress(ip),port2);
    ui->textEdit->append("与csp建立连接成功");
    type=1;
    //get selected users
    users.clear();
    QList<QListWidgetItem *>selected=ui->listWidget->selectedItems();
    for(int i=0;i<selected.size();i++){
        users.append(selected[i]->text());
    }
    //发送上传指令,文件名，文件大小，文件哈希
    command=QString("@@upload@%1@%2").arg(fileSize).arg(fileHash);
    qDebug()<<"发送文件上传命令"+command;
    qint64 len=tcpSocket1->write(command.toUtf8().data());
    if(len<=0){//发送头部信息成功
        ui->textEdit->append("command send false");
        file.close();
    }
}

//下载文件
void Client::on_pushButtonDownload_clicked()
{
    qDebug()<<"-----------------------------请求下载文件";
    //获取选择的文件名
    int curRow=ui->tableView->currentIndex().row(); //选中行
    QAbstractItemModel *modessl = ui->tableView->model();
    QModelIndex indextemp = modessl->index(curRow,0);//遍历第一行的所有列
    fileName = modessl->data(indextemp).toString();
    qDebug()<<"选中文件: "+fileName;
    filePath="/home/weakdog/clientfiles2/"+fileName;
    //获取服务器的IP和端口
    QString ip="127.0.0.1";
    qint64 port=10002;
    qint64 port2=10004;
    tcpSocket1->connectToHost(QHostAddress(ip),port);
    tcpSocket3->connectToHost(QHostAddress(ip),port2);
    ui->textEdit->append("与csp建立连接成功");
    type=2;
    //从数据库中读取文件名对应的哈希值
    QString sq=QString("select h from file where filename='%1'").arg(fileName);
    ui->textEdit->append(sq);
    sql_query.exec(sq);
    sql_query.next();
    fileHash=sql_query.value(0).toString();
    ui->textEdit->append("从数据库中读到哈希值:"+fileHash);
    //发送下载指令
    command=QString("@@download@%1").arg(fileHash);//换成文件哈希值
    //发送文件头信息
    ui->textEdit->append("发送下载文件命令:"+command);
    qint64 len=tcpSocket1->write(command.toUtf8().data());
    if(len>0){
        //打开文件
        file.setFileName(filePath);
        bool isOk=file.open(QIODevice::WriteOnly);
        if(isOk==false){
            ui->textEdit->append("open error");
        }
    }
    recvSize=0;
}

//发送文件内容
void Client::sendData()
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
        sendSize=0;
        file.remove();
//        tcpSocket3->disconnectFromHost();
//        tcpSocket3->close();
    }
}

QString Client::calcCs(QString s){
    //storage s into file
    QFile fileS;
    fileS.setFileName("/home/weakdog/clientfiles/s.txt");
    fileS.open(QIODevice::WriteOnly);
    fileS.write(s.toLatin1());
    fileS.close();
    //cpabe-enc s
    QString command="cd /home/weakdog/clientfiles&&cpabe-enc pub_key s.txt '";
    for(int i=0;i<users.size()-1;i++)command+=QString("%1 or ").arg(users[i]);
    command+=users[users.size()-1];
    command+="'";
    qDebug()<<command;
    system(command.toLatin1().data());
    //read cypher to bytearray
    QFile fileCypherS;
    fileS.setFileName("/home/weakdog/clientfiles/s.txt.cpabe");
    fileS.open(QIODevice::ReadOnly);
    QByteArray buf=fileS.readAll();
    qDebug()<<"buf.size():"<<buf.size();
    fileS.remove();
    QString Cs=buf.toHex();
    return Cs;
}

bool Client::DecCs(QString cs,QString&s)
{
    QByteArray qb = QByteArray::fromHex (cs.toLatin1().data());
    //story cs into file
    QFile file1;
    file1.setFileName("/home/weakdog/clientfiles/s.txt.cpabe");
    file1.open(QIODevice::WriteOnly);
    file1.write(qb);
    file1.close();
    //cpabe-dec
    QString command="cd /home/weakdog/clientfiles&&cpabe-dec pub_key user1_key s.txt.cpabe";
    qDebug()<<command;
    system(command.toLatin1().data());
    QFile file2;
    file2.setFileName("/home/weakdog/clientfiles/s.txt");
    bool isOk=file2.open(QIODevice::ReadOnly);
    if(isOk==false){
        qDebug()<<"dec false";
        file2.close();
        return false;
    }else{
        QFile file2;
        file2.setFileName("/home/weakdog/clientfiles/s.txt");
        file2.open(QIODevice::ReadOnly);
        QByteArray sb=file2.readAll();
        s=QString(sb);
        file2.remove();
        return true;
    }
}

QString Client::compareCs(QString Cs,QString& s){
    QStringList csList=Cs.split("#");
    for(int i=0;i<csList.size();i++){
        if(DecCs(csList[i],s)){
            return csList[i];
        }
    }
    return "mismatch";
}

//更新s
void Client::on_pushButtonChange_clicked()
{
    qDebug()<<"---------------------------请求更新s";
    QString ip="127.0.0.1";
    qint64 port=10002;
    tcpSocket1->connectToHost(QHostAddress(ip),port);
    //获取选取的文件名
    type=3;
    int curRow=ui->tableView->currentIndex().row(); //选中行
    QAbstractItemModel *modessl = ui->tableView->model();
    QModelIndex indextemp = modessl->index(curRow,0);//遍历第一行的所有列
    fileName = modessl->data(indextemp).toString();
    ui->textEdit->append("选中文件: "+fileName);
    filePath="/home/weakdog/clientfiles/"+fileName;
    //查询文件哈希值
    QString sq=QString("select h from file where filename='%1'").arg(fileName);
    qDebug()<<sq;
    sql_query.exec(sq);
    sql_query.next();
    fileHash=sql_query.value(0).toString();
    QString change="@@changes@"+fileHash;
    qDebug()<<change;
    tcpSocket1->write(change.toLatin1());
}
