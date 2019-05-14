#include "png.h"
#include "QrencodeImage.h"

extern "C" {
    #include "qrencode.h"
}

static QrencodeImage *pQrencodeImage = nullptr;

QrencodeImage::QrencodeImage()
{
    iCharNumOfVersion[0] = 25;
    iCharNumOfVersion[1] = 47;
    iCharNumOfVersion[2] = 77;
    iCharNumOfVersion[3] = 114;
    iCharNumOfVersion[4] = 154;
    iCharNumOfVersion[5] = 195;
    iCharNumOfVersion[6] = 224;
    iCharNumOfVersion[7] = 279;
    iCharNumOfVersion[8] = 335;
    iCharNumOfVersion[9] = 395;
    iCharNumOfVersion[10] = 468;
    iCharNumOfVersion[11] = 535;
    iCharNumOfVersion[12] = 619;
    iCharNumOfVersion[13] = 667;
    iCharNumOfVersion[14] = 758;
    iCharNumOfVersion[15] = 854;
    iCharNumOfVersion[16] = 938;
    iCharNumOfVersion[17] = 1046;
    iCharNumOfVersion[18] = 1153;
    iCharNumOfVersion[19] = 1249;
    iCharNumOfVersion[20] = 1352;
    iCharNumOfVersion[21] = 1460;
    iCharNumOfVersion[22] = 1588;
    iCharNumOfVersion[23] = 1704;
    iCharNumOfVersion[24] = 1853;
    iCharNumOfVersion[25] = 1990;
    iCharNumOfVersion[26] = 2132;
    iCharNumOfVersion[27] = 2223;
    iCharNumOfVersion[28] = 2369;
    iCharNumOfVersion[29] = 2520;
    iCharNumOfVersion[30] = 2677;
    iCharNumOfVersion[31] = 2840;
    iCharNumOfVersion[32] = 3009;
    iCharNumOfVersion[33] = 3183;
    iCharNumOfVersion[34] = 3351;
    iCharNumOfVersion[35] = 3537;
    iCharNumOfVersion[36] = 3729;
    iCharNumOfVersion[37] = 3927;
    iCharNumOfVersion[38] = 4087;
    iCharNumOfVersion[39] = 4296;
}

QrencodeImage::~QrencodeImage()
{
    if (nullptr != pQrencodeImage)
        delete pQrencodeImage;
}

QrencodeImage* QrencodeImage::getInstance()
{
    if (nullptr == pQrencodeImage)
        pQrencodeImage = new QrencodeImage;
    
    return pQrencodeImage;
}

Image* QrencodeImage::qrcodeToImage(string str)
{
    if (str.empty())
        return nullptr;
    
    int iVersion = selectVersion(str);
    if (iVersion < 0)
        return nullptr;
    
    auto pImage = new Image();
    if (nullptr == pImage)
        return nullptr;
    
    QRcode* pQRcode = QRcode_encodeString(str.c_str(), iVersion, QR_ECLEVEL_M, QR_MODE_8, 1);
    if (!pQRcode)
    {
        delete pImage;
        return nullptr;
    }
    
    int iImgWidth = pQRcode->width;
    int iPixelNum = iImgWidth * iImgWidth;
    unsigned char* pData = (unsigned char*)malloc(iPixelNum * 4);
    if (!pData)
    {
        delete pImage;
        QRcode_free(pQRcode);
        return nullptr;
    }
    
    memset(pData, 0xff, iPixelNum * 4);
    unsigned char* pDataIndex = pData;
    for (int i = 0; i < iPixelNum; i++)
    {
        if (pQRcode->data[i] & 1)
            pDataIndex[0] = pDataIndex[1] = pDataIndex[2] = 0;
        
        pDataIndex += 4;
    }
    free(pData);
    QRcode_free(pQRcode);
    
    pImage->initWithRawData(pData, 0, iImgWidth, iImgWidth, 0);
    return pImage;
}

int QrencodeImage::selectVersion(string str)
{
    int iLength = str.length();
    for (int i = 0; i < 40; i++)
    {
        if (iLength <= iCharNumOfVersion[i])
            return i;
    }
    return -1;
}
