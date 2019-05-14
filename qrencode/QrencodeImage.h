#ifndef __QRENCODE_IMAGE_H__
#define __QRENCODE_IMAGE_H__

#include <string>
#include "cocos2d.h"

using namespace std;
USING_NS_CC;

class QrencodeImage
{
public:
    QrencodeImage();
    ~QrencodeImage();
    
    static QrencodeImage* getInstance();
    
    Image* qrcodeToImage(string str);
private:
    int selectVersion(string str);
private:
    int iCharNumOfVersion[40];
};

#endif // __QRENCODE_IMAGE_H__
