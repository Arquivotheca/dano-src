
/*
** Macros to generate evenly spaced trig values of pi/(2^n)
** 
** Select from different levels of speed vs acuracy by setting the 
** TRIG_VERSION preprocessor variable.  This can make a drastic difference
** in the time and accuracy of a FFT.
*/
#ifndef TRIG_VERSION
#define TRIG_VERSION 3
#endif

#define T_NOT_USEFUL -1
#define T_CLIB_0      0
#define T_BUNE_0      1
#define T_BUNE_1      2
#define T_BUNE_2      3
#define T_NREC_0      4
#define T_SIMP_0      5
#define T_LKUP_0      6
#define T_CLLU_0      7


#define TRIG_TABLE_SIZE 22
/*
** Algorithm: Trig generation via library function calls.
** 
** Performance: Great accuracy, pathetic speed.
** 
**   Speed and accuracy are both at the mercy of your library function
**   supplier.  
**   
** Computation: unknown
** 
*/
#if (TRIG_VERSION == T_CLIB_0)


#include <math.h>
#define TRIG_VARS                                  \
      float angle=0;                              \
      int angle_index;
#define TRIG_INIT(k,c,s)                           \
     {                                             \
      angle=PI/2/(1<<k);                           \
      angle_index = 0;                             \
      c = 1;                                       \
      s = 0;                                       \
     }
#define TRIG_NEXT(k,c,s)                           \
     {                                             \
      angle_index++;                               \
      c=cos(angle*angle_index);                    \
      s=sin(angle*angle_index);                    \
     }
#define TRIG_23(k,c1,s1,c2,s2,c3,s3)               \
     {                                             \
         c2 = cos(angle*(angle_index*2));          \
         s2 = sin(angle*(angle_index*2));          \
         c3 = cos(angle*(angle_index*3));          \
         s3 = sin(angle*(angle_index*3));          \
     }
#endif



/*
** Algorithm: O. Buneman's trig generator.
** 
** Performance: Good accuracy, mediocre speed.
** 
**   Manipulates a log(n) table to stably create n evenly spaced trig
**   values. The newly generated values lay halfway between two
**   known values, and are calculated by appropriatelly scaling the
**   average of the known trig values appropriatelly according to the
**   angle between them.  This process is inherently stable; and is
**   about as accurate as most trig library functions.  The errors
**   caused by this code segment are primarily due to the less stable
**   method to calculate values for 2t and s 3t. Note: I believe this
**   algorithm is patented(!), see readme file for more details.
**
** Computation: 1 +, 1 *, much integer math,  per trig value
**
*/

#if ((TRIG_VERSION == T_BUNE_0) ||  (TRIG_VERSION == T_BUNE_1) || \
     (TRIG_VERSION == T_BUNE_2))
#define TRIG_VARS                                                \
      int t_lam=0;                                               \
      float coswrk[TRIG_TABLE_SIZE],sinwrk[TRIG_TABLE_SIZE];
#define TRIG_INIT(k,c,s)                                         \
     {                                                           \
      int i;                                                     \
      for (i=0 ; i<=k ; i++)                                     \
          {coswrk[i]=costab[i];sinwrk[i]=sintab[i];}             \
      t_lam = 0;                                                 \
      c = 1;                                                     \
      s = 0;                                                     \
     }
#define TRIG_NEXT(k,c,s)                                         \
     {                                                           \
         int i,j;                                                \
         (t_lam)++;                                              \
         for (i=0 ; !((1<<i)&t_lam) ; i++);                      \
         i = k-i;                                                \
         s = sinwrk[i];                                          \
         c = coswrk[i];                                          \
         if (i>1)                                                \
            {                                                    \
             for (j=k-i+2 ; (1<<j)&t_lam ; j++);                 \
             j         = k - j;                                  \
             sinwrk[i] = halsec[i] * (sinwrk[i-1] + sinwrk[j]);  \
             coswrk[i] = halsec[i] * (coswrk[i-1] + coswrk[j]);  \
            }                                                    \
     }
#endif

/*
** Algorithm: Variations on generating throwaway trig values.
** 
** Performance: 3 levels of trading off accuracy and speed
** 
** Ron's attempt to cleanup the results of the faster but
** unstable trig generator that I'm using to produce trig
** values that are not used to generate other trig values.
**
** It works based on the assumption that we know one piece
** of information about the generated values: that (sin+i*cos)
** must end up with a magnitude of 1.
** 
** Computation: 1 +, 2 *,      per trig value, plus
**              2 +, 4 *, 1 /  per cleaned up trig value
**
** A better way would be to have 3 Buneman-generators running
** at different rates.
*/

