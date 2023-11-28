/******************************************
ファイル名	:	FieldStruct.h

作成日	:	97.12.3
******************************************/
#ifndef __FIELD_STRUCT_H__
#define __FIELD_STRUCT_H__

#define kOneFieldPage	200		// １ページにつき動的に確保するフィールドの数

struct Field {
	Field		*nextField;
	short	evaluation;			// 形態素の頻度
	void		*contents;			// 単語の時は表記(ucahr *)．文節の時はHinshiを指すポインター
								// Hinshiを指すのは，Fieldは頻度で並びかえられてしまう恐れがあるため
//	DicInfo	dicInfo;				// 辞書上での位置
};

struct FieldPage {
	FieldPage		*next;				// 次ページへのポインタ
	Field			cell[1];					// このページにおけるヘッド
};

class FieldStruct{
public:
	FieldPage		*pageHead;
	Field			*freeField;

	FieldStruct(void);
	~FieldStruct(void);

	void NewFieldPage(void);
	Field* GetFreeField(void);
	void SetFreeField(Field* field);
};

#endif //__FIELD_STRUCT_H__