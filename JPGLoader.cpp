//*****************************************************************************************//
//**                                                                                     **//
//**                                JPGLoader                                            **//
//**                                                                                     **//
//*****************************************************************************************//

#define _CRT_SECURE_NO_WARNINGS
#include "JPGLoader.h"
#include <string.h>
#include <math.h>

static const int bitMask[]{
	0b0000000000000000,
	0b0000000000000001,
	0b0000000000000011,
	0b0000000000000111,
	0b0000000000001111,
	0b0000000000011111,
	0b0000000000111111,
	0b0000000001111111,
	0b0000000011111111,
	0b0000000111111111,
	0b0000001111111111,
	0b0000011111111111,
	0b0000111111111111,
	0b0001111111111111,
	0b0011111111111111,
	0b0111111111111111,
	0b1111111111111111
};

static const int byteArrayNumbit = 8;
static const int LEFT_INV_BIT1 = 0b0101010101010101;
static const int RIGHT_INV_BIT1 = 0b1010101010101010;
static const int LEFT_INV_BIT2 = 0b0011001100110011;
static const int RIGHT_INV_BIT2 = 0b1100110011001100;
static const int LEFT_INV_BIT4 = 0b0000111100001111;
static const int RIGHT_INV_BIT4 = 0b1111000011110000;

static unsigned short intInversion(unsigned short ba, int numBit) {
	unsigned short oBa = 0;
	const int baseNum = 16;
	oBa = ((ba & LEFT_INV_BIT1) << 1) | ((ba & RIGHT_INV_BIT1) >> 1);
	oBa = ((oBa & LEFT_INV_BIT2) << 2) | ((oBa & RIGHT_INV_BIT2) >> 2);
	oBa = ((oBa & LEFT_INV_BIT4) << 4) | ((oBa & RIGHT_INV_BIT4) >> 4);
	return ((oBa << 8) | (oBa >> 8)) >> (baseNum - numBit);
}

static void bitInversion(unsigned char* ba, unsigned int size) {
	for (unsigned int i = 0; i < size; i++) {
		ba[i] = (unsigned char)intInversion((unsigned short)ba[i], 8);
	}
}

static void getBit(unsigned long long* CurSearchBit, unsigned char* byteArray, unsigned char NumBit, unsigned short* outBinArr, bool firstRight) {
	unsigned int baind = (unsigned int)(*CurSearchBit / byteArrayNumbit);//配列インデックス
	unsigned int bitPos = (unsigned int)(*CurSearchBit % byteArrayNumbit);//要素内bit位置インデックス
	unsigned char shiftBit = byteArrayNumbit * 3 - NumBit - bitPos;
	*outBinArr = (((byteArray[baind] << 16) | (byteArray[baind + 1] << 8) |
		byteArray[baind + 2]) >> shiftBit)& bitMask[NumBit];
	if (firstRight) *outBinArr = intInversion(*outBinArr, NumBit);
	*CurSearchBit += NumBit;
}

void HuffmanNode2::setVal(outTree val, unsigned short bitArr, unsigned char numBit) {
	if (numBit == 0) {//葉まで到達したら値を格納
		Val = val;
		return;
	}
	if (bitArr >> (numBit - 1) == 0) {
		if (!bit0)bit0 = new HuffmanNode2();
		bit0->setVal(val, bitArr & bitMask[numBit - 1], numBit - 1);
	}
	else {
		if (!bit1)bit1 = new HuffmanNode2();
		bit1->setVal(val, bitArr & bitMask[numBit - 1], numBit - 1);
	}
}

outTree HuffmanNode2::getVal(unsigned long long* curSearchBit, unsigned char* byteArray) {
	if (!bit0 && !bit1) {//葉まで到達したら値を返す
		return Val;
	}
	unsigned short outBin = 0;
	getBit(curSearchBit, byteArray, 1, &outBin, false);
	if (outBin == 0) {
		return bit0->getVal(curSearchBit, byteArray);
	}
	else {
		return bit1->getVal(curSearchBit, byteArray);
	}
}