#if (TRIG_VERSION == T_BUNE_0)
#define TRIG_23(k,c1,s1,c2,s2,c3,s3)                             \
        {                                                        \
         float t;                                               \
         c2 = c1*c1 - s1*s1;                                     \
         s2 = 2*(c1*s1);                                         \
         t = 0.5/(c2*c2 + s2*s2) + 0.5; s2 *= t; c2 *= t;        \
         c3 = c2*c1 - s2*s1;                                     \
         s3 = c2*s1 + s2*c1;                                     \
         t = 0.5/(c3*c3 + s3*s3) + 0.5; s3 *= t; c3 *= t;        \
        }
#endif
#if (TRIG_VERSION == T_BUNE_1)
#define TRIG_23(k,c1,s1,c2,s2,c3,s3)                             \
        {                                                        \
         float t;                                               \
         c2 = c1*c1 - s1*s1;                                     \
         s2 = 2*(c1*s1);                                         \
         t = 0.5/(c2*c2 + s2*s2) + 0.5; s2 *= t; c2 *= t;        \
         c3 = c2*c1 - s2*s1;                                     \
         s3 = c2*s1 + s2*c1;                                     \
        }
#endif
#if (TRIG_VERSION == T_BUNE_2)
#define TRIG_23(k,c1,s1,c2,s2,c3,s3)                             \
        {                                                        \
         c2 = c1*c1 - s1*s1;                                     \
         s2 = 2*(c1*s1);                                         \
         c3 = c2*c1 - s2*s1;                                     \
         s3 = c2*s1 + s2*c1;                                     \
        }
#endif



/*
** Algorithm:  unknown origin
** 
** Performance:  Mediocre accuracy, good speed.
** 
** Comments:
**   A relatively straightforward way of generating trig functions.
**   It's really similar to the obvious method below, except that this
**   method takes advantage that 1-cos(x) is easier to represent
**   accurately than cos(x) for most CPUs and small x.  Among other
**   places, this algorithm is found in the Numerical Recipies book,
**   and a variation of it is commonly found in implementations of
**   Singleton's FFT.
**
** Computation: 2+, 2* per trig value
*/

#if (TRIG_VERSION == T_NREC_0)
#define TRIG_VARS                                        \
      float wpr,wpi;
#define TRIG_INIT(k,c,s)                                 \
    {                                                    \
     wpr = -2.0*sintab[k+1]*sintab[k+1];                 \
     wpi = sintab[k];                                    \
     c    = 1;                                           \
     s    = 0;                                           \
    }
#define TRIG_NEXT(k,c,s)                                 \
    {                                                    \
     float t;                                           \
     c+=(t=c)*wpr-s*wpi;                                 \
     s+=s*wpr+t*wpi;                                     \
    }
#define TRIG_23(k,c1,s1,c2,s2,c3,s3)                     \
        {                                                \
         c2 = c1*c1 - s1*s1;                             \
         s2 = 2*(c1*s1);                                 \
         c3 = c2*c1 - s2*s1;                             \
         s3 = c2*s1 + s2*c1;                             \
        }
#endif



/*
** Algorithm: complex multiplication
** 
** Performance: Bad accuracy, great speed.
** 
** The simplest, way of generating trig values.  Note, this method is
** not stable, and errors accumulate rapidly.
** 
** Computation: 2 *, 1 + per value.
*/

#if (TRIG_VERSION == T_SIMP_0)
#define TRIG_VARS                                        \
      float t_c,t_s;
#define TRIG_INIT(k,c,s)                                 \
    {                                                    \
     t_c  = costab[k];                                   \
     t_s  = sintab[k];                                   \
     c    = 1;                                           \
     s    = 0;                                           \
    }
#define TRIG_NEXT(k,c,s)                                 \
    {                                                    \
     float t = c;                                       \
     c   = t*t_c - s*t_s;                                \
     s   = t*t_s + s*t_c;                                \
    }
#define TRIG_23(k,c1,s1,c2,s2,c3,s3)                     \
        {                                                \
         c2 = c1*c1 - s1*s1;                             \
         s2 = 2*(c1*s1);                                 \
         c3 = c2*c1 - s2*s1;                             \
         s3 = c2*s1 + s2*c1;                             \
        }
#endif

/*
** Algorithm: Table lookups.
** 
** Performance: Great accuracy, great speed...
**              ...till you blow your cache. Then it sucks.
** Computation: a couple integer shifts and adds + a memory read 
** 
*/
#if (TRIG_VERSION == T_LKUP_0)

#include "bigtrigtab.h"

#define TRIG_VARS                                  \
      int angle_index;
#define TRIG_INIT(k,c,s)                           \
     {                                             \
      int i;                                       \
      angle_index = 0;                             \
      c = 1;                                       \
      s = 0;                                       \
     }
#define TRIG_NEXT(k,c,s)                           \
     {                                             \
      angle_index+=(256>>k);                       \
      c=bigtrigtab[angle_index+256];               \
      s=bigtrigtab[angle_index];                   \
     }
