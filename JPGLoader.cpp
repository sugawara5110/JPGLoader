//*****************************************************************************************//
//**                                                                                     **//
//**                                JPGLoader                                            **//
//**                                                                                     **//
//*****************************************************************************************//

#define _CRT_SECURE_NO_WARNINGS
#include "JPGLoader.h"
#include <string.h>

unsigned char* JPGLoader::loadJPG(char* pass, unsigned int outWid, unsigned int outHei) {
	FILE* fp = fopen(pass, "rb");
	if (fp == NULL) {
		return nullptr;
	}
	unsigned int count = 0;
	while (fgetc(fp) != EOF) {
		count++;
	}
	fseek(fp, 0, SEEK_SET);//�ŏ��ɖ߂�
	bytePointer* bp = new bytePointer(count, fp);
	fclose(fp);

	const unsigned int imageNumChannel = 4;
	const unsigned int numPixel = outWid * imageNumChannel * outHei;
	unsigned char* image = new unsigned char[numPixel];//�O���ŊJ��

	//�}�[�J�[
	const unsigned short SOI = 0xffd8;//�X�^�[�g
	const unsigned short DQT = 0xffdb;//�ʎq���e�[�u����`
	const unsigned short DHT = 0xffc4;//�n�t�}���e�[�u����`
	const unsigned short SOF0 = 0xffc0;//�t���[���w�b�_�[
	const unsigned short SOS = 0xffda;//�X�L�����w�b�_�[
	const unsigned short EOI = 0xffd9;//�G���h

	//---jpg�\��---//
	//SOI
	//�Z�O�����g �~ N��
	//�C���[�W�f�[�^
	//EOI

	//---�Z�O�����g�\��---//
	//�}�[�J�[2byte
	//�����O�X��2byte ���e: �Z�O�����g�p�����[�^�[byte�� + 2byte
	//�Z�O�����g�p�����[�^�[

	DQTpara dqpara[4] = {};
	DHTpara dhpara[4] = {};
	SOF0para sof0para = {};
	SOSpara sospara = {};
	unsigned int dqparaIndex = 0;
	unsigned int dhparaIndex = 0;
	unsigned char* compImage = nullptr;

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
				DHTpara* d = &dhpara[dhparaIndex++];
				d->Tcn = (pt >> 4) & 0xf;
				d->Thn = pt & 0xf;
				memcpy(d->L, bp->getPointer(16), 16);
				int lcnt = 0;
				for (int l = 0; l < 16; l++)lcnt += d->L[l];
				d->V = new unsigned char[lcnt];
				memcpy(d->V, bp->getPointer(lcnt), lcnt);
				len -= (17 + lcnt);
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
				s[nf].Tqn = bp->getChar();
			}
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
			//���̒���C���[�W�f�[�^
			unsigned int imageSt = bp->getIndex();
			//�T�C�Y�J�E���g
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
			cnt = 0;
			//���k�C���[�W�i�[
			while (bp->checkEOF()) {
				unsigned char t = bp->getChar();
				if (0xff == t) {
					if (0xd9 == bp->getChar()) {
						break;
					}
				}
				compImage[cnt++] = t;
			}
			break;
		}
		//��L�}�[�J�[�ȊO�X�L�b�v
		unsigned short len = bp->convertUCHARtoShort() - 2;
		bp->getPointer(len);
	} while (bp->checkEOF());
	if (!eoi)return nullptr;

	delete bp;
	bp = nullptr;
	delete[] compImage;
	compImage = nullptr;

	return image;
}