HuffmanNode2::~HuffmanNode2() {
	if (bit0)delete bit0;
	bit0 = nullptr;
	if (bit1)delete bit1;
	bit1 = nullptr;
}

void HuffmanTree2::createTree(signSet* sigArr, unsigned int arraySize) {
	for (unsigned int i = 0; i < arraySize; i++) {
		hn.setVal({ sigArr[i].valBit,sigArr[i].runlength }, sigArr[i].sign, sigArr[i].numBit);
	}
}

outTree HuffmanTree2::getVal(unsigned long long* curSearchBit, unsigned char* byteArray) {
	return hn.getVal(curSearchBit, byteArray);
}

unsigned char* JPGLoader::loadJPG(char* pass, unsigned int outWid, unsigned int outHei) {
	FILE* fp = fopen(pass, "rb");
	if (fp == NULL) {
		return nullptr;
	}
	unsigned int count = 0;
	while (fgetc(fp) != EOF) {
		count++;
	}
	fseek(fp, 0, SEEK_SET);//最初に戻す
	bytePointer* bp = new bytePointer(count, fp);
	fclose(fp);

	const unsigned int imageNumChannel = 4;
	const unsigned int numPixel = outWid * imageNumChannel * outHei;
	unsigned char* image = new unsigned char[numPixel];//外部で開放
	short* unResizeImage = nullptr;

	//---jpg構造---//
	//SOI
	//セグメント × N個
	//イメージデータ
	//EOI

	//---セグメント構造---//
	//マーカー2byte
	//レングス長2byte 内容: セグメントパラメーターbyte数 + 2byte
	//セグメントパラメーター

	DQTpara dqpara[4] = {};
	DHTpara dhpara[8] = {};
	SOF0para sof0para = {};
	unsigned int dqparaIndex = 0;
	unsigned int dhparaIndex = 0;
	unsigned char* compImage = nullptr;
	for (int i = 0; i < 4; i++) {
		htreeDC[i] = new HuffmanTree2();
		htreeAC[i] = new HuffmanTree2();
	}
	unsigned int unResizeImageSize = 0;
	unsigned char samplingX = 0;
	unsigned char samplingY = 0;

	if (SOI != bp->convertUCHARtoShort())return nullptr;

	bool eoi = false;
	do {
		unsigned short marker = bp->convertUCHARtoShort();
		if (DQT == marker) {
			unsigned short len = bp->convertUCHARtoShort() - 2;
			if (len == 0)continue;
			do {
				unsigned char pt = bp->getChar();
				DQTpara* d = &dqpara[dqparaIndex++];
				d->Pqn = (pt >> 4) & 0xf;
				d->Tqn = pt & 0xf;
				if (d->Pqn == 0) {
					memcpy(d->Qn0, bp->getPointer(64), 64);
					len -= 65;
				}
				else {
					memcpy(d->Qn1, bp->getPointer(128), 128);
					len -= 129;
				}
			} while (len > 0);
			continue;
		}
		if (DHT == marker) {
			unsigned short len = bp->convertUCHARtoShort() - 2;
			if (len == 0)continue;

			do {
				unsigned char pt = bp->getChar();
				DHTpara* d = &dhpara[dhparaIndex];
				d->Tcn = (pt >> 4) & 0xf;
				d->Thn = pt & 0xf;
				memcpy(d->L, bp->getPointer(16), 16);
				int lcnt = 0;
				for (int l = 0; l < 16; l++)lcnt += d->L[l];
				d->V = new unsigned char[lcnt];
				memcpy(d->V, bp->getPointer(lcnt), lcnt);
				len -= (17 + lcnt);
				//符号生成
				signSet* si = new signSet[lcnt];
				createSign(si, d->L, d->V, d->Tcn);
				//ツリー生成
				if (d->Tcn == 0)
					htreeDC[d->Thn]->createTree(si, lcnt);
				else
					htreeAC[d->Thn]->createTree(si, lcnt);
				delete[] si;
			} while (len > 0);
			continue;
		}
		if (SOF0 == marker) {
			unsigned short len = bp->convertUCHARtoShort() - 2;
			if (len == 0)continue;
			sof0para.P = bp->getChar();
			sof0para.Y = bp->convertUCHARtoShort();
			sof0para.X = bp->convertUCHARtoShort();
			sof0para.Nf = bp->getChar();
			SOF0component* s = sof0para.sofC;
			for (unsigned char nf = 0; nf < sof0para.Nf; nf++) {
				s[nf].Cn = bp->getChar();
				unsigned char pt = bp->getChar();
				s[nf].Hn = (pt >> 4) & 0xf;
				s[nf].Vn = pt & 0xf;
				if (s[nf].Cn == 1) {
					samplingX = s[nf].Hn;
					samplingY = s[nf].Vn;
				}
				s[nf].Tqn = bp->getChar();
			}
			unsigned short sx = 0;
			unsigned short sy = 0;
			if (samplingX == 1 && samplingY == 1) {//4:4:4
				sx = sof0para.X / 8 * 8 + 8;
				sy = sof0para.Y / 8 * 8 + 8;
			}
			if (samplingX == 1 && samplingY == 2) {//4:4:0
				sx = sof0para.X / 8 * 8 + 8;
				sy = sof0para.Y / 16 * 16 + 16;
			}
			if (samplingX == 2 && samplingY == 1) {//4:2:2
				sx = sof0para.X / 16 * 16 + 16;
				sy = sof0para.Y / 8 * 8 + 8;
			}
			if (samplingX == 2 && samplingY == 2) {//4:2:0
				sx = sof0para.X / 16 * 16 + 16;
				sy = sof0para.Y / 16 * 16 + 16;
			}
			if (samplingX == 4 && samplingY == 1) {//4:1:1
				sx = sof0para.X / 32 * 32 + 32;
				sy = sof0para.Y / 8 * 8 + 8;
			}
			if (samplingX == 4 && samplingY == 2) {//4:1:0
				sx = sof0para.X / 32 * 32 + 32;
				sy = sof0para.Y / 16 * 16 + 16;
			}
			unResizeImageSize = sx * 3 * sy;
			unResizeImage = new short[unResizeImageSize];
			memset(unResizeImage, 0, sizeof(short) * unResizeImageSize);
			continue;
		}
		if (SOS == marker) {
			unsigned short len = bp->convertUCHARtoShort() - 2;
			if (len == 0)continue;
			sospara.Ns = bp->getChar();
			sospara.sosC = new SOScomponent[sospara.Ns];
			SOScomponent* s = sospara.sosC;
			for (unsigned char ns = 0; ns < sospara.Ns; ns++) {
				s[ns].Csn = bp->getChar();
				unsigned char pt = bp->getChar();
				s[ns].Tdn = (pt >> 4) & 0xf;
				s[ns].Tan = pt & 0xf;
			}
			sospara.Ss = bp->getChar();
			sospara.Se = bp->getChar();
			unsigned char pt = bp->getChar();
			sospara.Ah = (pt >> 4) & 0xf;
			sospara.Al = pt & 0xf;
			//この直後イメージデータ
			unsigned int imageSt = bp->getIndex();
			//サイズカウント
			unsigned int cnt = 0;
			while (bp->checkEOF()) {
				if (0xff == bp->getChar()) {
					if (0xd9 == bp->getChar()) {
						eoi = true;
						break;
					}
				}
				cnt++;
			}
			if (!eoi)return nullptr;
			bp->setIndex(imageSt);
			compImage = new unsigned char[cnt];
			compSize = cnt;
			cnt = 0;
			//圧縮イメージ格納
			while (bp->checkEOF()) {
				bool f = true;
				unsigned char t = bp->getChar();
				if (0xff == t) {
					unsigned char tt = bp->getChar();
					if (0xd9 == tt) {
						break;
					}
					if (0x00 != tt) {
						f = false;
					}
				}
				if (f)
					compImage[cnt++] = t;
			}
			break;
		}
		//上記マーカー以外スキップ
		unsigned short len = bp->convertUCHARtoShort() - 2;
		bp->getPointer(len);
	} while (bp->checkEOF());
	if (!eoi)return nullptr;

	decompressHuffman(unResizeImage, compImage, unResizeImageSize, samplingX, samplingY);



	delete[] unResizeImage;
	unResizeImage = nullptr;
	for (int i = 0; i < 4; i++) {
		if (htreeDC[i]) {
			delete htreeDC[i];
			htreeDC[i] = nullptr;
		}
		if (htreeAC[i]) {
			delete htreeAC[i];
			htreeAC[i] = nullptr;
		}
	}
	delete bp;
	bp = nullptr;
	delete[] compImage;
	compImage = nullptr;

	return image;
}

