// ============================================================
//  KanaKan.h	by Hiroshi Lockheimer
// ============================================================

#ifndef _KANA_KAN
#define _KANA_KAN

#include <SupportDefs.h>
#include "Analysis.h"


class KanaKan {
public:
						KanaKan(const char *kana);
	virtual				~KanaKan();

	int32				PrevClause();
	int32				NextClause();
	void				SelectClause(int32 index);

	int32				ShrinkClause();
	int32				GrowClause();
	int32				ClauseWordLength(int32 clause) const;

	int32				PrevKouho();
	int32				NextKouho();
	void				SelectKouho(int32 index);
	
	void				Kakutei();

	int32				CountClauses() const;
	int32				ActiveClause() const;

	int32				CountKouhos(int32 clause) const;
	int32				SelectedKouho(int32 clause) const;

	const char*			KouhoAt(int32 clause, int32 index) const;
	int32				KouhoLengthAt(int32 clause, int32 index) const;

	const char*			SelectedKouhoAt(int32 clause) const;
	int32				SelectedKouhoLengthAt(int32 clause) const;

	char*				CompositeString() const;

protected:
	void				Reset(int32 fromClause = 0);

private:
	char*				fKana;
	KAnalysis			fAnalysis;
	KClauseArrayArray	fClauses;
	char**				fKanaKouhos;
	int32				fNumKanaKouhos;
	int32*				fNumKouhosList;
	int32*				fSelectedKouhoList;
	int32				fActiveClauseNum;
};

#endif
