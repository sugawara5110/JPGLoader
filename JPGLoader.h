//*****************************************************************************************//
//**                                                                                     **//
//**                                JPGLoader                                            **//
//**                                                                                     **//
//*****************************************************************************************//

#ifndef Class_JPGLoader_Header
#define Class_JPGLoader_Header

#include <stdio.h>

class JPGLoader;
class HuffmanTree2;

struct signSet {
	unsigned short sign = 0;
	unsigned char numBit = 0;
	unsigned char valBit = 0;
	unsigned char runlength = 0;
};

struct outTree {
	unsigned char valBit = 0;
	unsigned char runlength = 0;
};

class HuffmanNode2 {

private:
	friend HuffmanTree2;
	HuffmanNode2* bit0 = nullptr;
	HuffmanNode2* bit1 = nullptr;
	outTree Val = {};

	void setVal(outTree val, unsigned short bitArr, unsigned char numBit);
	outTree getVal(unsigned long long* curSearchBit, unsigned char* byteArray);
	~HuffmanNode2();
};

class HuffmanTree2 {

private:
	friend JPGLoader;
	HuffmanNode2 hn;

	void createTree(signSet* sigArr, unsigned int arraySize);
	outTree getVal(unsigned long long* curSearchBit, unsigned char* byteArray);
};

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
		//�r�b�N�G���f�B�A��  [0x1234]�Ȃ�[0x12][0x34]�̏� 
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

	class DQTpara { //4��ތŒ�,SOF����Q�Ƃ����
	public:
		unsigned char Pqn = 0;//���4bit �ʎq���e�[�u�����x 0:8bit, 1:16bit  �ʏ�8bit
		unsigned char Tqn = 0;//����4bit �ʎq���e�[�u���ԍ� 0~3�܂�
		unsigned char Qn0[64] = {};//�ʎq���e�[�u�� 64�̗ʎq���W�� Pqn=0�̏ꍇ
		unsigned short Qn1[64] = {};//�ʎq���e�[�u�� 64�̗ʎq���W�� Pqn=1�̏ꍇ
	};

	class DHTpara {
	public:
		unsigned char Tcn = 0;//���4bit �n�t�}���e�[�u���N���X 0=DC���� 1=AC����
		unsigned char Thn = 0;//����4bit �n�t�}���e�[�u���ԍ� 0����3�܂ł̂����ꂩ
		unsigned char L[16] = {};//�n�t�}���e�[�u��bit�z���� ��:L[0]��1bit�����̐�, L[15]��16bit�����̐�

		//DC���� ����+�f�[�^�l�Ƃ���bit���тɑ΂��Ẵf�[�^�lbit��
		//AC���� ���4bit�@���������O�X��
		//       ����4bit�@DC�Ɠ���,�f�[�^�lbit��
		unsigned char* V = nullptr;
		//�f�[�^�l��2�i���Ō����ꍇ�擪bit1�̏ꍇ���̂܂܂̒l�Ƃ���
		//�擪bit0�̏ꍇbit���]�������l�Ƀ}�C�i�X������t�����l�Ƃ���,�ʏ��2�i���Ƃ͈Ⴄ
		//���k�f�[�^�͍ŏ��ɐ�����������bit�Ƀf�[�^bit�����ĕ���ł���
		//DC�̏ꍇ,��L�̕�����v�̏ꍇ����bit����Vbit�����o�����l���𓀌�bit�ƂȂ�
		//AC�̏ꍇ,��Ǎς�bit�Ō�����烉�������O�X�������l0��byte��ǉ�,���̌�Ɏ��o�����f�[�^�lbit��ǉ�
		~DHTpara() {
			delete[] V;
			V = nullptr;
		}
	};

	void createSign(signSet* sig, unsigned char* L, unsigned char* V, unsigned char Tcn);

	class SOF0component {
	public:
		unsigned char Cn = 0;//����ID
		unsigned char Hn = 0;//���4bit�����T���v�����O�l
		unsigned char Vn = 0;//����4bit�����T���v�����O�l
		unsigned char Tqn = 0;//�Ή��ʎq���e�[�u���ԍ�
	};
	class SOF0para {
	public:
		unsigned char P = 0;//�T���v�����x
		unsigned short Y = 0;//�摜�c�T�C�Y
		unsigned short X = 0;//�摜���T�C�Y
		unsigned char Nf = 0;//�\���v�f�̐�(SOF0component)1:�O���[�X�P�[��, 3:�J���[(YCbCr or YIQ), 4:�J���[(CMYK)

		SOF0component sofC[4] = {};
	};

	class SOScomponent {
	public:
		unsigned char Csn = 0;//����ID
		unsigned char Tdn = 0;//���4bit DC�����n�t�}���e�[�u���ԍ�
		unsigned char Tan = 0;//����4bit AC�����n�t�}���e�[�u���ԍ�
	};
	class SOSpara {
	public:
		unsigned char Ns = 0;//������(SOScomponent)
		SOScomponent* sosC = nullptr;
		unsigned char Ss = 0;//�ʎq���W���J�n�ԍ�
		unsigned char Se = 0;//�ʎq���W���I���ԍ�
		unsigned char Ah = 0;//���4bit �O��̃X�L�����̌W���l�����V�t�g��(�ŏ��̃X�L������0)
		unsigned char Al = 0;//����4bit �W���l�����V�t�g��                (�Ō�̃X�L������0)

		~SOSpara() {
			delete[] sosC;
			sosC = nullptr;
		}
	};

	struct YCrCb {
		float Y = 0.0f;//�P�x
		float Cb = 0.0f;//�����̐F��
		float Cr = 0.0f;//�ԕ����̐F��
	};
	struct RGB {
		unsigned char R = 0;
		unsigned char G = 0;
		unsigned char B = 0;
	};
	void decodeYCrCbtoRGB(RGB& dst, YCrCb& src);

public:
	unsigned char* loadJPG(char* pass, unsigned int outWid, unsigned int outHei);
};

#endif