void JPGLoader::createSign(signSet* sig, unsigned char* L, unsigned char* V, unsigned char Tcn) {
	unsigned int firstNumBit = 1;
	while (L[firstNumBit - 1] == 0) { firstNumBit++; }
	const static char maxNumBit = 16;
	unsigned short sign = 0;
	unsigned int vInd = 0;
	unsigned char prevNumBit = firstNumBit;
	for (unsigned char i = firstNumBit; i <= maxNumBit; i++) {
		if (L[i - 1] <= 0)continue;
		sign = sign << (i - prevNumBit);
		for (unsigned char j = 0; j < L[i - 1]; j++) {
			sig[vInd].sign = sign++;
			sig[vInd].numBit = i;
			unsigned char v = V[vInd];
			if (Tcn == 1) {
				sig[vInd].runlength = (V[vInd] >> 4) & 0xf;
				v = V[vInd] & 0xf;
			}
			sig[vInd].valBit = v;
			vInd++;
		}
		prevNumBit = i;
	}
}

void JPGLoader::decompressHuffman(short* decomp, unsigned char* comp, unsigned int size,
	unsigned char samplingX, unsigned char samplingY) {

	unsigned char indexS[10] = {};
	unsigned char indexSize = 0;
	if (samplingX * samplingY == 1) {//4:4:4
		indexSize = 3;
		unsigned char ind[3] = { 0,1,2 };
		memcpy(indexS, ind, indexSize);
	}
	if (samplingX * samplingY == 2) {//4:4:0 4:2:2
		indexSize = 4;
		unsigned char ind[4] = { 0,0,1,2 };
		memcpy(indexS, ind, indexSize);
	}
	if (samplingX * samplingY == 4) {//4:2:0 4:1:1
		indexSize = 6;
		unsigned char ind[6] = { 0,0,0,0,1,2 };
		memcpy(indexS, ind, indexSize);
	}
	if (samplingX * samplingY == 8) {//4:1:0
		indexSize = 10;
		unsigned char ind[10] = { 0,0,0,0,0,0,0,0,1,2 };
		memcpy(indexS, ind, indexSize);
	}

	unsigned short decompTmp[64] = {};
	unsigned int decompTmpIndex = 0;
	unsigned long long curSearchBit = 0;
	unsigned int decompIndex = 0;
	outTree val = {};
	unsigned int numEmement = (unsigned int)sospara.Ns;
	unsigned int sosIndex = -1;
	unsigned int dcIndex = 0;
	unsigned int acIndex = 0;
	//bitInversion(comp, compSize);
	int dc = 0;
	int ac = 0;
	while (decompIndex < size) {
		if (decompTmpIndex == 0) {
			sosIndex = ++sosIndex % indexSize;
			dcIndex = sospara.sosC[indexS[sosIndex]].Tdn;
			acIndex = sospara.sosC[indexS[sosIndex]].Tan;
			val = htreeDC[dcIndex]->getVal(&curSearchBit, comp);

			short bit = 0;
			if (val.valBit > 0) {
				unsigned short outBit = 0;
				getBit(&curSearchBit, comp, val.valBit, &outBit, false);
				if ((outBit >> (val.valBit - 1)) == 0) {
					bit = (~outBit & bitMask[val.valBit]) * -1;
				}
				else {
					bit = outBit;
				}
			}
			decompTmp[decompTmpIndex++] = bit;
			dc++;
		}
		else {
			val = htreeAC[acIndex]->getVal(&curSearchBit, comp);

			if (val.valBit > 0) {
				unsigned short outBit = 0;
				getBit(&curSearchBit, comp, val.valBit, &outBit, false);
				for (unsigned char len = 0; len < val.runlength; len++) {
					decompTmp[decompTmpIndex++] = 0;
					ac++;
				}
				short bit = 0;
				if ((outBit >> (val.valBit - 1)) == 0) {
					bit = (~outBit & bitMask[val.valBit]) * -1;
				}
				else {
					bit = outBit;
				}
				decompTmp[decompTmpIndex++] = bit;
				ac++;
			}
			else {
				if (val.runlength == ZRL) {
					for (unsigned char len = 0; len < 16; len++) {
						decompTmp[decompTmpIndex++] = 0;
						ac++;
					}
				}
				if (val.runlength == EOB) {
					while (1) {
						decompTmp[decompTmpIndex] = 0;
						decompTmpIndex++;
						ac++;
						if (decompTmpIndex == 64)break;
					}
				}
			}
		}

		if (decompTmpIndex == 64) {
			int loop = 1;
			if (indexS[sosIndex] != 0)loop = indexSize - 2;
			for (int i = 0; i < loop; i++) {
				memcpy(&decomp[decompIndex], decompTmp, sizeof(short) * 64);
				decompIndex += 64;
			}
			decompTmpIndex = 0;
		}

	}
	int j = 0;
}

