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
		//�r�b�N�G���f�B�A��
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
		unsigned char L[16] = {};//�n�t�}���e�[�t��bit�z���� ��:L[0]��1bit�����̐�, L[15]��16bit�����̐�

		//DC���� ����+�f�[�^�l�Ƃ���bit���тɑ΂��Ẵf�[�^�lbit��
		//AC���� ���4bit�@���������O�X��
		//       ����4bit�@DC�Ɠ���,�f�[�^�lbit��
		unsigned char* V = nullptr;
		//�f�[�^�l��2�i���Ō����ꍇ�擪bit1�̏ꍇ���̂܂܂̒l�Ƃ���
		//�擪bit0�̏ꍇbit���]�������l�Ƀ}�C�i�X������t�����l�Ƃ���,�ʏ��2�i���Ƃ͈Ⴄ
		//�ŏ��ɐ�����������bit�Ƀf�[�^bit������, �f�[�^�l�Ƒg�ɂ����\�𐶐�
		//������c���[�\���ɂ���(DC,AC����)
		//DC�̏ꍇ,��L�̕�����v�̏ꍇ�͂��̃f�[�^�lbit����Ǎς݂Ƃ��Ēǉ����Ă���
		//AC�̏ꍇ,��Ǎς�bit�Ō�����烉�������O�X�������l0��byte��ǉ�,���̌�Ƀf�[�^�lbit��ǉ�
		~DHTpara() {
			delete[] V;
			V = nullptr;
		}
	};

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

public:
	unsigned char* loadJPG(char* pass, unsigned int outWid, unsigned int outHei);
};

#endif
