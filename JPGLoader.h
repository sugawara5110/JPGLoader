//*****************************************************************************************//
//**                                                                                     **//
//**                                JPGLoader                                            **//
//**                                                                                     **//
//*****************************************************************************************//

#ifndef Class_JPGLoader_Header
#define Class_JPGLoader_Header

#include <stdio.h>
#include <string.h>
#include <memory>

class JPGLoader {

private:
	struct signSet {
		unsigned short sign = 0;
		unsigned char numBit = 0;
		unsigned char valBit = 0;
		unsigned char runlength = 0;
	};

	struct outTree {
		unsigned char valBit = 0;
		unsigned char runlength = 0;//ZRL,EOBも有り
	};

	class HuffmanNode {
	public:
		HuffmanNode* bit0 = nullptr;
		HuffmanNode* bit1 = nullptr;
		outTree Val = {};

		void setVal(outTree val, unsigned short bitArr, unsigned char numBit);
		outTree getVal(unsigned long long* curSearchBit, unsigned char* byteArray);
		~HuffmanNode();
	};

	class HuffmanTree {
	public:
		HuffmanNode hn;

		void createTree(signSet* sigArr, unsigned int arraySize);
		outTree getVal(unsigned long long* curSearchBit, unsigned char* byteArray);
	};

	//マーカー 0xffがマーカー始まり, 0xff00はffを通常の数値として使う場合
	const unsigned short SOI = 0xffd8;//スタート
	const unsigned short DQT = 0xffdb;//量子化テーブル定義
	const unsigned short DHT = 0xffc4;//ハフマンテーブル定義
	const unsigned short SOF0 = 0xffc0;//フレームヘッダー
	const unsigned short SOF15 = 0xffcf;//フレームヘッダー
	const unsigned short SOS = 0xffda;//スキャンヘッダー
	const unsigned short EOI = 0xffd9;//エンド
	const unsigned short DRI = 0xffdd;//リスタート定義
	const unsigned short RST0 = 0xffd0;//0でリスタートする
	const unsigned short RST7 = 0xffd7;//7でリスタートする
	const unsigned char ZRL = 0xf;//16個の0データを表す, valBit = 0である事
	const unsigned char EOB = 0x0;//データ打ち切り,valBit = 0である事

	class bytePointer {
	private:
		unsigned char* byte = nullptr;
		unsigned int index = 0;
		unsigned int Size = 0;
	public:
		void setPointer(unsigned int size, unsigned char* byt) {
			byte = new unsigned char[size];
			Size = size;
			memcpy(byte, byt, size);
		}
		~bytePointer() {
			delete[] byte;
			byte = nullptr;
		}
		//ビックエンディアン  [0x1234]なら[0x12][0x34]の順 
		unsigned int convertUCHARtoUINT() {
			unsigned int ret = ((unsigned int)byte[index + 0] << 24) | ((unsigned int)byte[index + 1] << 16) |
				((unsigned int)byte[index + 2] << 8) | ((unsigned int)byte[index + 3]);
			index += 4;
			return ret;
		}
		unsigned short convertUCHARtoShort() {
			unsigned short ret = ((unsigned short)byte[index + 0] << 8) | ((unsigned short)byte[index + 1]);
			index += 2;
			return ret;
		}
		unsigned char getChar() {
			unsigned char ret = byte[index];
			index++;
			return ret;
		}
		unsigned char* getPointer(unsigned int addPointer) {
			unsigned char* by = &byte[index];
			index += addPointer;
			return by;
		}
		unsigned int getIndex() {
			return index;
		}
		void setIndex(unsigned int ind) {
			index = ind;
		}
		bool checkEOF() {
			if (index >= Size)return false;
			return true;
		}
	};

	class DQTpara { //4種類固定,SOFから参照される
	public:
		unsigned char Pqn = 0;//上位4bit 量子化テーブル精度 0:8bit, 1:16bit  通常8bit
		unsigned char Tqn = 0;//下位4bit 量子化テーブル番号 0~3まで
		unsigned char Qn0[64] = {};//量子化テーブル 64個の量子化係数 Pqn=0の場合
		unsigned short Qn1[64] = {};//量子化テーブル 64個の量子化係数 Pqn=1の場合
	};
	DQTpara dqpara[4] = {};

	class DHTpara {
	public:
		unsigned char Tcn = 0;//上位4bit ハフマンテーブルクラス 0=DC成分 1=AC成分
		unsigned char Thn = 0;//下位4bit ハフマンテーブル番号 0から3までのいずれか
		unsigned char L[16] = {};//ハフマンテーブルbit配分数 例:L[0]は1bit符号の数, L[15]は16bit符号の数

		//DC成分 符号+データ値というbit並びに対してのデータ値bit数
		//AC成分 上位4bit　ランレングス数
		//       下位4bit　DCと同じ,データ値bit数
		unsigned char* V = nullptr;
		//データ値を2進数で見た場合先頭bit1の場合そのままの値とする
		//先頭bit0の場合bit反転した数値にマイナス符号を付けた値とする,通常の2進数とは違う
		//圧縮データは最初に生成した符号bitにデータbitをつけて並んでいる
		//DCの場合,上記の符号一致の場合次のbitからVbit分取り出した値が解凍後bitとなる
		//ACの場合,解読済みbit最後尾からランレングス数だけ値0のbyteを追加,その後に取り出したデータ値bitを追加
		~DHTpara() {
			delete[] V;
			V = nullptr;
		}
	};
	std::unique_ptr<HuffmanTree> htreeDC[4] = {};
	std::unique_ptr<HuffmanTree> htreeAC[4] = {};
	void createSign(signSet* sig, unsigned char* L, unsigned char* V, unsigned char Tcn);