void JPGLoader::createZigzagIndex(unsigned char* zigIndex) {
	unsigned int zig[64] = {};
	int zIndex = 0;
	int x = 0;
	int y = 0;
	int index = 0;
	zig[zIndex++] = 0;
	bool down = true;
	while (zIndex < 64) {
		if (down) {
			x++;
			zig[zIndex++] = y * 8 + x;
			while (x > 0) {
				x--;
				y++;
				zig[zIndex++] = y * 8 + x;
			}
			if (x == 0 && y == 7) {
				down = false;
			}
			else {
				y++;
				zig[zIndex++] = y * 8 + x;
				while (y > 0) {
					x++;
					y--;
					zig[zIndex++] = y * 8 + x;
				}
			}
		}
		else {
			x++;
			zig[zIndex++] = y * 8 + x;
			if (x == 7 && y == 7)break;
			while (x < 7) {
				x++;
				y--;
				zig[zIndex++] = y * 8 + x;
			}
			y++;
			zig[zIndex++] = y * 8 + x;
			while (y < 7) {
				x--;
				y++;
				zig[zIndex++] = y * 8 + x;
			}
		}
	}
	for (int i = 0; i < 64; i++) {
		zigIndex[zig[i]] = i;
	}
}

void JPGLoader::inverseQuantization(char* dstDct, char* Qtbl, char* Qdata) {
	for (int i = 0; i < 64; i++)dstDct[i] = Qtbl[i] * Qdata[i];
}

