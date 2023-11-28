// ============================================================
//  KanaKan.cpp	by Hiroshi Lockheimer
// ============================================================
#include <SupportDefs.h>
#include <string.h>
#include <malloc.h>
#include <File.h>
#include <Entry.h>
#include <ByteOrder.h>

#include "KanaKan.h"
#include "KanaString.h"
#include <malloc.h>


const int32 kNumKanaKouhos			= 2;
const int32 kKanKouhoStart			= kNumKanaKouhos;
const int32 kZenkakuKatakanaKouho	= 0;
const int32 kHiraganaKouho 			= 1;


KanaKan::KanaKan(
	const char	*kana)
{
	fKana = strdup(kana);
	fKanaKouhos = NULL;
	fNumKanaKouhos = 0;
	fNumKouhosList = NULL;
	fSelectedKouhoList = NULL;
	fActiveClauseNum = 0;

	fAnalysis.AnalyzeText((uchar *)kana);
	fAnalysis.GetResult(&fClauses);

	Reset();
}


KanaKan::~KanaKan()
{
	free(fKana);
	for (int32 i = 0; i < fNumKanaKouhos; i++)
		free(fKanaKouhos[i]);
	free(fKanaKouhos);
	free(fNumKouhosList);
	free(fSelectedKouhoList);
}


int32
KanaKan::PrevClause()
{
	if (fActiveClauseNum > 0)
		fActiveClauseNum--;

	return (fActiveClauseNum);
}


int32
KanaKan::NextClause()
{
	if (fActiveClauseNum < (CountClauses() - 1))
		fActiveClauseNum++;

	return (fActiveClauseNum);
}


void
KanaKan::SelectClause(
	int32	index)
{
	int32 numClauses = CountClauses() - 1;
	index = (index > numClauses) ? numClauses : index;
	index = (index < 0) ? 0 : index;

	fActiveClauseNum = index;
}


int32
KanaKan::ShrinkClause()
{
	int32 size = fClauses[fActiveClauseNum].GetWordSize();
	size = (size > 3) ? size - 3 : size;

	fAnalysis.ReAnalyze(fActiveClauseNum, size, &fClauses);
	Reset(fActiveClauseNum);

	return (size);
}


int32
KanaKan::GrowClause()
{
	int32 size = fClauses[fActiveClauseNum].GetWordSize() + 3;

	fAnalysis.ReAnalyze(fActiveClauseNum, size, &fClauses);
	Reset(fActiveClauseNum);

	return (size);
}


int32
KanaKan::ClauseWordLength(
	int32	clause) const
{
	return (fClauses[clause].GetWordSize());
}


int32
KanaKan::PrevKouho()
{
	if (fSelectedKouhoList[fActiveClauseNum] > 0)
		fSelectedKouhoList[fActiveClauseNum]--;	

	return (fSelectedKouhoList[fActiveClauseNum]);
}


int32
KanaKan::NextKouho()
{
	if (fSelectedKouhoList[fActiveClauseNum] < (CountKouhos(fActiveClauseNum) - 1))
		fSelectedKouhoList[fActiveClauseNum]++;	

	return (fSelectedKouhoList[fActiveClauseNum]);
}


void
KanaKan::SelectKouho(
	int32	index)
{
	fSelectedKouhoList[fActiveClauseNum] = index;
}


void
KanaKan::Kakutei()
{
	int32 numClauses = CountClauses();

	for (int32 i = 0; i < numClauses; i++) {
		if (fSelectedKouhoList[i] < kKanKouhoStart)
			continue;

		fAnalysis.LearnResult(i, fSelectedKouhoList[i] - kKanKouhoStart);
	}
}


int32
KanaKan::CountClauses() const
{
	return (fClauses.GetSize());
}


int32
KanaKan::ActiveClause() const
{
	return (fActiveClauseNum);
}


int32
KanaKan::CountKouhos(
	int32	clause) const
{
	return (fNumKouhosList[clause]);
}


int32
KanaKan::SelectedKouho(
	int32	clause) const
{
	return (fSelectedKouhoList[clause]);
}


const char*
KanaKan::KouhoAt(
	int32	clause,
	int32	index) const
{
	if (index < kKanKouhoStart)
		return (fKanaKouhos[(clause * kNumKanaKouhos) + index]);

	return ((char *)fClauses[clause][index - kKanKouhoStart].GetClause());
}


int32
KanaKan::KouhoLengthAt(
	int32	clause, 
	int32	index) const
{
	if (index < kKanKouhoStart)
		return (fClauses[clause].GetWordSize());

	return (fClauses[clause][index - kKanKouhoStart].GetClauseSize());
}


const char*
KanaKan::SelectedKouhoAt(
	int32	clause) const
{
	return (KouhoAt(clause, SelectedKouho(clause)));
}


