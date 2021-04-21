#ifndef SECURITY_H
#define SECURITY_H

#include <QFile>
#include <math.h>
#include <QString>
#include <QDateTime>
#include <QCryptographicHash>
#include <QDebug>
#include <QSettings>
#include <QMessageBox>
#include <QApplication>
#include <QtSql/QSqlDatabase>
#include <QtSql/QSqlError>
#include <QtSql/QSqlQuery>
#include <QString>
#include<openssl/sha.h>
#include<openssl/md5.h>
#include<openssl/crypto.h>
#include <openssl/rand.h>
#include<openssl/aes.h>
#include "openssl/evp.h"
#include "openssl/x509.h"
#include <QtDebug>
#include <QFile>
#include <string.h>
#include <QMessageAuthenticationCode>

class Security
{
public:
    Security();
    QString HashMD5(QString password);
    QString TextToBase64(QString text);
    QString Base64ToText(QString text);
    unsigned char* md5_s(char* str);//计算字符串哈希值
    QString md5_s2(char* str);//计算字符串哈希值
    QString md5_f(QString filepath);//计算字符串哈希值
    unsigned char* sha_s(char* str);//计算字符串哈希值
    QString sha_s2(char* str);//计算字符串哈希值
    QString sha_f(QString filepath);//计算文件哈希值
    int Encrypt_File(unsigned char* key,QString);//aes加密文件
    int Decrypt_File(unsigned char* key,QString);//aes解密文件
    void get_rand_key(unsigned char * key,int l);//generate random key
    QString hmac(QByteArray key,QString filePath);
    QString hmacS(QByteArray key,QByteArray message);
};

#endif // SECURITY_H