static double C(int index) {
	double ret = 0.0;
	const double inverseRoute2 = 1 / 1.41421356237;
	if (index == 0)ret = inverseRoute2;
	else ret = 1;
	return ret;
}
void JPGLoader::inverseDCT(char* dst, char* src) {
	const double pi = 3.141592653589793;
	for (int y = 0; y < 8; y++) {
		for (int x = 0; x < 8; x++) {
			int dstIndex = y * 8 + x;
			double ds = 0.0;
			for (int i = 0; i < 8; i++) {
				double ds1 = 0.0;
				for (int j = 0; j < 8; j++) {
					int srcIndex = j * 8 + i;
					ds1 += C(i) * C(j) * src[srcIndex] *
						cos(((2 * x + 1) * i * pi) / 16) *
						cos(((2 * y + 1) * j * pi) / 16);
				}
				ds += ds1;
			}
			dst[dstIndex] = (char)ds;
		}
	}
}

void JPGLoader::decodeYCrCbtoRGB(RGB& dst, YCrCb& src) {
	dst.R = ((unsigned char)src.Y + 128) + (unsigned char)(1.402f * (src.Cr - 128.0f));
	dst.G = ((unsigned char)src.Y + 128) - (unsigned char)(0.34414f * (src.Cb - 128.0f) - 0.71414f * (src.Cr - 128.0f));
	dst.B = ((unsigned char)src.Y + 128) + (unsigned char)(1.772f * (src.Cb - 128.0f));
}