#define TRIG_23(k,c1,s1,c2,s2,c3,s3)               \
     {                                             \
         c2 = bigtrigtab[angle_index*2+256];       \
         s2 = bigtrigtab[angle_index*2];           \
         c3 = bigtrigtab[angle_index*3+256];       \
         s3 = bigtrigtab[angle_index*3];           \
     }

#endif



/*
** Algorithm: Table lookups.
** 
** Performance: Great accuracy, great speed...
**              ...except the first time, where it sucks alot.
**              ...till you blow your cache. Then it sucks a little.
** Computation: unknown
** 
*/
#if (TRIG_VERSION == T_CLLU_0)
#include <math.h>
#include <malloc.h>

float *hugetrigtab=0;
int     hugetrigtab_siz = 0;
void init_code(int n) {
  int i;
  if (hugetrigtab!=0) free(hugetrigtab);
  hugetrigtab = (float *)malloc(n*sizeof(float)*5/4);
  for (i=0;i<n*5/4;i++) {
    hugetrigtab[i] = sin(2*PI/n*i);
  }
  hugetrigtab_siz = n;
}


#define TRIG_VARS                               \
      int angle_index;          \
      if (hugetrigtab_siz != n) \
          init_code(n);         \

#define TRIG_INIT(k,c,s)                           \
     {                                             \
      int i;                                       \
      angle_index = 0;                             \
      c = 1;                                       \
      s = 0;                                       \
     }
#define TRIG_NEXT(k,c,s)                           \
     {                                             \
      angle_index+=((n/4)>>k);                     \
      c=hugetrigtab[angle_index+(n/4)];            \
      s=hugetrigtab[angle_index];                  \
     }
#define TRIG_23(k,c1,s1,c2,s2,c3,s3)               \
     {                                             \
         c2 = hugetrigtab[angle_index*2+(n/4)];    \
         s2 = hugetrigtab[angle_index*2];          \
         c3 = hugetrigtab[angle_index*3+(n/4)];    \
         s3 = hugetrigtab[angle_index*3];          \
     }

#endif




static float halsec[TRIG_TABLE_SIZE]=
    {
     0,
     0,
     .54119610014619698439972320536638942006107206337801,
     .50979557910415916894193980398784391368261849190893,
     .50241928618815570551167011928012092247859337193963,
     .50060299823519630134550410676638239611758632599591,
     .50015063602065098821477101271097658495974913010340,
     .50003765191554772296778139077905492847503165398345,
     .50000941253588775676512870469186533538523133757983,
     .50000235310628608051401267171204408939326297376426,
     .50000058827484117879868526730916804925780637276181,
     .50000014706860214875463798283871198206179118093251,
     .50000003676714377807315864400643020315103490883972,
     .50000000919178552207366560348853455333939112569380,
     .50000000229794635411562887767906868558991922348920,
     .50000000057448658687873302235147272458812263401372,
     .50000000014362164661654736863252589967935073278768,
     .50000000003590541164769084922906986545517021050714
    };
static float costab[TRIG_TABLE_SIZE]=
    {
     .00000000000000000000000000000000000000000000000000,
     .70710678118654752440084436210484903928483593768847,
     .92387953251128675612818318939678828682241662586364,
     .98078528040323044912618223613423903697393373089333,
     .99518472667219688624483695310947992157547486872985,
     .99879545620517239271477160475910069444320361470461,
     .99969881869620422011576564966617219685006108125772,
     .99992470183914454092164649119638322435060646880221,
     .99998117528260114265699043772856771617391725094433,
     .99999529380957617151158012570011989955298763362218,
     .99999882345170190992902571017152601904826792288976,
     .99999970586288221916022821773876567711626389934930,
     .99999992646571785114473148070738785694820115568892,
     .99999998161642929380834691540290971450507605124278,
     .99999999540410731289097193313960614895889430318945,
     .99999999885102682756267330779455410840053741619428,
     .99999999971275670684941397221864177608908945791828,
     .99999999992818917670977509588385049026048033310951
    };
static float sintab[TRIG_TABLE_SIZE]=
    {
     1.0000000000000000000000000000000000000000000000000,
     .70710678118654752440084436210484903928483593768846,
     .38268343236508977172845998403039886676134456248561,
     .19509032201612826784828486847702224092769161775195,
     .09801714032956060199419556388864184586113667316749,
     .04906767432741801425495497694268265831474536302574,
     .02454122852291228803173452945928292506546611923944,
     .01227153828571992607940826195100321214037231959176,
     .00613588464915447535964023459037258091705788631738,
     .00306795676296597627014536549091984251894461021344,
     .00153398018628476561230369715026407907995486457522,
     .00076699031874270452693856835794857664314091945205,
     .00038349518757139558907246168118138126339502603495,
     .00019174759731070330743990956198900093346887403385,
     .00009587379909597734587051721097647635118706561284,
     .00004793689960306688454900399049465887274686668768,
     .00002396844980841821872918657716502182009476147489,
     .00001198422490506970642152156159698898480473197753
    };