int32
KanaKan::SelectedKouhoLengthAt(
	int32	clause) const
{
	return (KouhoLengthAt(clause, SelectedKouho(clause)));
}


char*
KanaKan::CompositeString() const
{
	char	*string = NULL;
	int32	stringLen = 0;
	int32	numClauses = CountClauses();
	
	for (int32 i = 0; i < numClauses; i++)
		stringLen += SelectedKouhoLengthAt(i);

	string = (char *)malloc(stringLen + 1);

	char *stringHead = string;
	for (int32 i = 0; i < numClauses; i++) {
		int32 clauseLen = SelectedKouhoLengthAt(i);

		memcpy(stringHead, SelectedKouhoAt(i), clauseLen);
		stringHead += clauseLen;
	}

	string[stringLen] = '\0';

	return (string);		
}


void
KanaKan::Reset(
	int32	fromClause)
{
	int32 numClauses = CountClauses();
	
	// reset fKanaKouhos
	for (int32 i = 0; i < fNumKanaKouhos; i++)
		free(fKanaKouhos[i]);
	free(fKanaKouhos);
	fKanaKouhos = NULL;

	fNumKanaKouhos = numClauses * kNumKanaKouhos;
	fKanaKouhos = (char **)malloc(fNumKanaKouhos * sizeof(char *));

	int32 clauseKanaStart = 0;
	int32 kanaKouhoIndex = 0;
	for (int32 i = 0; i < numClauses; i++) {
		int32 kouhoLength;

		kouhoLength = KouhoLengthAt(i, 0);
		fKanaKouhos[kanaKouhoIndex++] = KanaString::ConvertKanaString(fKana + clauseKanaStart, 
											   				   		  &kouhoLength,
															   		  HIRA_TO_KATA);

		kouhoLength = KouhoLengthAt(i, 1);
		fKanaKouhos[kanaKouhoIndex++] = KanaString::ConvertKanaString(fKana + clauseKanaStart, 
										   				   			  &kouhoLength,
															   		  KATA_TO_HIRA);

		clauseKanaStart += fClauses[i].GetWordSize();
	}

	// reset fNumKouhosList and fSelectedKouhoList
	int32	*saveNumKouhosList = NULL;
	int32	saveNumKouhosListSize = 0;
	int32	*saveKouhoList = NULL;
	int32	saveKouhoListSize = 0;

	if (fNumKouhosList != NULL) {
		saveNumKouhosListSize = fromClause * sizeof(int32);
		if (saveNumKouhosListSize > 0) { 
			saveNumKouhosList = (int32 *)malloc(saveNumKouhosListSize);
			memcpy(saveNumKouhosList, fNumKouhosList, saveNumKouhosListSize);
		}

		free(fNumKouhosList);
		fNumKouhosList = NULL;
	}

	if (fSelectedKouhoList != NULL) {
		saveKouhoListSize = fromClause * sizeof(int32);
		if (saveKouhoListSize > 0) {
			saveKouhoList = (int32 *)malloc(saveKouhoListSize);
			memcpy(saveKouhoList, fSelectedKouhoList, saveKouhoListSize);
		}

		free(fSelectedKouhoList);
		fSelectedKouhoList = NULL;
	}

	fNumKouhosList = (int32 *)malloc(numClauses * sizeof(int32));
	fSelectedKouhoList = (int32 *)malloc(numClauses * sizeof(int32));

	for (int32 i = fromClause; i < numClauses; i++) {
		int32 numKanKouhos = fClauses[i].GetSize();
		int32 selectedKouho = kKanKouhoStart;

		fNumKouhosList[i] = kNumKanaKouhos + numKanKouhos;

		if (numKanKouhos == 1) {
			const char	*theKouho = (char *)fClauses[i][0].GetClause();
			int32		kanaKouhoIndex = i * kNumKanaKouhos;

			if (strcmp(theKouho, fKanaKouhos[kanaKouhoIndex]) == 0) { 
				fNumKouhosList[i]--;
				selectedKouho = 0;
			}

			if (strcmp(theKouho, fKanaKouhos[kanaKouhoIndex + 1]) == 0) {
				fNumKouhosList[i]--;
				selectedKouho = 1;
			}
		}

		selectedKouho = (selectedKouho >= fNumKouhosList[i]) ? fNumKouhosList[i] - 1 : selectedKouho;
		fSelectedKouhoList[i] = selectedKouho;
	}

	if (saveNumKouhosList != NULL) {
		memcpy(fNumKouhosList, saveNumKouhosList, saveNumKouhosListSize);

		free(saveNumKouhosList);
		saveNumKouhosList = NULL;
		saveNumKouhosListSize = 0;
	}

	if (saveKouhoList != NULL) {
		memcpy(fSelectedKouhoList, saveKouhoList, saveKouhoListSize);

		free(saveKouhoList);
		saveKouhoList = NULL;
		saveKouhoListSize = 0;
	}
}

