> 框架

#### 客户端：

上传：

主动与csp和ks通信：socket1、socket2

下载：

主动与csp通信socket1

接收ks消息serversocket

#### csp：

上传：

接收客户端命令serversocket1

接收ks消息serversocket2

处理客户端命令tcpsocket

处理ks消息tcpsocket2

下载：

接收客户端命令serversocket1

处理客户端请求tcpsocket

主动与ks通信tcpsocket2

#### ks：

上传：

接收客户端消息serversocket1

处理客户端消息tcpsocket1

主动与csp通信tcpsocket2

下载：

接收csp消息serversocket2

主动与客户端通信tcpsocket1

## 端口：

10001：客户端监听ks消息端口

------------------------------------------

10002：csp监听客户端消息端口

10003：csp监听ks消息端口

10004：csp监听客户端文件端口

---------------------------------------------

10005：ks监听客户端消息端口

10006：ks接收csp消息端口

## client

serversocket1：监听ks消息套接字

tcpsocket1：与csp消息传输套接字

tcpsocket2：与ks消息传输套接字

tcpsocket3：与csp文件传输套接字

## csp

serversocket1：监听客户端消息套接字

serversocket2：监听ks消息套接字

serversocket3：监听客户端文件套接字

tcpsocket1：与客户端消息传输套接字

tcpsocket2：与ks消息传输套接字

tcpsocket3：与客户端文件传输套接字

## ks

serversocket1：监听客户端消息套接字

serversocket2：监听csp消息套接字

tcpsocket1：与客户端消息传输套接字

tcpsocket2：与csp消息传输套接字

> cpabe命令

```shell
#cpabe初始化，生成master_key和pub_key
cpabe-setup

#创建私钥user1_key
cpabe-keygen -o user1_key pub_key master_key 'user1'
cpabe-keygen -o user2_key pub_key master_key 'user2'
cpabe-keygen -o user3_key pub_key master_key 'user3'

#使用pub_key加密，设置访问策略
cpabe-enc pub_key 2.png 'user1 or user2'

#使用user1_key解密
cpabe-dec pub_key user1_key 2.png.cpabe
```

> openssl驱动

```c
unix {
INCLUDEPATH += /usr/include
LIBS += -lcrypto
LIBS += -lssl
}
```

> 秘钥生成函数

```
RAND_bytes(key, 32);
```

> 命令行执行结果

```c++
//不支持管道
QProcess commandProcess;
commandProcess.start("ls");
commandProcess.waitForFinished();
//commandProcess.start("route", QStringList() << "print");
QByteArray cmdoutput = commandProcess.readAllStandardOutput();
QString txtoutput = cmdoutput;
qDebug()<<txtoutput;

//支持管道
QStringList ql;
ql<<"-c"<<"cd clientfiles && ls";
commandProcess.start("bash",ql);
//commandProcess.start(command);
commandProcess.waitForFinished();
QByteArray cmdoutput = commandProcess.readAllStandardOutput();
QString txtoutput = cmdoutput;
qDebug()<<txtoutput;
```

### qt类型转换

```c++
//qbytearray转16进制QString
QString str=buf.toHex();
//QString转qbytearray
QByteArray qb = QByteArray::fromHex (str.toLatin1().data());
```

> ​	qstring转char*

```cpp
//方法一    
QString str = “hello”; //QString转char *  
QByteArray ba = str.toLatin1();  
char *mm = ba.data();

//方法二（不支持中文）
std::string str = filename.toStdString();
const char* ch = str.c_str();
```

> char* 转qstring

```cpp
QString(const char *ch);
```

> qstring和string互转

```c++
//QString转换String
string s = qstr.toStdString();
//String转换QString
QString qstr2 = QString::fromStdString(s);
```

> unsigned char*和qstring、qbytearray的转化

```c++
unsigned char* test1=new unsigned char[32];
unsigned char* test5=new unsigned char[32];
RAND_bytes(test1, 32);
QByteArray test2=QByteArray((char*)test1,32);
QString test3=test2.toHex();
QByteArray test4=QByteArray::fromHex(test3.toLatin1());
test5=(unsigned char*)test4.data();
if(test2==test4)qDebug()<<"success";
if(test1[0]==test5[0]&&test1[1]==test5[1])qDebug()<<"success";
```

> gmp库

```c++
char plaintext[100]="hello world";
mpz_class test;
//输入
mpz_import(test.get_mpz_t(), strlen(plaintext), 1, 1, 0, 0, plaintext);
std::string s=test.get_str();
QString qs=QString::fromStdString(s);
qDebug()<<qs;
char buffer2[512];
//输出
mpz_export(buffer2, NULL, 1, 1, 0, 0, test.get_mpz_t());
QString qs2=QString(buffer2);
qDebug()<<qs2;
```

这玩意有bug

> openssl rsa生成秘钥

```shell
openssl genrsa -out prikey.pem 1024 
openssl rsa -in prikey.pem -pubout -out pubkey.pem
```

> SQLITE操作

```c++
QSqlDatabase database;
database=QSqlDatabase::addDatabase("QSQLITE");
database.setDatabaseName("HelloSQLITE.db");
if(!database.open()){
	qDebug()<<"Error: Fail to connect database"<<database.lastError();
}else{
	qDebug()<<"Succeed to connect database";
}

//创建表
QSqlQuery sql_query;
if(!sql_query.exec("create table login(id text primary key,password text)")){
	qDebug() << "Error: Fail to create table."<< sql_query.lastError();
}else
{
	qDebug() << "Table created!";
}

//插入数据
	if(!sql_query.exec("INSERT INTO login VALUES(\"1811387\",\"123456\")"))
{
	qDebug() << sql_query.lastError();
}
else
{
	qDebug() << "inserted!";
}
if(!sql_query.exec("INSERT INTO login VALUES(\"123\",\"123\")"))
{
	qDebug() << sql_query.lastError();
}
else
{
	qDebug() << "inserted!";
}

//修改数据
	sql_query.exec("update student set name = \"QT\" where id = 1");
	if(!sql_query.exec())
	{
		qDebug() << sql_query.lastError();
	}
	else
	{
		qDebug() << "updated!";
	}

//查询数据
sql_query.exec("select * from login");
if(!sql_query.exec())
{
	qDebug()<<sql_query.lastError();
}
else
{
	while(sql_query.next())
	{
		QString id = sql_query.value(0).toString();
		QString password = sql_query.value(1).toString();
		qDebug()<<QString("id:%1    password:%2").arg(id).arg(password);
	}
}

//登录
sql_query.exec(QString("select * from login where id='%1' and password='%2'").arg("1811387").arg("123456"));
if(!sql_query.exec())
{
	qDebug()<<sql_query.lastError();
}
else
{
	if(sql_query.next()){
		qDebug()<<"login success";
	}else{
		qDebug()<<"login fail";
	}

}

//删除数据
sql_query.exec("delete from student where id = 1");
if(!sql_query.exec())
{
	qDebug()<<sql_query.lastError();
}
else
{
	qDebug()<<"deleted!";
}

//删除表格
sql_query.exec("drop table student");
if(sql_query.exec())
{
	qDebug() << sql_query.lastError();
}
else
{
	qDebug() << "table cleared";
}

//关闭数据库
database.close();
return a.exec();
```



