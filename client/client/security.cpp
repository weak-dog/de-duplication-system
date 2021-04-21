#include "security.h"

Security::Security()
{
}

QString Security::HashMD5(QString password)
{
    // 创建加密对象
    QCryptographicHash hash(QCryptographicHash::Md5);
    // 添加明文数据
    hash.addData(password.toUtf8());
    // 获取加密后的数据
    // 16个字节的数据
    QByteArray new_password = hash.result();
    // 转16进制
    //qDebug()<<new_password.toHex();
    return new_password.toHex();
}

QString Security::TextToBase64(QString str)
{
    // base64进行加密
    QByteArray text = str.toLocal8Bit();
    QByteArray by = text.toBase64();
    return QString(by);
}

QString Security::Base64ToText(QString str)
{
    // base64进行解密
    QByteArray text = str.toLocal8Bit();
    QByteArray by = text.fromBase64(text);
    QString result = QString::fromLocal8Bit(by);
    return result;
}

//文件的sha256哈希值
QString Security::sha_f(QString filepath){
    SHA256_CTX c;
    unsigned char* digest=new unsigned char[32];
    QFile file;
    file.setFileName(filepath);
    file.open(QIODevice::ReadOnly);
    SHA256_Init(&c);
    QByteArray array = file.readAll();
    qDebug()<<array.size();
    SHA256_Update(&c,array,array.size());
    SHA256_Final(digest,&c);
    file.close();
    QString result=QByteArray((char*)digest,32).toHex();
    return result;
}

//字符串sha256哈希值
unsigned char* Security::sha_s(char* str){
    unsigned char* md=new unsigned char[SHA256_DIGEST_LENGTH];
    SHA256((unsigned char *)str, strlen(str), md);
    return md;
}

//字符串sha256哈希值
QString Security::sha_s2(char* str){
    unsigned char* md=new unsigned char[SHA256_DIGEST_LENGTH];
    SHA256((unsigned char *)str, strlen(str), md);
    QString result=QByteArray((char*)md,32).toHex();
    return result;
}

//文件的md5哈希值
QString Security::md5_f(QString filepath){
    MD5_CTX c;
    unsigned char* digest=new unsigned char[16];
    QFile file;
    file.setFileName(filepath);
    file.open(QIODevice::ReadOnly);
    MD5_Init(&c);
    QByteArray array = file.readAll();
    MD5_Update(&c,array,array.size());
    MD5_Final(digest,&c);
    file.close();
    QString result=QByteArray((char*)digest,16).toHex();
    return result;
}

//字符串md5哈希值
unsigned char* Security::md5_s(char* str){
    unsigned char* md=new unsigned char[MD5_DIGEST_LENGTH];
    MD5((unsigned char *)str, strlen(str), md);
    return md;
}

QString Security::md5_s2(char* str){
    unsigned char* md=new unsigned char[MD5_DIGEST_LENGTH];
    MD5((unsigned char *)str, strlen(str), md);
    QString result=QByteArray((char*)md,16).toHex();
    return result;
}

//aes加密文件
int Security::Encrypt_File(unsigned char* key,QString filename){
    unsigned char iv[EVP_MAX_KEY_LENGTH] = "EVP_AES_CTR"; //保存初始化向量的数组
    EVP_CIPHER_CTX* ctx;    //EVP加密上下文环境
    ctx = EVP_CIPHER_CTX_new();
    unsigned char out[1024];  //保存密文的缓冲区
    int outl;
    unsigned char in[1024];   //保存原文的缓冲区
    int inl;
    int rv;
    FILE *fpIn;
    FILE *fpOut;
    //打开待加密文件
    fpIn = fopen(filename.toStdString().c_str(), "rb");
    if(fpIn == NULL)
    {
       return -1;
    }
    //打开保存密文的文件
    char encryptedfname[100];
    strcpy(encryptedfname, filename.toStdString().c_str());
    strcat(encryptedfname, ".encrypted");
    fpOut = fopen(encryptedfname, "wb");
    if(fpOut == NULL)
    {
       fclose(fpIn);
       return -1;
    }
    //初始化ctx
    EVP_CIPHER_CTX_init(ctx);
    //设置密码算法、key和iv
    rv = EVP_EncryptInit_ex(ctx, EVP_aes_128_ctr(), NULL, key, iv);
    if(rv != 1)
    {
       printf("Err\n");
       return -1;
    }
    //循环读取原文，加密后后保存到密文文件。
    for(;;)
    {
       inl = fread(in,1,1024,fpIn);
       if(inl <= 0)//读取原文结束
          break;
       rv = EVP_EncryptUpdate(ctx, out, &outl, in, inl);//加密
       if(rv != 1)
       {
          fclose(fpIn);
          fclose(fpOut);
          EVP_CIPHER_CTX_cleanup(ctx);
          return -1;
       }
       fwrite(out, 1, outl, fpOut);//保存密文到文件
    }
    //加密结束
    rv = EVP_EncryptFinal_ex(ctx, out, &outl);
    if(rv != 1)
    {
       fclose(fpIn);
       fclose(fpOut);
       EVP_CIPHER_CTX_cleanup(ctx);
       return -1;
    }
    fwrite(out,1,outl,fpOut);  //保密密文到文件
    fclose(fpIn);
    fclose(fpOut);
    EVP_CIPHER_CTX_cleanup(ctx); //清除EVP加密上下文环境
    printf("加密已完成\n");
    return 1;
}

