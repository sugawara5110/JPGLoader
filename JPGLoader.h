//*****************************************************************************************//
//**                                                                                     **//
//**                                JPGLoader                                            **//
//**                                                                                     **//
//*****************************************************************************************//

#ifndef Class_JPGLoader_Header
#define Class_JPGLoader_Header

#include <stdio.h>

class JPGLoader {

private:

	class bytePointer {
	private:
		unsigned char* byte = nullptr;
		unsigned int index = 0;
		unsigned int Size = 0;
		bytePointer() {}
	public:
		bytePointer(unsigned int size, FILE* fp) {
			byte = new unsigned char[size];
			fread(byte, sizeof(unsigned char), size, fp);
			Size = size;
		}
		~bytePointer() {
			delete[] byte;
			byte = nullptr;
		}
		//ビックエンディアン
		unsigned int convertUCHARtoUINT() {
			unsigned int ret = ((unsigned int)byte[index + 0] << 24) | ((unsigned int)byte[index + 1] << 16) |
				((unsigned int)byte[index + 2] << 8) | ((unsigned int)byte[index + 3]);
			index += 4;
			return ret;
		}
		unsigned short convertUCHARtoShort() {
			unsigned int ret = ((unsigned int)byte[index + 0] << 8) | ((unsigned int)byte[index + 1]);
			index += 2;
			return ret;
		}
		unsigned char getChar() {
			unsigned char ret = byte[index];
			index++;
			return ret;
		}
		unsigned char* getPointer(unsigned int addPointer) {
			unsigned char* ret = &byte[index];
			index += addPointer;
			return ret;
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
		unsigned char Pqn = 0;//上位4bit 量子化テーブル制度 0:8bit, 1:16bit  通常8bit
		unsigned char Tqn = 0;//下位4bit 量子化テーブル番号 0~3まで
		unsigned char Qn0[64] = {};//量子化テーブル 64個の量子化係数 Pqn=0の場合
		unsigned short Qn1[64] = {};//量子化テーブル 64個の量子化係数 Pqn=1の場合
	};

	class DHTpara {
	public:
		unsigned char Tcn = 0;//上位4bit ハフマンテーブルクラス 0=DC成分 1=AC成分
		unsigned char Thn = 0;//下位4bit ハフマンテーブル番号 0から3までのいずれか
		unsigned char L[16] = {};//ハフマンテーフルbit配分数 例:L[0]は1bit符号の数, L[15]は16bit符号の数

		//DC成分 符号+データ値というbit並びに対してのデータ値bit数
		//AC成分 上位4bit　ランレングス数
		//       下位4bit　DCと同じ,データ値bit数
		unsigned char* V = nullptr;
		//データ値を2進数で見た場合先頭bit1の場合そのままの値とする
		//先頭bit0の場合bit反転した数値にマイナス符号を付けた値とする,通常の2進数とは違う
		//最初に生成した符号bitにデータbitをつけて, データ値と組にした表を生成
		//それをツリー構造にする(DC,AC共通)
		//DCの場合,上記の符号一致の場合はそのデータ値bitを解読済みとして追加していく
		//ACの場合,解読済みbit最後尾からランレングス数だけ値0のbyteを追加,その後にデータ値bitを追加
		~DHTpara() {
			delete[] V;
			V = nullptr;
		}
	};

	class SOF0component {
	public:
		unsigned char Cn = 0;//成分ID
		unsigned char Hn = 0;//上位4bit水平サンプリング値
		unsigned char Vn = 0;//下位4bit垂直サンプリング値
		unsigned char Tqn = 0;//対応量子化テーブル番号
	};
	class SOF0para {
	public:
		unsigned char P = 0;//サンプル精度
		unsigned short Y = 0;//画像縦サイズ
		unsigned short X = 0;//画像横サイズ
		unsigned char Nf = 0;//構成要素の数(SOF0component)1:グレースケール, 3:カラー(YCbCr or YIQ), 4:カラー(CMYK)

		SOF0component sofC[4] = {};
	};

	class SOScomponent {
	public:
		unsigned char Csn = 0;//成分ID
		unsigned char Tdn = 0;//上位4bit DC成分ハフマンテーブル番号
		unsigned char Tan = 0;//下位4bit AC成分ハフマンテーブル番号
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

public:
	unsigned char* loadJPG(char* pass, unsigned int outWid, unsigned int outHei);
};

#endif