	class SOF0component {
	public:
		unsigned char Cn = 0;//成分ID, 1:Y, 2:Cb, 3:Cr, 4:I, 5:Q
		unsigned char Hn = 0;//上位4bit水平サンプリング値
		unsigned char Vn = 0;//下位4bit垂直サンプリング値
		//  設定    Y   Cb  Cr
		//  4:4:4   11  11  11
		//  4:4:0   12  11  11
		//  4:2:2   21  11  11
		//  4:2:0   22  11  11
		//  4:1:1   41  11  11
		//  4:1:0   42  11  11

		unsigned char Tqn = 0;//対応量子化テーブル番号
	};
	class SOF0para {
	public:
		unsigned char P = 0;//サンプル精度, ほとんどが8(bit)1pixelサイズ
		unsigned short Y = 0;//画像縦サイズ
		unsigned short X = 0;//画像横サイズ
		unsigned char Nf = 0;//構成要素の数(SOF0component)1:グレースケール, 3:カラー(YCbCr or YIQ), 4:カラー(CMYK)

		SOF0component sofC[4] = {};
	};
	SOF0para sof0para = {};

	class SOScomponent {
	public:
		unsigned char Csn = 0;//成分ID, 1:Y, 2:Cb, 3:Cr, 4:I, 5:Q
		unsigned char Tdn = 0;//上位4bit DC成分ハフマンテーブル番号0~3
		unsigned char Tan = 0;//下位4bit AC成分ハフマンテーブル番号0~3
	};
	class SOSpara {
	public:
		unsigned char Ns = 0;//成分数(SOScomponent)
		SOScomponent* sosC = nullptr;
		unsigned char Ss = 0;//量子化係数開始番号
		unsigned char Se = 0;//量子化係数終了番号
		unsigned char Ah = 0;//上位4bit 前回のスキャンの係数値分割シフト量(最初のスキャンは0)
		unsigned char Al = 0;//下位4bit 係数値分割シフト量                (最後のスキャンは0)

		~SOSpara() {
			delete[] sosC;
			sosC = nullptr;
		}
	};
	std::unique_ptr<SOSpara> sospara = {};

	struct YCbCr {
		float Y = 0.0f;//輝度
		float Cb = 0.0f;//青方向の色相
		float Cr = 0.0f;//赤方向の色相
	};
	struct RGB {
		unsigned char R = 0;
		unsigned char G = 0;
		unsigned char B = 0;
	};

	const unsigned int imageNumChannel = 4;
	static double IdctTable[64][64];
	static bool IdctTableCreate;

	unsigned int unResizeImageSizeX = 0;
	unsigned int unResizeImageSizeY = 0;

	inline int Max(int a, int b) {
		if (a > b)
			return a;
		else
			return b;
	}
	inline int Min(int a, int b) {
		if (a < b)
			return a;
		else
			return b;
	}

	void createIdctTable();
	void decompressHuffman(short* decomp, unsigned char* comp, unsigned int decompSize,
		unsigned char mcuSize, unsigned char* componentIndex, unsigned short dri);
	void zigzagScan(short* decomp, unsigned char* zigIndex, unsigned int decompSize);
	void inverseQuantization(short* decomp, unsigned int decompSize,
		unsigned char mcuSize, unsigned char* componentIndex);//逆量子化
	void inverseDCT(short* dst, short* src);
	void inverseDiscreteCosineTransfer(short* decomp, unsigned int size);//逆離散コサイン変換(ﾟ∀ﾟ)
	void decodeYCbCrtoRGB(RGB& dst, YCbCr& src);
	void decodePixel(unsigned char* pix, short* decomp,
		unsigned char samplingX, unsigned char samplingY,
		unsigned int width, unsigned int height);
	void resize(unsigned char* dstImage, unsigned char* srcImage,
		unsigned int dstWid, unsigned int dstHei,
		unsigned int srcWid, unsigned int srcNumChannel, unsigned int srcHei);

public:
	unsigned char* loadJPG(char* Path, unsigned int outWid, unsigned int outHei,
		char* errorMessage = nullptr);

	unsigned char* loadJpgInByteArray(unsigned char* byteArray, unsigned int byteSize,
		unsigned int outWid, unsigned int outHei, char* errorMessage = nullptr);

	unsigned int getSrcWidth() { return unResizeImageSizeX; }
	unsigned int getSrcHeight() { return unResizeImageSizeY; }
};

#endif
//エンコード

//8 × 8 pixel単位をブロック
//     ↓ 1マスが1ブロック
// RR  GG  BB
// RR  GG  BB
//      ↓RGBをYCbCrに分解 4:2:0の場合
// YY  bb  cc
// YY  bb  cc
//     ↓
// YYYYbc  ←  MCU単位

// 1ブロック毎(8×8pixel)に量子化
//左上のピクセルを基準にデータの変化を周波数として分解
//低周波のものから並べて8×8byteのデータにする(離散コサイン変換(ﾟ∀ﾟ))
//左上のデータをDC成分, それ以外をAC成分

//量子化テーブルの値で割る
//再生の場合は掛ける

//ジグザグスキャン
//低周波から高周波になるように,左上から右下にジグザグ状に並べ替えます

//ハフマン圧縮