//aes解密文件
int Security::Decrypt_File(unsigned char* key,QString filename){
    unsigned char iv[EVP_MAX_KEY_LENGTH] = "EVP_AES_CTR";  //保存初始化向量的数组
    EVP_CIPHER_CTX* ctx;    //EVP加密上下文环境
    ctx=EVP_CIPHER_CTX_new();
    unsigned char out[1024+EVP_MAX_KEY_LENGTH]; //保存解密后明文的缓冲区数组
    int outl;
    unsigned char in[1024];    //保存密文数据的数组
    int inl;
    int rv;
    FILE *fpIn;
    FILE *fpOut;
  //打开待解密的密文文件
    fpIn = fopen(filename.toStdString().c_str(), "rb");
    if(fpIn == NULL)
    {
       return -1;
    }
    char decryptedfname[100];
    strcpy(decryptedfname, filename.toStdString().c_str());
    strcat(decryptedfname, ".decrypted");
    //打开保存明文的文件
    fpOut = fopen(decryptedfname, "wb");
    if(fpOut == NULL)
    {
       fclose(fpIn);
       return -1;
    }
    //初始化ctx
    EVP_CIPHER_CTX_init(ctx);
    //设置解密的算法、key和iv
    rv = EVP_DecryptInit_ex(ctx, EVP_aes_128_ctr(), NULL, key, iv);
    if(rv != 1)
    {
       EVP_CIPHER_CTX_cleanup(ctx);
       return -1;
    }
    //循环读取原文，解密后后保存到明文文件。
    for(;;)
    {
       inl = fread(in, 1, 1024, fpIn);
       if(inl <= 0)
          break;
       rv = EVP_DecryptUpdate(ctx, out, &outl, in, inl);//解密
       if(rv != 1)
       {
          fclose(fpIn);
          fclose(fpOut);
          EVP_CIPHER_CTX_cleanup(ctx);
          return -1;
       }
       fwrite(out, 1, outl, fpOut);//保存明文到文件
    }
    //解密结束
    rv = EVP_DecryptFinal_ex(ctx, out, &outl);
    if(rv != 1)
    {
       fclose(fpIn);
       fclose(fpOut);
       EVP_CIPHER_CTX_cleanup(ctx);
       return -1;
    }
    fwrite(out,1,outl,fpOut);//保存明文到文件
    fclose(fpIn);
    fclose(fpOut);
    EVP_CIPHER_CTX_cleanup(ctx);//清除EVP加密上下文环境
    printf("解密已完成\n");
    return 1;
}

void Security::get_rand_key(unsigned char * key,int l){
    RAND_bytes(key, l);
}

QString Security::hmac(QByteArray key,QString filePath){
    QMessageAuthenticationCode code(QCryptographicHash::Sha256);
    code.setKey(key);
    QFile file;
    file.setFileName(filePath);
    file.open(QIODevice::ReadOnly);
    QByteArray message=file.readAll();
    file.close();
    code.addData(message);
    return code.result().toHex();
}

QString Security::hmacS(QByteArray key,QByteArray message){
    QMessageAuthenticationCode code(QCryptographicHash::Sha256);
    code.setKey(key);
    code.addData(message);
    return code.result().toHex();
}
