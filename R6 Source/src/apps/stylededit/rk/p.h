#ifndef PROTO_H_INCLUDED
#define PROTO_H_INCLUDED

#ifdef __cplusplus
extern "C" {
#endif

/* RK2bun.c */
struct nstore *_RkReallocBunStorage(struct nstore *src, unsigned len);
int RkwBgnBun(int cx_num, Wchar *yomi, int n, uint kouhomode);
int RkwEndBun(int cx_num, int mode);
int RkwRemoveBun(int cx_num, int mode);
int RkwSubstYomi(int cx_num, int ys, int ye, Wchar *yomi, int newLen);
int RkwFlushYomi(int cx_num);
int _RkResize(int cx_num, int len, int t);
int RkwResize(int cx_num, int len);
int RkeResize(int cx_num, int len);
int RkwEnlarge(int cx_num);
int RkwShorten(int cx_num);
int RkwStoreYomi(int cx_num, Wchar *yomi, int nlen);
int RkwGoTo(int cx_num, int bnum);
int RkwLeft(int cx_num);
int RkwRight(int cx_num);
int RkwXfer(int cx_num, int knum);
int RkwNfer(int cx_num);
int RkwNext(int cx_num);
int RkwPrev(int cx_num);
int RkwGetStat(int cx_num, RkStat *st);
int RkeGetStat(int cx_num, RkStat *st);
int RkwGetYomi(int cx_num, Wchar *yomi, int maxyomi);
int RkwGetLastYomi(int cx_num, Wchar *yomi, int maxyomi);
int RkwGetKanji(int cx_num, Wchar *dst, int maxdst);
int RkwGetKanjiList(int cx_num, Wchar *dst, int maxdst);
int RkwGetLex(int cx_num, RkLex *dst, int maxdst);
int RkeGetLex(int cx_num, RkLex *dst, int maxdst);
int RkwGetHinshi(int cx_num, Wchar *dst, int maxdst);
int RkwQueryDic(int cx_num, uchar *dirname, uchar *dicname, struct DicInfo *status);
int _RkwSync(RkContext *cx, char *dicname);
int RkwSync(int cx_num, char *dicname);
int RkwGetSimpleKanji(int cxnum, char *dicname, Wchar *yomi, int maxyomi, Wchar *kanjis, int maxkanjis, Wchar *hinshis, int maxhinshis);
int RkwStoreRange(int cx_num, Wchar *yomi, int maxyomi);

/* RKbits.c */
int _RkPackBits(uchar *dst_bits, int dst_offset, int bit_size, unsigned *src_ints, int count);
int _RkUnpackBits(unsigned *dst_ints, uchar *src_bits, int src_offset, int bit_size, int count);
int _RkCopyBits(uchar *dst_bits, int dst_offset, int bit_size, uchar *src_bits, int src_offset, int count);
int _RkSetBitNum(uchar *dst_bits, int dst_offset, int bit_size, int n, int val);
int _RkCalcFqSize(int n);
int _RkPrintPackedBits(uchar *bits, int offset, int bit_size, int count);
int _RkCalcLog2(int n);

/* RKcontext.c */
int RkwInitialize(char *ddhome);
void RkwFinalize(void);
struct RkParam *RkGetSystem(void);
struct DD *RkGetSystemDD(void);
RkContext *RkGetContext(int cx_num);
RkContext *RkGetXContext(int cx_num);
void _RkEndBun(RkContext *cx);
int RkwSetDicPath(int cx_num, char *path);
int RkwCreateContext(void);
int RkwCloseContext(int cx_num);
int RkwDuplicateContext(int cx_num);
int RkwMountDic(int cx_num, char *name, int mode);
int RkwUnmountDic(int cx_num, char *name);
int RkwRemountDic(int cx_num, char *name, int mode);
int RkwGetMountList(int cx_num, char *mdname, int maxmdname);
int RkwGetDicList(int cx_num, char *mdname, int maxmdname);
int RkwGetDirList(int cx_num, char *ddname, int maxddname);
int RkwDefineDic(int cx_num, char *name, Wchar *word);
int RkwDeleteDic(int cx_num, char *name, Wchar *word);

/* RKdd.c */
char *allocStr(char *s);
int _RkRealizeDF(struct DF *df);
char *_RkCreatePath(struct DD *dd, char *name);
char *_RkCreateUniquePath(struct DD *dd, char *proto);
char *_RkMakePath(struct DF *df);
int _RkRealizeDD(struct DD *dd);
int _RkIsInDDP(struct DD **ddp, struct DD *dd);
struct DD **_RkCopyDDP(struct DD **ddp);
struct DD **_RkCreateDDP(char *ddpath);
void _RkFreeDDP(struct DD **ddp);
struct DM *_RkSearchDDP(struct DD **ddp, char *name);
struct DM *_RkSearchDDQ(struct DD **ddp, char *name, int type);
struct DM *_RkSearchUDDP(struct DD **ddp, uchar *name);
struct DM *_RkSearchDDMEM(struct DD **ddp, char *name);
struct DM *_RkSearchDicWithFreq(struct DD **ddpath, char *name, struct DM **qmp);
int DMcheck(char *spec, uchar *name);
struct DM *DMcreate(struct DD *dd, char *spec);
int DMremove(struct DM *dm);
int DMrename(struct DM *dm, uchar *nickname);
int DMchmod(struct DM *dm, int mode);
int DDchmod(struct DD *dd, int mode);
int _RkMountMD(RkContext *cx, struct DM *dm, struct DM *qm, int mode, int firsttime);
void _RkUmountMD(RkContext *cx, struct MD *md);

/* RKdic.c */
int RkwCreateDic(int cx_num, uchar *dicname, int mode);
int copyFile(struct DM *src, struct DM *dst);
int RkwListDic(int cx_num, uchar *dirname, uchar *buf, int size);
int RkwRemoveDic(int cx_num, uchar *dicname, int mode);
int RkwRenameDic(int cx_num, uchar *old, uchar *newc, int mode);
int RkwCopyDic(int co, char *dir, char *from, char *to, int mode);
int RkwChmodDic(int cx_num, char *dicname, int mode);
void freeTdn(RkContext *cx);
int RkwGetWordTextDic(int cx_num, uchar *dirname, uchar *dicname, Wchar *info, int infolen);

/* RKdicsw.c */

/* RKfq.c */
struct RUT *allocRUT(unsigned hn);
unsigned searchRut(struct RUT *ruc, unsigned csn);
int entryRut(struct RUT *ruc, unsigned csn, unsigned tick);
struct RUT *LoadRUC(int fr);
int SaveRUC(int fr, struct RUT *ruc);
int FQopen(struct DM *dm, struct DM *qm, char *file, int mode);
int FQclose(RkContext *cx, struct DM *dm, struct DM *qm, char *file);
int FQsync(RkContext *cx, struct DM *dm, struct DM *qm, char *file);

/* RKkana.c */
int RkCvtZen(uchar *zen, int maxzen, uchar *han, int maxhan);
int RkCvtHan(uchar *han, int maxhan, uchar *zen, int maxzen);
int RkCvtKana(uchar *kana, int maxkana, uchar *hira, int maxhira);
int RkCvtHira(uchar *hira, int maxhira, uchar *kana, int maxkana);
int RkCvtNone(uchar *dst, int maxdst, uchar *src, int maxsrc);
int RkCvtWide(Wchar *dst, int maxdst, char *src, int maxsrc);
int RkCvtNarrow(char *dst, int maxdst, Wchar *src, int maxsrc);
int RkCvtEuc(uchar *euc, int maxeuc, uchar *sj, int maxsj);
int RkwCvtSuuji(Wchar *dst, int maxdst, Wchar *src, int maxsrc, int format);
int RkwCvtHan(Wchar *dst, int maxdst, Wchar *src, int srclen);
int RkwCvtHira(Wchar *dst, int maxdst, Wchar *src, int srclen);
int RkwCvtKana(Wchar *dst, int maxdst, Wchar *src, int srclen);
int RkwCvtZen(Wchar *dst, int maxdst, Wchar *src, int srclen);
int RkwCvtNone(Wchar *dst, int maxdst, Wchar *src, int srclen);

/* RKncache.c */
int _RkInitializeCache(int size);
void _RkFinalizeCache(void);
int _RkRelease(void);
void _RkDerefCache(struct ncache *cache);
void _RkPurgeCache(struct ncache *cache);
void _RkKillCache(struct DM *dm);
struct ncache *_RkFindCache(struct DM *dm, uchar *addr);
void _RkRehashCache(struct ncache *cache, uchar *addr);
struct ncache *_RkReadCache(struct DM *dm, uchar *addr);

/* RKngram.c */
void RkCloseGram(struct RkKxGram *gram);
struct RkKxGram *RkReadGram(int fd);
struct RkKxGram *RkOpenGram(char *mydic);
struct RkKxGram *RkDuplicateGram(struct RkKxGram *ogram);
int _RkWordLength(uchar *wrec);
int _RkCandNumber(uchar *wrec);
int RkGetGramNum(struct RkKxGram *gram, char *name);
Wrec *RkParseWrec(struct RkKxGram *gram, Wchar *src, unsigned left, uchar *dst, unsigned maxdst);
Wrec *RkParseOWrec(struct RkKxGram *gram, Wchar *src, uchar *dst, unsigned maxdst, unsigned *lucks);
Wchar *RkParseGramNum(struct RkKxGram *gram, Wchar *src, int *row);
uchar *RkGetGramName(struct RkKxGram *gram, int row);
Wchar *RkUparseGramNum(struct RkKxGram *gram, int row, Wchar *dst, int maxdst);
int _RkRowNumber(uchar *wrec);
Wchar *_RkUparseWrec(struct RkKxGram *gram, Wrec *src, Wchar *dst, int maxdst, unsigned *lucks, int add);
Wchar *RkUparseWrec(struct RkKxGram *gram, Wrec *src, Wchar *dst, int maxdst, unsigned *lucks);
struct TW *RkCopyWrec(struct TW *src);
int RkScanWcand(Wrec *wrec, struct RkWcand *word, int maxword);
int RkUniqWcand(struct RkWcand *wc, int nwc);
int RkUnionWcand(struct RkWcand *wc1, int nc1, int wlen1, struct RkWcand *wc2, int nc2);
int RkSubtractWcand(struct RkWcand *wc1, int nc1, struct RkWcand *wc2, int nc2, unsigned *lucks);
struct TW *RkSubtractWrec(struct TW *tw1, struct TW *tw2);
struct TW *RkUnionWrec(struct TW *tw1, struct TW *tw2);

/* RKnheap.c */
int _RkInitializeHeap(int size);
void _RkFinalizeHeap(void);
uchar *_RkNewHeap(unsigned n);
void _RkFreeHeap(uchar *p);
void _RkShowHeap(void);

/* RKnword.c */
void _RkFreeBunq(struct nstore *st);
void _RkFreeQue(struct nstore *st, int s, int e);
int _RkRegisterNV(struct NV *nv, Wrec *yomi, int len, int half);
Wchar *_RkGetKanji(struct nword *cw, Wchar *key, unsigned long mode);
int _RkRenbun2(RkContext *cx, int firstlen);
int _RkSubstYomi(RkContext *cx, int ys, int ye, Wchar *yomi, int newLen);
int _RkFlushYomi(RkContext *cx);
void _RkLearnBun(RkContext *cx, int cur, int mode);

/* RKpermdic.c */
int _Rkpopen(struct DM *dm, char *dfnm, int mode, struct RkKxGram *gram);
int _Rkpclose(struct DM *dm, char *dfnm, struct RkKxGram *gram);
int _RkEql(Wchar *a, uchar *b, int n);
int _Rkpsearch(RkContext *cx, struct DM *dm, Wchar *key, int n, struct nread *nread, int mc, int *cf);
int _Rkpio(struct DM *dm, struct ncache *cp, int io);
int _Rkpctl(struct DM *dm, struct DM *qm, int what, Wchar *arg, struct RkKxGram *gram);
int _Rkpsync(RkContext *cx, struct DM *dm, struct DM *qm);

/* RKtempdic.c */
int _Rktopen(struct DM *dm, char *file, int mode, struct RkKxGram *gram);
int _Rktclose(struct DM *dm, char *file, struct RkKxGram *gram);
int _Rktsearch(RkContext *cx, struct DM *dm, Wchar *key, int n, struct nread *nread, int maxcache, int *cf);
int _Rktio(struct DM *dm, struct ncache *cp, int io);
int _Rktctl(struct DM *dm, struct DM *qm, int what, Wchar *arg, struct RkKxGram *gram);
int _Rktsync(RkContext *cx, struct DM *dm, struct DM *qm);

/* RKutil.c */
int uslen(Wchar *us);
void usncopy(Wchar *dst, Wchar *src, int len);
uchar *ustoeuc(Wchar *src, int srclen, uchar *dest, int destlen);
Wchar *euctous(uchar *src, int srclen, Wchar *dest, int destlen);
void _Rkpanic(char *fmt, int p, int q, int r);
int _RkCalcUnlog2(int x);
int _RkCalcLog2(int n);
Wchar uniqAlnum(Wchar c);
void _RkClearHeader(struct HD *hd);
int _RkReadHeader(int fd, struct HD *hd, off_t off_from_top);
uchar *_RkCreateHeader(struct HD *hd, unsigned *size);
unsigned _RkGetTick(int mode);
int set_hdr_var(struct HD *hd, int n, unsigned long var);
int _RkGetLink(struct ND *dic, int pgno, unsigned off, unsigned *lvo, unsigned *csn);
unsigned _RkGetOffset(struct ND *dic, uchar *pos);
int HowManyChars(Wchar *yomi, int len);
int HowManyBytes(Wchar *yomi, int len);
int printWord(struct nword *w);
int showWord(struct nword *w);

/* RKroma.c */
RkRxDic *RkOpenRoma(char *romaji);
void RkCloseRoma(RkRxDic *rdic);
int RkMapRoma(RkRxDic *rdic, uchar *dst, int maxdst, uchar *src, int maxsrc, int flags, int *status);
int RkMapPhonogram(RkRxDic *rdic, uchar *dst, int maxdst, uchar *src, int srclen, uchar key, int flags, int *used_len_return, int *dst_len_return, int *tmp_len_return, int *rule_id_inout);
int RkCvtRoma(RkRxDic *rdic, uchar *dst, int maxdst, uchar *src, int maxsrc, unsigned flags);

#ifdef __cplusplus
}
#endif

#endif // PROTO_H_INCLUDED
